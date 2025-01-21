#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "ed_common.h"

UnixAddress* open_unix_socket(char* path) {
    int fd;
    fd = socket(AF_UNIX, SOCK_STREAM, 0); // TODO: Try SOCK_SEQPACKET later
    if (fd == -1) {
        fprintf(stderr, "Failed to open AF_UNIX socket. ErrNo=%d\n", errno);
        cleanup(fd, path);
        exit(errno);
    }

    UnixAddress *addr = (UnixAddress *) malloc(sizeof(UnixAddress));
    addr->addr.sun_family = AF_UNIX;
    addr->fd = fd;
    strcpy(addr->addr.sun_path, path);
    return addr;
}

void cleanup(int fd, char *path) {
    int retval;

    if (fd != -1) {
        retval = close(fd);
        if (retval == -1) fprintf(stderr, "Failed to successfully close socket. ErrorNo=%d\n", errno);
    }

    remove(path);
}

int set_nonblock(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (-1 == flags) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int set_block(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (-1 == flags) return -1;
    return fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
}
