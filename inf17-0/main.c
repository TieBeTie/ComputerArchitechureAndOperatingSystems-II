//  Реализуйте функцию с сигнатурой:
//
//  extern size_t
//  read_data_and_count(size_t N, int in[N])
//
//
//  которая читает данные из файловых дескрипторов in[X] для всех 0 ≤ X < N ,
//  и возвращает суммарное количество прочитанных байт из всех файловых дескрипторов.
//
//  Скорость операций ввода-вывода у файловых дескрипторов - случайная.
//  Необходимо минимизировать суммарное астрономическое время чтения данных.
//
//  По окончании чтения необходимо закрыть все файловые дескрипторы.
//
//  Указание: используйте неблокирующий ввод-вывод. Для тестирования можно использовать socketpair.


#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/wait.h>

typedef enum {
  BUFFER_SIZE = 4096,
  MAX_PENDING_EVENTS = 8192
} constants;

typedef struct {
  int fd;
  ssize_t bytes_read;
  bool is_received;
} data_t;

extern size_t
read_data_and_count(size_t N, int32_t in[N]) {
  int epoll_fd = epoll_create1(0);
  data_t **files_data = calloc(N, sizeof(*files_data));

  for (int i = 0; i < N; ++i) {
    // установка неблокируемого ввода-вывода на дискрипторы
    int fd = in[i];
    int flags_fd = fcntl(fd, F_GETFL);
    // флаги in[i] с O_NONBLOCK
    fcntl(fd, F_SETFL, flags_fd | O_NONBLOCK);

    data_t *data = calloc(1, sizeof(*data));
    data->fd = fd;
    struct epoll_event event;
    // Ассоциированный файл доступен для операций read(2).
    event.events = EPOLLIN;
    event.data.ptr = data;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &event);

    files_data[i] = data;
  }

  size_t remaining_fds = N;
  struct epoll_event triggered_events[MAX_PENDING_EVENTS];

  while (remaining_fds > 0) {
    int triggered_events_count = epoll_wait(epoll_fd, triggered_events, MAX_PENDING_EVENTS, -1);
    for (size_t i = 0; i < triggered_events_count; ++i) {

      uint32_t mask = triggered_events[i].events;
      data_t *data = triggered_events[i].data.ptr;

      if (mask & EPOLLIN) {
        char buffer[BUFFER_SIZE];
        ssize_t bytes_read = read(data->fd, buffer, sizeof(buffer));

        if (bytes_read > 0) {
          data->bytes_read += bytes_read;
        } else if (bytes_read == 0) {
          data->is_received = true;
          close(data->fd);
        }
      }

      if (data->is_received) {
        remaining_fds--;
      }
    }
  }
  close(epoll_fd);

  uint64_t total_bytes = 0;
  for (int32_t i = 0; i < N; ++i) {
    total_bytes += files_data[i]->bytes_read;
    free(files_data[i]);
  }

  return total_bytes;
}