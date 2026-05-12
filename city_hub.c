#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
void start_monitor(){
    pid_t hub_mon=fork();
    if(hub_mon<0){
        printf("Error in creating child process.");
        return;
    }
    else if(hub_mon==0){ //pid=0 means we're in the child process
        int p[2];
        if(pipe(p)<0){
            printf("Error in creating piping process.");
            return;
        }
        pid_t mon_pid=fork();
        if(mon_pid<0){
            printf("Error in creating grandchild process.");
            return;
        }
        else if(mon_pid==0){
            dup2(p[1],STDOUT_FILENO); //redirects what the monitor prints to STDOUT into pipe
            close(p[0]);
            close(p[1]); //close directors so every process keeps their own pipe-ends
            execl("./monitor_reports.c", "./monitor_reports.c", NULL);
        }
        else{
            close(p[1]);
            char buffer[5000];
            int n;
            while(n=read(p[0],buffer,sizeof(buffer)-1)>0)
            {
                buffer[n]='\0';
                printf("%s\n");
                if(strstr(buffer,"Error")){
                    printf("The monitor process ended abruptly.");
                    break;
                }
            }
            close(p[0]);
            wait_pid(mon_pid,NULL,0);
        }

    }
}
int main(int argc, char *argv[])
{
    if (argc<2) {
        printf("Wrong number of arguments.");
        exit(-1);
    }
    char *command=argv[1];
    if(strstr(command,"start_monitor")){}
        else if(strstr(command,"calculate_scores")){
            int num_of_conditions=argc-2;
            char **conditions=&argv[2];
        }
    return 0;
}