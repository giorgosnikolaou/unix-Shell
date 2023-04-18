#include <unistd.h>
#include <sys/wait.h>
#include "Shell.hpp"

#define FORK(pid)                   \
    if ((pid = fork()) < 0) {       \
        perror("fork() failed");    \
        exit(EXIT_FAILURE);         \
    }                               \

int main() {

    pid_t pid;
    FORK(pid);

    if (pid == 0) {
        // sleep(1);
        // printf("Child group id: %d\n", getpgrp());

        Shell sh = Shell();
        sh.execute();
    }
    else {
        if (setpgid(pid, 0) != 0) 
            perror("setpgid() error");

        // printf("Parent group id: %d\n", getpgrp());

        tcsetpgrp(0, pid); // makefile hack
        
        waitpid(pid, NULL, WUNTRACED);
    }


    return 0;
}