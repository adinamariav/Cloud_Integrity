#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>
#include <signal.h>

#include <libvmi/libvmi.h>
#include <libvmi/events.h>

reg_t cr3;
reg_t lstar;
vmi_event_t cr3_event;
vmi_event_t msr_syscall_event;

bool mem_events_registered;
int num_sys = 0;
char **sys_index = NULL;
pid_t last_pid;

#pragma region syscall_handlers
void print_open_flags(int flags) {
    if (!flags)
        printf("%s", "O_RDONLY ");
    if (flags & O_WRONLY)
        printf("%s", "O_WRONLY ");
    if (flags & O_RDWR)
        printf("%s", "O_RDWR ");
    if (flags & O_CREAT)
        printf("%s", "O_CREAT ");
    if (flags & O_EXCL)
        printf("%s", "O_EXCL ");
    if (flags & O_NOCTTY)
        printf("%s", "O_NOCTTY ");
    if (flags & O_TRUNC)
        printf("%s", "O_TRUNC ");
    if (flags & O_APPEND)
        printf("%s", "O_APPEND ");
    if (flags & O_NONBLOCK)
        printf("%s", "O_NONBLOCK ");
    if (flags & O_DSYNC)
        printf("%s", "O_DSYNC ");
    if (flags & __O_DIRECT)
        printf("%s", "__O_DIRECT ");
    if (flags & O_DIRECTORY)
        printf("%s", "O_DIRECTORY ");
    if (flags & O_CLOEXEC)
        printf("%s", "O_CLOEXEC ");
}

void print_mprotect_flags(int flags) {
    if (!flags)
        printf("%s", "PROT_NONE ");
    if (flags & PROT_READ)
        printf("%s", "PROT_READ ");
    if (flags & PROT_WRITE)
        printf("%s", "PROT_WRITE ");
    if (flags & PROT_EXEC)
        printf("%s", "PROT_EXEC ");
}

void print_open(vmi_instance_t vmi, int pid, reg_t *regs, FILE* fp) {
    char *filename = NULL;
    filename = vmi_read_str_va(vmi, regs[0], pid);

    printf("filename: %s, mode: %u, flags: ", filename, (unsigned int)regs[1]);
    print_open_flags((unsigned int)regs[2]);

    free(filename);
}

void print_openat(vmi_instance_t vmi, int pid, reg_t *regs, FILE* fp) {
    char *filename = NULL;
    filename = vmi_read_str_va(vmi, regs[1], pid);

    printf("DFD: %u, filename: %s, mode: %u, flags: ", (unsigned int)regs[0], filename, (unsigned int)regs[2]);

    print_open_flags((unsigned int)regs[3]);
    free(filename);
}

void print_write(vmi_instance_t vmi, int pid, reg_t *regs, FILE* fp) {
    char *buffer = NULL;
    buffer = vmi_read_str_va(vmi, regs[1], pid);

    printf("fd: %u, buf: %s, count: %u", (unsigned int)regs[0], buffer, (unsigned int)regs[2]);
    free(buffer);
}

void print_execve(vmi_instance_t vmi, int pid, reg_t *regs, FILE* fp) {
    char *filename = NULL;

    char buffer[1000];

    vmi_pause_vm(vmi);

    filename = vmi_read_str_va(vmi, regs[0], pid);
    char *argv = NULL;
    argv = vmi_read_str_va(vmi, regs[1], pid);
    char *envp = NULL;
    envp = vmi_read_str_va(vmi, regs[2], pid);

    printf(" last-pid: %d ", pid);
    printf("filename: %s", filename);

    vmi_resume_vm(vmi);
    free(filename);
    free(argv);
    free(envp);
}

void print_mprotect(vmi_instance_t vmi, int pid, reg_t *regs, FILE* fp) {
    printf("start addr: 0x%lx, len: %u, prot: ", (unsigned long)regs[0], (unsigned int)regs[1]);
    print_mprotect_flags((unsigned int)regs[2]);
}

void print_args(vmi_instance_t vmi, vmi_event_t *event, int pid, int syscall_id, FILE* fp) {
    int args_number = 6;
    
    if (args_number <= 0)
        return;
    
    reg_t regs[6];

    vmi_get_vcpureg(vmi, &regs[0], RDI, event->vcpu_id);
    vmi_get_vcpureg(vmi, &regs[1], RSI, event->vcpu_id);
    vmi_get_vcpureg(vmi, &regs[2], RDX, event->vcpu_id);
    vmi_get_vcpureg(vmi, &regs[3], R10, event->vcpu_id);
    vmi_get_vcpureg(vmi, &regs[4], R8, event->vcpu_id);
    vmi_get_vcpureg(vmi, &regs[5], R9, event->vcpu_id);

    switch (syscall_id) {
        case 1:
            print_write(vmi, pid, regs, fp);
            break;
        case 2:
            print_open(vmi, pid, regs, fp);
            break;
        case 10:
            print_mprotect(vmi, pid, regs, fp);
            break;
        case 59:
            print_execve(vmi, last_pid, regs, fp);
            break;
        case 57:
            last_pid = pid;
            break;
        case 257:
            print_openat(vmi, pid, regs, fp);
            break;
        
        default:
            for (int i = 0; i < args_number; i++) {
                printf("%u ", (unsigned int)regs[i]);
            }
            
            break;
    }

    fprintf(fp, "%u, %u, %u, %u, %u, %u\n", (unsigned int)regs[0], (unsigned int)regs[1], (unsigned int)regs[2], (unsigned int)regs[3], (unsigned int)regs[4], (unsigned int)regs[5]);
}
#pragma endregion

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
    }

    print_args(vmi, event, pid, _index, fp);

    fclose(fp);
    printf("\n");

    vmi_clear_event(vmi, event, NULL);
    vmi_step_event(vmi, event, event->vcpu_id, 1, NULL);

    return 0;
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

void create_csv_file() {
    FILE *fp = fopen("syscall-trace.csv", "w");
    fprintf(fp, "PID, SyscallID, Syscall, RDI, RSI, RDX, R10, R8, R9\n");
    fclose(fp);
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

static int interrupted = 0;
static void close_handler(int sig) {
    interrupted = sig;
}

int introspect_syscall_trace (char *name) {
    vmi_instance_t vmi = NULL;
    status_t status = VMI_SUCCESS;
    struct sigaction act;
    vmi_init_data_t *init_data = NULL;

    act.sa_handler = close_handler;
    act.sa_flags = 0;

    read_syscall_table();

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

    memset(&cr3_event, 0, sizeof(vmi_event_t));
    cr3_event.version = VMI_EVENTS_VERSION;
    cr3_event.type = VMI_EVENT_REGISTER;
    cr3_event.callback = cr3_register_task_callback;
    cr3_event.reg_event.reg = CR3;
    cr3_event.reg_event.in_access = VMI_REGACCESS_W;

    create_csv_file();

    if ( VMI_SUCCESS == vmi_register_event(vmi, &cr3_event) ) {
        while (!interrupted) {
            status = vmi_events_listen(vmi,500);
            if (status != VMI_SUCCESS) {
                printf("Error waiting for events, quitting...\n");
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

    return 0;
}