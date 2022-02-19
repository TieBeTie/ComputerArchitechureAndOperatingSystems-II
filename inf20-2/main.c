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

volatile sig_atomic_t signal_count = 0;

typedef struct {
    sem_t bartender;
    sem_t received;
    char smoker;
} shared_state_t;

int expels;

shared_state_t *state;

shared_state_t *create_state() {
    shared_state_t *state = mmap(
            /* desired addr, addr = */ NULL,
            /* length = */ sizeof(shared_state_t),
            /* access attributes, prot = */ PROT_READ | PROT_WRITE,
            /* flags = */ MAP_SHARED | MAP_ANONYMOUS,
            /* fd = */ -1,
            /* offset in file, offset = */ 0
    );
    sem_init(&state->bartender, 1, 0);
    sem_init(&state->received, 1, 0);
    return state;
}

void delete_state(shared_state_t *state) {
    sem_destroy(&state->bartender);
    sem_destroy(&state->received);
    munmap(state, sizeof(shared_state_t));
}

static void handler(int signum) {
    expels = 1;
}

int main() {
    pid_t T, P, M;
    state = create_state();
    expels = 0;
    if ((T = fork()) == 0) {
        sigaction(SIGTERM,
                  &(struct sigaction) {.sa_handler = handler, .sa_flags = SA_RESTART},
                  NULL);
        while (1) {
            sem_wait(&state->bartender);
            if (expels == 0) {
                sem_post(&state->received);
                printf("%c ", state->smoker);
                fflush(stdout);
            } else {
                exit(1);
            }
        }
    }
    if ((P = fork()) == 0) {
        sigaction(SIGTERM,
                  &(struct sigaction) {.sa_handler = handler, .sa_flags = SA_RESTART},
                  NULL);
        while (1) {
            sem_wait(&state->bartender);
            if (expels == 0) {
                sem_post(&state->received);
                printf("%c ", state->smoker);
                fflush(stdout);
            } else {
                exit(1);
            }
        }
    }
    if ((M = fork()) == 0) {
        sigaction(SIGTERM,
                  &(struct sigaction) {.sa_handler = handler, .sa_flags = SA_RESTART},
                  NULL);
        while (1) {
            sem_wait(&state->bartender);
            if (expels == 0) {
                sem_post(&state->received);
                printf("%c ", state->smoker);
                fflush(stdout);
            } else {
                exit(1);
            }
        }
    }
    int letter = 'a';
    while ((letter = getchar()) != '\n') {
        if (letter == 't') {
            state->smoker = 'T';
            sem_post(&state->bartender);
            sem_wait(&state->received);
        } else if (letter == 'p') {
            state->smoker = 'P';
            sem_post(&state->bartender);
            sem_wait(&state->received);
        } else if (letter == 'm') {
            state->smoker = 'M';
            sem_post(&state->bartender);
            sem_wait(&state->received);
        }
    }
    kill(T, SIGTERM);
    kill(P, SIGTERM);
    kill(M, SIGTERM);
    sem_post(&state->bartender);
    sem_post(&state->bartender);
    sem_post(&state->bartender);

    int status;
    waitpid(T, &status, 0);
    waitpid(P, &status, 0);
    waitpid(M, &status, 0);

    delete_state(state);
}