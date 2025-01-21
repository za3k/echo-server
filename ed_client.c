#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
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

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(STDIN_FILENO, &read_fds);
        FD_SET(sock_address->fd, &read_fds);
        retval = select(sock_address->fd+1, &read_fds, 0, 0, 0);
        if (retval <= 0) {
            fprintf(stderr, "[client] select() problem\n");
            return 1;
        }

        if (FD_ISSET(sock_address->fd, &read_fds)) {
            // Read from network
            set_nonblock(sock_address->fd);
            bytes = read(sock_address->fd, buf, sizeof(buf));
            if (bytes <= 0) {
                fprintf(stderr, "[client] server closed connection\n");
                return 0;
            }
            set_block(sock_address->fd);
            // Write to standard output
            write(STDOUT_FILENO, buf, bytes);
        }
        if (FD_ISSET(STDIN_FILENO, &read_fds)) {
            // Read from standard input
            bytes = read(STDIN_FILENO, buf, sizeof(buf)); // TODO: Handle \004, C-d
            if (bytes == -1) return EXIT_SUCCESS;
            // Write to network
            if (-1 == write(sock_address->fd, buf, bytes)) {
                fprintf(stderr, "[client] server closed connection\n");
                return 0;
            }
        }
    }
}
