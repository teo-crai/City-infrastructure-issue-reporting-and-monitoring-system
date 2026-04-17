#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define TOTAL_PROCESSES 20
#define CHILDREN_COUNT (TOTAL_PROCESSES-1)
#define LINE_COUNT 10

int main(void) {
    pid_t pid;
    for (int i = 1; i <= CHILDREN_COUNT; i++) {
        pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            for (int j = 0; j < LINE_COUNT; j++) {
                printf("Child process | PID: %d | PPID: %d\n", getpid(), getppid());
            }
            exit(i);
        }
    }

    for (int j = 0; j < LINE_COUNT; j++) {
        printf("Parent | PID %d | PPID: %d", getpid(), getppid());
    }

    int status;
    pid_t terminated_pid;

    for (int i = 0; i < CHILDREN_COUNT; i++) {
        terminated_pid = wait(&status);
        if (terminated_pid > 0) {
            if (WIFEXITED(status)) {
                int return_value = WEXITSTATUS(status);
                printf("Parent notification: child PID %d terminated with return value %d\n", terminated_pid, return_value);
            }
            else {
                printf("Parent notification: child PID %d terminated abnormally\n", terminated_pid);
            }
        }
    }
    return 0;
}