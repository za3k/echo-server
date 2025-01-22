#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/time.h>
#include <termios.h>
#include <unistd.h>

#include "ed_common.h" 

struct termios saved_attributes;
void reset_input_mode() {
    tcsetattr(STDIN_FILENO, TCSANOW, &saved_attributes);
}
void set_noncanonical_mode() {
    tcgetattr(STDIN_FILENO, &saved_attributes);
    atexit(reset_input_mode);

    struct termios tattr;

    tcgetattr (STDIN_FILENO, &tattr);
    tattr.c_lflag &= ~(ICANON|ECHO); /* Clear ICANON and ECHO. */
    tattr.c_cc[VMIN] = 1;
    tattr.c_cc[VTIME] = 0;
    tcsetattr (STDIN_FILENO, TCSAFLUSH, &tattr);
}

int client(char* path) {
    char buf[100];
    int retval, bytes, flags;
    fd_set read_fds;

    fprintf(stderr, "[client] Starting with Path=%s\n", path);

    // Set standard input to non-canonical mode (so we don't wait for enter)
    set_noncanonical_mode();

    // And to non-blocking mode (so we can read what's available) -- done once instead of for each read.
    retval = set_nonblock(STDIN_FILENO);
    if (retval == -1) {
        fprintf(stderr, "[client] Failed to set stdin to non-blocking mode. Err=%s\n", strerror(errno));
        return 1;
    }

    // Connect to the server
    fprintf(stderr, "[client] Starting AF_UNIX on Path=%s\n", path);
    signal(SIGPIPE, SIG_IGN);
    UnixAddress *sock_address = open_unix_socket(path);
    fprintf(stderr, "[client] Opened socket with fd=%d\n", sock_address->fd);

    int isConnected = connect(sock_address->fd, (struct sockaddr *) &sock_address->addr, sizeof(struct sockaddr));
    if (isConnected == -1) {
        fprintf(stderr, "[client] Failed to connect to Path=%s. Err=%s\n", sock_address->addr.sun_path, strerror(errno));
        return 1;
    }
    
    bytes = 1;
    buf[0] = 'a';
    struct timeval tv1, tv2, tv_result;
    
    gettimeofday(&tv1, 0);
    long i;
    for (i=0; i<100000; i++) {
        write(sock_address->fd, buf, bytes);
        bytes = read(sock_address->fd, buf, sizeof(buf));
        if (bytes <= 0) {
            fprintf(stderr, "[client] server closed connection\n");
            break;
        }
    }
    gettimeofday(&tv2, 0);

    timersub(&tv2, &tv1, &tv_result);
    printf("Iterations: %ld\n", i);
    printf("Total time: %ld.%06ld\n", (long int)tv_result.tv_sec, (long int)tv_result.tv_usec);
}
