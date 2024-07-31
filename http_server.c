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
// #include <netinet/tcp.h>
#include <errno.h>
#include <stdarg.h>
#include <pthread.h>
#include <fcntl.h>

#include "log/src/log.h"

#define ADDR_BUF_SIZE 128
#define MAX_REQUEST_SIZE 2048
#define BUF_SIZE 2048
#define ROOT "./"

void handling(int client_socket) {
    char request[MAX_REQUEST_SIZE] = {0};
    recv(client_socket, request, MAX_REQUEST_SIZE, 0);

    char method[10], path[255], protocol[20];
    sscanf(request, "%s%s%s", method, path, protocol);

    char filePath[255];
    sprintf(filePath, "%s%s", ROOT, path);

    if (strcmp(path, "/") == 0) {
        sprintf(filePath, "%s/index.html", ROOT);
    }

    int file = open(filePath, O_RDONLY);
    if (file == -1) {
        char res[] = "HTTP/1.1 404 NOT FOUND\r\n\r\n";
        send(client_socket, res, strlen(res), 0);
    } else {
        char res[] = "HTTP/1.1 200 OK\r\n\r\n";
        send(client_socket, res, strlen(res), 0);

        char buf[BUF_SIZE];
        size_t bytes_read;

        while ((bytes_read = read(file, buf, sizeof(buf))) > 0) {
            send(client_socket, buf, bytes_read, 0);
        }
        close(file);
    }

    close(client_socket);
};

int main(int argc, char **argv) {
    if (argc != 3) {
        log_info("Usage: %s <server-hostname> <port>", argv[0]);
        return 1;
    }

    const char *server_hostname = argv[1];
    const in_port_t port = (in_port_t)atoi(argv[2]);

    int server_socket, client_socket;
    // struct protoent *protoent;
    struct sockaddr_in server_addr, client_addr;
    struct hostent *server = gethostbyname(server_hostname);
    socklen_t len_addr = sizeof(struct sockaddr_in);

    if (server == NULL) {
        fprintf(stderr, "ERROR: no such host as %s\n", server_hostname);
        exit(EXIT_FAILURE);
    }

    // protoent = getprotobyname(protoname);
    // if (protoent == NULL) {
    //     perror("getprotobyname");
    //     exit(EXIT_FAILURE);
    // }

    // log_info("Protocol: %s %d\n", protoent->p_name, protoent->p_proto);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    log_info("Server Socket: %d\n", server_socket);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind fail");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 10) < 0) {
        perror("listen server socket fail");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
    // if (client_socket < 0) {
    //     perror("Accept failed");
    //     close(server_socket);
    //     exit(EXIT_FAILURE);
    // }

    log_info("Server running and listenning on addr %s:%d", server_hostname, port);


    while (1) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &len_addr);

        if (client_socket < 0) {
            perror("Client acception socket declined");
            exit(EXIT_FAILURE);
        }

        handling(client_socket);
    }

    close(server_socket);

    return 0;
}