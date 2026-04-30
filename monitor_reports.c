#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
int end_process=0;
/*
// volatile sig_atomic_t is the "best practice" way to handle signal flags
volatile sig_atomic_t end_process = 0; ????
 */
static void handler(int signum)
{
    char msg_SIGINT[]="Caught SIGINT. Ending process.\n";
    char msg_SIGUSR1[]="Caught SIGUSR1. New report has been added.\n";
    if (signum == SIGINT) {
        // write() is async-signal-safe, unlike printf(), it is safe to call even if the signal interrupts another I/O operation
        write(STDOUT_FILENO, msg_SIGINT, strlen(msg_SIGINT));
        end_process=1;
    }
    else if (signum == SIGUSR1)
        write(STDOUT_FILENO, msg_SIGUSR1, strlen(msg_SIGUSR1));
}
int main() {
    //set up signal handling parameters
    struct sigaction sa;
    sa.sa_handler = handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    //create hidden file path
    char path_hidden[128];
    snprintf(path_hidden, sizeof(path_hidden), ".monitor_pid");
    //open file
    int fd = open(path_hidden, O_WRONLY |  O_CREAT  | O_TRUNC, 0644); //O_TRUNC empties file before writing in it -> overwriting
    if (fd == -1) {
        printf("Could not open file for writing");
        exit(-1);
    }
    //writing the process id in the hidden file
    char val_to_string[10];
    int len=snprintf(val_to_string, sizeof(val_to_string), "%d", getpid()); //turn integer into string
    if (write(fd,val_to_string,len)!=len) {
        printf("Error writing to file");
        close(fd);
        exit(-1);
    }
    close(fd);
    //handle signals with sigaction()
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        printf("Error setting up SIGUSR1");
        exit(-1);
    }
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        printf("Error setting up SIGINT");
        exit(-1);
    }
    //loop to keep the program running while waiting for any signal; without this, it would reach return 0 and interrupt too early
    while (!end_process) {
        pause(); // efficiently waits for any signal
    }
    //deleting file
    if (end_process==1) {
        if (unlink(path_hidden) == -1) {
            printf("Error deleting .monitor_pid");
        }
    }
    return 0;
}
