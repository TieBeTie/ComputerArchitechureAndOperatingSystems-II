#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    char *cmd = argv[1];
    char *input_name = argv[2];

    int32_t fd[2];
    pipe(fd);
    pid_t pid = fork();

    if (pid == 0) {
        close(fd[0]);

        int in = open(input_name, O_RDONLY);
        dup2(in, 0);
        close(in);

        dup2(fd[1], 1);
        close(fd[1]);
        execlp(cmd, cmd, NULL);
    } else {
        close(fd[1]);

        char buffer[4096];
        size_t count;
        size_t total_count = 0;
        while ((count = read(fd[0], buffer, sizeof(buffer))) > 0) {
            total_count += count;
        }

        int status;
        waitpid(pid, &status, 0);
        printf("%lu\n", total_count);
    }
}