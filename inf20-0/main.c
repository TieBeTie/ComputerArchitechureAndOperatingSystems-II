#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <stdint.h>
#include <limits.h>
#include <sys/mman.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include <sys/wait.h>
#include <assert.h>
#include <semaphore.h>
#include <fcntl.h>

typedef struct {
  sem_t semaphore;
} shared_state_t;

int main(int argc, char *argv[]) {
  char semaphore_name[NAME_MAX];
  char shm_name[NAME_MAX];
  uint64_t N;
  scanf("%s %s %ld", semaphore_name, shm_name, &N);

  int fd = shm_open(shm_name, O_RDWR, 0644);
  ftruncate(fd, sizeof(shared_state_t));
  shared_state_t* state = mmap(
      /* desired addr, addr = */ NULL,
      /* length = */ N * sizeof(int),
      /* access attributes, prot = */ PROT_READ,
      /* flags = */ MAP_SHARED,
      /* fd = */ fd,
      /* offset in file, offset = */ 0
  );
  int *array = (int *) state;
  sem_t *sem = sem_open(semaphore_name, 0);
  sem_wait(sem);
  for (uint64_t i = 0; i < N; ++i) {
    printf("%d ", array[i]);
  }
  close(fd);
  munmap(state, N * sizeof(int));
}