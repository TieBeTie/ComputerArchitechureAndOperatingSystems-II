#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/signalfd.h>
#include <unistd.h>
#include <stdlib.h>

volatile sig_atomic_t must_exit = 0;
volatile sig_atomic_t val = 0;

void handler(int sig, siginfo_t* info, void* ucontext)
{
    if (info->si_value.sival_int == 0) {
        must_exit = 1;
    } else {
        union sigval value;
        val = info->si_value.sival_int - 1;
        printf("%d\n", val);
        value.sival_int = info->si_value.sival_int - 1;
        sigqueue(info->si_pid, sig, value);
    }
}

int main(int argc, char* argv[])
{

    struct sigaction action_signal;
    memset(&action_signal, 0, sizeof(action_signal));
    action_signal.sa_sigaction = handler;
    action_signal.sa_flags = SA_RESTART | SA_SIGINFO;
    sigaction(SIGRTMIN, &action_signal, NULL);

    sigset_t mask;
    sigfillset(&mask);
    sigdelset(&mask, SIGRTMIN);
    sigprocmask(SIG_BLOCK, &mask, NULL);

    union sigval value;
    value.sival_int = 6;
    val = 6;

    printf("%d", atoi(argv[1]));
    sigqueue(atoi(argv[1]), SIGRTMIN, value);

    while (!must_exit) {
        pause();
    }

    return 0;
}
