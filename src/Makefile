CC = gcc
LIBS = -lvmi -lm
OPENSSL_LIBS = -lcrypto

RM = rm -rf

MAIN = vmi
CLIENT = client
PROCESS_LIST = process-list
MODULE_LIST = module-list
NETWORK_CHECK = network-check
SYSCALL_TRACE = syscall-trace
SOCKETAPI_TRACE = socketapi-trace
PROCESS_KILL = process-kill
TRAP_EXEC = trap-exec

OBJECTS = $(MAIN).o $(SYSCALL_TRACE).o \

$(MAIN): $(OBJECTS)
	$(CC) -o $(MAIN) $(OBJECTS) $(LIBS) $(OPENSSL_LIBS)

$(OBJECTS): $(MAIN).h $(CLIENT).h

.PHONE: clean
clean:
	$(RM) $(MAIN) $(OBJECTS)
