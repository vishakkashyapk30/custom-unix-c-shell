#include "ping.h"
#include "shell.h"
#include <errno.h>
#include <ctype.h>

// LLM Code begins
// Helper function to check if string is a valid number
int is_valid_number(const char *str) {
    if (!str || *str == '\0') return 0;
    
    // Handle negative numbers
    if (*str == '-') str++;
    
    // Check if all remaining characters are digits
    while (*str) {
        if (!isdigit(*str)) return 0;
        str++;
    }
    return 1;
}

void builtin_ping(char **args) {
    // Check if we have the required arguments
    if (args[1] == NULL || args[2] == NULL) {
        printf("Invalid syntax!\n");
        return;
    }
    
    // Validate PID
    if (!is_valid_number(args[1])) {
        printf("Invalid syntax!\n");
        return;
    }
    
    // Validate signal number
    if (!is_valid_number(args[2])) {
        printf("Invalid syntax!\n");
        return;
    }
    
    // Parse PID and signal number
    int pid = atoi(args[1]);
    int signal_num = atoi(args[2]);
    
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
    
    // Success message - use original signal number, not modulo
    printf("Sent signal %d to process with pid %d\n", signal_num, pid);
}
// LLM Code ends