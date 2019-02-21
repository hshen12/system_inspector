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
#define STR_SZ 1024

/* Preprocessor Directives */
#ifndef DEBUG
#define DEBUG 1
#endif

// #define BUF_SZ 128

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

    //read the given directory if provided
    //if does not exist, exit
    int procfs_fd = open(procfs_loc, O_RDONLY);
    if (procfs_fd == -1) {
        perror("open");
        return EXIT_FAILURE;
    }
    
    //read the hostname
    char *hostname = calloc(40, sizeof(char));
    get_hostname(procfs_loc, hostname);

    //read the kernel version
    char *version = calloc(1024, sizeof(char));
    get_kernel_version(procfs_loc, version);

    char *temp = calloc(1024, sizeof(char));
    get_uptime(procfs_loc, temp);
    int uptime = atof(temp);
    int uptime_temp = uptime;

    int year = uptime_temp/31536000;
    if(year > 0) {
        // year+=year_temp;
        uptime_temp-=year*31536000;
    }

    int day = uptime_temp/86400;
    if(day > 0){
        // day+=day_temp;
        uptime_temp-=day*86400;
    }

    int hour = uptime_temp/3600;
    if(hour > 0) {
        // hour+=hour_temp;
        uptime_temp-=hour*3600;
    }

    int min = uptime_temp/60;
    if(min > 0) {
        // min+=min_temp;
        uptime_temp-=min*60;
    }

    int second = uptime_temp;

    // char str_year[10];
    // itoa(year, str_year, 10);
    // printf("str year is %s\n", str_year);
    // char str_day[10] = itoa(day);
    // char str_hour[10] = itoa(hour);
    // char str_min[10] = itoa(min);
    // char str_second[10] = itoa(second);
    



    printf("uptime %d\n", uptime);

    LOG("Options selected: %s%s%s%s\n",
            options.hardware ? "hardware " : "",
            options.system ? "system " : "",
            options.task_list ? "task_list " : "",
            options.task_summary ? "task_summary" : "");


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

    printf("Hardware Information\n");
    printf("------------------\n" );
    // printf("CPU Model: %s\n", );
    // printf("Processing Units: %s\n", );
    // printf("Load Average %s\n", );
    printf("\n");

    printf("Task Information\n");
    printf("------------------\n" );
    printf("%5s | %12s | %25s | %15s | %s \n", "PID", "State", "Task Name", "User", "Tasks");


    return 0;
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
    printf("uptime is %s\n", uptime);

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
    
    strcpy(version, ker_version);
    
}

void get_hostname(char* procfs_loc, char* hostname)
{

    char fp[255];
    strcpy(fp, procfs_loc);
    strcat(fp, "/sys/kernel/hostname");
    
    char *buf = calloc(BUF_SZ, sizeof(char));
    ssize_t read_sz;

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
