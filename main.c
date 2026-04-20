#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
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
int check_permission(char const *path, char const *role, char action) { //path->file path; role->2nd command line argument; action->decided depending on 3rd command line argument
    struct stat st;
    if (stat(path, &st) == -1) return 0; //file does not exist

    int current_perm = st.st_mode & 0777; //extracting the permission bits by masking with 0777
    //check general permissions depending on file type
    if (S_ISDIR(st.st_mode)) {
        if (current_perm!=0750) {
            printf("Error: %s has wrong permissions (%o). Expected 750.\n", path, current_perm);
            return 0;
        }
    }
    else if (strstr(path,".dat")) {
        if (current_perm!=0664) {
            printf("Error: %s has wrong permissions (%o). Expected 664.\n", path, current_perm);
            return 0;
        }
    }
    else if (strstr(path,".cfg")) {
        if (current_perm!=0640) {
            printf("Error: %s has wrong permissions (%o). Expected 640.\n", path, current_perm);
            return 0;
        }
    }
    else if (strstr(path,"logged_district")) {
        if (current_perm!=0644) {
            printf("Error: %s has wrong permissions (%o). Expected 644.\n", path, current_perm);
            return 0;
        }
    }
    //check owner bits
    if (strcmp(role, "manager") == 0) {
        if (action == 'r' && !(st.st_mode & S_IRUSR)) {
            printf("Error: Cannot perform desired action on %s, manager does not have permission to read file.",path);
            return 0;
        }
        if (action == 'w' && !(st.st_mode & S_IWUSR)) {
            printf("Error: Cannot perform desired action on %s, manager does not have permission to write in file.",path);
            return 0;
        }
    }
    //check group bits
    else if (strcmp(role, "inspector") == 0) {
        if (action == 'r' && !(st.st_mode & S_IRGRP)) {
            printf("Error: Cannot perform desired action on %s, inspector does not have permission to read file.",path);
            return 0;
        }
        if (action == 'w' && !(st.st_mode & S_IWGRP)) {
            printf("Error: Cannot perform desired action on %s, inspector does not have permission to write in file.",path);
            return 0;
        }
    }

    return 1;
}
int report_count=0; //global variable to count number of reports and assign a report id
void add_report(char *district_id,char *role,char *user) {
    struct stat st;
    if (stat(district_id, &st) == -1)
        mkdir(district_id,0750); //function fails if directory already exists
    //does not work the same on windows
    //create report file
    char path[128];
    snprintf(path, sizeof(path), "%s/reports.dat", district_id);
    chmod(path, 0664); //set specified permissions
    //prepare data
    Report new_report;
    new_report.id = report_count++;
    strcpy(new_report.ins_name, user);
    new_report.gps.lat = 45.0;
    new_report.gps.lng = 25.0;
    strcpy(new_report.issue_categ, "road");
    new_report.sev_lvl = 2;
    new_report.timestamp = time(NULL);
    strcpy(new_report.desc, "some desc");
    //check for necessary permissions
    if (check_permission(path, role, 'w')==0) {
        printf("Error: %s %s does not have permission to write in file.",role,user);
        return;
    }
    //write the data to the newly created file
    int fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0664); //open the file, pipeline a series of flags:
    //open to writitng only, create if not created, otherwise no effect and file offset shall be set to the end of the file prior to each write
    if (fd == -1) {
        printf("Failed to open reports file");
        return;
    }
    if (write(fd, &new_report, sizeof(Report)) == -1) {
        printf("Failed to write report");
    }
    close(fd);
    printf("Successfully added report to reports file");
}
void symbolic_form(mode_t mode,char str[]) {
    //check permissions for user -> first 3 characters of permissions string
    if (mode & S_IRUSR) str[0]='r'; //check read permission
    else str[0]='-';
    if (mode & S_IWUSR) str[1]='w'; //check write permission
    else str[1]='-';
    if (mode & S_IXUSR) str[2]='x'; //check execute permission
    else str[2]='-';
    //check permissions for group
    if (mode & S_IRGRP) str[3]='r'; //check read permission
    else str[3]='-';
    if (mode & S_IWGRP) str[4]='w'; //check write permission
    else str[4]='-';
    if (mode & S_IXGRP) str[5]='x'; //check execute permission
    else str[5]='-';
    //check permissions for other
    if (mode & S_IROTH) str[6]='r'; //check read permission
    else str[6]='-';
    if (mode & S_IWOTH) str[7]='w'; //check write permission
    else str[7]='-';
    if (mode & S_IXOTH) str[8]='x'; //check execute permission
    else str[8]='-';
    //null terminator for string
    str[9]='\0';
}
void list(const char *district_id,const char *role) {
    //create the full path for the report file
    char path[512];
    snprintf(path, sizeof(path), "%s/reports.dat", district_id);
    //check for necessary permissions
    if (check_permission(path, role, 'r')==0) {
        printf("Error: %s does not have permission to read reports file in %s.",role,district_id);
        return;
    }
    //get file stats
    struct stat st;
    if (stat(path, &st) == -1) {
        printf("Error accessing reports.dat in directory: %s",district_id);
        return;
    }
    //get permissions in the specified format
    char permissions[10];
    symbolic_form(st.st_mode,permissions);
    //convert modification time to a readable string
    char *mod_time=ctime(&st.st_mtime);
    mod_time[strlen(mod_time)-1]='\0';
    //print file stats
    printf("File: %s\n", path);
    printf("Permissions: %s\n", permissions);
    printf("Size: %ld bytes\n", (long)st.st_size);
    printf("Last Modified: %s\n", mod_time);
    //list all reports in the file
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        printf("Could not open file for reading");
        return;
    }
    Report rep;
    while (read(fd,&rep,sizeof(Report))==sizeof(Report)) {
        //Q: trb sa afis toate datele unui report?
        printf("Report %d submitted by %s: %s\n",rep.id,rep.ins_name,rep.desc);
    }
    close(fd);
}
void view(char *district_id,int report_id,char *role) {
    char path[512];
    snprintf(path, sizeof(path), "%s/reports.dat", district_id);
    //check for necessary permissions
    if (check_permission(path, role, 'r')==0) {
        printf("Error: %s does not have permission to read reports file in %s.",role,district_id);
        return;
    }
    //open reports file
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        printf("Could not open file for reading");
        return;
    }
    Report rep;
    int ok=0; //binary variable to check if report was found
    //loop through file until we find the report
    while (read(fd,&rep,sizeof(Report))==sizeof(Report)) {
        if (rep.id == report_id) {
            ok=1;
            printf("\n--- Report Details ---\n");
            printf("ID:        %d\n", rep.id);
            printf("Category:  %s\n", rep.issue_categ);
            printf("Severity:  %d\n", rep.sev_lvl);
            printf("Inspector: %s\n", rep.ins_name);
            printf("Location:  %.4f, %.4f\n", rep.gps.lat, rep.gps.lng);
            printf("Date:      %s\n", ctime(&rep.timestamp));
            printf("Description: %s\n", rep.desc);
            break;
        }
    }
    if (ok==0) printf("Could not find report %d.",report_id);
    close(fd);
}
void remove_report(const char *district_id,int report_id,const char *role) {
    if (strcmp(role,"manager")!=0) {
        printf("Only managers can perform this operation!");
        return;
    }
    char path[512];
    snprintf(path, sizeof(path), "%s/reports.dat", district_id);
    //check for necessary permissions
    if (check_permission(path, role, 'r')==0 || check_permission(path, role, 'w')==0) {
        printf("Error: %s does not have permission to read/write in reports file in %s.",role,district_id);
        return;
    }
    //open reports file
    int fd = open(path, O_RDWR);
    if (fd == -1) {
        printf("Could not open file for reading/writing");
        return;
    }
    Report rep;
    long pos=-1;
    while (read(fd,&rep,sizeof(Report))==sizeof(Report)) {
        if (rep.id == report_id) {
            report_count--;
            pos=lseek(fd,0,SEEK_CUR)-sizeof(Report); //starting position of the report we must delete
            break;
        }
    }
    if (pos==-1) return;
    long current_pos=pos;
    while (1) {
        lseek(fd,current_pos+sizeof(Report),SEEK_SET); //shift forward one record
        if (read(fd,&rep,sizeof(Report))!=sizeof(Report)) {
            break; //if we can't read the next report in line, it means we reached eof
            //and can end the back and forth shift process
        }
        lseek(fd,current_pos,SEEK_SET);//shift back to current record
        write(fd,&rep,sizeof(Report));//overwrite it with the data that was just read
        current_pos+=sizeof(Report); //advance through the reports
    }
    if (pos!=-1) ftruncate(fd,report_count*sizeof(Report)); //readjusts file size to one less report
    close(fd);
}
void update_treshold(char *district_id,int value,char *role) {
    if (strcmp(role,"manager")!=0) {
        printf("Only managers can perform this operation!");
        return;
    }
    char path[512];
    snprintf(path, sizeof(path), "%s/district.cfg", district_id);
    //check for necessary permissions
    if (check_permission(path, role, 'w')==0) {
        printf("Error: %s does not have permission to write in reports file in %s.",role,district_id);
        return;
    }
    //q: trebuie sa mai verific o data explicit 640? ce inseamna diagnostic?
    //open configuration file
    int fd = open(path, O_WRONLY | O_TRUNC); //O_TRUNC empties file before writing in it
    if (fd == -1) {
        printf("Could not open file for writing");
        return;
    }
    char val_to_string[10];
    int len=snprintf(val_to_string, sizeof(val_to_string), "%d", value); //turn integer into string as district.cfg is a text file
    if (write(fd,val_to_string,len)!=len) {
        printf("Error writing to file");
    }
    close(fd);
}
int main(int argc, char * argv[]) {
    //Q: erorile trebuie afisate in log sau intr-un fisier dedicat pt erori? (stderr)
    //Implementation steps:
    //1. Parse command line arguments (--role, --user, --add, etc.)
    //Q: cum arata o lista de agrs?
    //Q: de unde luam datele pt fiecare report?
    if (argc!=5) {
        printf("Wrong number of arguments.");
        exit(-1);
    }
    char *dir=NULL,*role=NULL,*name=NULL,*command=NULL;
    strcpy(dir,argv[1]); //1st arg-> main directory name (city_manager)
    strcpy(role,argv[2]); //2nd arg-> role(inspector or manager)
    strcpy(name,argv[3]); //3rd arg-> name of user
    strcpy(command,argv[4]); //4th arg-> operation
    //2. Validate roles (Manager vs Inspector)
    //3. Call functions for action

    printf("Hello World\n");
    return 0;
}