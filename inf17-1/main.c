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

#define conditional_handle_error(stmt, msg) \
    do { if (stmt) { perror(msg " (" #stmt ")"); exit(EXIT_FAILURE); } } while (0)

typedef enum {
    BUFFER_SIZE = 4096,
    MAX_CONN_QUEUE = 1024,
    DONT_EXIST = 1,
    CANT_READ = 2,
    ALL_OK = 3,
    END_CHARACTERS_COUNT = 2,
    MAX_FD = 1024
} constants;


//...

// должен работать до тех пор,
// пока в stop_fd не появится что-нибудь доступное для чтения

typedef struct {
    char byte;
    int label;
} ConnectionEntity;

int ServerMain(int argc, char **argv, int stop_fd) {
    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    uint16_t portnum = strtoul(argv[1], NULL, 10);
    struct sockaddr_in addr = {
            .sin_family = AF_INET, .sin_port = htons(portnum), .sin_addr = 0};
    int bind_ret = bind(socket_fd, (struct sockaddr *) &addr, sizeof(addr));
    int listen_ret = listen(socket_fd, BUFFER_SIZE);
    struct sockaddr_in peer_address = {0};
    socklen_t peer_address_size = sizeof(struct sockaddr_in);

    // Говорим что готовы принимать соединения. Не больше чем MAX_CONN_QUEUE за раз


    // создаем специальную штуку, чтобы ждать
    // события из двух файловых дескрипторов разом:
    // из сокета и из останавливающего дескриптора
    int epoll_fd = epoll_create1(0);

    int fds[MAX_FD + 2] = {stop_fd, socket_fd};
    for (int i = 0; i < 2; ++i) {
        struct epoll_event event = {
                .events = EPOLLIN | EPOLLHUP,
                .data = {.u32 = i}
        };
        epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fds[i], &event);
    }
    struct epoll_event events[MAX_FD + 2];
    ConnectionEntity entities[MAX_FD + 2];
    for (int i = 0; i < MAX_FD + 2; ++i) {
        entities[i].label = 0;
    }
    int fds_iter = 1;
    bool to_terminate = false;
    while (!to_terminate) {
        int epoll_ret = epoll_wait(epoll_fd, events, MAX_FD + 2, 1000);
        if (epoll_ret <= 0) {
            continue;
        }
        for (int i = 0; i < epoll_ret; ++i) {
            int index = events[i].data.u32;

            if (fds[index] == stop_fd) {
                to_terminate = true;
                break;
            }

            if ((events[i].events & (EPOLLIN | EPOLLHUP)) != 0) {
                if (fds[index] == socket_fd) {
                    fds[++fds_iter] =
                            accept(socket_fd, (struct sockaddr *) &peer_address, &peer_address_size);

                    struct epoll_event read_event = {
                            .events = EPOLLIN | EPOLLHUP,
                            .data = {.u32 = fds_iter}
                    };
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fds[fds_iter], &read_event);
                } else {
                    int ret = read(fds[index], &entities[index].byte, 1);
                    if (ret > 0) {
                        entities[index].byte = (char) toupper(entities[index].byte);
                        entities[index].label = ret;
                    } else if (ret == 0) {
                        if (entities[index].label == 0) {
                            entities[index].label = -1;
                        }
                    }
                    struct epoll_event write_event = {
                            .events = EPOLLOUT | EPOLLHUP,
                            .data = {.u32 = index}
                    };
                    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fds[index], &write_event);
                }
            } else if ((events[i].events & (EPOLLOUT | EPOLLHUP)) != 0) {
                if (entities[index].label > 0) {
                    write(fds[index], &entities[index].byte, 1);
                    entities[index].label = 0;

                    struct epoll_event read_event = {
                            .events = EPOLLIN | EPOLLHUP,
                            .data = {.u32 = index}
                    };
                    epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fds[index], &read_event);
                } else if (entities[index].label == -1) {
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fds[index], NULL);
                    shutdown(fds[index], SHUT_RDWR);
                    close(fds[index]);
                    fds[index] = -1;
                }
            }
        }
    }
    for (int i = 2; i < fds_iter; ++i) {
        if (fds[i] != -1) {
            shutdown(fds[i], SHUT_RDWR);
            close(fds[i]);
        }
    }

    close(epoll_fd);

    shutdown(socket_fd, SHUT_RDWR);
    close(socket_fd);

    return 0;
}


static int stop_pipe_fds[2] = {-1, -1};

static void StopSignalHandler(int signum) {
    write(stop_pipe_fds[1], "X", 1);
    // Самая первая запись одного символа в пайп пройдет всегда успешно,
    // так как буффер пуст.
}

int main(int argc, char **argv) {
    // Идея такая: в момент прихода терминирующего сигнала запишем что-то в пайп
    pipe(stop_pipe_fds);
    fcntl(stop_pipe_fds[1], F_SETFL, fcntl(stop_pipe_fds[1], F_GETFL, 0) | O_NONBLOCK);
    // Делаем запись неблокирующей,
    // чтобы никогда не зависнуть в хендлере (даже если придет 100500 сигналов)

    // Пусть при получении указанных сигналов, что-нибудь запишется в пайп
    int signals[] = {SIGINT, SIGTERM, 0};
    for (int *signal = signals; *signal; ++signal) {
        sigaction(*signal, &(struct sigaction)
                {.sa_handler = StopSignalHandler, .sa_flags = SA_RESTART}, NULL);
    }

    int return_0 = ServerMain(argc, argv, stop_pipe_fds[0]);

    close(stop_pipe_fds[0]);
    close(stop_pipe_fds[1]);
    return return_0;
}