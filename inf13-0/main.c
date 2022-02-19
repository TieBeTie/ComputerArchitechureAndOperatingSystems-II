/*
 * Программа при запуске сообщает на стандартный поток вывода свой PID,
 * выталкивает буфер вывода с помощью fflush,
 * после чего начинает обрабатывать поступающие сигналы.
 * При поступлении сигнала SIGTERM необходимо вывести
 * на стандартный поток вывода целое число:
 * количество ранее поступивших сигналов SIGINT и завершить свою работу.
 * Семантика повединия сигналов (Sys-V или BSD) считается не определенной.
 */

#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>

int main() {
    sigset_t full_mask;
    sigfillset(&full_mask);
    sigprocmask(SIG_BLOCK, &full_mask, NULL);

    int parent_pid = getpid();
    printf("%d", parent_pid);
    fflush(stdout);

    int count_sig_int = 0;
    while (1) {
        siginfo_t info;
        sigwaitinfo(&full_mask, &info);
        int received_signal = info.si_signo;
        if (received_signal == SIGINT) {
            count_sig_int++;
        } else if (received_signal == SIGTERM) {
            printf("%d\n", count_sig_int);
            fflush(stdout);
            return 0;
        }
    }
    return 0;
}