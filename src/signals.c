#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700
#define _DEFAULT_SOURCE
#define _GNU_SOURCE

#include "signals.h"
#include "jobs.h"

void setup_signal_handlers(void) {
    struct sigaction sa_int, sa_tstp, sa_chld;
    
    // Set up SIGINT handler (Ctrl-C)
    sa_int.sa_handler = sigint_handler;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = SA_RESTART;
    if (sigaction(SIGINT, &sa_int, NULL) == -1) {
        perror("sigaction SIGINT");
    }
    
    // Set up SIGTSTP handler (Ctrl-Z)
    sa_tstp.sa_handler = sigtstp_handler;
    sigemptyset(&sa_tstp.sa_mask);
    sa_tstp.sa_flags = SA_RESTART;
    if (sigaction(SIGTSTP, &sa_tstp, NULL) == -1) {
        perror("sigaction SIGTSTP");
    }
    
    // Set up SIGCHLD handler
    sa_chld.sa_handler = sigchld_handler;
    sigemptyset(&sa_chld.sa_mask);
    sa_chld.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    if (sigaction(SIGCHLD, &sa_chld, NULL) == -1) {
        perror("sigaction SIGCHLD");
    }
}

void sigint_handler(int sig) {
    (void)sig;  // Suppress unused parameter warning
    
    if (foreground_pgid > 0) {
        // Send SIGINT to foreground process group
        if (kill(-foreground_pgid, SIGINT) == -1) {
            perror("kill SIGINT");
        }
    }
    
    // Don't terminate the shell
    if (write(STDOUT_FILENO, "\n", 1) == -1) {
        perror("write");
    }
}

void sigtstp_handler(int sig) {
    (void)sig;  // Suppress unused parameter warning
    
    if (foreground_pgid > 0) {
        // Send SIGTSTP to foreground process group
        if (kill(-foreground_pgid, SIGTSTP) == -1) {
            perror("kill SIGTSTP");
        }
        
        // Move to background with stopped status
        for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
            if (background_jobs[i].active && background_jobs[i].pgid == foreground_pgid) {
                background_jobs[i].status = JOB_STOPPED;
                
                // Extract command name
                char command_copy[MAX_INPUT_SIZE];
                strncpy(command_copy, background_jobs[i].command, MAX_INPUT_SIZE - 1);
                command_copy[MAX_INPUT_SIZE - 1] = '\0';
                
                char *command_name = strtok(command_copy, " ");
                if (!command_name) command_name = background_jobs[i].command;
                
                printf("\n[%d] Stopped %s\n", background_jobs[i].job_id, command_name);
                fflush(stdout);
                break;
            }
        }
        
        foreground_pgid = 0;  // No foreground process now
    }
    
    if (write(STDOUT_FILENO, "\n", 1) == -1) {
        perror("write");
    }
}

void sigchld_handler(int sig) {
    (void)sig;  // Suppress unused parameter warning
    
    pid_t pid;
    int status;
    
    // Reap all available children
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
        // Find the job
        for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
            if (background_jobs[i].active && background_jobs[i].pid == pid) {
                if (WIFEXITED(status) || WIFSIGNALED(status)) {
                    // Job finished
                    background_jobs[i].status = JOB_DONE;
                } else if (WIFSTOPPED(status)) {
                    // Job stopped
                    background_jobs[i].status = JOB_STOPPED;
                } else if (WIFCONTINUED(status)) {
                    // Job continued
                    background_jobs[i].status = JOB_RUNNING;
                }
                break;
            }
        }
    }
}

void ignore_terminal_signals(void) {
    signal(SIGINT, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
}

void restore_terminal_signals(void) {
    signal(SIGINT, SIG_DFL);
    signal(SIGTSTP, SIG_DFL);
    signal(SIGTTOU, SIG_DFL);
    signal(SIGTTIN, SIG_DFL);
}