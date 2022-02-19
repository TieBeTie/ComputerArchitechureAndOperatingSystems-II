//  Программе передается два аргумента: имя файла с библиотекой и имя функции из этой библиотеки.
//
//  Гарантируется, что функция имеет сигнатуру:
//
//  double function(double argument);
//  На стандартном потоке ввода подаются вещественные числа. Необходимо применить к ним эту функцию, и вывести полученные значения. Для однозначности вывода используйте формат %.3f.

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
#include <limits.h>

typedef enum {
  MY_STACK_SIZE = 16834
} constants;

int main(int argc, char *argv[]) {
  char *shared_library = argv[1];
  char *shared_function = argv[2];

  void *exec_func;
  exec_func = dlopen(shared_library, RTLD_NOW);
  // проверяем открылась ли разделяемая бибилотека
  if (!exec_func) {
    dlclose(exec_func);
    return 0;
  }
  double value;
  while (scanf("%lf", &value) > 0) {
    double (*ret_func) (double) = dlsym(exec_func, shared_function);
    value = ret_func(value);
    printf("%.3f\n", value);
  }


  dlclose(exec_func);
  return 0;
}