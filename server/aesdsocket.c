#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <syslog.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define PORT 9000
#define DATAFILE "/var/tmp/aesdsocketdata"
#define BUFSIZE 1024

static int server_fd = -1;
static int client_fd = -1;
static int caught_signal = 0;

void signal_handler(int sig) {
    caught_signal = 1;
}

void cleanup() {
    syslog(LOG_INFO, "Caught signal, exiting");
    if (client_fd != -1) close(client_fd);
    if (server_fd != -1) close(server_fd);
    remove(DATAFILE);
    closelog();
}

int main(int argc, char *argv[]) {
    int daemon_mode = 0;
    if (argc == 2 && strcmp(argv[1], "-d") == 0)
        daemon_mode = 1;

    openlog("aesdsocket", LOG_PID, LOG_USER);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = signal_handler;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        syslog(LOG_ERR, "socket failed: %s", strerror(errno));
        return -1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        syslog(LOG_ERR, "bind failed: %s", strerror(errno));
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, 10) == -1) {
        syslog(LOG_ERR, "listen failed: %s", strerror(errno));
        close(server_fd);
        return -1;
    }

    if (daemon_mode) {
        pid_t pid = fork();
        if (pid < 0) { close(server_fd); return -1; }
        if (pid > 0) exit(0);
        setsid();
    }

    while (!caught_signal) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd == -1) {
            if (caught_signal) break;
            continue;
        }

        char *client_ip = inet_ntoa(client_addr.sin_addr);
        syslog(LOG_INFO, "Accepted connection from %s", client_ip);

        // Receive data and append to file
        char buf[BUFSIZE];
        int fd = open(DATAFILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
        if (fd == -1) {
            close(client_fd);
            continue;
        }

        ssize_t bytes;
        int found_newline = 0;
        while (!found_newline && (bytes = recv(client_fd, buf, BUFSIZE, 0)) > 0) {
            if (write(fd, buf, bytes) == -1) break;
            if (memchr(buf, '\n', bytes)) found_newline = 1;
        }
        // Ensure data is flushed to disk before reading back
        fsync(fd);
        close(fd);

        // Send file content back to client
        fd = open(DATAFILE, O_RDONLY);
        if (fd != -1) {
            while ((bytes = read(fd, buf, BUFSIZE)) > 0)
                send(client_fd, buf, bytes, 0);
            close(fd);
        }

        syslog(LOG_INFO, "Closed connection from %s", client_ip);
        close(client_fd);
        client_fd = -1;
    }

    cleanup();
    return 0;
}
