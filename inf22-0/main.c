/*
    Аргументом программы является целое число - номер порта на сервере localhost.

    Программа читает со стандартного потока ввода целые числа в тектовом формате,
    и отправляет их в бинарном виде (little-endian) на сервер как UDP-сообщение.

    В ответ сервер отправляет целое число (также в бинарном виде, little-endian),
    которое необходимо вывести на стандартный поток вывода.
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
#include <arpa/inet.h>

typedef enum {
  ERROR = -1
} constants;

int main(int argc, char *argv[]) {
  uint64_t port = strtoll(argv[1], NULL, 10);
  int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);

  if (socket_fd == ERROR) {
    perror("Socket");
    abort();
  }

  struct sockaddr_in addr = {
      .sin_family = AF_INET,
      .sin_addr = inet_addr("127.0.0.1"),
      .sin_port = htons(port)
  };

  int send_number, receive_number;
  while (scanf("%d", &send_number) > 0) {
    sendto(socket_fd,
           &send_number, sizeof(send_number),
           0,
           (const struct sockaddr *) &addr, sizeof(addr));
    recvfrom(socket_fd,
             &receive_number, sizeof(receive_number),
             0,
             NULL, NULL);
    printf("%d\n", receive_number);
  }
  close(socket_fd);
}
