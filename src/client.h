#define MAX_BUFSIZE 2048
#define SEPARATOR ","
#define END "."

#define RDI_IDX 0
#define RSI_IDX 1
#define RDX_IDX 2
#define R10_IDX 3
#define R8_IDX  4
#define R9_IDX  5

#define ANALYSIS_SERVER_PORT 1201
#define FLASK_SERVER_PORT 1234

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include <libvmi/libvmi.h>
#include <libvmi/events.h>

#include "string.h"

void connect_server(int* cs, int port) {
    struct addrinfo hints;
    struct addrinfo *res, *tmp;
    char host[256];
    char hostname[1024];
    char ip_server[] = "127.0.0.1";
    struct sockaddr_in sockcl;
    struct sockaddr_in to;

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

    if (setsockopt(*cs, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int)) < 0)
        perror("setsockopt(SO_REUSEADDR) failed");

    sockcl.sin_family=AF_INET;
	sockcl.sin_addr.s_addr=inet_addr(host);

	to.sin_family=AF_INET;
	to.sin_port = htons(port);
	to.sin_addr.s_addr = inet_addr(ip_server);

	if (bind(*cs, (struct sockaddr *)&sockcl, sizeof(sockcl)) < 0)
	{
		perror("client: bind");
		_exit(1);
	}

	if (connect(*cs, (struct sockaddr *)&to, sizeof(to)) < 0)
	{
		perror("client: connect");
		_exit(1);
	}

    freeaddrinfo(res);
    freeaddrinfo(tmp);
}

void append_syscall(char* buffer, char* syscall, int* socket, int* frame_index, int window_size) {
    if (*frame_index == window_size) {
        *frame_index = 0;
        strcat(buffer, END);
        write(*socket, buffer, strlen(buffer));
        strcpy(buffer, "");
    }
    else {
        strcat(buffer, syscall);
        strcat(buffer, SEPARATOR);
       (*frame_index)++;
    }
}

void create_csv_file() {
    FILE *fp = fopen("syscall-trace.csv", "w");
    fprintf(fp, "PID,SyscallID,Syscall,RDI,RSI,RDX,R10,R8,R9\n");
    fclose(fp);
}
#pragma region syscall_handlers
void get_open_flags(int flags, char** args, int index) {
    args[index] = (char*)malloc(100 * sizeof(char));
    strcpy(args[index], "");

    if (!flags)
        strcat(args[index], "O_RDONLY ");
    if (flags & O_WRONLY)
        strcat(args[index], "O_WRONLY ");
    if (flags & O_RDWR)
        strcat(args[index], "O_RDWR ");
    if (flags & O_CREAT)
        strcat(args[index], "O_CREAT ");
    if (flags & O_EXCL)
        strcat(args[index], "O_EXCL ");
    if (flags & O_NOCTTY)
        strcat(args[index], "O_NOCTTY ");
    if (flags & O_TRUNC)
        strcat(args[index], "O_TRUNC ");
    if (flags & O_APPEND)
        strcat(args[index], "O_APPEND ");
    if (flags & O_NONBLOCK)
        strcat(args[index], "O_NONBLOCK ");
    if (flags & O_DSYNC)
        strcat(args[index], "O_DSYNC ");
    if (flags & __O_DIRECT)
        strcat(args[index], "__O_DIRECT ");
    if (flags & O_DIRECTORY)
        strcat(args[index], "O_DIRECTORY ");
    if (flags & O_CLOEXEC)
        strcat(args[index], "O_CLOEXEC ");
}

void get_mprotect_flags(int flags, char** args) {
    args[4] = (char*)malloc(100 * sizeof(char));
    strcpy(args[4], "");

    if (!flags)
        strcat(args[4], "PROT_NONE ");
    if (flags & PROT_READ)
        strcat(args[4], "PROT_READ ");
    if (flags & PROT_WRITE)
        strcat(args[4], "PROT_WRITE ");
    if (flags & PROT_EXEC)
        strcat(args[4], "PROT_EXEC ");
}

void get_open_args(vmi_instance_t vmi, int pid, reg_t *regs, FILE* fp, char** args) {
    char *filename = NULL;
    filename = vmi_read_str_va(vmi, regs[RDI_IDX], pid);

    if (filename) {
        args[2] = strdup(filename);
        free(filename);
    } else {
        args[2] = strdup("null");
    }

    args[3] = (char*)malloc(30 * sizeof (char));
    sprintf(args[3], "mode: %u", (unsigned int)regs[RSI_IDX]);

    get_open_flags((unsigned int)regs[RDX_IDX], args, 4);
}

void get_execve_args(vmi_instance_t vmi, int pid, reg_t *regs, FILE* fp, char** args) {
    char *filename = NULL;
    filename = vmi_read_str_va(vmi, regs[RDI_IDX], pid);

    if (filename) {
        args[2] = strdup(filename);
        free(filename);
    } else {
        args[2] = strdup("null");
    }
}


void get_openat_args(vmi_instance_t vmi, int pid, reg_t *regs, FILE* fp, char** args) {
    char *filename = NULL;
    filename = vmi_read_str_va(vmi, regs[RSI_IDX], pid);

    args[2] = (char*)malloc(30 * sizeof (char));
    sprintf(args[2], "DFD: %u", (unsigned int)regs[RDI_IDX]);

    if (filename) {
        args[3] = strdup(filename);
        free(filename);
    } else {
        args[3] = strdup("null");
    }

    args[4] = (char*)malloc(30 * sizeof (char));
    sprintf(args[4], "mode: %u", (unsigned int)regs[RDX_IDX]);

    get_open_flags((unsigned int)regs[R10_IDX], args, 5);
}

void get_write_args(vmi_instance_t vmi, int pid, reg_t *regs, FILE* fp, char** args) {
    char *buffer = NULL;
    buffer = vmi_read_str_va(vmi, regs[RSI_IDX], pid);

    args[2] = (char*)malloc(30 * sizeof (char));
    sprintf(args[2], "fd: %u", (unsigned int)regs[RDI_IDX]);

    if (buffer) {
        args[3] = strdup(buffer);
        free(buffer);
    } else {
        args[3] = strdup("null");
    }

    args[4] = (char*)malloc(30 * sizeof (char));
    sprintf(args[4], "count:  %u", (unsigned int)regs[RDX_IDX]);
}

void get_mprotect_args(vmi_instance_t vmi, int pid, reg_t *regs, FILE* fp, char** args) {
    args[2] = (char*)malloc(30 * sizeof (char));
    sprintf(args[2], "start addr: 0x%lx", (unsigned long)regs[RDI_IDX]);

    args[3] = (char*)malloc(30 * sizeof (char));
    sprintf(args[3], "len: %u", (unsigned int)regs[RSI_IDX]);

    get_mprotect_flags((unsigned int)regs[RDX_IDX], args);
}

void get_args(vmi_instance_t vmi, vmi_event_t *event, int pid, int syscall_id, FILE* fp, int mode, char** args) {
    int args_number = 6;
    
    reg_t regs[6];

    vmi_get_vcpureg(vmi, &regs[RDI_IDX], RDI, event->vcpu_id);
    vmi_get_vcpureg(vmi, &regs[RSI_IDX], RSI, event->vcpu_id);
    vmi_get_vcpureg(vmi, &regs[RDX_IDX], RDX, event->vcpu_id);
    vmi_get_vcpureg(vmi, &regs[R10_IDX], R10, event->vcpu_id);
    vmi_get_vcpureg(vmi, &regs[R8_IDX], R8, event->vcpu_id);
    vmi_get_vcpureg(vmi, &regs[R9_IDX], R9, event->vcpu_id);

    switch (syscall_id) {
        case 1:
            get_write_args(vmi, pid, regs, fp, args);
            break;
        case 2:
            get_open_args(vmi, pid, regs, fp, args);
            break;
        case 10:
            get_mprotect_args(vmi, pid, regs, fp, args);
            break;
        case 59:
            get_execve_args(vmi, pid, regs, fp, args);
            break;
        case 257:
            get_openat_args(vmi, pid, regs, fp, args);
            break;
        
        default:
            for (int i = 0; i < args_number; i++) {
                args[i + 2] = (char*)malloc(50 * sizeof(char));

                sprintf(args[i + 2], "%u", (unsigned int)regs[i]);
            }
            break;
    }

    if (mode == LEARN_MODE)
        fprintf(fp, "%u,%u,%u,%u,%u,%u\n", (unsigned int)regs[RDI_IDX], (unsigned int)regs[RSI_IDX], (unsigned int)regs[RDX_IDX], (unsigned int)regs[R10_IDX], (unsigned int)regs[R8_IDX], (unsigned int)regs[R9_IDX]);
}
#pragma endregion

bool make_readable(char* str) {
    int len = strlen(str);

    for (int i = 0; i < len; i++) {
        if (!isprint(str[i]))
            str[i] = '.';
    }

    return true;
}

void send_syscall_verbose(char** syscall, int* socket) {
    int total_size = 0;

    for (int i = 0; i < 8; i++) {
        if (syscall[i] != NULL) {
            total_size += strlen(syscall[i]);
        } else {
            total_size += 15;
        }
    }

    char* socket_buffer = (char*)malloc((total_size + 100) * sizeof (char));
    strcpy(socket_buffer, "");

    for (int i = 0; i < 8; i++) {
        
        if (syscall[i] != NULL) {
            make_readable(syscall[i]);

            strcat(socket_buffer, syscall[i]);
            strcat(socket_buffer, "*");
        }
    }

    strcat(socket_buffer, "end*");

    total_size = strlen(socket_buffer);

    write(*socket, socket_buffer, total_size);

    free(socket_buffer);
}
