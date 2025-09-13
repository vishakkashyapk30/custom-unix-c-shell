#include "activities.h"
#include "shell.h"
#include "fg_bg.h"
#include <sys/stat.h>

// LLM Code begins
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

void builtin_activities(char **args) {
    (void)args;  // Suppress unused parameter warning
    
    // Collect active jobs for sorting
    ProcessInfo activities[MAX_BACKGROUND_JOBS];
    int count = 0;
    
    for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
        if (background_jobs[i].active && 
            (background_jobs[i].status == JOB_RUNNING || background_jobs[i].status == JOB_STOPPED)) {
            
            // Extract command name (first word)
            char command_copy[MAX_INPUT_SIZE];
            strncpy(command_copy, background_jobs[i].command, MAX_INPUT_SIZE - 1);
            command_copy[MAX_INPUT_SIZE - 1] = '\0';
            
            char *command_name = strtok(command_copy, " ");
            if (!command_name) command_name = background_jobs[i].command;
            
            strncpy(activities[count].command, command_name, 255);
            activities[count].command[255] = '\0';
            activities[count].pid = background_jobs[i].pid;
            
            // Set state string
            if (background_jobs[i].status == JOB_RUNNING) {
                strcpy(activities[count].state, "Running");
            } else {
                strcpy(activities[count].state, "Stopped");
            }
            
            count++;
        }
    }
    
    // Sort lexicographically by command name
    qsort(activities, count, sizeof(ProcessInfo), compare_processes);
    
    // Print sorted activities
    for (int i = 0; i < count; i++) {
        printf("[%d] : %s - %s\n", activities[i].pid, activities[i].command, activities[i].state);
    }
}
// LLM Code ends