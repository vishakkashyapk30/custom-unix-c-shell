#include "jobs.h"
#include "signals.h"

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
    printf("%s\n", job->command);
    
    // Set as foreground process group
    foreground_pgid = job->pgid;
    if (tcsetpgrp(STDIN_FILENO, job->pgid) == -1) {
        perror("tcsetpgrp");
    }
    
    // If stopped, send SIGCONT
    if (job->status == JOB_STOPPED) {
        kill(-job->pgid, SIGCONT);
    }
    
    job->status = JOB_RUNNING;
    
    // Wait for the job
    int status;
    pid_t result = waitpid(job->pid, &status, WUNTRACED);
    
    // Reset terminal control to shell
    if (tcsetpgrp(STDIN_FILENO, getpgrp()) == -1) {
        perror("tcsetpgrp");
    }
    foreground_pgid = 0;
    
    if (result > 0) {
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            // Job finished
            job->active = 0;
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
    
    if (job->status == JOB_RUNNING) {
        printf("Job already running\n");
        return;
    }
    
    if (job->status != JOB_STOPPED) {
        printf("No such job\n");
        return;
    }
    
    // Resume the job in background
    kill(-job->pgid, SIGCONT);
    job->status = JOB_RUNNING;
    
    char command_copy[MAX_INPUT_SIZE];
    strncpy(command_copy, job->command, MAX_INPUT_SIZE - 1);
    command_copy[MAX_INPUT_SIZE - 1] = '\0';
    
    char *command_name = strtok(command_copy, " ");
    if (!command_name) command_name = job->command;
    printf("[%d] %s &\n", job->job_id, command_name);
}

void builtin_activities(char **args) {
    (void)args;  // Suppress unused parameter warning
    
    // Collect active jobs for sorting
    typedef struct {
        char command_name[MAX_INPUT_SIZE];
        pid_t pid;
        JobStatus status;
    } ActivityInfo;
    
    ActivityInfo activities[MAX_BACKGROUND_JOBS];
    int count = 0;
    
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
        if (background_jobs[i].active && 
            (background_jobs[i].status == JOB_RUNNING || background_jobs[i].status == JOB_STOPPED)) {
            
            // Extract command name
            char command_copy[MAX_INPUT_SIZE];
            strncpy(command_copy, background_jobs[i].command, MAX_INPUT_SIZE - 1);
            command_copy[MAX_INPUT_SIZE - 1] = '\0';
            
            char *command_name = strtok(command_copy, " ");
            if (!command_name) command_name = background_jobs[i].command;
            
            strncpy(activities[count].command_name, command_name, MAX_INPUT_SIZE - 1);
            activities[count].command_name[MAX_INPUT_SIZE - 1] = '\0';
            activities[count].pid = background_jobs[i].pid;
            activities[count].status = background_jobs[i].status;
            count++;
        }
    }
    
    // Sort lexicographically by command name
    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (strcmp(activities[i].command_name, activities[j].command_name) > 0) {
                ActivityInfo temp = activities[i];
                activities[i] = activities[j];
                activities[j] = temp;
            }
        }
    }
    
    // Print sorted activities
    for (int i = 0; i < count; i++) {
        const char *status_str = (activities[i].status == JOB_RUNNING) ? "Running" : "Stopped";
        printf("[%d] : %s - %s\n", activities[i].pid, activities[i].command_name, status_str);
    }
}

void builtin_ping(char **args) {
    if (!args[1] || !args[2]) {
        printf("Usage: ping <pid> <signal_number>\n");
        return;
    }
    
    pid_t pid = atoi(args[1]);
    int signal_number = atoi(args[2]);
    
    // Take modulo 32
    int actual_signal = signal_number % 32;
    
    if (kill(pid, actual_signal) == -1) {
        if (errno == ESRCH) {
            printf("No such process found\n");
        } else {
            perror("kill");
        }
    } else {
        printf("Sent signal %d to process with pid %d\n", signal_number, pid);
    }
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
    int recent_index = -1;
    int max_job_id = 0;
    
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
        if (background_jobs[i].active && background_jobs[i].job_id > max_job_id) {
            max_job_id = background_jobs[i].job_id;
            recent_index = i;
        }
    }
    
    return recent_index;
}

void print_job_status(void) {
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
        if (background_jobs[i].active && background_jobs[i].status == JOB_DONE) {
            char command_copy[MAX_INPUT_SIZE];
            strncpy(command_copy, background_jobs[i].command, MAX_INPUT_SIZE - 1);
            command_copy[MAX_INPUT_SIZE - 1] = '\0';
            
            char *command_name = strtok(command_copy, " ");
            if (!command_name) command_name = background_jobs[i].command;
            
            printf("%s with pid %d exited normally\n", command_name, background_jobs[i].pid);
            background_jobs[i].active = 0;
        }
    }
}

void cleanup_finished_jobs(void) {
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
        if (background_jobs[i].active && background_jobs[i].status == JOB_DONE) {
            background_jobs[i].active = 0;
        }
    }
}