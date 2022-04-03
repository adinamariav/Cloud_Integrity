#include <string.h>
#include "vmi.h"

vmi_event_t syscall_enter_event;
vmi_event_t syscall_step_event;

reg_t virt_lstar;
addr_t phys_lstar;

int num_sys = 0;
char **sys_index = NULL;

#ifndef MEM_EVENT
uint32_t syscall_orig_data;
#endif

event_response_t syscall_step_cb(vmi_instance_t vmi, vmi_event_t *event) {
    /**
     * enable the syscall entry interrupt
     */
#ifdef MEM_EVENT
    vmi_register_event(vmi, &syscall_enter_event);
#else
    syscall_enter_event.interrupt_event.reinject = 1;
    if (set_breakpoint(vmi, virt_lstar, 0) < 0) {
        printf("Could not set break points\n");
        exit(1);
    }
#endif

    /** 
     * disable the single event
     */
    vmi_clear_event(vmi, &syscall_step_event, NULL);
    return 0;
}

int get_arg_number(int syscall_id) {
    char command[256] = "grep -hR 'SYSCALL_DEFINE.\\?(";
    strcat(command, sys_index[syscall_id]);
    strcat(command, ",' * /usr/src/linux-source-5.4.0/ | head -1 | sed 's/^SYSCALL_DEFINE\\([1-9]\\).*$/\\1/'");
    int args_nr;

    FILE* fp = NULL;

    fp = popen(command, "r");

    if (fp == NULL)
    {
        printf("%s\n", "Failed to run command");
        return -1;
    }

    fscanf(fp, "%d", &args_nr);
    pclose(fp);

    if (args_nr > 6)
        return -1;

    return args_nr;
}

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

void print_open(vmi_instance_t vmi, int pid, reg_t *regs) {
    char *filename = NULL;
    filename = vmi_read_str_va(vmi, regs[0], pid);

    printf("filename: %s, mode: %u, flags: ", filename, (unsigned int)regs[1]);
    print_open_flags((unsigned int)regs[2]);
    free(filename);
}

void print_openat(vmi_instance_t vmi, int pid, reg_t *regs) {
    char *filename = NULL;
    filename = vmi_read_str_va(vmi, regs[1], pid);

    printf("DFD: %u, filename: %s, mode: %u, flags: ", (unsigned int)regs[0], filename, (unsigned int)regs[2]);
    print_open_flags((unsigned int)regs[3]);
    free(filename);
}

void print_write(vmi_instance_t vmi, int pid, reg_t *regs) {
    char *buffer = NULL;
    buffer = vmi_read_str_va(vmi, regs[1], pid);

    printf("fd: %u, buf: %s, count: %u", (unsigned int)regs[0], buffer, (unsigned int)regs[2]);
    free(buffer);
}

void print_execve(vmi_instance_t vmi, int pid, reg_t *regs) {
    char *filename = NULL;
    filename = vmi_read_str_va(vmi, regs[0], pid);
    char *argv = NULL;
    argv = vmi_read_str_va(vmi, regs[1], pid);
    char *envp = NULL;
    envp = vmi_read_str_va(vmi, regs[2], pid);

    printf("filename: %s, argv[0]: %s, envp[0]: %u", filename, argv, envp);
    free(filename);
    free(argv);
    free(envp);
}

void print_mprotect(vmi_instance_t vmi, int pid, reg_t *regs) {
    printf("start addr: 0x%lx, len: %u, prot: ", (unsigned long)regs[0], (unsigned int)regs[1]);

    print_mprotect_flags((unsigned int)regs[2]);
}

#pragma endregion

void print_args(vmi_instance_t vmi, vmi_event_t *event, int pid, int syscall_id) {
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
            print_write(vmi, pid, regs);
            break;
        case 2:
            print_open(vmi, pid, regs);
            break;
        case 10:
            print_mprotect(vmi, pid, regs);
            break;
        case 59:
            print_execve(vmi, pid, regs);
            break;
        case 257:
            print_openat(vmi, pid, regs);
            break;
        
        default:
            for (int i = 0; i < args_number; i++) {
                printf("%u ", (unsigned int)regs[i]);
            }
            break;
    }
}

event_response_t syscall_enter_cb(vmi_instance_t vmi, vmi_event_t *event) {
#ifdef MEM_EVENT
    if (event->mem_event.gla == virt_lstar) {
#else
    if (event->interrupt_event.gla == virt_lstar) {
#endif
        reg_t rdi, rax, cr3;
        vmi_get_vcpureg(vmi, &rax, RAX, event->vcpu_id);
        vmi_get_vcpureg(vmi, &rdi, RDI, event->vcpu_id);
        vmi_get_vcpureg(vmi, &cr3, CR3, event->vcpu_id);

        vmi_pid_t pid = -1;
        vmi_dtb_to_pid(vmi, cr3, &pid);

        uint16_t _index = (uint16_t)rax;
        if (_index >= num_sys) {
            printf("Process[%d]: unknown syscall id: %d\n", pid, _index);
        }
        else {
            printf("PID[%d]: %s with args ", pid, sys_index[_index]);
            print_args(vmi, event, pid, _index);
        }
        
        printf("\n");
    }

    /**
     * disable the syscall entry interrupt
     */
#ifdef MEM_EVENT
    vmi_clear_event(vmi, event, NULL);
#else
    event->interrupt_event.reinject = 0;
    if (VMI_FAILURE == vmi_write_32_va(vmi, virt_lstar, 0, &syscall_orig_data)) {
        printf("failed to write memory.\n");
        exit(1);
    }
#endif

    /**
     * set the single event to execute one instruction
     */
//    vmi_step_mem_event();
    vmi_step_event(vmi, event, event->vcpu_id, 1, NULL);
//    vmi_register_event(vmi, &syscall_step_event);
    return VMI_EVENT_RESPONSE_NONE;
}

int introspect_syscall_trace (char *name) {

    struct sigaction act;
    act.sa_handler = close_handler;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    sigaction(SIGHUP,  &act, NULL);
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGINT,  &act, NULL);
    sigaction(SIGALRM, &act, NULL);

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

    vmi_instance_t vmi = NULL;
    vmi_init_data_t *init_data = NULL;
    uint8_t init = VMI_INIT_DOMAINNAME | VMI_INIT_EVENTS, config_type = VMI_CONFIG_GLOBAL_FILE_ENTRY;
    void *input = NULL, *config = NULL;
    vmi_init_error_t *error = NULL;
    vmi_mode_t mode;
    
    /* initialize the libvmi library */
    if (VMI_FAILURE == vmi_get_access_mode(NULL, name, VMI_INIT_DOMAINNAME| VMI_INIT_EVENTS, init_data, &mode)) {
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

    if ( VMI_OS_UNKNOWN == vmi_init_os(vmi, VMI_CONFIG_GLOBAL_FILE_ENTRY, config, error) ) {
        printf("Failed to init os.\n");
        vmi_destroy(vmi);
        return 1;
    }


    printf("LibVMI init succeeded!\n");

    memset(&syscall_enter_event, 0, sizeof(vmi_event_t));

#ifdef MEM_EVENT
    if (VMI_FAILURE ==  vmi_pause_vm(vmi)) {
        fprintf(stderr, "Failed to pause vm\n");
        return 1;
    }

    if (VMI_FAILURE == vmi_get_vcpureg(vmi, &virt_lstar, MSR_LSTAR, 0)) {
        fprintf(stderr, "Failed to get current RIP\n");
        return 1;
    }

    uint64_t cr3;
    if (VMI_FAILURE == vmi_get_vcpureg(vmi, &cr3, CR3, 0)) {
        fprintf(stderr, "Failed to get current CR3\n");
        return 1;
    }
    uint64_t dtb = cr3 & ~(0xfff);

    uint64_t paddr;
    if (VMI_FAILURE == vmi_pagetable_lookup(vmi, dtb, virt_lstar, &paddr)) {
        fprintf(stderr, "Failed to find current paddr\n");
        return 1;
    }

    uint64_t gfn = paddr >> 12;

    SETUP_MEM_EVENT(&syscall_enter_event, gfn, VMI_MEMACCESS_X, syscall_enter_cb, false);


    printf("Setting X memory event at LSTAR 0x%"PRIx64", GPA 0x%"PRIx64", GFN 0x%"PRIx64"\n",
           virt_lstar, paddr, gfn);

    if (VMI_FAILURE == vmi_register_event(vmi, &syscall_enter_event)) {
        fprintf(stderr, "Failed to register mem event\n");
        return 1;
    }

    // resuming
    if (VMI_FAILURE == vmi_resume_vm(vmi)) {
        fprintf(stderr, "Failed to resume vm\n");
        return 1;
    }

#else
    /**
     * iniialize the interrupt event for INT3.
     */
    syscall_enter_event.type = VMI_EVENT_INTERRUPT;
    syscall_enter_event.interrupt_event.intr = INT3;
    syscall_enter_event.callback = syscall_enter_cb;
#endif

    /**
     * iniialize the single step event.
     */
    memset(&syscall_step_event, 0, sizeof(vmi_event_t));
    SETUP_SINGLESTEP_EVENT(&syscall_step_event, 1, syscall_step_cb, 0);

#ifndef MEM_EVENT
    /**
     * store the original data for syscall entry function
     */
    if (VMI_FAILURE == vmi_read_32_va(vmi, virt_lstar, 0, &syscall_orig_data)) {
        printf("failed to read the original data.\n");
        vmi_destroy(vmi);
        return -1;
    }

    /**
     * insert breakpoint into the syscall entry function
     */
    if (set_breakpoint(vmi, virt_lstar, 0) < 0) {
        printf("Could not set break points\n");
        goto exit;
    }
#endif

    while(!interrupted){
        if (vmi_events_listen(vmi, 1000) != VMI_SUCCESS) {
            printf("Error waiting for events, quitting...\n");
            interrupted = -1;
        }
    }

exit:

#ifndef MEM_EVENT
    /**
     * write back the original data
     */
    if (VMI_FAILURE == vmi_write_32_va(vmi, virt_lstar, 0, &syscall_orig_data)) {
        printf("failed to write back the original data.\n");
    }
#endif
    vmi_clear_event(vmi, &syscall_enter_event, NULL);
    vmi_destroy(vmi);
    return 0;
}
