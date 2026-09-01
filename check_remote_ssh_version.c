#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#define BUFFER_SIZE 1024
#define SSH_DEFAULT_PORT "22"
#define CONNECT_TIMEOUT_SEC 5

static void print_usage(const char *program)
{
    fprintf(stderr, "Usage: %s <hostname|IP> [port]\n", program);
    fprintf(stderr, "  Default port is %s\n", SSH_DEFAULT_PORT);
}

int main(int argc, char *argv[])
{
    if (argc < 2 || argc > 3) {
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    const char *host = argv[1];
    const char *port = (argc == 3) ? argv[2] : SSH_DEFAULT_PORT;

    struct addrinfo hints = {0};
    struct addrinfo *res = NULL;

    hints.ai_family = AF_UNSPEC;      // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;

    int gai_err = getaddrinfo(host, port, &hints, &res);
    if (gai_err != 0) {
        fprintf(stderr, "Error resolving %s: %s\n", host, gai_strerror(gai_err));
        return EXIT_FAILURE;
    }

    int sock = -1;
    struct addrinfo *rp;
    for (rp = res; rp != NULL; rp = rp->ai_next) {
        sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
        if (sock == -1) {
            continue;
        }

        struct timeval tv;
        tv.tv_sec = CONNECT_TIMEOUT_SEC;
        tv.tv_usec = 0;
        if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1 ||
            setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) == -1) {
            perror("setsockopt");
            close(sock);
            freeaddrinfo(res);
            return EXIT_FAILURE;
        }

        if (connect(sock, rp->ai_addr, rp->ai_addrlen) == 0) {
            break; // connected
        }

        close(sock);
        sock = -1;
    }
    freeaddrinfo(res);

    if (sock == -1) {
        fprintf(stderr, "Could not connect to %s on port %s: %s\n", host, port, strerror(errno));
        return EXIT_FAILURE;
    }

    char buffer[BUFFER_SIZE];
    memset(buffer, 0, sizeof(buffer));

    ssize_t received = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (received == -1) {
        perror("recv");
        close(sock);
        return EXIT_FAILURE;
    }
    if (received == 0) {
        fprintf(stderr, "Connection closed by %s before receiving banner.\n", host);
        close(sock);
        return EXIT_FAILURE;
    }

    // SSH banner line ends with \r\n (RFC 4253).
    char *newline = strpbrk(buffer, "\r\n");
    if (newline) {
        *newline = '\0';
    }

    if (strncmp(buffer, "SSH-", 4) != 0) {
        fprintf(stderr, "Warning: received banner does not look like an SSH banner:\n  %s\n", buffer);
    }

    printf("%s\n", buffer);

    close(sock);
    return EXIT_SUCCESS;
}
