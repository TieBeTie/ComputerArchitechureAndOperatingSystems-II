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

typedef enum {
    MY_STACK_SIZE = 16834
} constants;

typedef struct {
    int sum;
} thread_task_result_t;

volatile unsigned int reader = 0;
unsigned int N;

static thread_task_result_t *ScanfInt(void *args) {
    int value;
    int sum = 0;
    thread_task_result_t *result =
            (thread_task_result_t *) malloc(sizeof(thread_task_result_t));

    while (scanf("%d", &value) != EOF) {
        sum += value;
    }
    result->sum = sum;
    return result;
}

int main(int argc, char *argv[]) {
    N = strtol(argv[1], NULL, 10);

    pthread_attr_t thread_attr;
    pthread_attr_init(&thread_attr);
    pthread_attr_setstacksize(&thread_attr, MY_STACK_SIZE);

    pthread_t*  thread_array = (pthread_t*)malloc(N * sizeof(pthread_t));
    for (int i = 0; i < N; i++) {
        pthread_create(thread_array + i,
                       &thread_attr,
                       (void *(*)(void *)) ScanfInt,
                       0);
    }
    pthread_attr_destroy(&thread_attr);
    int sum = 0;
    for (int i = 0; i < N; i++) {
        thread_task_result_t *result;
        pthread_join(thread_array[i], (void **) &result);
        sum += result->sum;
        free(result);
    }
    free(thread_array);
    printf("%d\n", sum);
    return 0;
}