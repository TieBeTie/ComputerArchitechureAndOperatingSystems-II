//  Программа принимает три аргумента: два 64-битных числа A и B, и 32-битное число N.
//
//  Затем создается дополнительный поток, которые генерирует простые числа в диапазоне от A до B включительно, и сообщает об этом основному потоку, с которого началось выполнение функции main.
//
//  Главный поток выводит на стандартный поток вывода каждое полученное число и завершает свою работу после того, как получит N чисел.
//
//  Запрещено использовать глобальные переменные.

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <inttypes.h>
#include <stdatomic.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <assert.h>
#include <limits.h>

typedef enum {
  MY_STACK_SIZE = 16834
} constants;

typedef struct {
  uint64_t A;
  uint64_t B;
  pthread_cond_t *cv;
  int *ready;
  int pipe;
} args_t;

void *GeneratePrimes(void *args) {
  uint64_t A = ((args_t *) args)->A;
  uint64_t B = ((args_t *) args)->B;
  pthread_cond_t *cv = ((args_t *) args)->cv;
  int *ready = ((args_t *) args)->ready;
  int pipe = ((args_t *) args)->pipe;

  if (A == 1) {
    A++;
  }
  while (A <= B) {
    uint64_t flag = 1;
    for (int i = 2; i * i <= A; i++) {
      if (A % i == 0) {
        flag = 0;
      }
    }
    if (flag) {
      write(pipe, &A, sizeof(A));
      *ready += 1;
      pthread_cond_signal(cv);
    }
    A++;
  }
  return 0;
}

int main(int argc, char *argv[]) {
  uint64_t A = strtoll(argv[1], NULL, 10);
  uint64_t B = strtoll(argv[2], NULL, 10);
  uint32_t N = strtol(argv[3], NULL, 10);
  pthread_mutex_t mutex;
  pthread_mutex_init(&mutex, NULL);

  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setguardsize(&attr, 0);
  pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN);

  pthread_t thread;
  pthread_cond_t cv;
  pthread_cond_init(&cv, NULL);

  int pipes[2];
  socketpair(PF_LOCAL, SOCK_STREAM, 0, pipes);

  int ready = 0;
  args_t args = {
      .cv = &cv,
      .A = A,
      .B = B,
      .pipe = pipes[1],
      .ready = &ready,
  };
  pthread_mutex_lock(&mutex);
  pthread_create(&thread, &attr, GeneratePrimes, &args);

  uint64_t value;
  for (int i = 0; i < N; i++) {
    while (ready == 0) {
      pthread_cond_wait(&cv, &mutex);
    }
    ready--;
    read(pipes[0], &value, sizeof(value));
    printf("%ld" "\n", value);
  }
  pthread_mutex_unlock(&mutex);
  pthread_cond_destroy(&cv);
  pthread_mutex_destroy(&mutex);
}