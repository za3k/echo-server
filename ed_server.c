#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "ed_common.h"


static int clean_fd;
static char *clean_path;
void _abort(int code) {
    cleanup(clean_fd, clean_path);
    exit(code);
}
void clean_handler(int sig) { _abort(1); }


void remove_client(int *client_fds, int *clients, int removed) {
    // client_fds[removed] is invalid now. Shift things over to cover the gap
    for (int s=removed; s<(*clients)-1; s++) client_fds[s] = client_fds[s+1];
    (*clients)--;
}

void wait(long long delay_us) {
    struct timeval tv;
    tv.tv_usec = delay_us % 1000000;
    tv.tv_sec = delay_us / 1000000;
    select(0, 0, 0, 0, &tv);
}


int server(char *path, long delay) {
    fprintf(stderr, "[server] Starting with Path=%s\n", path);

    int client_fds[10];
    int clients=0;
    char buf[100];
    int bytes;
    fd_set read_fds;

    for (int fdi=0; fdi<10; fdi++) client_fds[fdi] = -1;

    signal(SIGPIPE, SIG_IGN); // If a client disconnects, we don't want to shut down, just handle it.

    fprintf(stderr, "[server] Starting AF_UNIX on Path=%s\n", path);
    UnixAddress *sock_address = open_unix_socket(path);
    fprintf(stderr, "[server] Opened socket with fd=%d\n", sock_address->fd);
    clean_fd = sock_address->fd;
    clean_path = path;
    signal(SIGINT, clean_handler);

    int hasBind = bind(sock_address->fd, (struct sockaddr *) &sock_address->addr, sizeof(struct sockaddr));
    if (hasBind == -1) {
        fprintf(stderr, "[server] Failed to bind AF_UNIX socket on Path=%s. Err=%s\n", path, strerror(errno));
        _abort(1);
    }

    int isListening = listen(sock_address->fd, 10);
    if (isListening == -1) {
        fprintf(stderr, "[server] Failed to connect to Path=%s. Err=%s\n", sock_address->addr.sun_path, strerror(errno));
        _abort(1);
    }

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(sock_address->fd, &read_fds); // Listen for new connections
        int mfid = sock_address->fd;
        for (int fdi=0; fdi<clients; fdi++) {
            FD_SET(client_fds[fdi], &read_fds); // Listen for incoming messages
            if (client_fds[fdi] > mfid) mfid = client_fds[fdi];
        }

        int retval = select(mfid+1, &read_fds, 0, 0, 0);
        if (retval <= 0) {
            fprintf(stderr, "[server] select() failed");
            _abort(1);
        }

        if (FD_ISSET(sock_address->fd, &read_fds)) {
            // A new client connected
            int connFd = accept(sock_address->fd, 0, 0);
            if (connFd == -1) {
                fprintf(stderr, "[server] Error while accepting connection. Error=%s\n", strerror(errno));
                _abort(1);
            }
            client_fds[clients] = connFd;
            clients++;
            fprintf(stderr, "[server] New client connected. Num=%d, Fd=%d.\n", clients, connFd);
            continue; // Call select(2) again to check for any *additional* new clients, to make sure we get them all.
        }
        for (int fdi=0; fdi<clients; fdi++) {
            if (FD_ISSET(client_fds[fdi], &read_fds)) {
                //fprintf(stderr, "[server] Data received from client. Fd=%d\n", client_fds[fdi]);
                // Message received from a client
                // Read from network, one client
                set_nonblock(client_fds[fdi]);
                bytes = read(client_fds[fdi], buf, sizeof(buf));
                set_block(client_fds[fdi]);
                if (bytes == -1) {
                    // Client connection closed
                    fprintf(stderr, "[server] Client disconnected. Fd=%d\n", client_fds[fdi]);
                    remove_client(client_fds, &clients, fdi);
                    continue;
                }
                wait(delay);
                // Write to network, all clients (including sender)
                for (int toi=0; toi<clients; toi++) {
                    int success = write(client_fds[toi], buf, bytes);
                    if (success == -1) remove_client(client_fds, &clients, toi);
                }
            }
        }

        if (clients == 0) { // Exit when the last client disconnects
            fprintf(stderr, "[server] All clients disconnected.\n");
            _abort(EXIT_SUCCESS);
        }
    }

    _abort(2);
}
