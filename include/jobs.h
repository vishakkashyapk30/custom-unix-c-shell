// #ifndef JOBS_H
// #define JOBS_H

// #include "shell.h"
// #include <sys/types.h>
// #include <signal.h>
// #include <sys/wait.h>
// #include <unistd.h>
// #include <stdio.h>

// #define MAX_BACKGROUND_JOBS 100

// // Job status enumeration
// typedef enum {
//     JOB_RUNNING,
//     JOB_STOPPED,
//     JOB_DONE
// } JobStatus;

// // Background job structure
// typedef struct {
//     pid_t pid;
//     pid_t pgid;
//     int job_id;
//     char command[MAX_INPUT_SIZE];
//     JobStatus status;
//     int active;
// } BackgroundJob;

// // Global background jobs array
// extern BackgroundJob background_jobs[MAX_BACKGROUND_JOBS];
// extern int next_job_id;

// // Job control builtin commands
// void builtin_fg(char **args);
// void builtin_bg(char **args);
// void builtin_activities(char **args);
// void builtin_ping(char **args);

// // Job management utilities
// int find_job_by_id(int job_id);
// int find_most_recent_job(void);
// void print_job_status(void);
// void cleanup_finished_jobs(void);

// // Background job management
// int add_background_job(pid_t pid, pid_t pgid, const char *command);
// void remove_background_job(int index);
// void kill_all_background_jobs(void);
// void wait_for_background_jobs(void);

// // Process group management functions (these should be defined elsewhere)
// void set_foreground_process_group(pid_t pgid);
// void reset_foreground_process_group(void);

// #endif