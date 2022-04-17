#include "vmi.h"

vmi_event_t trap_event;
vmi_event_t singlestep_event;

addr_t execve_addr;
uint8_t backup_byte;
uint8_t trap = 0xCC;

event_response_t singlestep_cb(vmi_instance_t vmi, vmi_event_t *event) {
	vmi_write_8_va(vmi, execve_addr, 0, &trap);
	return VMI_EVENT_RESPONSE_TOGGLE_SINGLESTEP;
}

event_response_t trap_cb(vmi_instance_t vmi, vmi_event_t *event) {

    reg_t rdi, cr3;

    vmi_write_8_va(vmi, execve_addr, 0, &backup_byte);
    vmi_get_vcpureg(vmi, &rdi, RDI, event->vcpu_id);

    vmi_get_vcpureg(vmi, &cr3, CR3, event->vcpu_id);

    vmi_pid_t pid = -1;
    vmi_dtb_to_pid(vmi, cr3, &pid);

    char *filename = NULL;
    filename = vmi_read_str_va(vmi, rdi, pid);

    printf("Received a trap event for syscall execve, pid %d, filename %s!\n", pid, filename);

    event->interrupt_event.reinject = 0;

    free(filename);
    return VMI_EVENT_RESPONSE_TOGGLE_SINGLESTEP;
}

void set_execve_breakpoint(vmi_instance_t vmi) {

    vmi_translate_ksym2v(vmi, "__x64_sys_execve", &execve_addr);

    vmi_pause_vm(vmi);

    vmi_read_8_va(vmi, execve_addr, 0, &backup_byte);
    vmi_write_8_va(vmi, execve_addr, 0, &trap);

    SETUP_INTERRUPT_EVENT(&trap_event, trap_cb);
    SETUP_SINGLESTEP_EVENT(&singlestep_event, 1, singlestep_cb, 0);
	vmi_register_event(vmi, &trap_event);
    vmi_register_event(vmi, &singlestep_event);

    vmi_resume_vm(vmi);
}

int introspect_trap_exec (char *name) {

    struct sigaction act;
    act.sa_handler = close_handler;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    sigaction(SIGHUP,  &act, NULL);
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGINT,  &act, NULL);
    sigaction(SIGALRM, &act, NULL);

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
    set_execve_breakpoint(vmi);

    while (!interrupted){
        if (vmi_events_listen(vmi, 1000) != VMI_SUCCESS) {
            printf("Error waiting for events, quitting...\n");
            interrupted = -1;
        }
    }

exit:
    vmi_write_8_va(vmi, execve_addr, 0, &backup_byte);
    vmi_clear_event(vmi, &trap_event, NULL);

    vmi_destroy(vmi);
    return 0;
}
