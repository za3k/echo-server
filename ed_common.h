#include <sys/socket.h>
#include <sys/un.h>

typedef struct {
    int fd;
    struct sockaddr_un addr;
} UnixAddress;

// sockets.c
UnixAddress* open_unix_socket(char* path);
void cleanup(int fd, char *path);
int set_block(int fd);
int set_nonblock(int fd);

// client.c
int client(char *path);

// server.c
int server(char *path);
