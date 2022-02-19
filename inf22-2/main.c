/*
    Программа читает со стандартного потока последовательность лексем - имен хостов.

    Необходимо для каждого имени сформировать UDP-запрос к DNS-серверу 8.8.8.8 для того,
    чтобы получить IP-адрес сервера для записи типа A.
    Далее - получить ответ от сервера и вывести IP-адрес на стандартный поток вывода.

    Гарантируется, что для каждого запроса существует ровно 1 IP-адрес.

    Указание: используйте инструменты dig и wireshark для того,
    чтобы исследовать формат запросов и ответов.

    Примеры
    Входные данные

    ejudge.ru
    ejudge.atp-fivt.org


    Результат работы

    89.108.121.5
    87.251.82.74
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/time.h>
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
#include <netdb.h>
#include <arpa/inet.h>

typedef enum {
  ERROR = -1,
  BUFFER_SIZE = 8192,
  HEAD_SIZE = 12,
  TALE_SIZE = 5,
  DNS_PORT = 53
} constants;

int main() {
  int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (socket_fd == ERROR) {
    perror("Socket");
    abort();
  }

  char google_public_dns[] = "8.8.8.8";
  struct sockaddr_in addr = {
      .sin_family = AF_INET,
      .sin_addr = inet_addr(google_public_dns),
      .sin_port = htons(DNS_PORT)
  };

  unsigned char buffer[BUFFER_SIZE];
  unsigned char head[HEAD_SIZE] = {0xAA, 0xAA, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  unsigned char request[BUFFER_SIZE];
  char answer[BUFFER_SIZE];
  unsigned char tale[TALE_SIZE] = {0x00, 0x00, 0x01, 0x00, 0x01};


  unsigned char host[BUFFER_SIZE];
  while (scanf("%s", &host) > 0) {
    // request = 1) header + 2) host + 3)
    memset(request, 0, sizeof(request));
    int request_iter = 0;
    // 1) header
    for (; request_iter < HEAD_SIZE; ++request_iter) {
      request[request_iter] = head[request_iter];
    }
    // 2) host
    int host_iter = 0;
    int j = 0;
    memset(buffer, 0, sizeof(buffer));
    int buffer_iter = 0;

    while (host_iter < strlen(host)) {
      if (host[host_iter] == '.') {
        buffer_iter = 0;
        request[request_iter++] = strlen(buffer);
        // len(buffer) + buffer
        while (buffer_iter < strlen(buffer)) {
          request[request_iter++] = buffer[buffer_iter++];
        }

        memset(buffer, 0, sizeof(buffer));
        ++host_iter;
        j = 0;
        continue;
      }
      buffer[j++] = host[host_iter++];
    }
    // buffer
    buffer_iter = 0;
    request[request_iter++] = strlen(buffer);
    while (buffer_iter < strlen(buffer)) {
      request[request_iter++] = buffer[buffer_iter++];
    }

    int tale_iter = 0;
    while (tale_iter < TALE_SIZE) {
      request[request_iter++] = tale[tale_iter++];
    }

    sendto(socket_fd,
           &request,
           request_iter,
           0,
           (const struct sockaddr *) &addr,
           sizeof(addr));

    unsigned char response[BUFFER_SIZE];
    size_t total_read = read(socket_fd, response, sizeof(response));
    strcpy(answer, inet_ntoa(*(struct in_addr *)(response + (total_read - 4))));
    printf("%s\n", answer);
  }
  close(socket_fd);
}