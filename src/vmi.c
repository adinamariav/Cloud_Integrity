#include "vmi.h"

int main (int argc, char *argv[]) {
    int opt = 0;
    char *vm_name = NULL;
    char *mode = NULL;
    char *arg = NULL;
    int window_size = 10;
    int time = -1;
    bool print = false;

    /**
     * Parsing Parameters
     * -v: vm name listed by xl list
     * -m: mode option
     */
    while ((opt = getopt(argc, argv, "hv:m:sw:t:")) != -1) {
        switch(opt) {
            case 'h':
                printf("Usage: ./vmi -v [vm_name] -m [mode]\n");
                printf("Supported Mode: \n");
                printf("learn:		        Create learning database. Can be followed by -s to show output, by -w [window size] and -t [time]\n");
                printf("analyze:		    Analyze syscalls inside given vm\n");
                printf("sandbox:		    Print relevant events inside the vm. See README\n");
                printf("educational:		Print system calls and send them to server. See README\n");
                return 0;
            case 'v':
                vm_name = optarg;
                break;
            case 'm':
                mode = optarg;
                break;
            case 's':
                print = true;
                break;
            case 'w':
                window_size = atoi(optarg);
                break;
            case 't':
                time = atoi(optarg);
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

    if (!strcmp(mode, "learn")) {
        introspect_syscall_trace(vm_name, LEARN_MODE, window_size, time);
    } else if (!strcmp(mode, "analyze")) {
        introspect_syscall_trace(vm_name, ANALYSIS_MODE, window_size, time);
    } else if (!strcmp(mode, "educational")) {
        introspect_syscall_trace(vm_name, EDUCATIONAL_MODE, window_size, time);
    } else if (!strcmp(mode, "sandbox")) {
        introspect_syscall_trace(vm_name, SANDBOX_MODE, window_size, time);
    } else {
        printf("Mode %s is not supported\n", mode);
    }

    return 0;
}
