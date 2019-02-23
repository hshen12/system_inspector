#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <math.h>
#include <pwd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define BUF_SZ 128

/* Preprocessor Directives */
#ifndef DEBUG
#define DEBUG 1
#endif

/**
 * Logging functionality. Set DEBUG to 1 to enable logging, 0 to disable.
 */
#define LOG(fmt, ...) \
do { if (DEBUG) fprintf(stderr, "%s:%d:%s(): " fmt, __FILE__, \
    __LINE__, __func__, __VA_ARGS__); } while (0)


/* Function prototypes */
void print_usage(char *argv[]);
char *next_token(char **str_ptr, const char *delim);
void get_hostname(char* host_location, char* hostname);
void get_kernel_version(char* version_location, char* version);
void get_uptime(char* procfs_loc, char* uptime);
void get_CPU_mode(char* procfs_loc, char* CPU_mode);
int get_proc_unit(char* procfs_loc);
void get_load_avg(char* procfs_loc, char* load_avg_1, char* load_avg_5, char* load_avg_15);
int get_task_running(char* procfs_loc);
int is_digit(char d_name[], int len);
void get_interrupts(char* procfs_loc, char interrupts[], char c_switch[], char fork[]);
void get_task_list(char* procfs_loc, char* process, char state[], char task_name[], char user[], char task[]);
void get_memo_info(char* procfs_loc, char total[], char used[]);
float get_cpu_usage(char* procfs_loc);

/* This struct is a collection of booleans that controls whether or not the
 * various sections of the output are enabled. */
struct view_opts {
    bool hardware;
    bool system;
    bool task_list;
    bool task_summary;
};

void print_usage(char *argv[])
{
    printf("Usage: %s [-ahlrst] [-p procfs_dir]\n" , argv[0]);
    printf("\n");
    printf("Options:\n"
        "    * -a              Display all (equivalent to -lrst, default)\n"
        "    * -h              Help/usage information\n"
        "    * -l              Task List\n"
        "    * -p procfs_dir   Change the expected procfs mount point (default: /proc)\n"
        "    * -r              Hardware Information\n"
        "    * -s              System Information\n"
        "    * -t              Task Information\n");
    printf("\n");
}


int main(int argc, char *argv[])
{

    /* Default location of the proc file system */
    char *procfs_loc = "/proc";

    /* Set to true if we are using a non-default proc location */
    bool alt_proc = false;

    struct view_opts all_on = { true, true, true, true };
    struct view_opts options = { false, false, false, false };

    int c;
    opterr = 0;
    while ((c = getopt(argc, argv, "ahlp:rst")) != -1) {
        switch (c) {
            case 'a':
            options = all_on;
            break;
            case 'h':
            print_usage(argv);
            return 0;
            case 'l':
            options.task_list = true;
            break;
            case 'p':
            procfs_loc = optarg;
            alt_proc = true;
            break;
            case 'r':
            options.hardware = true;
            break;
            case 's':
            options.system = true;
            break;
            case 't':
            options.task_summary = true;
            break;
            case '?':
            if (optopt == 'p') {
                fprintf(stderr,
                    "Option -%c requires an argument.\n", optopt);
            } else if (isprint(optopt)) {
                fprintf(stderr, "Unknown option `-%c'.\n", optopt);
            } else {
                fprintf(stderr,
                    "Unknown option character `\\x%x'.\n", optopt);
            }
            print_usage(argv);
            return 1;
            default:
            abort();
        }
    }

    if (alt_proc == true) {
        LOG("Using alternative proc directory: %s\n", procfs_loc);

        /* Remove two arguments from the count: one for -p, one for the
         * directory passed in: */
        argc = argc - 2;
    }

    if (argc <= 1) {
        /* No args (or -p only). Enable all options: */
        options = all_on;
    }

    LOG("Options selected: %s%s%s%s\n",
        options.hardware ? "hardware " : "",
        options.system ? "system " : "",
        options.task_list ? "task_list " : "",
        options.task_summary ? "task_summary" : "");

    //read the given directory if provided
    //if does not exist, exit
    int procfs_fd = open(procfs_loc, O_RDONLY);
    if (procfs_fd == -1) {
        perror("open");
        close(procfs_fd);
        return EXIT_FAILURE;
    }

    if(options.system) {
        //read the hostname
        char *hostname = calloc(40, sizeof(char));
        get_hostname(procfs_loc, hostname);
        // printf("before version\n");
        //read the kernel version
        char *version = calloc(1024, sizeof(char));
        // printf("before version\n");
        get_kernel_version(procfs_loc, version);
        // printf("after ker_version\n");
        char *temp = calloc(1024, sizeof(char));
        get_uptime(procfs_loc, temp);
        int uptime = atof(temp);
        int uptime_temp = uptime;

        int year = uptime_temp/31536000;
        if(year > 0) {
            uptime_temp-=year*31536000;
        }

        int day = uptime_temp/86400;
        if(day > 0){
            uptime_temp-=day*86400;
        }

        int hour = uptime_temp/3600;
        if(hour > 0) {
            uptime_temp-=hour*3600;
        }

        int min = uptime_temp/60;
        if(min > 0) {
            uptime_temp-=min*60;
        }
        printf("here\n");
        int second = uptime_temp;
        printf("System Information\n");
        printf("------------------\n" );
        printf("Hostname: %s\n", hostname);
        printf("Kernel Version: %s\n", version);
        printf("Uptime: ");
        if(year > 0) {
            printf("%d years, ", year);
        }
        if(day > 0) {
            printf("%d days, ", day);
        }
        if(hour > 0) {
            printf("%d hours, ", hour);
        }
        if(min > 0) {
            printf("%d minutes, ", min);
        }
        if(second > 0) {
            printf("%d seconds", second);
        }
        printf("\n");
        printf("here\n");
        free(hostname);
        free(version);
        free(temp);
    }

    if(options.hardware) {

        char *CPU_mode = calloc(50, sizeof(char));
        get_CPU_mode(procfs_loc, CPU_mode);
        printf("cpu unit\n");

        int proc_unit = get_proc_unit(procfs_loc);
        printf("proc unit\n");

        char *load_avg_1 = calloc(10, sizeof(char));
        char *load_avg_5 = calloc(10, sizeof(char));
        char *load_avg_15 = calloc(10, sizeof(char));
        get_load_avg(procfs_loc, load_avg_1, load_avg_5, load_avg_15);
        printf("load avg\n");
        char total[10];
        char used[10];
        get_memo_info(procfs_loc, total, used);
        printf("meminfo\n");
        float result = ((float)(atoi(used)))/atoi(total)*100;
        float total_float = ((float) atoi(total))/1024/1024;
        float used_float = ((float) atoi(used))/1024/1024;
        int num = result/5;
        int remain = 20-num;
        int i;

        float usage = get_cpu_usage(procfs_loc)*100;
        int usage_num = usage/5;
        int usage_remain = 20-usage_num;

        printf("Hardware Information\n");
        printf("------------------\n" );
        printf("CPU Model: %s\n", CPU_mode);
        printf("Processing Units: %d\n", proc_unit);
        printf("Load Average (1/5/15 min) %s %s %s\n", load_avg_1, load_avg_5, load_avg_15);
        printf("CPU Usage:\t[");

        for(i = 0; i < usage_num; i++) {
            printf("#");
        }
        for(i = 0; i < usage_remain; i++) {
            printf("-");
        }
        printf("] %.1f%%\n", usage);

        printf("Memory Usage:\t[");
        
        for (i = 0; i < num; i++) {
            printf("#");
        }
        for(i = 0; i < remain; i++) {
            printf("-");
        }
        printf("] %.1f%% (%.1f GB / %.1f GB)\n", result, used_float, total_float);
        printf("\n");

        free(CPU_mode);
        free(load_avg_1);
        free(load_avg_5);
        free(load_avg_15);
    }


    if(options.task_summary) {

        int task_running = get_task_running(procfs_loc);

        char interrupts[20];
        char c_switch[20];
        char fork[20];
        get_interrupts(procfs_loc, interrupts, c_switch, fork);

        printf("Task Information\n");
        printf("------------------\n" );
        printf("Tasks running: %d\n", task_running);
        printf("Since boot:\n");
        printf("\tInterrupts: %s\n", interrupts);
        printf("\tContext Switches: %s\n", c_switch);
        printf("\tForks: %s\n", fork);
        printf("\n");
    }

    if(options.task_list) {

        char state[15];
        char task_name[30];
        char user[20];
        char task[5];

        printf("%5s | %12s | %25s | %15s | %s \n", "PID", "State", "Task Name", "User", "Tasks");
        printf("------+--------------+---------------------------+-----------------+-------\n");

        DIR *directory;
        if ((directory = opendir(procfs_loc)) == NULL) {
            perror("opendir");
            closedir(directory);
            return EXIT_FAILURE;
        }

        struct dirent *entry;
        while ((entry = readdir(directory)) != NULL) {
            if((is_digit(entry->d_name, strlen(entry->d_name)) == 1) && (entry->d_type == 4)) {

                entry->d_name[strlen(entry->d_name)] = '\0';

                get_task_list(procfs_loc, entry->d_name, state, task_name, 
                    user, task);

                // struct passwd *pwd = getpwuid(atoi(user));

                // printf("%5s | %12s | %25s | %15s | %s \n", entry->d_name, state, 
                    // task_name, pwd == NULL ? user:pwd->pw_name, task);
            }
        }

        closedir(directory);
    }
    return 0;
}

float get_cpu_usage(char* procfs_loc){

    char fp[255];
    strcpy(fp, procfs_loc);
    strcat(fp, "/stat");

    int fd = open(fp, O_RDONLY);
    ssize_t read_sz;
    char buf[1];
    char one_line[100];
    int length = 0;

    char one_cpu[80];
    char two_cpu[80];

    while((read_sz = read(fd, buf, 1)) >0) {
        if(buf[0] == '\n') {
            one_line[length] = '\0';

            strcpy(one_cpu, one_line);
            one_line[0] = '\0';
            length = 0;
            break;

        } else {
            one_line[length] = buf[0];
            length++;
        }
    }
    close(fd);

    sleep(1);

    fd = open(fp, O_RDONLY);

    while((read_sz = read(fd, buf, 1)) >0) {

        if(buf[0] == '\n') {
            one_line[length] = '\0';
            strcpy(two_cpu, one_line);
            one_line[0] = '\0';
            length = 0;
            break;

        } else {
            one_line[length] = buf[0];
            length++;
        }
    }
    close(fd);

    char *one_tok = one_cpu;
    char *one_temp;
    long one_total = 0;
    int token = 0;
    long idel1;

    while((one_temp = next_token(&one_tok, " ")) != NULL){
        if(token == 4) {
            idel1 = atol(one_temp);
        }
        if(token > 0){
            one_total+=atol(one_temp);
        }
        token++;
    }

    long two_total = 0;
    long idel2;
    char *two_tok = two_cpu;
    char *two_temp;
    token = 0;

    while((two_temp = next_token(&two_tok, " ")) != NULL){
        if(token == 4) {
            idel2 = atol(two_temp);
        }
        if(token > 0){
            two_total+=atol(two_temp);
        }
        token++;
    }

    // free(two_tok);
    // free(two_temp);
    // free(one_tok);
    // free(one_temp);

    float usage = 1-(float)(idel2-idel1)/(two_total-one_total);
    if (isnan(usage)) {
        return 0.0;
    } else {
        return usage;
    }

}

void get_memo_info(char* procfs_loc, char total[], char used[]){

    char *total_pre = "MemTotal:";
    char *active_pre = "Active:";

    char total_line[30];
    char active_line[30];

    char fp[255];
    strcpy(fp, procfs_loc);
    strcat(fp, "/meminfo");

    int fd = open(fp, O_RDONLY);
    ssize_t read_sz;
    char buf[1];
    char one_line[100];
    int length = 0;

    while((read_sz = read(fd, buf, 1)) >0) {

        if(buf[0] == '\n') {
            one_line[length] = '\0';

            if(strncmp(total_pre, one_line, strlen(total_pre)) == 0) {
                strcpy(total_line, one_line);
            }

            if(strncmp(active_pre, one_line, strlen(active_pre)) == 0) {
                strcpy(active_line, one_line);
            }

            one_line[0] = '\0';
            length = 0;
        } else {
            one_line[length] = buf[0];
            length++;
        }
    }
    close(fd);
    // free(total_pre);
    // free(active_pre);

    int i;
    int flag = 0;
    int index = 0;
    for(i = 0; total_line[i] != '\0'; i++) {

        if(flag == 1) {
            total[index] = total_line[i];
            index++;
        } else if(total_line[i] > 47 && total_line[i] < 58) {
            total[index] = total_line[i];
            flag = 1;
            index++;
        }
        if(total_line[i] == 32) {
            flag = 0;
        }
    }

    total[index] = '\0';
    index = 0;
    flag = 0;

    for(i = 0; active_line[i] != '\0'; i++) {

        if(flag == 1) {
            used[index] = active_line[i];
            index++;
        } else if(active_line[i] > 47 && total_line[i] < 58) {
            used[index] = active_line[i];
            flag = 1;
            index++;
        }
        if(active_line[i] == 32) {
            if(index > 0) {
                break;
            }
            flag = 0;
        }
    }
    used[index] = '\0';
}

void get_task_list(char* procfs_loc, char* process, char state[], 
    char task_name[], char user[], char task[]) {

    char *thread_pre = "Threads:";
    char *state_pre = "State:";
    char *uid_pre = "Uid:";
    char *name_pre = "Name:";

    char state_line[30];
    char uid_line[30];
    char thread_line[15];
    char name_line[99];

    char fp[255];
    strcpy(fp, procfs_loc);
    strcat(fp, "/");
    strcat(fp, process);
    strcat(fp, "/status");
    int fd = open(fp, O_RDONLY);
    ssize_t read_sz;
    char buf[1];
    char one_line[100];
    int length = 0;

    while((read_sz = read(fd, buf, 1)) >0) {
        if(buf[0] == '\n') {
            one_line[length] = '\0';

            if(strncmp(thread_pre, one_line, strlen(thread_pre)) == 0) {
                strcpy(thread_line, one_line);
                break;
            }

            if(strncmp(name_pre, one_line, strlen(name_pre)) == 0) {
                strcpy(name_line, one_line);
            }

            if(strncmp(state_pre, one_line, strlen(state_pre)) == 0) {
                strcpy(state_line, one_line);
            }

            if(strncmp(uid_pre, one_line, strlen(uid_pre)) == 0) {
                strcpy(uid_line, one_line);
            }

            one_line[0] = '\0';
            length = 0;
        } else {
            one_line[length] = buf[0];
            length++;
        }

    }
    close(fd);
    // free(thread_pre);
    // free(state_pre);
    // free(uid_pre);
    // free(name_pre);

    int i;
    int flag = 0;
    int index = 0;
    for(i = 0; i < 31; i++) {
        if(flag == 1) {
            task_name[index] = name_line[i];
            index++;
        }
        if(name_line[i] == 9) {
            flag = 1;
        }
    }

    task_name[index] = '\0';
    index = 0;
    flag = 0;

    for(i = 0; state_line[i] != 41; i++) {
        if(flag == 1) {
            state[index] = state_line[i];
            index++;
        }
        if(state_line[i] == 40) {
            flag = 1;
        }
    }

    state[index] = '\0';
    index = 0;
    flag = 0;

    for(i = 0; thread_line[i] != '\0'; i++) {
        if(flag == 1) {
            task[index] = thread_line[i];
            index++;
        }
        if(thread_line[i] == 9) {
            flag = 1;
        }
    }

    task[index] = '\0';
    index = 0;
    flag = 0;

    for(i = 0; uid_line[i] != '\0'; i++) {
        if(flag == 1) {
            user[index] = uid_line[i];
            index++;
        }
        if(uid_line[i] == 9) {
            flag = 1;
        }
    }

    user[index] = '\0';

    for(i = 0; user[i] != 9; i++) {
        if(user[i+1] == 9){
            user[i+1] = '\0';
            break;
        }
    }
}


void get_interrupts(char* procfs_loc, char interrupts[], char c_switch[], 
    char fork[]) {

    char *inter_pre = "intr";
    char *c_switch_pre = "ctxt";
    char *fork_pre = "processes";
    char fp[255];
    strcpy(fp, procfs_loc);
    strcat(fp, "/stat");

    char one_line[12000];
    char inte_line[12000];
    char c_switch_line[50];
    char fork_line[50];
    int length = 0;
    char buf[1];
    
    ssize_t read_sz;

    int fd = open(fp, O_RDONLY);
    while((read_sz = read(fd, buf, 1)) >0) {

        if(buf[0] == '\n') {
            //one line has been read
            one_line[length] = '\0';
            if(strncmp(inter_pre, one_line, strlen(inter_pre)) == 0) {
                strcpy(inte_line, one_line);
            }

            if(strncmp(c_switch_pre, one_line, strlen(c_switch_pre)) == 0) {
                strcpy(c_switch_line, one_line);
            }

            if(strncmp(fork_pre, one_line, strlen(fork_pre)) == 0) {
                strcpy(fork_line, one_line);
            }

            one_line[0] = '\0';
            length = 0;
        } else {
            //in the middle of the line
            one_line[length] = buf[0];
            length++;
        }

    }
    close(fd);
    // free(inter_pre);
    // free(c_switch_pre);
    // free(fork_pre);

    int i, inter_index;
    for(i = 0; i < 12000; i++) {
        if(inte_line[i] > 47 && inte_line[i] < 58) {
            //is number

            interrupts[inter_index] = inte_line[i];
            inter_index++;
        }
        if(inte_line[i] == 32 && inte_line[i-1] != 114) {
            interrupts[inter_index] = '\0';
            break;
        }
    }

    int index;
    for(i = 0; i < 50; i++) {
        if(c_switch_line[i] > 47 && c_switch_line[i] < 58) {

            c_switch[index] = c_switch_line[i];
            index++;
        }
        if(c_switch_line[i] == 0) {
            c_switch[index] = '\0';
            break;
        }
    }

    index = 0;
    for(i = 0; i < 50; i++) {
        if(fork_line[i] > 47 && fork_line[i] < 58) {

            fork[index] = fork_line[i];
            index++;
        }
        if(fork_line[i] == 0) {
            fork[index] = '\0';
            break;
        }
    }
}

int is_digit(char d_name[], int len) {
    int i;
    for(i = 0; i < len; i++) {
        if(d_name[i]<48 || d_name[i]>57) {
            return 0;
        }
    }
    return 1;
}


int get_task_running(char* procfs_loc) {

    DIR *directory;
    if ((directory = opendir(procfs_loc)) == NULL) {
        perror("opendir");
        closedir(directory);
        return 1;
    }
    int result = 0;
    struct dirent *entry;
    while ((entry = readdir(directory)) != NULL) {
        if((is_digit(entry->d_name, strlen(entry->d_name)) == 1) && (entry->d_type == 4)) {
            result++;
        }
    }

    closedir(directory);
    return result;
}

void get_load_avg(char* procfs_loc, char* load_avg_1, char* load_avg_5, char* load_avg_15){

    char fp[255];
    strcpy(fp, procfs_loc);
    strcat(fp, "/loadavg");

    char one_line[BUF_SZ];
    int length = 0;
    char buf[1];
    
    ssize_t read_sz;

    int fd = open(fp, O_RDONLY);

    while((read_sz = read(fd, buf, 1)) >0) {

        one_line[length] = buf[0];
        length++;
        
    }
    one_line[length-1] = '\0';
    close(fd);

    char *load_tok = one_line;
    char *temp;
    int tokens = 0;
    while((temp = next_token(&load_tok, " ")) != NULL){
        if(tokens == 0) {
            strcpy(load_avg_1, temp);
        }

        if (tokens == 1) {
            strcpy(load_avg_5, temp);
        }

        if (tokens == 2) {
            strcpy(load_avg_15, temp);
            break;
        }
        tokens++;
    }

    // free(load_tok);
    // free(temp);
}

int get_proc_unit(char* procfs_loc) {

    int result = 0;
    char *pre = "cpu";
    char *stop = "intr";
    char fp[255];
    strcpy(fp, procfs_loc);
    strcat(fp, "/stat");

    char one_line[BUF_SZ];
    int length = 0;
    char buf[1];
    
    ssize_t read_sz;

    int fd = open(fp, O_RDONLY);
    while((read_sz = read(fd, buf, 1)) >0) {

        if(buf[0] == '\n') {
            //one line has been read
            one_line[length] = '\0';
            if(strncmp(pre, one_line, strlen(pre)) == 0) {
                result++;
            }

            //stop reading when find "intr"
            if(strncmp(stop, one_line, strlen(stop)) == 0) {
                // printf("before break: %s\n", one_line);
                break;
            }

            one_line[0] = '\0';
            length = 0;
        } else {
            if(strlen(one_line) >= 127) {
                break;
            }

            //in the middle of the line
            one_line[length] = buf[0];
            length++;
        }
    }
    close(fd);
    // free(pre);
    // free(stop);
    return result-1;
}



void get_CPU_mode(char* procfs_loc, char* CPU_mode){
    char *pre = "model name";
    char fp[255];
    strcpy(fp, procfs_loc);
    strcat(fp, "/cpuinfo");

    char one_line[BUF_SZ];
    int length = 0;
    char buf[1];
    
    ssize_t read_sz;

    int fd = open(fp, O_RDONLY);

    while((read_sz = read(fd, buf, 1)) >0) {

        if(buf[0] == '\n') {
            //one line has been read
            one_line[length] = '\0';
            int result =strncmp(pre, one_line, strlen(pre));
            if(result == 0) {
                strcpy(CPU_mode, one_line);
                break;
            }
            one_line[0] = '\0';
            length = 0;
        } else {
            //in the middle of the line
            one_line[length] = buf[0];
            length++;
        }
    }

    close(fd);
    // free(pre);
    
    char *cpu_tok = CPU_mode;
    char *cpu_mode;
    int tokens = 0;
    while((cpu_mode = next_token(&cpu_tok, "\t:")) != NULL){
        if(tokens == 1) {
            break;
        }
        tokens++;
    }
    strcpy(CPU_mode, cpu_mode+1);
    // free(cpu_tok);
    // free(cpu_mode);

}



void get_uptime(char* procfs_loc, char* uptime)
{

    char fp[255];
    strcpy(fp, procfs_loc);
    strcat(fp, "/uptime");
    
    char *buf = calloc(BUF_SZ, sizeof(char));
    ssize_t read_sz;

    int fd = open(fp, O_RDONLY);
    int file_size = 0;
    while((read_sz = read(fd, buf, BUF_SZ)) >0) {

        int i;
        for(i = 0; i < read_sz; ++i) {
            if(buf[i] == EOF) {
                file_size+=i;
                strncat(uptime, buf, i);
            }
        }
        strncat(uptime, buf, read_sz);
        file_size+=read_sz;
    }

    uptime[file_size-1] = '\0';
    close(fd);
    free(buf);

    char *up_tok = uptime;
    char *up_time;
    int tokens = 0;
    while((up_time = next_token(&up_tok, " ")) != NULL){
        if(tokens == 0) {
            break;
        }
        tokens++;
    }
    
    strcpy(uptime, up_time);
    // free(up_tok);
    // free(up_time);
}


void get_kernel_version(char* procfs_loc, char* version)
{
    char fp[255];
    strcpy(fp, procfs_loc);
    strcat(fp, "/version");
    
    char *buf = calloc(BUF_SZ, sizeof(char));
    ssize_t read_sz;

    int fd = open(fp, O_RDONLY);
    int file_size = 0;
    while((read_sz = read(fd, buf, BUF_SZ)) >0) {

        int i;
        for(i = 0; i < read_sz; ++i) {
            if(buf[i] == EOF) {
                file_size+=i;
                strncat(version, buf, i);
            }
        }
        strncat(version, buf, read_sz);
        file_size+=read_sz;
    }

    version[file_size-1] = '\0';
    close(fd);
    free(buf);
    // printf("1!!!!!!\n");
    //token version string
    char *ver_tok = version;
    char *ker_version;
    int tokens = 0;
    while((ker_version = next_token(&ver_tok, " ")) != NULL){
        if(tokens == 2) {
            break;
        }
        tokens++;
    }
    // printf("1!!!!!!\n");
    strcpy(version, ker_version);
    // printf("1!!!!!!\n");
    // free(ver_tok);
    // free(ker_version);
    // printf("1!!!!!!\n");
}

void get_hostname(char* procfs_loc, char* hostname)
{
    printf("here\n");
    char fp[255];
    strcpy(fp, procfs_loc);
    strcat(fp, "/sys/kernel/hostname");
    
    char *buf = calloc(BUF_SZ, sizeof(char));
    ssize_t read_sz;
    printf("here\n");
    int fd = open(fp, O_RDONLY);
    int file_size = 0;
    while((read_sz = read(fd, buf, BUF_SZ)) >0) {

        int i;
        for(i = 0; i < read_sz; ++i) {
            if(buf[i] == EOF) {
                file_size+=i;
                strncat(hostname, buf, i);
            }
        }
        strncat(hostname, buf, read_sz);
        file_size+=read_sz;
    }
    printf("here\n");
    hostname[file_size-1] = '\0';
    close(fd);
    free(buf);

}


char *next_token(char **str_ptr, const char *delim)
{
    if (*str_ptr == NULL) {
        return NULL;
    }

    size_t tok_start = strspn(*str_ptr, delim);
    size_t tok_end = strcspn(*str_ptr + tok_start, delim);

    /* Zero length token. We must be finished. */
    if (tok_end  <= 0) {
        *str_ptr = NULL;
        return NULL;
    }

    /* Take note of the start of the current token. We'll return it later. */
    char *current_ptr = *str_ptr + tok_start;

    /* Shift pointer forward (to the end of the current token) */
    *str_ptr += tok_start + tok_end;

    if (**str_ptr == '\0') {
        /* If the end of the current token is also the end of the string, we
         * must be at the last token. */
        *str_ptr = NULL;
    } else {
        /* Replace the matching delimiter with a NUL character to terminate the
         * token string. */
        **str_ptr = '\0';

        /* Shift forward one character over the newly-placed NUL so that
         * next_pointer now points at the first character of the next token. */
        (*str_ptr)++;
    }

    return current_ptr;
}
