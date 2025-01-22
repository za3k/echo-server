#include <execinfo.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ed_common.h" 

static char *USAGE = "Usage: ed -s\n"
                     "       ed -c\n";
static char *path = "/tmp/multicat";

void handler(int sig) {
    void* array[10];
    size_t size;

    size = backtrace(array, 10);

    fprintf(stderr, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, 2);
    exit(1);
}

// In the initial version, just cat everything everyone inputs
int main(int argc, char **argv) {
    #define CLIENT 1
    #define SERVER 2
    int type = 0;
    int delay = 0;

    for (int an=1; an<argc; an++) {
        if (strcmp("-c", argv[an]) == 0) {
            type = CLIENT;
        } else if (strcmp("-s", argv[an]) == 0) {
            type = SERVER;
        } else {
            delay = atoi(argv[an]);
        }
    }

    if (type == 0) {
        fprintf(stderr, USAGE);
        exit(-1);
    }

    signal(SIGSEGV, handler);

    if (type == CLIENT) {
        return client(path);
    } else if (type == SERVER) {
        if (fork()) {
            return server(path, delay);
        } else {
            sleep(0.1);
            return client(path);
        }
    }
}
