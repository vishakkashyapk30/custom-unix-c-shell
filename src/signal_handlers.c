#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "signal_handlers.h"
#include "fg_bg.h"
#include <sys/wait.h>
#include <termios.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void setup_signal_handlers(void) {
    struct sigaction sa;
    
    // Handle SIGINT
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Error setting up SIGINT handler");
        exit(EXIT_FAILURE);
    }

    // Handle SIGTSTP  
    sa.sa_handler = sigtstp_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    if (sigaction(SIGTSTP, &sa, NULL) == -1) {
        perror("Error setting up SIGTSTP handler");
        exit(EXIT_FAILURE);
    }
    
    // Set up SIGCHLD handler
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction SIGCHLD");
    }
}

void sigint_handler(int sig) {
    (void)sig;
    if (foreground_pgid > 0) {
        kill(foreground_pgid, SIGKILL);
    }
}

void sigtstp_handler(int sig) {
    (void)sig;
    // Handle Ctrl-Z - this will be handled in execute_single_command via WUNTRACED
}

void sigchld_handler(int sig) {
    (void)sig;  // Suppress unused parameter warning
    
    pid_t pid;
    int status;
    
    // Reap all available children
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        // Find the job
        for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
            if (background_jobs[i].active && background_jobs[i].pid == pid) {
                if (WIFEXITED(status) || WIFSIGNALED(status)) {
                    // Job finished - print notification
                    char command_copy[MAX_INPUT_SIZE];
                    strncpy(command_copy, background_jobs[i].command, MAX_INPUT_SIZE - 1);
                    command_copy[MAX_INPUT_SIZE - 1] = '\0';
                    
                    char *command_name = strtok(command_copy, " ");
                    if (!command_name) command_name = background_jobs[i].command;
                    
                    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
                        printf("%s with pid %d exited normally\n", command_name, pid);
                    } else {
                        printf("%s with pid %d exited abnormally\n", command_name, pid);
                    }
                    fflush(stdout);
                    
                    background_jobs[i].active = 0; // Mark as inactive
                }
                break;
            }
        }
    }
}

void handle_eof(void) {
    // Kill all background processes
    kill_all_background_jobs();
    
    // Wait for all background processes to terminate
    wait_for_background_jobs();
    
    // Print logout message
    printf("logout\n");
    fflush(stdout);
    
    // Exit with status 0
    exit(0);
}

void set_foreground_process_group(pid_t pgid) {
    // Ignore SIGTTOU to avoid being stopped when changing terminal process group
    signal(SIGTTOU, SIG_IGN);
    
    // Set the foreground process group
    if (tcsetpgrp(STDIN_FILENO, pgid) == -1) {
        perror("tcsetpgrp");
    }
    
    foreground_pgid = pgid;
}

void reset_foreground_process_group(void) {
    // Reset the foreground process group back to the shell
    if (tcsetpgrp(STDIN_FILENO, getpgrp()) == -1) {
        perror("tcsetpgrp");
    }
    
    foreground_pgid = 0;
}