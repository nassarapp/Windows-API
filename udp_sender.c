#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: %s <host> <port> <message>\n", argv[0]);
        return 1;
    }

    const char *host = argv[1];
    int port = atoi(argv[2]);
    const char *message = argv[3];

    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port: %s\n", argv[2]);
        return 1;
    }

    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    struct hostent *server = gethostbyname(host);
    if (server == NULL) {
        fprintf(stderr, "Failed to resolve host: %s\n", host);
        close(sock);
        return 1;
    }

    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(port);
    memcpy(&dest_addr.sin_addr.s_addr, server->h_addr, server->h_length);

    ssize_t sent = sendto(
        sock,
        message,
        strlen(message),
        0,
        (struct sockaddr *)&dest_addr,
        sizeof(dest_addr)
    );

    if (sent < 0) {
        perror("sendto");
        close(sock);
        return 1;
    }

    printf("Sent %zd bytes to %s:%d\n", sent, host, port);

    close(sock);
    return 0;
}
