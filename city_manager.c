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
void write_in_log(char *district_id,char *role,char *user,char *command,time_t timestamp)
{
    char path[128];
    snprintf(path, sizeof(path), "%s/logged_district", district_id);
    //check for necessary permissions
    if (check_permission(path, role, 'w')==0) {
        printf("Error: %s %s does not have permission to write in file.",role,user);
        return;
    }
    FILE *f=fopen(path,"a");
    fprintf(f,"%ld    %s  %s  %s\n",timestamp,role,user,command);
    fclose(f);
}
void manage_symlink(const char *district_id, const char *target_path) {
    char link_name[128];
    snprintf(link_name, sizeof(link_name), "active_reports-%s", district_id);
    #ifdef _WIN32
    #else
    // Remove existing link if it exists to avoid errors, then recreate
    unlink(link_name);
    if (symlink(target_path, link_name) == -1) {
        perror("Failed to create symbolic link");
    }
    #endif
}
void add_report(char *district_id,char *role,char *user) {
    struct stat st;
    if (stat(district_id, &st) == -1) {
        if (mkdir(district_id, 0750) == -1) { //<-for linux
        //if (mkdir(district_id)==-1){   //<-for windows
            perror("Failed to create district directory");
            return;
        }
    }
    chmod(district_id, 0750);//set permission for directory
    //does not work the same on windows
    //create files
    char path_dat[128], path_cfg[128], path_log[128];
    snprintf(path_dat, sizeof(path_dat), "%s/reports.dat", district_id);
    snprintf(path_cfg, sizeof(path_cfg), "%s/district.cfg", district_id);
    snprintf(path_log, sizeof(path_log), "%s/logged_district", district_id);
    //initialize district.cfg if it doesn't exist
    if (stat(path_cfg, &st) == -1) {
        int fd_cfg = open(path_cfg, O_WRONLY | O_CREAT | O_TRUNC, 0640);
        if (fd_cfg != -1) {
            write(fd_cfg, "1", 1); //default severity threshold
            close(fd_cfg);
        }
    }
    chmod(path_cfg, 0640); //set specified permissions
    //initialize logged_district if it doesn't exist
    if (stat(path_log, &st) == -1) {
        int fd_log = open(path_log, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd_log != -1) close(fd_log);

    }
    chmod(path_log, 0644);//set specified permissions
    //initialize reports.dat if it doesn't exist
    if (stat(path_dat, &st) == -1) {
        int fd_dat = open(path_dat, O_WRONLY | O_CREAT | O_TRUNC, 0664);
        if (fd_dat != -1) close(fd_dat);

    }
    chmod(path_dat, 0664);//set specified permissions
    //capture input data
    Report new_report;
    printf("Report ID:"); scanf("%d",&new_report.id);
    printf("Latitude: "); scanf("%f", &new_report.gps.lat);
    printf("Longitude: "); scanf("%f", &new_report.gps.lng);
    getchar(); // Clear newline <-put this in AI file
    printf("Category (road/lighting/flooding/other): ");
    fgets(new_report.issue_categ, 30, stdin);
    new_report.issue_categ[strcspn(new_report.issue_categ, "\n")] = 0;
    printf("Severity level (1/2/3): ");
    scanf("%d", &new_report.sev_lvl);
    getchar(); // Clear newline
    printf("Description: ");
    fgets(new_report.desc, 100, stdin);
    new_report.desc[strcspn(new_report.desc, "\n")] = 0;
    new_report.timestamp = time(NULL);
    strncpy(new_report.ins_name, user, 30);
    //check for necessary permissions
    if (check_permission(path_dat, role, 'w')==0) {
        printf("Error: %s %s does not have permission to write in file.",role,user);
        return;
    }
    //write the data to the newly created file
    int fd = open(path_dat, O_WRONLY | O_CREAT | O_APPEND, 0664); //open the file, pipeline a series of flags:
    //open to writitng only, create if not created, otherwise no effect and file offset shall be set to the end of the file prior to each write
    if (fd == -1) {
        printf("Failed to open reports file");
        return;
    }
    if (write(fd, &new_report, sizeof(Report)) == -1) {
        printf("Failed to write report");
    }
    close(fd);
    manage_symlink(district_id, path_dat); //creates/updates symbolic link pointing to reports.dat file
    printf("Successfully added report to reports file");
    write_in_log(district_id, role, user, "add", new_report.timestamp); //log in the addition
    //obtaining the pid from monitor_reports.c
    int monitor_informed = 0; //binary variable to check if we were able to send the signal to the monitor
    FILE *fp = fopen(".monitor_pid", "r");
    if (fp!=NULL) {
        int monitor_pid;
        if (fscanf(fp, "%d", &monitor_pid) == 1) {
            //send SIGUSR1 signal to the monitor
            if (kill(monitor_pid, SIGUSR1) == 0) {
                monitor_informed = 1;
            }
        }
        fclose(fp);
    }
    else printf("Failed to open hidden file in add_report function.");
    //writing a message based on if the program succesfully received the signal or an error occured
    char notification_msg[128];
    if (monitor_informed) {
        snprintf(notification_msg, sizeof(notification_msg), "Monitor notified (PID found)");
    }
    else {
        snprintf(notification_msg, sizeof(notification_msg), "Monitor could NOT be informed (PID not found or signal failed)");
    }
    write_in_log(district_id, role, user, notification_msg, new_report.timestamp); //log in the succesful/failed monitoring of addition
}
void remove_district(char *district_id, char *role, char *user){
    if (strcmp(role,"manager")!=0) {
        printf("Only managers can perform this operation!");
        return;
    }
    pid_t pid=fork(); //creating child process for removal of directory
    if(pid<0){
        printf("Error in creating child process.");
        return;
    }
    else if(pid==0){ //pid=0 means we are in the child process
        char *arguments[]={"rm","-rf",district_id,NULL};
        execvp("rm",arguments);
        exit(0);
    }
    else{ //pid<0 means we're in the parent process
        int status_ptr;
        if(waitpid(pid,&status_ptr,WUNTRACED)==-1){ //wait for child project to end
            printf("Error: Child process did not end.");
            return;
        }
        if(status_ptr==0){ //check if child's return status is 0 -> uccessfully completed the removal
            //in this case, the corresponding simlink can be deleted
            char link_name[128];
            snprintf(link_name, sizeof(link_name), "active_reports-%s", district_id);
            if(unlink(link_name)==-1)//if the name referred to a symbolic link, the link is removed
                printf("Error in unlinking symbolic file.");
        }
    }
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
void list(char *district_id,char *role,char *user) {
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
        printf("Report %d submitted by %s at %ld , at coordinates: %.4f %.4f: issue category(%s),severity level(%d),description(%s)\n",rep.id,rep.ins_name,rep.timestamp,rep.gps.lat,rep.gps.lng,rep.issue_categ,rep.sev_lvl,rep.desc);
    }
    close(fd);
    write_in_log(district_id, role, user, "list", time(NULL)); //log in the listing
}
void view(char *district_id,int report_id,char *role,char *user) {
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
    write_in_log(district_id, role, user, "view", time(NULL)); //log in the viewing
}
void remove_report(char *district_id,int report_id,char *role,char *user) {
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
    //get the initial size of the file
    struct stat st;
    fstat(fd, &st);
    long total_size = st.st_size;
    Report rep;
    long pos=-1;
    while (read(fd,&rep,sizeof(Report))==sizeof(Report)) {
        if (rep.id == report_id) {
            pos=lseek(fd,0,SEEK_CUR)-sizeof(Report); //starting position of the report we must delete
            break;
        }
    }
    if (pos==-1) {
        close(fd);
        return; //report was not found
    }
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
    if (ftruncate(fd,total_size-sizeof(Report))==-1) printf("Error truncating file"); //readjusts file size to one less report
    close(fd);
    write_in_log(district_id, role, user, "remove_report", time(NULL)); //log in the removal
}
void update_treshold(char *district_id,int value,char *role,char *user) {
    if (strcmp(role,"manager")!=0) {
        printf("Only managers can perform this operation!");
        return;
    }
    char path[512];
    snprintf(path, sizeof(path), "%s/district.cfg", district_id);
    //check for necessary permissions

    //explicitly check for 640 permission as specified in the project description
    struct stat st;
    if (stat(path, &st) == -1) return; //file does not exist
    char permissions[10];
    symbolic_form(st.st_mode,permissions);
    mode_t expected_perms = S_IRUSR | S_IWUSR | S_IRGRP;
    mode_t perm_mask = S_IRWXU | S_IRWXG | S_IRWXO;
    if ((st.st_mode & perm_mask) != expected_perms){ //file does not have required permission -> 640
        printf("Current user does not have required permissions(640), instead has:%s",permissions);
        return;
    }
    if (check_permission(path, role, 'w')==0) {
        printf("Error: %s does not have permission to write in reports file in %s.",role,district_id);
        return;
    }

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
    write_in_log(district_id, role, user, "update_treshold", time(NULL)); //log in the update
}
// AI-GENERATED: Function to parse a condition string like "sev_lvl:>:1"
int parse_condition(const char *cond_str, char *field, char *op, char *val) {
    // Expected format: "field:op:value"
    if (sscanf(cond_str, "%[^:]:%[^:]:%s", field, op, val) != 3) {
        return 0; // Failed to parse
    }
    return 1;
}
// AI-GENERATED: Function to check if a report matches the parsed condition
int match_condition(Report r, const char *field, const char *op, const char *val) {
    if (strcmp(field, "severity") == 0) {
        int v = atoi(val);
        if (strcmp(op, "==") == 0) return r.sev_lvl == v;
        if (strcmp(op, "!=") == 0) return r.sev_lvl != v;
        if (strcmp(op, ">") == 0)  return r.sev_lvl > v;
        if (strcmp(op, ">=") == 0)  return r.sev_lvl >= v;
        if (strcmp(op, "<") == 0)  return r.sev_lvl < v;
        if (strcmp(op, "<=") == 0)  return r.sev_lvl <= v;
    }
    else if (strcmp(field, "category") == 0) {
        if (strcmp(op, "==") == 0) return strcmp(r.issue_categ, val) == 0;
        if (strcmp(op, "!=") == 0) return strcmp(r.issue_categ, val) != 0;
    }
    else if (strcmp(field, "timestamp") == 0) {
        time_t v = (time_t)atoll(val); //convert string value to time_t

        if (strcmp(op, "==") == 0) return r.timestamp == v;
        if (strcmp(op, "!=") == 0) return r.timestamp != v;
        if (strcmp(op, ">") == 0)  return r.timestamp > v;
        if (strcmp(op, ">=") == 0) return r.timestamp >= v;
        if (strcmp(op, "<") == 0)  return r.timestamp < v;
        if (strcmp(op, "<=") == 0) return r.timestamp <= v;
    }
    else if (strcmp(field, "inspector") == 0) {
        if (strcmp(op, "==") == 0) return strcmp(r.ins_name, val) == 0;
        if (strcmp(op, "==") == 0) return strcmp(r.ins_name, val) != 0;
    }
    return 0; // No match or unknown field
}
void filter(char *district_id,int num_of_conditions,char *conditions[],char *role,char *user) {
    char path[512];
    snprintf(path, sizeof(path), "%s/reports.dat", district_id);
    //check read permissions for file
    if (check_permission(path, role, 'r')==0) {
        printf("Error: %s does not have permission to read file",user);
        return;
    }
    //open file for reading
    int fd = open(path, O_RDONLY);
    if (fd == -1) {
        printf("Could not open file for reading");
        return;
    }
    Report rep;
    int rep_count=0; //variable that counts the number of reports that match the condition/s
    //loop through file until we find the report/s
    while (read(fd,&rep,sizeof(Report))==sizeof(Report)) {
        int match_all=1; //variable that checks if all given conditions are matched
        for (int i=0;i<num_of_conditions;i++){
            //call parse function
            char field[30], op[5], val[100];
            if (!parse_condition(conditions[i], field, op, val)) {
                printf("Invalid condition format.'\n");
                match_all=0; //if parsing fails, we can't match our condition
                break;
            }
            //call match function
            if (!match_condition(rep,field, op, val)){
                match_all=0; //if one condition isn't met, the report is not valid, as the conditions are joined by AND
                break;
            }
        }
        if (match_all==1) {
            rep_count++;
            printf("Report %d submitted by %s at %ld , at coordinates: %.4f %.4f: issue category(%s),severity level(%d),description(%s)\n",rep.id,rep.ins_name,rep.timestamp,rep.gps.lat,rep.gps.lng,rep.issue_categ,rep.sev_lvl,rep.desc);
        }
    }
    if (rep_count==0) printf("No report matches given condition."); //display message if no matching reports were found
    close(fd);
    write_in_log(district_id, role, user, "filter", time(NULL));
}
int main(int argc, char * argv[]) {
    //  uncomment on linux -> lstat() is posix only
    //check for dangling symbolic links
    struct dirent *entry;
    DIR *dp = opendir(".");
    if (dp != NULL) {
        while ((entry = readdir(dp))) {
            struct stat l_st, s_st;
            //check if the entry is a symbolic link
            if (lstat(entry->d_name, &l_st) == 0 && S_ISLNK(l_st.st_mode)) {
                //use stat to see if the target exists
                if (stat(entry->d_name, &s_st) == -1) {
                    printf("Dangling link detected -> %s\n", entry->d_name);
                }
            }
        }
        closedir(dp);
    }
    if (argc<6) {
        printf("Wrong number of arguments.");
        exit(-1);
    }
    //parsing the arguments
    char *role = argv[2]; //2nd arg-> role(inspector or manager)
    char *name = argv[4]; //4th arg-> name of user
    char *command = argv[5];//5th arg -> operation
    char *dir = argv[6];//6th agr -> dir name
    if(strstr(command,"add")) add_report(dir,role,name);
    if(strstr(command,"list")) list(dir,role,name);
    if(strstr(command,"view")) {
        int id=atoi(argv[7]);
        view(dir,id,role,name);
    }
    if(strstr(command,"remove_report")) {
        int id=atoi(argv[7]);
        remove_report(dir,id,role,name);
    }
    if(strstr(command,"remove_district")) remove_district(dir,role,name);
    if(strstr(command,"update_treshold")) {
        int val=atoi(argv[7]);
        update_treshold(dir,val,role,name);
    }
    if(strstr(command,"filter")){
        int num_of_conditions=argc-7; //we know what the first 7 arguments are, so the rest are all conditions for filter
        char **conditions=&argv[7]; //points to the first condition string
        filter(dir, num_of_conditions, conditions, role, name);
    }
    return 0;
}