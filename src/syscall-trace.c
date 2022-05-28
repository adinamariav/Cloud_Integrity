#include "vmi.h"
#include "client.h"

#include <fcntl.h>
#include <signal.h>
#include <time.h>

#include <libvmi/libvmi.h>
#include <libvmi/events.h>

reg_t cr3;
reg_t lstar;
vmi_event_t cr3_event;
vmi_event_t msr_syscall_event;
bool mem_events_registered;
bool learning;
int cs;

#ifndef MEM_EVENT
struct bp_cb_data {
    addr_t sym_vaddr;
    char saved_opcode;
    uint64_t hit_count;
};

char BREAKPOINT = 0xcc;
#endif

int num_sys = 0;
char **sys_index = NULL;
char buffer[MAX_BUFSIZE];
int syscall_index;
int window_size = 10;

void append_syscall(int syscall_id) {
    if (syscall_index == window_size) {
        syscall_index = 0;
        strcat(buffer, END);
        write(cs, buffer, strlen(buffer));
        strcpy(buffer, "");
    }
    else {
        strcat(buffer, sys_index[syscall_id]);
        strcat(buffer, SEPARATOR);
        syscall_index++;
    }
}

void read_syscall_table() {
    char _line[256];
    char _name[256];
    int _index[256];

    FILE *_file = fopen("data/syscall_index.linux", "r");
    if (_file == NULL)
        printf("Failed to read syscall file\n");

    while(fgets(_line, sizeof(_line), _file) != NULL){
        sscanf(_line, "%d\t%s", _index, _name);
        sys_index = realloc(sys_index, sizeof(char*) * ++num_sys);
        sys_index[num_sys-1] = (char*) malloc(256);
        strcpy(sys_index[num_sys-1], _name);
    }
    fclose(_file);
}

event_response_t syscall_cb(vmi_instance_t vmi, vmi_event_t *event) {
    if ( event->mem_event.offset != (VMI_BIT_MASK(0,11) & lstar) ) {
        vmi_clear_event(vmi, event, NULL);
        vmi_step_event(vmi, event, event->vcpu_id, 1, NULL);
        return 0;
    }

    reg_t rdi, rax, cr3;
    vmi_pid_t pid = -1;
    FILE* fp = fopen("syscall-trace.csv", "a");

    vmi_get_vcpureg(vmi, &rax, RAX, event->vcpu_id);
    vmi_get_vcpureg(vmi, &rdi, RDI, event->vcpu_id);
    vmi_get_vcpureg(vmi, &cr3, CR3, event->vcpu_id);

    
    vmi_dtb_to_pid(vmi, cr3, &pid);

    uint16_t _index = (uint16_t)rax;

    if (_index >= num_sys) {
        printf("Process[%d]: unknown syscall id: %d\n", pid, _index);
        fprintf(fp, "%d, %d, , ", pid, _index);
    }
    else {
        printf("PID[%d]: %s with args ", pid, sys_index[_index]);
        fprintf(fp, "%d, %d, %s, ", pid, _index, sys_index[_index]);
        
        if (!learning)
            append_syscall(_index);
    }

    print_args(vmi, event, pid, _index, fp);

    fclose(fp);
    printf("\n");

    vmi_clear_event(vmi, event, NULL);
    vmi_step_event(vmi, event, event->vcpu_id, 1, NULL);

    return 0;
}

#ifdef MEM_EVENT
bool register_lstar_mem_event(vmi_instance_t vmi, vmi_event_t *event) {

    addr_t cr3 = event->reg_event.value;
    addr_t phys_lstar = 0;
    addr_t phys_sysenter_ip = 0;
    bool ret = false;
    addr_t phys_vsyscall = 0;

    lstar = event->x86_regs->msr_lstar;

    vmi_pagetable_lookup(vmi, event->x86_regs->cr3, lstar, &phys_lstar);
    printf("Physical LSTAR == %llx\n", (unsigned long long)phys_lstar);

    msr_syscall_event.version = VMI_EVENTS_VERSION;
    msr_syscall_event.type = VMI_EVENT_MEMORY;
    msr_syscall_event.mem_event.gfn = phys_lstar >> 12;
    msr_syscall_event.mem_event.in_access = VMI_MEMACCESS_X;
    msr_syscall_event.callback=syscall_cb;

    if ( phys_lstar && VMI_SUCCESS == vmi_register_event(vmi, &msr_syscall_event) )
        ret = true;
    else
        printf("Failed to register memory event on MSR_LSTAR page\n");

    return ret;
}

event_response_t cr3_register_task_callback(vmi_instance_t vmi, vmi_event_t *event) {
    printf("%s\n", "cr3");
    if ( !mem_events_registered )
        mem_events_registered = register_lstar_mem_event(vmi, event);
    vmi_clear_event(vmi, &cr3_event, NULL);
    return 0;
}

event_response_t register_mem_events(vmi_instance_t vmi) {
    memset(&cr3_event, 0, sizeof(vmi_event_t));
    cr3_event.version = VMI_EVENTS_VERSION;
    cr3_event.type = VMI_EVENT_REGISTER;
    cr3_event.callback = cr3_register_task_callback;
    cr3_event.reg_event.reg = CR3;
    cr3_event.reg_event.in_access = VMI_REGACCESS_W;

    return vmi_register_event(vmi, &cr3_event);
}

#else
event_response_t breakpoint_cb(vmi_instance_t vmi, vmi_event_t *event)
{
    (void)vmi;
    if (!event->data) {
        fprintf(stderr, "Empty event data in breakpoint callback !\n");
        interrupted = true;
        return VMI_EVENT_RESPONSE_NONE;
    }
    struct bp_cb_data *cb_data = (struct bp_cb_data*)event->data;

    event->interrupt_event.reinject = 1;
    if ( !event->interrupt_event.insn_length )
        event->interrupt_event.insn_length = 1;

    if (event->x86_regs->rip != cb_data->sym_vaddr) {
        printf("Not our breakpoint. Reinjecting INT3\n");
        return VMI_EVENT_RESPONSE_NONE;
    } else {
        event->interrupt_event.reinject = 0;
        printf("[%"PRIu32"] Breakpoint hit. Count: %"PRIu64"\n", event->vcpu_id, cb_data->hit_count);
        cb_data->hit_count++;


        reg_t rax;
        vmi_get_vcpureg(vmi, &rax, RAX, 0);
        

        addr_t file_struct;
        vmi_read_addr_va(vmi, rax, 0, &file_struct);

        char* filename;
        filename = vmi_read_str_va(vmi, file_struct, 0);

        printf("%s\n", filename);

        if (VMI_FAILURE == vmi_write_va(vmi, event->x86_regs->rip, 0, sizeof(BREAKPOINT), &cb_data->saved_opcode, NULL)) {
            fprintf(stderr, "Failed to write back original opcode at 0x%" PRIx64 "\n", event->x86_regs->rip);
            interrupted = true;
            return VMI_EVENT_RESPONSE_NONE;
        }
        // enable singlestep
        return VMI_EVENT_RESPONSE_TOGGLE_SINGLESTEP;
    }
}

event_response_t single_step_cb(vmi_instance_t vmi, vmi_event_t *event)
{
    (void)vmi;

    if (!event->data) {
        fprintf(stderr, "Empty event data in singlestep callback !\n");
        interrupted = true;
        return VMI_EVENT_RESPONSE_NONE;
    }

    // get back callback data struct
    struct bp_cb_data *cb_data = (struct bp_cb_data*)event->data;

    printf("Single-step event: VCPU:%u  GFN %"PRIx64" GLA %016"PRIx64"\n",
           event->vcpu_id,
           event->ss_event.gfn,
           event->ss_event.gla);

    // restore breakpoint
    if (VMI_FAILURE == vmi_write_va(vmi, cb_data->sym_vaddr, 0, sizeof(BREAKPOINT), &BREAKPOINT, NULL)) {
        fprintf(stderr, "Failed to write breakpoint at 0x%" PRIx64 "\n",event->x86_regs->rip);
        interrupted = true;
        return VMI_EVENT_RESPONSE_NONE;
    }

    // disable singlestep
    return VMI_EVENT_RESPONSE_TOGGLE_SINGLESTEP;
}

event_response_t set_lstar_breakpoint(vmi_instance_t vmi) {
    char saved_opcode = 0;

    if (VMI_FAILURE == vmi_pause_vm(vmi)) {
        fprintf(stderr, "Failed to pause VM\n");
        return VMI_FAILURE;
    }

    if (VMI_FAILURE == vmi_get_vcpureg(vmi, &lstar, MSR_LSTAR, 0)) {
        fprintf(stderr, "Failed to get current RIP\n");
        return 1;
    }

    if (VMI_FAILURE == vmi_read_va(vmi, lstar, 0, sizeof(BREAKPOINT), &saved_opcode, NULL)) {
        fprintf(stderr, "Failed to read opcode\n");
        return VMI_FAILURE;
    }

    printf("Write breakpoint at 0x%" PRIx64 "\n", lstar);
    if (VMI_FAILURE == vmi_write_va(vmi, lstar, 0, sizeof(BREAKPOINT), &BREAKPOINT, NULL)) {
        fprintf(stderr, "Failed to write breakpoint\n");
        return VMI_FAILURE;
    }

    // register int3 event
    vmi_event_t int_event;
    memset(&int_event, 0, sizeof(vmi_event_t));
    int_event.version = VMI_EVENTS_VERSION;
    int_event.type = VMI_EVENT_INTERRUPT;
    int_event.interrupt_event.intr = INT3;
    int_event.callback = breakpoint_cb;

    // fill and pass struct bp_cb_data
    struct bp_cb_data cb_data = {
        .sym_vaddr = lstar,
        .saved_opcode = saved_opcode,
        .hit_count = 0,
    };
    int_event.data = (void*)&cb_data;

    printf("Register interrupt event\n");
    if (VMI_FAILURE == vmi_register_event(vmi, &int_event)) {
        fprintf(stderr, "Failed to register interrupt event\n");
        return VMI_FAILURE;
    }

    unsigned int num_vcpus = vmi_get_num_vcpus(vmi);

    vmi_event_t sstep_event = {0};
    sstep_event.version = VMI_EVENTS_VERSION;
    sstep_event.type = VMI_EVENT_SINGLESTEP;
    sstep_event.callback = single_step_cb;
    sstep_event.ss_event.enable = false;

    // allow singlestep on all VCPUs
    for (unsigned int vcpu=0; vcpu < num_vcpus; vcpu++)
        SET_VCPU_SINGLESTEP(sstep_event.ss_event, vcpu);
    // pass struct bp_cb_data
    
    sstep_event.data = (void*)&cb_data;

    printf("Register singlestep event\n");
    if (VMI_FAILURE == vmi_register_event(vmi, &sstep_event)) {
        fprintf(stderr, "Failed to register singlestep event\n");
        return VMI_FAILURE;
    }

    if (VMI_FAILURE == vmi_resume_vm(vmi)) {
        fprintf(stderr, "Failed to resume VM\n");
        return VMI_FAILURE;
    }

    return VMI_SUCCESS;
}
#endif

int introspect_syscall_trace (char *name, bool learning_mode, int window_size, int set_time) {
    vmi_instance_t vmi = NULL;
    status_t status = VMI_SUCCESS;
    struct sigaction act;
    vmi_init_data_t *init_data = NULL;
    learning = learning_mode;

    act.sa_handler = close_handler;
    act.sa_flags = 0;

    struct sockaddr_in sockcl;
    struct sockaddr_in to;

    read_syscall_table();

    if (!learning)
        connect_server(&cs, &sockcl, &to);

    sigemptyset(&act.sa_mask);
    sigaction(SIGHUP,  &act, NULL);
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGINT,  &act, NULL);
    sigaction(SIGALRM, &act, NULL);

    vmi_mode_t mode;
    if (VMI_FAILURE == vmi_get_access_mode(NULL, name, VMI_INIT_DOMAINNAME, init_data, &mode)) {
        printf("Failed to find a supported hypervisor with LibVMI\n");
        return 1;
    }

    if (VMI_FAILURE == vmi_init(&vmi, mode, name, VMI_INIT_DOMAINNAME | VMI_INIT_EVENTS, init_data, NULL)) {
        printf("Failed to init LibVMI library.\n");
        return 1;
    }

    if ( VMI_PM_UNKNOWN == vmi_init_paging(vmi, 0) ) {
        printf("Failed to init determine paging.\n");
        vmi_destroy(vmi);
        return 1;
    }

    if ( VMI_OS_UNKNOWN == vmi_init_os(vmi, VMI_CONFIG_GLOBAL_FILE_ENTRY, NULL, NULL) ) {
        printf("Failed to init os.\n");
        vmi_destroy(vmi);
        return 1;
    }

    printf("LibVMI init succeeded!\n");

    create_csv_file();

    time_t start_time;
    float f_set_time = set_time * 60;
    time(&start_time);

#ifndef MEM_EVENT
    if ( VMI_SUCCESS == set_lstar_breakpoint(vmi) ) {
#else
    if ( VMI_SUCCESS == register_mem_events(vmi) ) {
#endif
        while (!interrupted) {
            status = vmi_events_listen(vmi,500);

            if (status != VMI_SUCCESS) {
                printf("Error waiting for events, quitting...\n");
                interrupted = -1;
            }

            if (set_time > 0) {
                if (difftime(time(NULL), start_time) > f_set_time)
                    interrupted = -1;
            }
        }
    }

    vmi_pause_vm(vmi);

    if ( vmi_are_events_pending(vmi) > 0 )
        vmi_events_listen(vmi, 0);

    vmi_clear_event(vmi, &cr3_event, NULL);
    vmi_clear_event(vmi, &msr_syscall_event, NULL);
    vmi_resume_vm(vmi);

    vmi_destroy(vmi);

    if (init_data) {
        free(init_data->entry[0].data);
        free(init_data);
    }

    if (!learning)
        close(cs);

    return 0;
}