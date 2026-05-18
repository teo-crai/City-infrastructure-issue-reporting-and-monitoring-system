#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#define MAX 100
typedef struct {
    float lat,lng; //latitude and longitude
}GPS_coord;
typedef struct {
    int id; //report id
    char ins_name[30];  //inspector name
    GPS_coord gps; //gps coordinates
    char issue_categ[30]; // issue category
    int sev_lvl; //1 = minor, 2 = moderate, 3 = critical
    time_t timestamp;
    char desc[100]; //description text
}Report;
typedef struct {
    int total_score;
    char *inspector;
}inspectorList;
inspectorList list[MAX];
int n=0;
//function that looks for inspector in given array and returns index if found, -1 if not
int found_inspector(char *inspector) {
    for (int i=0;i<n;i++) {
        if (strcmp(inspector,list[i].inspector)==0)
            return i;
    }
    return -1;
}
void score(char *district_id) {
    //check if district directory exists
    struct stat st;
    if (stat(district_id, &st) == -1) {
        printf("Directory %s does not exist.\n",district_id);
        exit(-1);
    }
    if (!S_ISDIR(st.st_mode)) {
        printf("Not a directory.\n");
        exit(-1);
    }
    //create the full path for the report file
    char path[512];
    snprintf(path, sizeof(path), "%s/reports.dat", district_id);
    //open file
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        printf("Could not open file for reading\n");
        exit(-1);
    }
    //loop through reports in reports.dat file
    //and calculate the score for each inspector found in the file
    int index;
    Report rep;
    while (read(fd,&rep,sizeof(Report))==sizeof(Report)) {
        index=found_inspector(rep.ins_name); //check if we memorised the current's report inspector in the global list
        if (index!=-1) { //if found, we update the inspector's score
            list[index].total_score+=rep.sev_lvl;
        }
        else { //if not found (checker function returned -1)
            strcpy(list[n].inspector,rep.ins_name); //we append the inspector to the list
            list[n].total_score=rep.sev_lvl; //and initialize their score with the current report's severity level
            n++; //increment the lenght of the list
        }
    }
    close(fd);
}
int main(int argc,char *argv[]) {
    //the scorer process receives the needed data: district name
    //through the execl() call where first arg is the file name and the rest
    //will be stored in argv[]
    if (argc!=2) {
        printf("Wrong number of arguments for scorer prorgam.\n");
        exit(-1);
    }
    char *district_id=argv[1];
    score(district_id);
    //print the score for each inspector in the file
    for (int i=0;i<n;i++)
        printf("Inspector %s - score:%d\n",list[i].inspector,list[i].total_score);
    return 0;
}