#define MAX_BUFSIZE 2048
#define SEPARATOR ","
#define END "."

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>


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