/*
 * Программа при запуске сообщает на стандартный поток вывода свой PID,
 * после чего читает со стандартного потока вывода целое число - начальное значение, которое затем будет изменяться.
 *
 * При поступлении сигнала SIGUSR1 увеличить текущее значение на 1 и вывести его на стандартный поток вывода.
 * При поступлении сигнала SIGUSR2 - умножить текущее значение на -1 и вывести его на стандартный поток вывода.
 * При поступлении одного из сигналов SIGTERM или SIGINT необходимо завершить свою работу с кодом возврата 0.
 * Семантика повединия сигналов (Sys-V или BSD) считается не определенной.
 * Не забывайте выталкивать буфер вывода.
 */

#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    sigset_t full_mask;
    sigfillset(&full_mask);
    sigprocmask(SIG_BLOCK, &full_mask, NULL);

    int parent_pid = getpid();
    printf("%d\n", parent_pid);
    fflush(stdout);

    int num;
    scanf("%d", &num);

    while (1) {
        siginfo_t info;
        sigwaitinfo(&full_mask, &info);
        int received_signal = info.si_signo;
        if (received_signal == SIGUSR1) {
            num++;
            printf("%d\n", num);
            fflush(stdout);
        } else if (received_signal == SIGUSR2) {
            num *= -1;
            printf("%d\n", num);
            fflush(stdout);
        } else if (received_signal == SIGTERM || received_signal == SIGINT) {
            return 0;
        }
    }
}