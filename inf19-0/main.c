//Программа запускается с двумя целочисленными аргументами: N>0 - количество итераций; и k>0 - количество потоков.
//
//Необходимо создать массив из k вещественных чисел, после чего запустить k потоков, каждый из которых работает со своим элементом массива и двумя соседними.
//
//Каждый поток N раз увеличивает значение своего элемента на 1, увеличивает значение соседа слева на 0.99, и увеличивает значение соседа справа на 1.01.
//
//Для потоков, у которых нет соседей справа (k-1) или слева (0), соседними считать первое и последнее значение массива соответственно.
//
//После того, как все потоки проведут N итераций, необходимо вывести значения всех элементов.
//
//Запрещено использовать глобальные переменные.
//
//Для вывода используйте формат %.10g.

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <stdint.h>
#include <sys/mman.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <sys/wait.h>
#include <assert.h>
#include <limits.h>

typedef enum {
  BUFFER_SIZE = 16834
} constants;

typedef struct {
  uint64_t N;
  uint64_t k;
  uint64_t index;
  double *array;
  pthread_mutex_t *mutex;
  pthread_t thread;
} thread_args;

void OrderSwap(uint64_t *a) {
  uint64_t tmp = *(a + 1);
  if (*(a + 1) < *a) {
    *(a + 1) = *a;
    *a = tmp;
  }
}

void Sort3Values(uint64_t *a) {
  OrderSwap(a);
  OrderSwap(a + 1);
  OrderSwap(a);
}

void *ChangeThreeValues(void *arg) {
  thread_args *thread_arg = arg;
  uint64_t I_am = thread_arg->index;
  uint64_t threads_count = thread_arg->k;

  uint64_t left = I_am - 1;
  if (I_am == 0) {
    left = threads_count - 1;
  }

  uint64_t right = I_am + 1;
  if (I_am == threads_count - 1) {
    right = 0;
  }
  // идея в том, чтобы не создать случая, когда к примеру
  // блокируется 0, блокируется 1
  // блокируется 1, потом 0 и тут уже дедлок
  uint64_t trio[3] = {left, I_am, right};
  // то есть можно просто отсортировывать индексы
  Sort3Values(trio);

  for (size_t i = 0; i < thread_arg->N; ++i) {
    // lock order IMPORTANT
    pthread_mutex_lock(&thread_arg->mutex[trio[0]]);
    pthread_mutex_lock(&thread_arg->mutex[trio[1]]);
    pthread_mutex_lock(&thread_arg->mutex[trio[2]]);
    thread_arg->array[left] += 0.99;
    thread_arg->array[I_am] += 1;
    thread_arg->array[right] += 1.01;
    // unlock no
    pthread_mutex_unlock(&thread_arg->mutex[trio[0]]);
    pthread_mutex_unlock(&thread_arg->mutex[trio[1]]);
    pthread_mutex_unlock(&thread_arg->mutex[trio[2]]);
  }
  return 0;
}

int main(int argc, char *argv[]) {
  uint64_t N = strtol(argv[1], NULL, 10);
  uint64_t k = strtol(argv[2], NULL, 10);

  thread_args threads[k];
  double array[k];
  for (uint64_t i = 0; i < k; ++i) {
    array[i] = 0;
  }
  // init threads with mutexes
  pthread_attr_t attr[k];
  pthread_mutex_t mutexes[k];
  for (uint64_t i = 0; i < k; ++i) {
    pthread_attr_init(attr + i);
    pthread_attr_setguardsize(attr + i, 0);
    pthread_attr_setstacksize(attr + i, PTHREAD_STACK_MIN);
    pthread_mutex_init(mutexes + i, NULL);
  }
  // put function to thread
  for (uint64_t index = 0; index < k; ++index) {
    threads[index].k = k;
    threads[index].N = N;
    threads[index].index = index;
    threads[index].array = array;
    threads[index].mutex = mutexes;
    pthread_create(&threads[index].thread, attr + index, ChangeThreeValues, &threads[index]);
  }
  // perform
  for (uint64_t i = 0; i < k; ++i) {
    pthread_join(threads[i].thread, NULL);
  }
  // die
  for (uint64_t i = 0; i < k; ++i) {
    pthread_attr_destroy(attr + i);
    printf("%.10g\n", array[i]);
  }
}