#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <time.h>
#include <errno.h>

volatile int flag = 0;

static void handler(int signum)
{
    flag = 1;
}
int main(int argc, char *argv[])
{
         sigaction(SIGPIPE,
                  &(struct sigaction){
                      .sa_handler = handler,
                      .sa_flags = SA_RESTART},
                  NULL);
    char *FIFO_name = argv[1];
    int n = strtol(argv[2], 0, 10);

    int other_pid;
    scanf("%d", &other_pid);



    mkfifo(FIFO_name, 0644);
    kill(other_pid, SIGHUP);
    int fd = open(FIFO_name, O_WRONLY);

    char buffer[1024];
    int count = 0;
    for (int i = 0; i <= n; ++i)
    {
        sprintf(buffer, "%d ", i);
        if (flag)
        {
            break;
        }
        if (write(fd, buffer, strlen(buffer)) == strlen(buffer))
        {
            count++;
        }
    }
    close(fd);
    printf("%d\n", count);
    return 0;
}
