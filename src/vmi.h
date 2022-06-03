#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <inttypes.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/mount.h>
#include <openssl/md5.h>
#include <libvmi/libvmi.h>
#include <libvmi/events.h>

/**
 * default is using INT 3 for event notification
 * if MEM_EVENT is defined, then using EPT violation
 */

//#define MEM_EVENT
#define LEARN_MODE          0
#define ANALYSIS_MODE       1
#define SANDBOX_MODE        2
#define EDUCATIONAL_MODE    3

/* task_struct offsets */
unsigned long tasks_offset;
unsigned long pid_offset;
unsigned long name_offset;

static int interrupted = 0;

static void close_handler(int sig){
    interrupted = sig;
}

int introspect_syscall_trace(char *name, int set_mode, int window_size, int time);
