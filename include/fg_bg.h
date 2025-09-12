#ifndef FG_BG_H
#define FG_BG_H

#include "shell.h"

// Job status constants
#define JOB_RUNNING 1
#define JOB_STOPPED 2
#define JOB_DONE 3

// Maximum number of background jobs
#define MAX_BACKGROUND_JOBS 100

// Job structure
typedef struct {
    int job_id;
    pid_t pid;
    pid_t pgid;
    int status;
    char command[MAX_INPUT_SIZE];
    int active;
} BackgroundJob;

// Global variables
extern BackgroundJob background_jobs[MAX_BACKGROUND_JOBS];
extern int next_job_id;
extern pid_t foreground_pgid;
extern int background_job_count;

// Job control functions
void init_job_control_system(void);
int add_background_job(pid_t pid, pid_t pgid, const char *command);
void remove_background_job(int job_index);
int find_job_by_pid(pid_t pid);
int find_job_by_id(int job_id);
int find_most_recent_job(void);
void cleanup_finished_jobs(void);
void kill_all_background_jobs(void);
void wait_for_background_jobs(void);

// fg and bg commands
void builtin_fg(char **args);
void builtin_bg(char **args);

// Process group management
void set_foreground_process_group(pid_t pgid);
void reset_foreground_process_group(void);

#endif
