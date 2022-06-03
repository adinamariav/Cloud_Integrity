#define MAX_BUFSIZE 2048
#define SEPARATOR ","
#define END "."

#define RDI_ regs[0]
#define RSI_ regs[1]
#define RDX_ regs[2]
#define R10_ regs[3]
#define R8_  regs[4]
#define R9_  regs[5]


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <libvmi/libvmi.h>
#include <libvmi/events.h>


void connect_server(int* cs, struct sockaddr_in* sockcl, struct sockaddr_in* to) {
    struct addrinfo hints;
    struct addrinfo *res, *tmp;
    char host[256];
    char hostname[1024];
    char ip_server[] = "127.0.0.1";

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    gethostname(hostname, 1024);

    int ret = getaddrinfo(hostname, NULL, &hints, &res);
    if (ret != 0) {
        exit(EXIT_FAILURE);
    }

    for (tmp = res; tmp != NULL; tmp = tmp->ai_next) {
        getnameinfo(tmp->ai_addr, tmp->ai_addrlen, host, sizeof(host), NULL, 0, NI_NUMERICHOST);
    }

    if ((*cs = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("client: socket");
		_exit(1);
	}

    sockcl->sin_family=AF_INET;
	sockcl->sin_addr.s_addr=inet_addr(host);

	to->sin_family=AF_INET;
	to->sin_port = htons(1233);
	to->sin_addr.s_addr = inet_addr(ip_server);

	if (bind(*cs, (struct sockaddr *)sockcl, sizeof(*sockcl)) < 0)
	{
		perror("client: bind");
		_exit(1);
	}

	if (connect(*cs, (struct sockaddr *)to, sizeof(*to)) < 0)
	{
		perror("client: connect");
		_exit(1);
	}

    freeaddrinfo(res);
}

void create_csv_file() {
    FILE *fp = fopen("syscall-trace.csv", "w");
    fprintf(fp, "PID, SyscallID, Syscall, RDI, RSI, RDX, R10, R8, R9\n");
    fclose(fp);
}

event_response_t print_task_struct(vmi_instance_t vmi) {
    addr_t list_head = 0, cur_list_entry = 0, next_list_entry = 0;
    addr_t current_process = 0;
    char *procname = NULL;
    vmi_pid_t pid = 0;
    unsigned long tasks_offset = 0, pid_offset = 0, name_offset = 0;
    status_t status = VMI_FAILURE;
    int retcode = 1;

    if ( VMI_FAILURE == vmi_get_offset(vmi, "linux_tasks", &tasks_offset) )
        return VMI_FAILURE;
    if ( VMI_FAILURE == vmi_get_offset(vmi, "linux_name", &name_offset) )
        return VMI_FAILURE;
    if ( VMI_FAILURE == vmi_get_offset(vmi, "linux_pid", &pid_offset) )
        return VMI_FAILURE;

    if (vmi_pause_vm(vmi) != VMI_SUCCESS) {
        printf("Failed to pause VM\n");
        return VMI_FAILURE;
    }

    if ( VMI_FAILURE == vmi_translate_ksym2v(vmi, "init_task", &list_head) )
        return VMI_FAILURE;

    list_head += tasks_offset;

    cur_list_entry = list_head;
    if (VMI_FAILURE == vmi_read_addr_va(vmi, cur_list_entry, 0, &next_list_entry)) {
        printf("Failed to read next pointer at %"PRIx64"\n", cur_list_entry);
        return VMI_FAILURE;
    }

    while (1) {
        current_process = cur_list_entry - tasks_offset;
        vmi_read_32_va(vmi, current_process + pid_offset, 0, (uint32_t*)&pid);

        procname = vmi_read_str_va(vmi, current_process + name_offset, 0);

        if (!procname) {
            printf("Failed to find procname\n");
            return VMI_FAILURE;
        }

        /* print out the process name */
        printf("[%5d] %s (struct addr:%"PRIx64")\n", pid, procname, current_process);
        if (procname) {
            free(procname);
            procname = NULL;
        }

        /* follow the next pointer */
        cur_list_entry = next_list_entry;
        status = vmi_read_addr_va(vmi, cur_list_entry, 0, &next_list_entry);
        if (status == VMI_FAILURE) {
            printf("Failed to read next pointer in loop at %"PRIx64"\n", cur_list_entry);
            return VMI_FAILURE;
        } else if (cur_list_entry == list_head) {
            break;
        }
    };

    vmi_resume_vm(vmi);
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
    filename = vmi_read_str_va(vmi, RDI_, pid);

    printf("filename: %s, mode: %u, flags: ", filename, (unsigned int)RSI_);
    print_open_flags((unsigned int)RDX_);

    free(filename);
}

void print_execve(vmi_instance_t vmi, int pid, reg_t *regs, FILE* fp) {
    char *filename = NULL;
    filename = vmi_read_str_va(vmi, RDI_, pid);

    printf("filename: %s\n", filename);

    free(filename);
}


void print_openat(vmi_instance_t vmi, int pid, reg_t *regs, FILE* fp) {
    char *filename = NULL;
    filename = vmi_read_str_va(vmi, RSI_, pid);

    printf("DFD: %u, filename: %s, mode: %u, flags: ", (unsigned int)RDI_, filename, (unsigned int)RDX_);

    print_open_flags((unsigned int)R10_);
    free(filename);
}

void print_write(vmi_instance_t vmi, int pid, reg_t *regs, FILE* fp) {
    char *buffer = NULL;
    buffer = vmi_read_str_va(vmi, RSI_, pid);

    printf("fd: %u, buf: %s, count: %u", (unsigned int)RDI_, buffer, (unsigned int)RDX_);
    free(buffer);
}

void print_mprotect(vmi_instance_t vmi, int pid, reg_t *regs, FILE* fp) {
    printf("start addr: 0x%lx, len: %u, prot: ", (unsigned long)RDI_, (unsigned int)RSI_);
    print_mprotect_flags((unsigned int)RDX_);
}

void print_args(vmi_instance_t vmi, vmi_event_t *event, int pid, int syscall_id, FILE* fp, int mode) {
    int args_number = 6;
    
    if (args_number <= 0)
        return;
    
    reg_t regs[6];

    vmi_get_vcpureg(vmi, &RDI_, RDI, event->vcpu_id);
    vmi_get_vcpureg(vmi, &RSI_, RSI, event->vcpu_id);
    vmi_get_vcpureg(vmi, &RDX_, RDX, event->vcpu_id);
    vmi_get_vcpureg(vmi, &R10_, R10, event->vcpu_id);
    vmi_get_vcpureg(vmi, &R8_, R8, event->vcpu_id);
    vmi_get_vcpureg(vmi, &R9_, R9, event->vcpu_id);

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
            print_execve(vmi, pid, regs, fp);
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

    if (mode == LEARN_MODE)
        fprintf(fp, "%u, %u, %u, %u, %u, %u\n", (unsigned int)RDI_, (unsigned int)RSI_, (unsigned int)RDX_, (unsigned int)R10_, (unsigned int)R8_, (unsigned int)R9_);
}
#pragma endregion
