#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "signal_handlers.h"
#include "jobs.h"
#include <sys/wait.h>
#include <termios.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Global variable definition
pid_t foreground_pgid = 0;

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

// ...existing code...

void sigint_handler(int sig) {
    (void)sig;  // Suppress unused parameter warning
    
    if (foreground_pgid > 0) {
        // Send SIGINT to foreground process group
        if (kill(-foreground_pgid, SIGINT) == -1) {
            if (errno != ESRCH) {
                perror("kill SIGINT");
            }
        }
    }
    
    // Print newline but don't terminate shell
    if (write(STDOUT_FILENO, "\n", 1) == -1) {
        perror("write");
    }
}

void sigtstp_handler(int sig) {
    (void)sig;  // Suppress unused parameter warning
    
    if (foreground_pgid > 0) {
        pid_t fg_pid = foreground_pgid;
        
        // Send SIGTSTP to foreground process group
        if (kill(-foreground_pgid, SIGTSTP) == -1) {
            if (errno != ESRCH) {
                perror("kill SIGTSTP");
            }
            return;
        }
        
        // Wait for process to stop
        int status;
        pid_t result = waitpid(-fg_pid, &status, WUNTRACED);
        
        if (result > 0 && WIFSTOPPED(status)) {
            // Get command name
            char command_name[256] = "unknown";
            char cmdline_path[512];
            snprintf(cmdline_path, sizeof(cmdline_path), "/proc/%d/comm", fg_pid);
            
            FILE *comm_file = fopen(cmdline_path, "r");
            if (comm_file) {
                if (fgets(command_name, sizeof(command_name), comm_file)) {
                    // Remove newline
                    command_name[strcspn(command_name, "\n")] = '\0';
                }
                fclose(comm_file);
            }
            
            // Find or add job
            int job_index = -1;
            int job_id = 1;
            
            // Look for existing job
            for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
                if (background_jobs[i].active && 
                    (background_jobs[i].pgid == fg_pid || background_jobs[i].pid == fg_pid)) {
                    job_index = i;
                    job_id = background_jobs[i].job_id;
                    break;
                }
            }
            
            // If not found, add new job
            if (job_index == -1) {
                job_index = add_background_job(fg_pid, fg_pid, command_name);
                if (job_index >= 0) {
                    job_id = background_jobs[job_index].job_id;
                }
            }
            
            // Update job status
            if (job_index >= 0) {
                background_jobs[job_index].status = JOB_STOPPED;
                printf("\n[%d] Stopped %s\n", job_id, command_name);
                fflush(stdout);
            }
        }
        
        foreground_pgid = 0;  // Clear foreground process
    }
    
    // Print newline for prompt
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
    foreground_pgid = pgid;
    if (tcsetpgrp(STDIN_FILENO, pgid) == -1) {
        perror("tcsetpgrp");
    }
}

void reset_foreground_process_group(void) {
    foreground_pgid = 0;
    if (tcsetpgrp(STDIN_FILENO, getpgrp()) == -1) {
        perror("tcsetpgrp");
    }
}