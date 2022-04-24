#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <stdio.h>
#include <inttypes.h>
#include <signal.h>
#include <unistd.h>

#include <libvmi/libvmi.h>
#include <libvmi/events.h>

char BREAKPOINT = 0xcc;

static int interrupted = 0;
static void close_handler(int sig)
{
    interrupted = sig;
}

struct bp_cb_data {
    char *symbol;
    addr_t sym_vaddr;
    char saved_opcode;
    uint64_t hit_count;
};

void print_execve_params(vmi_instance_t vmi) {
    
    reg_t regs[6], cr3;

    vmi_get_vcpureg(vmi, &cr3, CR3, 0);
    vmi_get_vcpureg(vmi, &cr3, CR3, 0);

    vmi_pid_t pid = -1;
    vmi_dtb_to_pid(vmi, cr3, &pid);

    printf("%d\n", pid);

    vmi_get_vcpureg(vmi, &regs[0], RDI, 0);
    vmi_get_vcpureg(vmi, &regs[1], RSI, 0);
    vmi_get_vcpureg(vmi, &regs[2], RDX, 0);
    vmi_get_vcpureg(vmi, &regs[3], R10, 0);
    vmi_get_vcpureg(vmi, &regs[4], R8, 0);
    vmi_get_vcpureg(vmi, &regs[5], R9, 0);
    
    char *filename = NULL;

    char buffer[1000];

    vmi_pause_vm(vmi);

    filename = vmi_read_str_va(vmi, regs[0], 0);
    char *argv = NULL;
    argv = vmi_read_str_va(vmi, regs[1], 0);
    char *envp = NULL;
    envp = vmi_read_str_va(vmi, regs[2], 0);

  //  printf("last-pid: %d ", pid);
    printf("filename: %s, argv[0]: %s. envp[0] :%s\n", filename, argv, envp);

    vmi_resume_vm(vmi);
    free(filename);
    free(argv);
    free(envp);
}

event_response_t breakpoint_cb(vmi_instance_t vmi, vmi_event_t *event)
{
    (void)vmi;
    if (!event->data) {
        fprintf(stderr, "Empty event data in breakpoint callback !\n");
        interrupted = true;
        return VMI_EVENT_RESPONSE_NONE;
    }
    // get back callback data struct
    struct bp_cb_data *cb_data = (struct bp_cb_data*)event->data;

    // default reinjection behavior
    event->interrupt_event.reinject = 1;
    // printf("INT3 event: GFN=%"PRIx64" RIP=%"PRIx64" Length: %"PRIu32"\n",
    //        event->interrupt_event.gfn, event->interrupt_event.gla,
    //        event->interrupt_event.insn_length);

    /*
     * By default int3 instructions have length of 1 byte unless
     * there are prefixes attached. As adding prefixes to int3 have
     * no effect, under normal circumstances no legitimate compiler/debugger
     * would add any. However, a malicious guest could add prefixes to change
     * the instruction length. Older Xen versions (prior to 4.8) don't include this
     * information and thus this length is reported as 0. In those cases the length
     * have to be established manually, or assume a non-malicious guest as we do here.
     */
    if ( !event->interrupt_event.insn_length )
        event->interrupt_event.insn_length = 1;

    if (event->x86_regs->rip != cb_data->sym_vaddr) {
        // not our breakpoint
        printf("Not our breakpoint. Reinjecting INT3\n");
        return VMI_EVENT_RESPONSE_NONE;
    } else {
        // our breakpoint
        // do not reinject
        event->interrupt_event.reinject = 0;
        printf("[%"PRIu32"] Breakpoint hit at %s. Count: %"PRIu64"\n", event->vcpu_id, cb_data->symbol, cb_data->hit_count);
        cb_data->hit_count++;
       // print_execve_params(vmi);

        reg_t rax;
        vmi_get_vcpureg(vmi, &rax, RAX, 0);
        

        addr_t file_struct;
        vmi_read_addr_va(vmi, rax, 0, &file_struct);

        char* filename;
        filename = vmi_read_str_va(vmi, file_struct, 0);

        printf("%s\n", filename);

        // recoil
        // write saved opcode
        if (VMI_FAILURE == vmi_write_va(vmi, event->x86_regs->rip, 0, sizeof(BREAKPOINT), &cb_data->saved_opcode, NULL)) {
            fprintf(stderr, "Failed to write back original opcode at 0x%" PRIx64 "\n", event->x86_regs->rip);
            interrupted = true;
            return VMI_EVENT_RESPONSE_NONE;
        }
        // enable singlestep
        return VMI_EVENT_RESPONSE_TOGGLE_SINGLESTEP;
    }
}

event_response_t breakpoint_cb2(vmi_instance_t vmi, vmi_event_t *event)
{
    (void)vmi;
    if (!event->data) {
        fprintf(stderr, "Empty event data in breakpoint callback !\n");
        interrupted = true;
        return VMI_EVENT_RESPONSE_NONE;
    }
    // get back callback data struct
    struct bp_cb_data *cb_data = (struct bp_cb_data*)event->data;

    // default reinjection behavior
    event->interrupt_event.reinject = 1;
    // printf("INT3 event: GFN=%"PRIx64" RIP=%"PRIx64" Length: %"PRIu32"\n",
    //        event->interrupt_event.gfn, event->interrupt_event.gla,
    //        event->interrupt_event.insn_length);

    /*
     * By default int3 instructions have length of 1 byte unless
     * there are prefixes attached. As adding prefixes to int3 have
     * no effect, under normal circumstances no legitimate compiler/debugger
     * would add any. However, a malicious guest could add prefixes to change
     * the instruction length. Older Xen versions (prior to 4.8) don't include this
     * information and thus this length is reported as 0. In those cases the length
     * have to be established manually, or assume a non-malicious guest as we do here.
     */
    if ( !event->interrupt_event.insn_length )
        event->interrupt_event.insn_length = 1;

    if (event->x86_regs->rip != cb_data->sym_vaddr) {
        // not our breakpoint
        printf("Not our breakpoint. Reinjecting INT3\n");
        return VMI_EVENT_RESPONSE_NONE;
    } else {
        // our breakpoint
        // do not reinject
        event->interrupt_event.reinject = 0;
        printf("[%"PRIu32"] Breakpoint hit at %s. Count: %"PRIu64"\n", event->vcpu_id, cb_data->symbol, cb_data->hit_count);
        cb_data->hit_count++;
        print_execve_params(vmi);
        // recoil
        // write saved opcode
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

    // printf("Single-step event: VCPU:%u  GFN %"PRIx64" GLA %016"PRIx64"\n",
    //        event->vcpu_id,
    //        event->ss_event.gfn,
    //        event->ss_event.gla);

    // restore breakpoint
    if (VMI_FAILURE == vmi_write_va(vmi, cb_data->sym_vaddr, 0, sizeof(BREAKPOINT), &BREAKPOINT, NULL)) {
        fprintf(stderr, "Failed to write breakpoint at 0x%" PRIx64 "\n",event->x86_regs->rip);
        interrupted = true;
        return VMI_EVENT_RESPONSE_NONE;
    }

    // disable singlestep
    return VMI_EVENT_RESPONSE_TOGGLE_SINGLESTEP;
}


int introspect_trap_exec (char* name)
{
    struct sigaction act = {0};
    vmi_instance_t vmi = {0};
    vmi_init_data_t *init_data = NULL;
    int retcode = 1;
    char saved_opcode = 0;
    char saved_opcode2 = 0;

    /* for a clean exit */
    act.sa_handler = close_handler;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);
    sigaction(SIGHUP,  &act, NULL);
    sigaction(SIGTERM, &act, NULL);
    sigaction(SIGINT,  &act, NULL);
    sigaction(SIGALRM, &act, NULL);

    // init complete since we need symbols translation
    if (VMI_FAILURE ==
            vmi_init_complete(&vmi, name, VMI_INIT_DOMAINNAME | VMI_INIT_EVENTS, init_data,
                              VMI_CONFIG_GLOBAL_FILE_ENTRY, NULL, NULL)) {
        fprintf(stderr, "Failed to init LibVMI library.\n");
        goto error_exit;
    }

    printf("LibVMI init succeeded!\n");

    // translate symbol
    addr_t sym_vaddr = 0;
    if (VMI_FAILURE == vmi_translate_ksym2v(vmi, "__x64_sys_execve", &sym_vaddr)) {
        fprintf(stderr, "Failed to translate symbol %s\n", "__x64_sys_execve");
        goto error_exit;
    }
    printf("Symbol %s translated to virtual address: 0x%" PRIx64 "\n", "__x64_sys_execve", sym_vaddr);

    // pause VM
    printf("Pause VM\n");
    if (VMI_FAILURE == vmi_pause_vm(vmi)) {
        fprintf(stderr, "Failed to pause VM\n");
        goto error_exit;
    }

    sym_vaddr += 20;

    // save opcode
    printf("Save opcode\n");
    if (VMI_FAILURE == vmi_read_va(vmi, sym_vaddr, 0, sizeof(BREAKPOINT), &saved_opcode, NULL)) {
        fprintf(stderr, "Failed to read opcode\n");
        goto error_exit;
    }

    // write breakpoint
    printf("Write breakpoint at 0x%" PRIx64 "\n", sym_vaddr);
    if (VMI_FAILURE == vmi_write_va(vmi, sym_vaddr, 0, sizeof(BREAKPOINT), &BREAKPOINT, NULL)) {
        fprintf(stderr, "Failed to write breakpoint\n");
        goto error_exit;
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
        .symbol = "__x64_sys_execve",
        .sym_vaddr = sym_vaddr,
        .saved_opcode = saved_opcode,
        .hit_count = 0,
    };
    int_event.data = (void*)&cb_data;

    printf("Register interrupt event\n");
    if (VMI_FAILURE == vmi_register_event(vmi, &int_event)) {
        fprintf(stderr, "Failed to register interrupt event\n");
        goto error_exit;
    }

    // get number of vcpus
    unsigned int num_vcpus = vmi_get_num_vcpus(vmi);

    // register singlestep event
    // disabled by default
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
        goto error_exit;
    }

/*

// translate symbol
    addr_t sym_vaddr2 = 0;
    if (VMI_FAILURE == vmi_translate_ksym2v(vmi, "__x64_sys_fork", &sym_vaddr2)) {
        fprintf(stderr, "Failed to translate symbol %s\n", "__x64_sys_fork");
        goto error_exit;
    }
    printf("Symbol %s translated to virtual address: 0x%" PRIx64 "\n", "__x64_sys_fork", sym_vaddr2);

    // pause VM
    printf("Pause VM\n");
    if (VMI_FAILURE == vmi_pause_vm(vmi)) {
        fprintf(stderr, "Failed to pause VM\n");
        goto error_exit;
    }

    // save opcode
    printf("Save opcode\n");
    if (VMI_FAILURE == vmi_read_va(vmi, sym_vaddr2, 0, sizeof(BREAKPOINT), &saved_opcode2, NULL)) {
        fprintf(stderr, "Failed to read opcode\n");
        goto error_exit;
    }

    // write breakpoint
    printf("Write breakpoint at 0x%" PRIx64 "\n", sym_vaddr2);
    if (VMI_FAILURE == vmi_write_va(vmi, sym_vaddr2, 0, sizeof(BREAKPOINT), &BREAKPOINT, NULL)) {
        fprintf(stderr, "Failed to write breakpoint\n");
        goto error_exit;
    }

    // register int3 event
    vmi_event_t int_event2;
    memset(&int_event2, 0, sizeof(vmi_event_t));
    int_event2.version = VMI_EVENTS_VERSION;
    int_event2.type = VMI_EVENT_INTERRUPT;
    int_event2.interrupt_event.intr = INT3;
    int_event2.callback = breakpoint_cb2;

    // fill and pass struct bp_cb_data
    struct bp_cb_data cb_data2 = {
        .symbol = "__x64_sys_fork",
        .sym_vaddr = sym_vaddr2,
        .saved_opcode = saved_opcode2,
        .hit_count = 0,
    };
    int_event2.data = (void*)&cb_data2;

    printf("Register interrupt event\n");
    if (VMI_FAILURE == vmi_register_event(vmi, &int_event2)) {
        fprintf(stderr, "Failed to register interrupt event\n");
        goto error_exit;
    }

    // register singlestep event
    // disabled by default
    vmi_event_t sstep_event2 = {0};
    sstep_event2.version = VMI_EVENTS_VERSION;
    sstep_event2.type = VMI_EVENT_SINGLESTEP;
    sstep_event2.callback = single_step_cb;
    sstep_event2.ss_event.enable = false;
    // allow singlestep on all VCPUs
    for (unsigned int vcpu=0; vcpu < num_vcpus; vcpu++)
        SET_VCPU_SINGLESTEP(sstep_event2.ss_event, vcpu);
    // pass struct bp_cb_data
    sstep_event.data = (void*)&cb_data2;

    printf("Register singlestep event\n");
    if (VMI_FAILURE == vmi_register_event(vmi, &sstep_event2)) {
        fprintf(stderr, "Failed to register singlestep event\n");
        goto error_exit;
    }*/

    // resume VM
    printf("Resume VM\n");
    if (VMI_FAILURE == vmi_resume_vm(vmi)) {
        fprintf(stderr, "Failed to resume VM\n");
        goto error_exit;
    }

    // event loop
    while (!interrupted) {
        //printf("Waiting for events...\n");
        if (VMI_FAILURE == vmi_events_listen(vmi,500)) {
            fprintf(stderr, "Failed to listen on events\n");
            goto error_exit;
        }
    }
    printf("Finished with test.\n");

    retcode = 0;
error_exit:
    vmi_pause_vm(vmi);
    // restore opcode if needed
    if (saved_opcode) {
        printf("Restore previous opcode at 0x%" PRIx64 "\n", sym_vaddr);
        vmi_write_va(vmi, sym_vaddr, 0, sizeof(BREAKPOINT), &saved_opcode, NULL);
    }

   /* if (saved_opcode2) {
        printf("Restore previous opcode at 0x%" PRIx64 "\n", sym_vaddr2);
        vmi_write_va(vmi, sym_vaddr2, 0, sizeof(BREAKPOINT), &saved_opcode2, NULL);
    }*/

    // cleanup queue
    if (vmi_are_events_pending(vmi))
        vmi_events_listen(vmi, 0);

    vmi_clear_event(vmi, &int_event, NULL);
 //   vmi_clear_event(vmi, &int_event2, NULL);
    vmi_clear_event(vmi, &sstep_event, NULL);
//    vmi_clear_event(vmi, &sstep_event2, NULL);

    vmi_resume_vm(vmi);
    vmi_destroy(vmi);

    return retcode;
}