#include "ping.h"
#include "shell.h"
#include <errno.h>

void builtin_ping(char **args) {
    // Check if we have the required arguments
    if (args[1] == NULL || args[2] == NULL) {
        printf("Invalid syntax!\n");
        return;
    }
    
    // Parse PID
    char *pid_str = args[1];
    int pid = atoi(pid_str);
    
    // Validate PID (check if it's a valid number)
    if (pid == 0 && strcmp(pid_str, "0") != 0) {
        printf("Invalid syntax!\n");
        return;
    }
    
    // Parse signal number
    char *signal_str = args[2];
    int signal_num = atoi(signal_str);
    
    // Validate signal number (check if it's a valid number)
    if (signal_num == 0 && strcmp(signal_str, "0") != 0) {
        printf("Invalid syntax!\n");
        return;
    }
    
    // Check if process exists by sending signal 0
    if (kill(pid, 0) == -1) {
        if (errno == ESRCH) {
            printf("No such process found\n");
        } else {
            printf("No such process found\n");
        }
        return;
    }
    
    // Apply modulo 32 to signal number
    int actual_signal = signal_num % 32;
    
    // Send the signal
    if (kill(pid, actual_signal) == -1) {
        if (errno == ESRCH) {
            printf("No such process found\n");
        } else {
            printf("No such process found\n");
        }
        return;
    }
    
    // Success message
    printf("Sent signal %d to process with pid %d\n", actual_signal, pid);
}

