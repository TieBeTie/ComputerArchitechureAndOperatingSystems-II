/*
  В аргументах командной строки передаются:
  1) имя сервера;
  2) путь к скрипту на сервере, начинающийся с символа /;
  3) имя локального файла для отправки.

  Необходимо выполнить HTTP-POST запрос к серверу, в котором отправить содержимое файла.

  На стандартный поток ввода вывести ответ сервера (исключая заголовки).

  Запрещено использовать сторонние библиотеки.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <string.h>
#include <limits.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include <netdb.h>

typedef enum {
  PERRROR = 1,
  ERROR = -1,
  BUFFER_SIZE = 8192,
  MEGA_BUFFER_SIZE = 1000000,
  HTTP_PORT = 80
} constants;

int ConnectTo(char *server_name) {
  signal(SIGPIPE, SIG_IGN);
  struct addrinfo hints = {
      .ai_family = AF_INET,
      .ai_socktype = SOCK_STREAM
  };
  struct addrinfo *result = NULL;
  if (getaddrinfo(server_name, "http", &hints, &result)) {
    perror("Failed to get addr\n");
    exit(PERRROR);
  }

  int socket_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (connect(socket_fd, result->ai_addr, result->ai_addrlen)) {
    perror("Failed to get connect\n");
    exit(PERRROR);
  }

  return socket_fd;
}

FILE *HTTPPost(int socket_fd, char *server_name, char *path_to_script, char *path_to_file) {
  FILE *local_file = fopen(path_to_file, "r");
  if (!local_file) {
    printf("error opening local_file: \"%s\"\n", strerror(errno));
  }
  fseek(local_file, 0, SEEK_END);
  int64_t content_lenght = ftell(local_file);
  rewind(local_file);

  char *file_by_string = malloc(content_lenght + 1);
  fread(file_by_string, 1, content_lenght, local_file);
  file_by_string[content_lenght] = '\0';

  int64_t buffer_size = content_lenght + BUFFER_SIZE;
  char *request = (char *) malloc((buffer_size) * sizeof(char));
  snprintf(request,
           buffer_size,
           "POST %s HTTP/1.1\r\n"
           "Host: %s\r\n"
           "Content-Type: multipart/form-data\r\n"
           "Connection: close\r\n"
           "Content-Length: %ld\r\n\r\n"
           "%s\r\n\r\n",
           path_to_script,
           server_name,
           content_lenght,
           file_by_string);

  write(socket_fd, request, strnlen(request, buffer_size));
  fclose(local_file);
  return fdopen(socket_fd, "r");
}

void PrintHTTPFile(FILE *file) {
  char buffer[MEGA_BUFFER_SIZE];
  // skip header
  while (fgets(buffer, sizeof(buffer), file)) {
    if (strcmp(buffer, "\r\n") == 0) {
      break;
    }
  }
  while (fgets(buffer, sizeof(buffer), file)) {
    printf("%s", buffer);
  }
}

int main(int argc, char *argv[]) {
  char *server_name = argv[1];
  char *path_to_script = argv[2];
  char *path_to_file = argv[3];
  if (argc != 4) {
    printf("Invalid count of arguments\n");
  }

  int socket_fd = ConnectTo(server_name);
  FILE *file_http = HTTPPost(socket_fd, server_name, path_to_script, path_to_file);
  PrintHTTPFile(file_http);

  fclose(file_http);
}