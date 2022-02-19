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
    END_CHARACTERS_COUNT = 2
} constants;


//...

// должен работать до тех пор,
// пока в stop_fd не появится что-нибудь доступное для чтения

void WaitEndOfInput(int32_t in_fd, char *input) {
    char *current;
    do {
        current = input;
        while (NULL != (current = strstr(current, "\r\n"))) {
            if (*(current - 1) == '\n')
                return;
            current += END_CHARACTERS_COUNT;
        }
    } while (read(in_fd, input, BUFFER_SIZE) > 0);
}

int16_t FileStatus(char *file) {
    if (access(file, F_OK) == -1) {
        return DONT_EXIST;
    }
    if (access(file, R_OK) == -1) {
        return CANT_READ;
    }
    return ALL_OK;
}

void SendData(int32_t fd, char *file) {
    char *success = "HTTP/1.1 200 OK\r\n\0";
    char content[BUFFER_SIZE];
    char buffer[BUFFER_SIZE];

    struct stat info;
    stat(file, &info);

    uint32_t already_read = 0;
    uint32_t left_to_read = info.st_size;

    sprintf(content, "Content-Length: %lu\r\n\r\n", info.st_size);

    write(fd, success, strlen(success) * sizeof(char));
    write(fd, content, strlen(content) * sizeof(char));

    int file_fd = open(file, O_RDONLY);
    while ((already_read = read(file_fd, buffer, left_to_read)) > 0) {
        write(fd, buffer, already_read);
        left_to_read -= already_read;
    }

    close(file_fd);
}

void SendNotFound(int32_t fd) {
    char *not_found = "HTTP/1.1 404 Not Found\r\n\0";
    char *empty_content = "Content-Length: 0\r\n\r\n\0";
    write(fd, not_found, strlen(not_found) * sizeof(char));
    write(fd, empty_content, strlen(empty_content) * sizeof(char));
}

void SendForbidden(int32_t fd) {
    char *forbidden = "HTTP/1.1 403 Forbidden\r\n\0";
    char *empty_content = "Content-Length: 0\r\n\r\n\0";
    write(fd, forbidden, strlen(forbidden) * sizeof(char));
    write(fd, empty_content, strlen(empty_content) * sizeof(char));
}


//read file_name from first line
//create full path
//wait empty line from input
//check file and print answer for task

void ServerHandler(int32_t in_fd, const char *path) {
    char file_name[BUFFER_SIZE];
    char input[BUFFER_SIZE];
    memset(file_name, 0, sizeof(file_name));
    memset(input, 0, sizeof(input));


    read(in_fd, input, BUFFER_SIZE);
    sscanf(input, "GET %s HTTP/1.1", file_name);

    WaitEndOfInput(in_fd, input);

    strcpy(input, path);
    strcat(input, "/");
    strcat(input, file_name);

    int16_t status = FileStatus(input);

    if (status == ALL_OK) {
        SendData(in_fd, input);
    } else if (status == DONT_EXIST) {
        SendNotFound(in_fd);
    } else if (status == CANT_READ) {
        SendForbidden(in_fd);
    }
}

int ServerMain(int argc, char **argv, int stop_fd) {
    assert(argc >= 2);
    const char *path_to_data = argv[2];

    int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    uint16_t portnum = strtoul(argv[1], NULL, 10);
    struct sockaddr_in addr = {
            .sin_family = AF_INET, .sin_port = htons(portnum), .sin_addr = 0};
    int bind_ret = bind(socket_fd, (struct sockaddr*)&addr, sizeof(addr));
    int listen_ret = listen(socket_fd, BUFFER_SIZE);
    struct sockaddr_in peer_address = {0};
    socklen_t peer_address_size = sizeof(struct sockaddr_in);

    // Говорим что готовы принимать соединения. Не больше чем MAX_CONN_QUEUE за раз


    // создаем специальную штуку, чтобы ждать
    // события из двух файловых дескрипторов разом:
    // из сокета и из останавливающего дескриптора
    int epoll_fd = epoll_create1(0);
    {
        int fds[] = {stop_fd, socket_fd, -1};
        for (int *fd = fds; *fd != -1; ++fd) {
            struct epoll_event event = {
                    .events = EPOLLIN | EPOLLERR | EPOLLHUP,
                    .data = {.fd = *fd}
            };
            epoll_ctl(epoll_fd, EPOLL_CTL_ADD, *fd, &event);
        }
    }

    while (true) {
        // Ждем события в epoll_fd
        // (произойдет при появлении данных в stop_fd или socket_fd)
        struct epoll_event event;
        int epoll_ret = epoll_wait(epoll_fd, &event, 1, 1000);
        // Читаем события из epoll-объект (то есть из множества файловых дескриптотров, по которым есть события)
        if (epoll_ret <= 0) {
            continue;
        }
        // Если пришло событие из stop_fd - пора останавливаться
        if (event.data.fd == stop_fd) {
            break;
        }
        // Иначе пришло событие из socket_fd и accept
        // отработает мгновенно, так как уже подождали в epoll

        int connection_fd =
                accept(socket_fd, (struct sockaddr*)&peer_address, &peer_address_size);

        ServerHandler(connection_fd, path_to_data);
        // ... а тут обрабатываем соединение
        shutdown(connection_fd, SHUT_RDWR);
        close(connection_fd);
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