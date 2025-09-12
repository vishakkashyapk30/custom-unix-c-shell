#include "activities.h"
#include "shell.h"
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
    // Check if activities.txt exists
    FILE *activities_file = fopen("activities.txt", "r");
    if (activities_file == NULL) {
        printf("No activities found\n");
        return;
    }
    
    // Check if file is empty
    fseek(activities_file, 0, SEEK_END);
    if (ftell(activities_file) == 0) {
        printf("No activities found\n");
        fclose(activities_file);
        return;
    }
    
    fseek(activities_file, 0, SEEK_SET);
    
    // Read all processes and collect valid ones
    ProcessInfo processes[100];
    int process_count = 0;
    char line[512];
    
    while (fgets(line, sizeof(line), activities_file) && process_count < 100) {
        // Parse line format: "pid: command_name - State"
        char *pid_str = strtok(line, ":");
        if (pid_str == NULL) continue;
        
        int pid = atoi(pid_str);
        if (pid == 0 && strcmp(pid_str, "0") != 0) continue;
        
        // Check if process still exists
        if (!process_exists(pid)) {
            continue; // Skip non-existent processes
        }
        
        // Get command name and current state
        char *command_part = strtok(NULL, "-");
        if (command_part == NULL) continue;
        
        // Trim whitespace from command
        char *command_start = command_part;
        while (*command_start == ' ' || *command_start == '\t') command_start++;
        char *command_end = command_start + strlen(command_start) - 1;
        while (command_end > command_start && (*command_end == ' ' || *command_end == '\t' || *command_end == '\n')) {
            command_end--;
        }
        *(command_end + 1) = '\0';
        
        // Get current process state
        char *current_state = get_process_state(pid);
        if (current_state == NULL) continue;
        
        // Store process information
        processes[process_count].pid = pid;
        strncpy(processes[process_count].command, command_start, 255);
        processes[process_count].command[255] = '\0';
        strncpy(processes[process_count].state, current_state, 31);
        processes[process_count].state[31] = '\0';
        
        process_count++;
    }
    
    fclose(activities_file);
    
    if (process_count == 0) {
        printf("No activities found\n");
        return;
    }
    
    // Sort processes by command name (lexicographically)
    qsort(processes, process_count, sizeof(ProcessInfo), compare_processes);
    
    // Print sorted processes
    for (int i = 0; i < process_count; i++) {
        printf("[%d] : %s - %s\n", processes[i].pid, processes[i].command, processes[i].state);
    }
    
    // Update activities.txt with only valid processes
    FILE *updated_file = fopen("activities.txt", "w");
    if (updated_file != NULL) {
        for (int i = 0; i < process_count; i++) {
            fprintf(updated_file, "%d: %s - %s\n", processes[i].pid, processes[i].command, processes[i].state);
        }
        fclose(updated_file);
    }
}

