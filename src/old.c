#include <string.h>
#include "vmi.h"

vmi_event_t syscall_enter_event;
vmi_event_t syscall_step_event;
vmi_event_t trap_event;
vmi_event_t singlestep_event;
pid_t last_pid;

reg_t virt_lstar;
addr_t phys_lstar;
addr_t execve_addr;
uint8_t backup_byte;
uint8_t trap = 0xCC;

struct filename {
        const char              *name;  /* pointer to actual string */
        const char       *uptr;  /* original userland pointer */
        int                     refcnt;
        struct audit_names      *aname;
        const char              iname[];
};

int num_sys = 0;
char **sys_index = NULL;

#ifndef MEM_EVENT
uint32_t syscall_orig_data;
#endif

event_response_t syscall_step_cb(vmi_instance_t vmi, vmi_event_t *event) {
    printf("step\n");
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

void print_open(vmi_instance_t vmi, int pid, reg_t *regs, FILE* fp) {
    char *filename = NULL;
    filename = vmi_read_str_va(vmi, regs[0], pid);

    printf("filename: %s, mode: %u, flags: ", filename, (unsigned int)regs[1]);
    print_open_flags((unsigned int)regs[2]);
    fprintf(fp, "%s, %u, %u, %u, %u, %u\n", filename, (unsigned int)regs[1], (unsigned int)regs[2], (unsigned int)regs[3], (unsigned int)regs[4], (unsigned int)regs[5]);

    free(filename);
}

void print_openat(vmi_instance_t vmi, int pid, reg_t *regs, FILE* fp) {
    char *filename = NULL;
    filename = vmi_read_str_va(vmi, regs[1], pid);

    printf("DFD: %u, filename: %s, mode: %u, flags: ", (unsigned int)regs[0], filename, (unsigned int)regs[2]);
    fprintf(fp, "%u, %s, %u, %u, %u, %u\n", (unsigned int)regs[0], filename, (unsigned int)regs[2], (unsigned int)regs[3], (unsigned int)regs[4], (unsigned int)regs[5]);
    print_open_flags((unsigned int)regs[3]);
    free(filename);
}

void print_write(vmi_instance_t vmi, int pid, reg_t *regs, FILE* fp) {
    char *buffer = NULL;
    buffer = vmi_read_str_va(vmi, regs[1], pid);

    printf("fd: %u, buf: %s, count: %u", (unsigned int)regs[0], buffer, (unsigned int)regs[2]);
    fprintf(fp, "%u, %s, %u, %u, %u, %u\n", (unsigned int)regs[0], buffer, (unsigned int)regs[2], (unsigned int)regs[3], (unsigned int)regs[4], (unsigned int)regs[5]);
    free(buffer);
}


void read_task_struct_for_pid(vmi_instance_t vmi, int pid) {
    printf("AICI %d\n", pid);
    unsigned long tasks_offset = 0, pid_offset = 0, name_offset = 0;
    status_t status = VMI_FAILURE;
    addr_t list_head = 0, cur_list_entry = 0, next_list_entry = 0;
    addr_t current_process = 0;
    char *procname = NULL;
    vmi_pid_t read_pid = 0;

    if (vmi_pause_vm(vmi) != VMI_SUCCESS) {
        printf("Failed to pause VM\n");
        return;
    } // if

    if ( VMI_FAILURE == vmi_get_offset(vmi, "linux_tasks", &tasks_offset) ) {
        printf("Error reading linux_tasks\n");
        return;
    }
    if ( VMI_FAILURE == vmi_get_offset(vmi, "linux_name", &name_offset) ) {
        printf("Error reading linux_name\n");
        return; 
    }
    if ( VMI_FAILURE == vmi_get_offset(vmi, "linux_pid", &pid_offset) ) {
        printf("Error reading linux_pid\n");
        return; 
    }

    if ( VMI_FAILURE == vmi_translate_ksym2v(vmi, "init_task", &list_head) ) {
        printf("Error reading linux_pid\n");
        return; 
    }

    list_head += tasks_offset;
    cur_list_entry = list_head;
    if (VMI_FAILURE == vmi_read_addr_va(vmi, cur_list_entry, 0, &next_list_entry)) {
        printf("Failed to read next pointer at %"PRIx64"\n", cur_list_entry);
        return;
    }

    while(1) {

        current_process = cur_list_entry - tasks_offset;

        /* Note: the task_struct that we are looking at has a lot of
         * information.  However, the process name and id are burried
         * nice and deep.  Instead of doing something sane like mapping
         * this data to a task_struct, I'm just jumping to the location
         * with the info that I want.  This helps to make the example
         * code cleaner, if not more fragile.  In a real app, you'd
         * want to do this a little more robust :-)  See
         * include/linux/sched.h for mode details */

        /* NOTE: _EPROCESS.UniqueProcessId is a really VOID*, but is never > 32 bits,
         * so this is safe enough for x64 Windows for example purposes */
        vmi_read_32_va(vmi, current_process + pid_offset, 0, (uint32_t*)&read_pid);

        procname = vmi_read_str_va(vmi, current_process + name_offset, 0);

        if (!procname) {
            printf("Failed to find procname\n");
            return;
        }

        /* print out the process name */
        if (read_pid == pid) {
            printf("[%5d] %s (struct addr:%"PRIx64")\n", read_pid, procname, current_process);
                if (procname) {
                free(procname);
                procname = NULL;
            }

            break;   
        }
       
        /* follow the next pointer */
        cur_list_entry = next_list_entry;
        status = vmi_read_addr_va(vmi, cur_list_entry, 0, &next_list_entry);
        if (status == VMI_FAILURE) {
            printf("Failed to read next pointer in loop at %"PRIx64"\n", cur_list_entry);
            return;
        }
        /* In Windows, the next pointer points to the head of list, this pointer is actually the
         * address of PsActiveProcessHead symbol, not the address of an ActiveProcessLink in
         * EPROCESS struct.
         * It means in Windows, we should stop the loop at the last element in the list, while
         * in Linux, we should stop the loop when coming back to the first element of the loop
         */
       /* if (next_list_entry == list_head) {
            break;
        }*/

        vmi_resume_vm(vmi);
    }
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

  //  printf("last-pid: %d ", pid);
    printf("filename: %s", filename);
    fprintf(fp, "%s, %u, %u, %u, %u, %u\n", filename, (unsigned int)regs[0], (unsigned int)regs[1], (unsigned int)regs[2], (unsigned int)regs[3], (unsigned int)regs[5]);

    vmi_resume_vm(vmi);
    free(filename);
    free(argv);
    free(envp);
}

void print_mprotect(vmi_instance_t vmi, int pid, reg_t *regs, FILE* fp) {
    printf("start addr: 0x%lx, len: %u, prot: ", (unsigned long)regs[0], (unsigned int)regs[1]);
    print_mprotect_flags((unsigned int)regs[2]);
    fprintf(fp, "0x%lx, %u, %u, %u, %u, %u\n", (unsigned long)regs[0], (unsigned int)regs[1], (unsigned int)regs[2], (unsigned int)regs[3], (unsigned int)regs[4], (unsigned int)regs[5]);
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
            read_task_struct_for_pid(vmi, pid);
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
            fprintf(fp, "%u, %u, %u, %u, %u, %u\n", (unsigned int)regs[0], (unsigned int)regs[1], (unsigned int)regs[2], (unsigned int)regs[3], (unsigned int)regs[4], (unsigned int)regs[5]);
            
            break;
    }
}

#pragma endregion

event_response_t singlestep_cb(vmi_instance_t vmi, vmi_event_t *event) {
	vmi_write_8_va(vmi, execve_addr, 0, &trap);

	return VMI_EVENT_RESPONSE_TOGGLE_SINGLESTEP;
}


event_response_t trap_cb(vmi_instance_t vmi, vmi_event_t *event) {
    
 if (event->mem_event.gla == execve_addr) {
        
        reg_t rdi, rsi, rax, cr3, rip;
      //  vmi_pause_vm(vmi);

        vmi_get_vcpureg(vmi, &rdi, RDI, event->vcpu_id);
        vmi_get_vcpureg(vmi, &rsi, RSI, event->vcpu_id);
        vmi_get_vcpureg(vmi, &rax, RAX, event->vcpu_id);
        vmi_get_vcpureg(vmi, &cr3, CR3, event->vcpu_id);
        vmi_get_vcpureg(vmi, &rip, RIP, event->vcpu_id);

        vmi_pid_t pid = -1;
        vmi_dtb_to_pid(vmi, cr3, &pid);
event->interrupt_event.reinject = 0;
       // char *filename = NULL;
        //filename = vmi_read_str_va(vmi, rax, pid);
/*
        char *filename2 = NULL;
        filename2 = vmi_read_str_va(vmi, rsi, pid);*/
        

        int size_read = 0;
        uint8_t buffer[2048]; 
        addr_t read_address = rax;

        for (int i = size_read; i < sizeof(struct filename); i++) {
            vmi_read_8_va(vmi, read_address, pid, buffer + i);
            read_address++;
        }

        struct filename name_file;
       // memcpy(&name_file, buffer, sizeof(name_file));

        for (int i=0; i<sizeof(name_file); i++) {
            printf("%c", buffer[i]);
        }
        printf("\n");
        

        printf("Received a trap event for syscall execve, pid 0x%d, RDI 0x%lx, RSI 0x%lx, RIP 0x%lx, RAX 0x%lx\n", pid, rdi, rsi, rip, rax);
//
        
        vmi_write_8_va(vmi, execve_addr, 0, &backup_byte);
      //  vmi_resume_vm(vmi);
       //free(filename);
   }
    // return VMI_EVENT_RESPONSE_NONE;
    return VMI_EVENT_RESPONSE_TOGGLE_SINGLESTEP;
}


void set_execve_breakpoint(vmi_instance_t vmi) {

    vmi_translate_ksym2v(vmi, "__x64_sys_execve", &execve_addr);
    printf("__x64_sys_execve addr: 0x%lx", execve_addr);
    execve_addr += 0x24;

    vmi_pause_vm(vmi);

    vmi_read_8_va(vmi, execve_addr, 0, &backup_byte);
    vmi_write_8_va(vmi, execve_addr, 0, &trap);

    SETUP_INTERRUPT_EVENT(&trap_event, trap_cb);
    SETUP_SINGLESTEP_EVENT(&singlestep_event, 1, singlestep_cb, 0);
	vmi_register_event(vmi, &trap_event);
    vmi_register_event(vmi, &singlestep_event);

    printf("Setting event at 0x%"PRIx64"\n", execve_addr);

    vmi_resume_vm(vmi);
}

void create_csv_file() {
    
    FILE *fp = fopen("syscall-trace.csv", "w");
    fprintf(fp, "PID, SyscallID, Syscall, RDI, RSI, RDX, R10, R8, R9\n");
    fclose(fp);
}

event_response_t syscall_enter_cb(vmi_instance_t vmi, vmi_event_t *event) {
    printf("syscall1\n");
#ifdef MEM_EVENT
  //  if (event->mem_event.gla == virt_lstar) {
        printf("syscall2\n");
#else
    if (event->interrupt_event.gla == virt_lstar) {
#endif
        FILE* fp = fopen("syscall-trace.csv", "a");
        reg_t rdi, rax, cr3;
        vmi_get_vcpureg(vmi, &rax, RAX, event->vcpu_id);
        vmi_get_vcpureg(vmi, &rdi, RDI, event->vcpu_id);
        vmi_get_vcpureg(vmi, &cr3, CR3, event->vcpu_id);

        vmi_pid_t pid = -1;
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
   // }

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
    vmi_step_event(vmi, event, event->vcpu_id, 1, NULL);
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

    create_csv_file();

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

    memset(&trap_event, 0, sizeof(vmi_event_t));
    memset(&singlestep_event, 0, sizeof(vmi_event_t));
  //  set_execve_breakpoint(vmi);

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

    syscall_enter_event.version = VMI_EVENTS_VERSION;
    syscall_enter_event.type = VMI_EVENT_MEMORY;
    syscall_enter_event.mem_event.gfn = phys_lstar >> 12;
    syscall_enter_event.mem_event.in_access = VMI_MEMACCESS_X;
    syscall_enter_event.callback = syscall_enter_cb;

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
   /*syscall_enter_event.type = VMI_EVENT_INTERRUPT;
    syscall_enter_event.interrupt_event.intr = INT3;
    syscall_enter_event.callback = syscall_enter_cb;*/

#endif

    /**
     * iniialize the single step event.
     */
 //   memset(&syscall_step_event, 0, sizeof(vmi_event_t));
 //   SETUP_SINGLESTEP_EVENT(&syscall_step_event, 1, syscall_step_cb, 0); 

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
    }*/
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
   /* vmi_write_8_va(vmi, execve_addr, 0, &backup_byte);
    vmi_clear_event(vmi, &trap_event, NULL);*/
    vmi_destroy(vmi);
    return 0;
}
