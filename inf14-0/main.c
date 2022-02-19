#include <sys/signalfd.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

static void transfer(FILE *from) {
    char buffer[4096];
    memset(buffer, 0, sizeof(buffer));

    fgets(buffer, sizeof(buffer), from);
    fputs(buffer, stdout);
    fflush(stdout);
}

int main(int argc, char *argv[]) {
    FILE *files[argc - 1];

    for (int i = 1; i < argc; i++) {
        files[i - 1] = fopen(argv[i], "r");
    }

    sigset_t block_mask, catch_mask;
    sigfillset(&block_mask);
    sigprocmask(SIG_BLOCK, &block_mask, NULL);

    sigemptyset(&catch_mask);
    for (int i = 0; i < argc; i++) {
        sigaddset(&catch_mask, SIGRTMIN + i);
    }
    int32_t fd = signalfd(-1, &catch_mask, 0);

    struct signalfd_siginfo sig_info;
    int32_t index = 0;
    while (1) {
        read(fd, &sig_info, sizeof(sig_info));
        index = sig_info.ssi_signo - SIGRTMIN;
        if (index == 0) {
            break;
        }
        transfer(files[index - 1]);
    }

    close(fd);
    for (int i = 1; i < argc; i++) {
        fclose(files[i - 1]);
    }

    return 0;
}