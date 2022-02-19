#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <wait.h>
#include <sys/epoll.h>
#include <assert.h>
#include <arpa/inet.h>

#define conditional_handle_error(stmt, msg) \
    do { if (stmt) { perror(msg " (" #stmt ")"); exit(EXIT_FAILURE); } } while (0)

typedef enum {
    BUFFER_SIZE = 4096,
    MAX_CONN_QUEUE = 1024,
    DONT_EXIST = 1,
    CANT_READ = 2,
    ALL_OK = 3,
    END_CHARACTERS_COUNT = 2
} constants;

void ServerHandler(int32_t out_fd) {
    int32_t send_num;
    int32_t receive_num;
    while (scanf("%d", &send_num) != EOF) {
        if (write(out_fd, &send_num, sizeof(send_num)) <= 0) {
            break;
        }
        if (read(out_fd, &receive_num, sizeof(receive_num)) <= 0) {
            break;
        }

        printf("%d ", receive_num);
        fflush(stdout);
    }
}

int ServerMain(int argc, char **argv) {
    in_addr_t ip = inet_addr(argv[1]);
    uint16_t port = strtoul(argv[2], NULL, 10);

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {
            .sin_family = AF_INET, .sin_port = htons(port), .sin_addr = ip};

    connect(socket_fd, (const struct sockaddr *) &addr, sizeof(addr));

    ServerHandler(socket_fd);
    close(socket_fd);
    return 0;
}

int main(int argc, char **argv) {
    int return_0 = ServerMain(argc, argv);
    return return_0;
}