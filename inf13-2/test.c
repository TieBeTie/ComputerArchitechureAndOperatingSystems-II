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

int main() {
    int pid = getpid();
    printf("%d\n", pid);
    
    sigset_t full_mask;
    sigfillset(&full_mask);
    sigprocmask(SIG_BLOCK, &full_mask, NULL);

    siginfo_t info;
    sigwaitinfo(&full_mask, &info);
    int received_signal = info.si_signo;

    if (received_signal == SIGHUP) {
        printf("YES\n");
    } 
   char * name = malloc(100);
    scanf("%s", name);
    int fd;
    //printf("%s", name);
    char daata[100];
    fd = open("./jabba", O_RDONLY);
   read(fd, daata, 100);
    
    close(fd);
    fflush(stdout);

    return 0;
}
