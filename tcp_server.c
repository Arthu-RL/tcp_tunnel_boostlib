#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <stdarg.h>
#include "vars.h"
#include <pthread.h>

void log_info(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Usage: %s <server-hostname> <port>\n", argv[0]);
        return 1;
    }

    strncpy(server_hostname, argv[1], ADDR_BUF_SIZE - 1);
    port = (uint32_t)atoi(argv[2]);

    static char buffer[BUFF_SIZE];
    int bytes_read;
    static int client_sockets[MAX_CLIENTS];

    struct protoent *protoent;
    struct sockaddr_in server_addr, client_addr;
    struct hostent *server = gethostbyname(server_hostname);
    socklen_t client_len = sizeof(client_addr);

    if (server == NULL) {
        fprintf(stderr, "ERROR: no such host as %s\n", server_hostname);
        exit(EXIT_FAILURE);
    }

    protoent = getprotobyname(protoname);
    if (protoent == NULL) {
        perror("getprotobyname");
        exit(EXIT_FAILURE);
    }

    log_info("Protocol: %s %d\n", protoent->p_name, protoent->p_proto);

    int server_socket = socket(AF_INET, SOCK_STREAM, protoent->p_proto);
    if (server_socket < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    log_info("Server Socket: %d\n", server_socket);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("listen");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
    if (client_socket < 0) {
        perror("Accept failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    log_info("Server running on addr %s:%d, forwarding to %s:%d\n", "0.0.0.0", port, server_hostname, port);


    while (1) {
        int c
    }

    close(server_socket);

    return 0;
}