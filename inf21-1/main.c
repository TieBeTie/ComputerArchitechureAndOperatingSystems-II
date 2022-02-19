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


typedef struct {
    sem_t request_ready;  // начальное значение 0
    sem_t response_ready; // начальное значение 0
    char func_name[20];
    double value;
    double result;
} shared_data_t;

int main(int argc, char *argv[]) {
    char shared_memory[NAME_MAX] = "/TieBeTie(ami-ypech-25)";

    printf("%s\n", shared_memory);
    fflush(stdout);

    int shm_id = shm_open(shared_memory, O_RDWR | O_CREAT, 0642);
    ftruncate(shm_id, sizeof(shared_data_t));
    shared_data_t *semaphores = mmap(NULL,
                                     sizeof(shared_data_t),
                                          PROT_READ | PROT_WRITE,
                                     MAP_SHARED,
                                     shm_id,
                                     0);
    close(shm_id);

    sem_init(&semaphores->response_ready, 0642, 0);
    sem_init(&semaphores->request_ready, 0642, 0);

    void *exec_func;
    exec_func = dlopen(argv[1], RTLD_LAZY);
    // проверяем открылась ли разделяемая бибилотека
    if (!exec_func) {
        sem_close(&semaphores->response_ready);
        sem_close(&semaphores->request_ready);
        munmap(semaphores, sizeof(shared_data_t));
        shm_unlink(shared_memory);
        dlclose(exec_func);
        return 0;
    }

    while (1) {
        sem_wait(&semaphores->request_ready);
        if (strlen(semaphores->func_name) == 0) {
            break;
        }
        double (*ret_func) (double) = dlsym(exec_func, semaphores->func_name);
        semaphores->result = ret_func(semaphores->value);
        fflush(stdout);
        sem_post(&semaphores->response_ready);
    }

    sem_close(&semaphores->response_ready);
    sem_close(&semaphores->request_ready);
    munmap(semaphores, sizeof(shared_data_t));
    shm_unlink(shared_memory);
    dlclose(exec_func);
    return 0;
}