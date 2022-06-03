#include "vmi.h"
#include "client.h"

#include <fcntl.h>
#include <signal.h>
#include <time.h>

reg_t lstar;

int running_mode;
int cs;

#ifndef MEM_EVENT
char saved_opcode = 0;
vmi_event_t int_event;
vmi_event_t sstep_event = {0};

struct bp_cb_data {
    reg_t sym_vaddr;
    char saved_opcode;
    uint64_t hit_count;
};

struct bp_cb_data cb_data;
char BREAKPOINT = 0xcc;
#else
vmi_event_t cr3_event;
vmi_event_t msr_syscall_event;
reg_t cr3;
bool mem_events_registered;
#endif

int num_sys = 0;
int num_set = 0;
char **sys_index = NULL;
char **set_syscalls = NULL; 
char buffer[MAX_BUFSIZE];
int syscall_index;
int window_size = 10;

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

    if (running_mode == SANDBOX_MODE) {
        char set_line[256];

        FILE *file = fopen("data/syscalls.txt", "r");
        if (file == NULL)
            return;

        while(fgets(set_line, sizeof(set_line), file) != NULL){
            set_line[strlen(set_line) - 1] = '\0';
            set_syscalls = realloc(set_syscalls, sizeof(char*) * ++num_set);
            set_syscalls[num_set-1] = (char*) malloc(256);
            strcpy(set_syscalls[num_set-1], set_line);
        }

        fclose(file); 
    }
}

bool check_set_syscalls(int index) {
    if (index < num_sys) {
        for (int i = 0; i < num_set; i++) {
            if (strcmp(sys_index[index], set_syscalls[i]) == 0) {
                return true;
            }
        }
    }

    return false;
}

void process_syscall(vmi_instance_t vmi, vmi_event_t* event) {
    reg_t rdi, rax, cr3;
    vmi_pid_t pid = -1;
    FILE* fp = NULL;

    if (running_mode == LEARN_MODE)
        fp = fopen("syscall-trace.csv", "a");

    vmi_get_vcpureg(vmi, &rax, RAX, event->vcpu_id);
    vmi_get_vcpureg(vmi, &rdi, RDI, event->vcpu_id);
    vmi_get_vcpureg(vmi, &cr3, CR3, event->vcpu_id);

    vmi_dtb_to_pid(vmi, cr3, &pid);

    uint16_t _index = (uint16_t)rax;

    char** args = (char**)malloc(7 * sizeof (char*));

    for (int i = 0; i < 7; i++) {
        args[i] = NULL;
    }

    char* output = (char*)malloc (100 * sizeof(char));

    if ((running_mode == SANDBOX_MODE) && (check_set_syscalls(_index) == false)) 
        return;


    if (_index >= num_sys) {
        sprintf(output, "Process[%d]: unknown syscall id: %d ", pid, _index);

        if (running_mode == LEARN_MODE)
            fprintf(fp, "%d, %d, , ", pid, _index);
    }
    else {
        sprintf(output, "PID[%d]: %s with args", pid, sys_index[_index]);

        if (running_mode == LEARN_MODE)
            fprintf(fp, "%d, %d, %s, ", pid, _index, sys_index[_index]);
        
        if (running_mode == ANALYSIS_MODE) {
            append_syscall(buffer, sys_index[_index], &cs, &syscall_index, window_size);
        }
    }
    args[0] = output;
    print_args(vmi, event, pid, _index, fp, running_mode, args);
    
    for (int i=0;i<7;i++) {
        if (args[i] != NULL) {
            printf("%s ", args[i]);
            free(args[i]);
        }
    }

    free(args);

    printf("\n");

    if (running_mode == LEARN_MODE)
        fclose(fp);
    
}

#ifdef MEM_EVENT
event_response_t syscall_cb(vmi_instance_t vmi, vmi_event_t *event) {
    if ( event->mem_event.offset != (VMI_BIT_MASK(0,11) & lstar) ) {
        vmi_clear_event(vmi, event, NULL);
        vmi_step_event(vmi, event, event->vcpu_id, 1, NULL);
        return 0;
    }
    
    process_syscall(vmi, event);

    vmi_clear_event(vmi, event, NULL);
    vmi_step_event(vmi, event, event->vcpu_id, 1, NULL);

    return 0;
}


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
       
        process_syscall(vmi, event);

        cb_data->hit_count++;


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

    struct bp_cb_data *cb_data = (struct bp_cb_data*)event->data;

    if (VMI_FAILURE == vmi_write_va(vmi, cb_data->sym_vaddr, 0, sizeof(BREAKPOINT), &BREAKPOINT, NULL)) {
        fprintf(stderr, "Failed to write breakpoint at 0x%" PRIx64 "\n",event->x86_regs->rip);
        interrupted = true;
        return VMI_EVENT_RESPONSE_NONE;
    }

    return VMI_EVENT_RESPONSE_TOGGLE_SINGLESTEP;
}

event_response_t set_lstar_breakpoint(vmi_instance_t vmi) {

if (VMI_FAILURE == vmi_get_vcpureg(vmi, &lstar, MSR_LSTAR, 0)) {
        fprintf(stderr, "Failed to get current RIP\n");
        return 1;
    }

    printf("Pause VM\n");
    if (VMI_FAILURE == vmi_pause_vm(vmi)) {
        fprintf(stderr, "Failed to pause VM\n");
        return VMI_FAILURE;
    }

    printf("Save opcode\n");
    if (VMI_FAILURE == vmi_read_va(vmi, lstar, 0, sizeof(BREAKPOINT), &saved_opcode, NULL)) {
        fprintf(stderr, "Failed to read opcode\n");
        return VMI_FAILURE;
    }

    printf("Write breakpoint at 0x%" PRIx64 "\n", lstar);
    if (VMI_FAILURE == vmi_write_va(vmi, lstar, 0, sizeof(BREAKPOINT), &BREAKPOINT, NULL)) {
        fprintf(stderr, "Failed to write breakpoint\n");
        return VMI_FAILURE;
    }


    memset(&int_event, 0, sizeof(vmi_event_t));
    int_event.version = VMI_EVENTS_VERSION;
    int_event.type = VMI_EVENT_INTERRUPT;
    int_event.interrupt_event.intr = INT3;
    int_event.callback = breakpoint_cb;

    cb_data.sym_vaddr = lstar;
    cb_data.saved_opcode = saved_opcode;
    cb_data.hit_count = 0;

    int_event.data = (void*)&cb_data;

    printf("Register interrupt event\n");
    if (VMI_FAILURE == vmi_register_event(vmi, &int_event)) {
        fprintf(stderr, "Failed to register interrupt event\n");
        return VMI_FAILURE;
    }

    unsigned int num_vcpus = vmi_get_num_vcpus(vmi);

    sstep_event.version = VMI_EVENTS_VERSION;
    sstep_event.type = VMI_EVENT_SINGLESTEP;
    sstep_event.callback = single_step_cb;
    sstep_event.ss_event.enable = false;

    for (unsigned int vcpu=0; vcpu < num_vcpus; vcpu++)
        SET_VCPU_SINGLESTEP(sstep_event.ss_event, vcpu);

    sstep_event.data = (void*)&cb_data;

    printf("Register singlestep event\n");
    if (VMI_FAILURE == vmi_register_event(vmi, &sstep_event)) {
        fprintf(stderr, "Failed to register singlestep event\n");
        return VMI_FAILURE;
    }

    printf("Resume VM\n");
    if (VMI_FAILURE == vmi_resume_vm(vmi)) {
        fprintf(stderr, "Failed to resume VM\n");
        return VMI_FAILURE;
    }
}
#endif

int introspect_syscall_trace (char *name, int set_mode, int window_size, int set_time) {
    vmi_instance_t vmi = NULL;
    status_t status = VMI_SUCCESS;
    struct sigaction act;
    vmi_init_data_t *init_data = NULL;
    running_mode = set_mode;
    act.sa_handler = close_handler;
    act.sa_flags = 0;

    read_syscall_table();

    if (running_mode == ANALYSIS_MODE)
        connect_server(&cs);

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
    
    if (running_mode == LEARN_MODE)
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

error_exit:
#ifndef MEM_EVENT
    vmi_pause_vm(vmi);
    if (saved_opcode) {
        printf("Restore previous opcode at 0x%" PRIx64 "\n", lstar);
        vmi_write_va(vmi, lstar, 0, sizeof(BREAKPOINT), &saved_opcode, NULL);
    }

        // cleanup queue
    if (vmi_are_events_pending(vmi))
        vmi_events_listen(vmi, 0);

    vmi_clear_event(vmi, &int_event, NULL);
    vmi_clear_event(vmi, &sstep_event, NULL);

    vmi_resume_vm(vmi);
#else
    if ( vmi_are_events_pending(vmi) > 0 )
        vmi_events_listen(vmi, 0);

    vmi_clear_event(vmi, &cr3_event, NULL);
    vmi_clear_event(vmi, &msr_syscall_event, NULL);
#endif

    vmi_destroy(vmi);

    if (running_mode == ANALYSIS_MODE)
        close(cs);

    return 0;
}