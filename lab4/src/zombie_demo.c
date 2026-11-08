#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int main(void) {
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }
    if (pid == 0) _exit(0);
    printf("Parent PID: %ld, child PID: %ld\n", (long)getpid(), (long)pid);
    printf("Child has exited; before waitpid it is a zombie. Inspect with ps.\n");
    fflush(stdout);
    sleep(15);
    if (waitpid(pid, NULL, 0) < 0) { perror("waitpid"); return 1; }
    printf("Parent called waitpid: zombie has been reaped.\n");
    return 0;
}
