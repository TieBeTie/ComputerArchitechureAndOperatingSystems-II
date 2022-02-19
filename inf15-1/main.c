#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
    char* cmd1 = argv[1];
    char* cmd2 = argv[2];
    int fd[2];
    pipe(fd);
    pid_t pid_1, pid_2;

    if ((pid_1 = fork()) == 0) {
        dup2(fd[1], 1);
        close(fd[0]);
        close(fd[1]);
        system(cmd1);
        return 0;
    }
    close(fd[1]);

    if ((pid_2 = fork()) == 0) {
        dup2(fd[0], 0);
        close(fd[0]);
        system(cmd2);
        return 0;
    }
    close(fd[0]);

    int status;
    assert(waitpid(pid_1, &status, 0) != -1);
    assert(waitpid(pid_2, &status, 0) != -1);
    return 0;
}