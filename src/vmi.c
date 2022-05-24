#include "vmi.h"

int main (int argc, char *argv[]) {
    int opt = 0;
    char *vm_name = NULL;
    char *mode = NULL;
    char *arg = NULL;
    bool learning_mode = true;
    int window_size = 10;

    /**
     * Parsing Parameters
     * -v: vm name listed by xl list
     * -m: mode option
     */
    while ((opt = getopt(argc, argv, "v:m:r:h")) != -1) {
        switch(opt) {
            case 'h':
                printf("Usage: ./vmi -v [vm_name] -m [mode]\n");
                printf("Supported Mode: \n");
                printf("process-list:		List the processes\n");
                printf("module-list:		List the modules\n");
                printf("network-check:		Check if any network connection is hidden\n");
                printf("syscall-trace:		Trace the system calls made by any processes. Must be followed by -l [learning] (default) or -a [analyzing], and by -w [window size]\n");
                printf("socketapi-trace:	Trace the socket API made by any processes\n");
                printf("process-kill:		Kill a process at runtime given its pid\n");
                return 0;
            case 'v':
                vm_name = optarg;
                break;
            case 'm':
                mode = optarg;
                break;
            case 'r':
                arg = optarg;
                break;
            case 'l':
                learning_mode = true;
                break;
            case 'a':
                learning_mode = false;
                break;
            case 'w':
                window_size = atoi(optarg);
                break;
            case '?':
                if (optopt == 'v') {
                    printf("Missing mandatory VM name option\n");
                } else if (optopt == 'm') {
                    printf("Missing mandatory Mode option\n");
                } else {
                    printf("Invalid option received\n");
                }
                break;
        }
    }

    if ((!vm_name) || (!mode)) {
        printf("Missing mandatory VM name or Mode option\n");
        return 0;
    } 

    printf("Introspect VM %s with the Mode %s\n", vm_name, mode);

    if (!strcmp(mode, "process-list")) {
        introspect_process_list(vm_name);
    } else if (!strcmp(mode, "module-list")) {
        introspect_module_list(vm_name);
    } else if (!strcmp(mode, "network-check")) {
        introspect_network_check(vm_name);
    } else if (!strcmp(mode, "syscall-trace")) {
        introspect_syscall_trace(vm_name, learning_mode, window_size);
    } else if (!strcmp(mode, "socketapi-trace")) {
        introspect_socketapi_trace(vm_name);
    } else if (!strcmp(mode, "trap-exec")) {
        introspect_trap_exec(vm_name);
    } else if (!strcmp(mode, "process-kill")) {
        if (arg == NULL) {
            printf("Missing process id to kill\n");
            return 0;
        }
        introspect_process_kill(vm_name, arg);
    } else {
        printf("Mode %s is not supported\n", mode);
    }

    return 0;
}
