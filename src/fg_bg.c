#include "fg_bg.h"
#include <sys/wait.h>
#include <termios.h>
#include <errno.h>

// Global variables
BackgroundJob background_jobs[MAX_BACKGROUND_JOBS];
int next_job_id = 1;
pid_t foreground_pgid = 0;
int background_job_count = 0;

void init_job_control_system(void) {
    // Initialize all job slots as inactive
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
        background_jobs[i].active = 0;
        background_jobs[i].job_id = 0;
        background_jobs[i].pid = 0;
        background_jobs[i].pgid = 0;
        background_jobs[i].status = JOB_DONE;
        background_jobs[i].command[0] = '\0';
    }
    next_job_id = 1;
    foreground_pgid = 0;
    background_job_count = 0;
}

int add_background_job(pid_t pid, pid_t pgid, const char *command) {
    // Find an empty slot
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
        if (!background_jobs[i].active) {
            background_jobs[i].active = 1;
            background_jobs[i].job_id = next_job_id++;
            background_jobs[i].pid = pid;
            background_jobs[i].pgid = pgid;
            background_jobs[i].status = JOB_RUNNING;
            strncpy(background_jobs[i].command, command, MAX_INPUT_SIZE - 1);
            background_jobs[i].command[MAX_INPUT_SIZE - 1] = '\0';
            background_job_count++;
            return i;
        }
    }
    return -1; // No available slots
}

void remove_background_job(int job_index) {
    if (job_index >= 0 && job_index < MAX_BACKGROUND_JOBS && background_jobs[job_index].active) {
        background_jobs[job_index].active = 0;
        background_job_count--;
    }
}

int find_job_by_pid(pid_t pid) {
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
        if (background_jobs[i].active && background_jobs[i].pid == pid) {
            return i;
        }
    }
    return -1;
}

int find_job_by_id(int job_id) {
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
        if (background_jobs[i].active && background_jobs[i].job_id == job_id) {
            return i;
        }
    }
    return -1;
}

int find_most_recent_job(void) {
    int most_recent = -1;
    int highest_id = -1;
    
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
        if (background_jobs[i].active && background_jobs[i].job_id > highest_id) {
            highest_id = background_jobs[i].job_id;
            most_recent = i;
        }
    }
    return most_recent;
}

void cleanup_finished_jobs(void) {
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
        if (background_jobs[i].active && background_jobs[i].status == JOB_DONE) {
            remove_background_job(i);
        }
    }
}

void kill_all_background_jobs(void) {
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
        if (background_jobs[i].active) {
            kill(background_jobs[i].pid, SIGKILL);
        }
    }
}

void wait_for_background_jobs(void) {
    // Wait for all active background jobs to terminate
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
        if (background_jobs[i].active) {
            int status;
            waitpid(background_jobs[i].pid, &status, 0);
        }
    }
}

void set_foreground_process_group(pid_t pgid) {
    foreground_pgid = pgid;
    // Only try to set process group if we're in a terminal
    if (isatty(STDIN_FILENO)) {
        if (tcsetpgrp(STDIN_FILENO, pgid) == -1) {
            // Don't print error for non-terminal cases
            if (errno != ENOTTY) {
                perror("tcsetpgrp");
            }
        }
    }
}

void reset_foreground_process_group(void) {
    // Only try to reset process group if we're in a terminal
    if (isatty(STDIN_FILENO)) {
        if (tcsetpgrp(STDIN_FILENO, getpgrp()) == -1) {
            // Don't print error for non-terminal cases
            if (errno != ENOTTY) {
                perror("tcsetpgrp");
            }
        }
    }
    foreground_pgid = 0;
}

void builtin_fg(char **args) {
    int job_index;
    
    if (args[1]) {
        // Job number provided
        int job_id = atoi(args[1]);
        job_index = find_job_by_id(job_id);
        if (job_index == -1) {
            printf("No such job\n");
            return;
        }
    } else {
        // Use most recent job
        job_index = find_most_recent_job();
        if (job_index == -1) {
            printf("No jobs\n");
            return;
        }
    }
    
    BackgroundJob *job = &background_jobs[job_index];
    
    // Print the entire command when bringing it to foreground
    printf("%s\n", job->command);
    
    // Set as foreground process group
    set_foreground_process_group(job->pgid);
    
    // If stopped, send SIGCONT to resume it
    if (job->status == JOB_STOPPED) {
        if (kill(-job->pgid, SIGCONT) == -1) {
            perror("kill SIGCONT");
        }
    }
    
    job->status = JOB_RUNNING;
    
    // Wait for the job to complete or stop again
    int status;
    pid_t result = waitpid(job->pid, &status, WUNTRACED);
    
    // Reset terminal control to shell
    reset_foreground_process_group();
    
    if (result > 0) {
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            // Job finished
            remove_background_job(job_index);
        } else if (WIFSTOPPED(status)) {
            // Job stopped again
            job->status = JOB_STOPPED;
            char command_copy[MAX_INPUT_SIZE];
            strncpy(command_copy, job->command, MAX_INPUT_SIZE - 1);
            command_copy[MAX_INPUT_SIZE - 1] = '\0';
            
            char *command_name = strtok(command_copy, " ");
            if (!command_name) command_name = job->command;
            printf("[%d] Stopped %s\n", job->job_id, command_name);
        }
    }
}

void builtin_bg(char **args) {
    int job_index;
    
    if (args[1]) {
        // Job number provided
        int job_id = atoi(args[1]);
        job_index = find_job_by_id(job_id);
        if (job_index == -1) {
            printf("No such job\n");
            return;
        }
    } else {
        // Use most recent stopped job
        job_index = -1;
        for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
            if (background_jobs[i].active && background_jobs[i].status == JOB_STOPPED) {
                if (job_index == -1 || background_jobs[i].job_id > background_jobs[job_index].job_id) {
                    job_index = i;
                }
            }
        }
        if (job_index == -1) {
            printf("No stopped jobs\n");
            return;
        }
    }
    
    BackgroundJob *job = &background_jobs[job_index];
    
    // Check if job is already running
    if (job->status == JOB_RUNNING) {
        printf("Job already running\n");
        return;
    }
    
    // Only stopped jobs can be resumed with bg
    if (job->status != JOB_STOPPED) {
        printf("No such job\n");
        return;
    }
    
    // Resume the job in background by sending SIGCONT
    if (kill(-job->pgid, SIGCONT) == -1) {
        perror("kill SIGCONT");
        return;
    }
    
    job->status = JOB_RUNNING;
    
    // Print [job_number] command_name & when resuming
    char command_copy[MAX_INPUT_SIZE];
    strncpy(command_copy, job->command, MAX_INPUT_SIZE - 1);
    command_copy[MAX_INPUT_SIZE - 1] = '\0';
    
    char *command_name = strtok(command_copy, " ");
    if (!command_name) command_name = job->command;
    printf("[%d] %s &\n", job->job_id, command_name);
}
