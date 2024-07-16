#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <errno.h>
#include <stdarg.h>

#define ADDR_BUF_SIZE 128

void log_info(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

int main(int argc, char **argv) {
    char buffer[BUFSIZ];
    const char* protoname = "tcp";
    struct protoent *protoent;
    char server_hostname[ADDR_BUF_SIZE] = "0.0.0.0";
    uint32_t port = 25565;

    if (argc == 3) {
        memset(server_hostname, 0, ADDR_BUF_SIZE);
        strncpy(server_hostname, argv[1], ADDR_BUF_SIZE - 1);

        port = (uint32_t)atoi(argv[2]);
    }

    // SERVER
    protoent = getprotobyname(protoname);
    if (protoent == NULL) {
        perror("getprotobyname");
        exit(EXIT_FAILURE);
    }

    int server_socket = socket(AF_INET, SOCK_STREAM, protoent->p_proto);
    if (server_socket < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // CLIENT
    int client_socket = socket(AF_INET, SOCK_STREAM, protoent->p_proto);
    if (client_socket < 0) {
        perror("socket");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // SERVER
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_socket);
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 20) < 0) {
        perror("listen");
        close(server_socket);
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    // CLIENT
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_conn = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
    if (client_conn < 0) {
        perror("accept");
        close(server_socket);
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    // SERVER
    struct sockaddr_in target_addr;
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(port);

    struct hostent *server = gethostbyname(server_hostname);
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host\n");
        close(client_conn);
        close(server_socket);
        close(client_socket);
        exit(EXIT_FAILURE);
    }
    memcpy(&target_addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);

    // CLIENT
    if (connect(client_socket, (struct sockaddr*)&target_addr, sizeof(target_addr)) < 0) {
        perror("connect");
        close(client_conn);
        close(server_socket);
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    fd_set read_fds;
    int max_fd = (client_conn > client_socket) ? client_conn : client_socket;

    log_info("Server running on addr %s:%d, in Minecraft, type the addr public_ip:%d to enter the server\n", server_hostname, 25565, 25565);

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(client_conn, &read_fds);
        FD_SET(client_socket, &read_fds);

        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("select");
            break;
        }

        if (FD_ISSET(client_conn, &read_fds)) {
            int n = read(client_conn, buffer, sizeof(buffer));
            if (n <= 0) break;
            write(client_socket, buffer, n);
        }

        if (FD_ISSET(client_socket, &read_fds)) {
            int n = read(client_socket, buffer, sizeof(buffer));
            if (n <= 0) break;
            write(client_conn, buffer, n);
        }
    }

    close(client_conn);
    close(client_socket);
    close(server_socket);

    return 0;
}
