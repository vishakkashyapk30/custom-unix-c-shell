#include "activities.h"
#include "shell.h"
#include "fg_bg.h"
#include <sys/stat.h>

// Structure to hold process information for sorting
typedef struct {
    int pid;
    char command[256];
    char state[32];
} ProcessInfo;

// Comparison function for sorting by command name
int compare_processes(const void *a, const void *b) {
    const ProcessInfo *proc_a = (const ProcessInfo *)a;
    const ProcessInfo *proc_b = (const ProcessInfo *)b;
    return strcmp(proc_a->command, proc_b->command);
}

// Function to get process state from /proc/pid/status
char* get_process_state(int pid) {
    char path[256];
    sprintf(path, "/proc/%d/status", pid);
    
    FILE *file = fopen(path, "r");
    if (file == NULL) {
        return NULL; // Process doesn't exist
    }
    
    char line[512];
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "State:", 6) == 0) {
            fclose(file);
            
            // Extract state from "State: R (running)" format
            char *state_start = strchr(line, ' ');
            if (state_start) {
                state_start++; // Skip the space
                char *state_end = strchr(state_start, ' ');
                if (state_end) {
                    *state_end = '\0';
                } else {
                    // Remove newline if no space found
                    char *newline = strchr(state_start, '\n');
                    if (newline) {
                        *newline = '\0';
                    }
                }
                
                // Map process states to required format
                if (strcmp(state_start, "R") == 0 || strcmp(state_start, "S") == 0) {
                    return "Running";
                } else if (strcmp(state_start, "T") == 0) {
                    return "Stopped";
                } else {
                    return "Running"; // Default to Running for other states
                }
            }
        }
    }
    
    fclose(file);
    return "Running"; // Default state
}

// Function to check if a process exists
int process_exists(int pid) {
    char path[256];
    sprintf(path, "/proc/%d/status", pid);
    return access(path, F_OK) == 0;
}

void builtin_activities(char **args) {
    // Use the new job control system
    int found_jobs = 0;
    
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
        if (background_jobs[i].active && 
            (background_jobs[i].status == JOB_RUNNING || background_jobs[i].status == JOB_STOPPED)) {
            
            // Extract command name
            char command_copy[MAX_INPUT_SIZE];
            strncpy(command_copy, background_jobs[i].command, MAX_INPUT_SIZE - 1);
            command_copy[MAX_INPUT_SIZE - 1] = '\0';
            
            char *command_name = strtok(command_copy, " ");
            if (!command_name) command_name = background_jobs[i].command;
            
            const char *status_str = (background_jobs[i].status == JOB_RUNNING) ? "Running" : "Stopped";
            printf("[%d] : %s - %s\n", background_jobs[i].pid, command_name, status_str);
            found_jobs = 1;
        }
    }
    
    if (!found_jobs) {
        printf("No activities found\n");
    }
}

