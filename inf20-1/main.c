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
#include <semaphore.h>

typedef struct {
  sem_t semaphore;
} shared_state_t;

shared_state_t *create_state() {
  shared_state_t *state = mmap(
      /* desired addr, addr = */ NULL,
      /* length = */ sizeof(shared_state_t),
      /* access attributes, prot = */ PROT_READ | PROT_WRITE,
      /* flags = */ MAP_SHARED | MAP_ANONYMOUS,
      /* fd = */ -1,
      /* offset in file, offset = */ 0
  );
  sem_init(&state->semaphore, 1, 0);
  return state;
}

void delete_state(shared_state_t *state) {
  sem_destroy(&state->semaphore);
  munmap(state, sizeof(shared_state_t));
}

typedef double (*function_t)(double);

double *pmap_process(function_t func, const double *in, size_t count) {
  int N = get_nprocs();
  pid_t pid[N];
  shared_state_t *state = create_state();
  void *mapped = mmap(
      NULL,
      count * sizeof(double),
      PROT_READ | PROT_WRITE,
      MAP_SHARED | MAP_ANONYMOUS,
      -1,
      0);
  double *result = (double *) mapped;

  for (int id_son = 0; id_son < N; ++id_son) {
    if ((pid[id_son] = fork()) == 0) {
      for (int j = id_son; j < count; j += N) {
        result[j] = func(in[j]);
      }
      sem_post(&state->semaphore);
      exit(1);
    }
  }
  int status;

  for (int i = 0; i < N; ++i) {
    sem_wait(&state->semaphore);
  }

  for (int i = 0; i < N; ++i) {
    waitpid(pid[i], &status, 0);
  }
  delete_state(state);
  return result;
}
void pmap_free(double *ptr, size_t count) {
  munmap((void *) ptr, sizeof(double) * count);
}