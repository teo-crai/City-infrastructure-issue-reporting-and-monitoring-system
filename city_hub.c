#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
void start_monitor(){
    pid_t hub_mon=fork();
    if(hub_mon<0){
        printf("Error in creating child process.");
        return;
    }
    else if(hub_mon==0){ //pid=0 means we're in the child process
        printf("Parent pid:%d\n",getpid());
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
            close(p[0]);
            dup2(p[1],STDOUT_FILENO); //redirects what the monitor prints to STDOUT into pipe
            close(p[1]); //close directors so every process keeps their own pipe-ends
            execl("./m", "./m", NULL);
        }
        else{
            close(p[1]);
            char buffer[5000];
            int n;
            while((n=read(p[0],buffer,sizeof(buffer)-1))>0)
            {
                buffer[n]='\0';
                printf("%s\n",buffer);
                if(strstr(buffer,"Error")){
                    printf("The monitor process ended abruptly.");
                    break;
                }
            }
            close(p[0]);
            exit(0);
        }
    }
}
void stop_monitor(){
    //create hidden file path
    char path_hidden[128];
    snprintf(path_hidden, sizeof(path_hidden), ".monitor_pid");
    //open file
    int fd = open(path_hidden, O_RDONLY);
    if (fd == -1) {
        printf("Error:Could not open file for writing");
        exit(-1);
    }
    char buffer[32];
    int n = read(fd,buffer,sizeof(buffer) - 1);
    close(fd);
    buffer[n] = '\0';
    pid_t mon_pid = atoi(buffer);
    kill(mon_pid,SIGINT);

}
void calculate_scores(int num_of_districts,char *district_list[]) {
    for (int i = 0; i < num_of_districts; i++) {
        int p[2];
        if(pipe(p)<0){
            printf("Error in creating piping process.");
            return;
        }
        pid_t scorer_pid = fork();
        if(scorer_pid<0){
            printf("Error in creating child scorer process.");
            return;
        }
        else if(scorer_pid==0) {
            close(p[0]);
            dup2(p[1],STDOUT_FILENO);
            close(p[1]);
            execl("./s","./s",district_list[i],NULL);
        }
        else {
            close(p[1]);
            char buffer[5000];
            int n;
            while((n=read(p[0],buffer,sizeof(buffer)-1))>0)
            {
                buffer[n]='\0';
                printf("%s\n",buffer);
            }
            close(p[0]);
            exit(0);
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
    if(strstr(command,"start_monitor")){start_monitor();}
    else if(strstr(command,"stop_monitor")){stop_monitor();}
    else if(strstr(command,"calculate_scores")){
        int num_of_districtss=argc-2;
        char **district_list=&argv[2];
        calculate_scores(num_of_districtss,district_list);
    }
    return 0;
}
