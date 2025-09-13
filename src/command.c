// #include "shell.h"
// #include "builtins.h"
// #include "input.h"
// #include "fg_bg.h"
// #include <unistd.h>
// #include <fcntl.h>
// #include <sys/wait.h>

// pid_t create_process(char **args, int input_fd, int output_fd, int pipe_fds[][2], int num_pipes) {
//     pid_t pid = fork();
    
//     if (pid == 0) {
//         // Child process
//         if (input_fd != STDIN_FILENO) {
//             dup2(input_fd, STDIN_FILENO);
//             close(input_fd);
//         }
        
//         if (output_fd != STDOUT_FILENO) {
//             dup2(output_fd, STDOUT_FILENO);
//             close(output_fd);
//         }
        
//         // Close all pipe file descriptors in child
//         for (int i = 0; i < num_pipes; i++) {
//             close(pipe_fds[i][0]);
//             close(pipe_fds[i][1]);
//         }
        
//         execvp(args[0], args);
//         // If execvp returns, the command could not be executed
//         fprintf(stderr, "Command not found!\n");
//         _exit(127);
//     }
    
//     return pid;
// }

// pid_t create_builtin_process(char **args, int input_fd, int output_fd, int pipe_fds[][2], int num_pipes) {
//     pid_t pid = fork();
    
//     if (pid == 0) {
//         // Child process - redirect I/O
//         if (input_fd != STDIN_FILENO) {
//             dup2(input_fd, STDIN_FILENO);
//             close(input_fd);
//         }
        
//         if (output_fd != STDOUT_FILENO) {
//             dup2(output_fd, STDOUT_FILENO);
//             close(output_fd);
//         }
        
//         // Close all pipe file descriptors in child
//         for (int i = 0; i < num_pipes; i++) {
//             close(pipe_fds[i][0]);
//             close(pipe_fds[i][1]);
//         }
        
//         // Execute builtin command
//         if (strcmp(args[0], "reveal") == 0) {
//             builtin_reveal(args);
//         } else if (strcmp(args[0], "log") == 0) {
//             builtin_log(args);
//         } else if (strcmp(args[0], "hop") == 0) {
//             builtin_hop(args);
//         }
        
//         exit(0);
//     }
    
//     return pid;
// }

// int execute_builtin_with_redirection(const char *builtin_name, char **args, char *input_file, char *output_file, int append_output) {
//     int input_fd = STDIN_FILENO;
//     int output_fd = STDOUT_FILENO;
//     int saved_stdout = -1;
//     int saved_stdin = -1;
    
//     if (input_file) {
//         input_fd = open(input_file, O_RDONLY);
//         if (input_fd == -1) {
//             perror(input_file);
//             return -1;
//         }
//         saved_stdin = dup(STDIN_FILENO);
//         if (dup2(input_fd, STDIN_FILENO) == -1) {
//             perror("dup2");
//             close(input_fd);
//             if (saved_stdin != -1) {
//                 close(saved_stdin);
//             }
//             return -1;
//         }
//     }
    
//     if (output_file) {
//         int flags = O_WRONLY | O_CREAT;
//         flags |= append_output ? O_APPEND : O_TRUNC;
//         output_fd = open(output_file, flags, 0644);
//         if (output_fd == -1) {
//             printf("Unable to create file for writing\n");
//             if (input_fd != STDIN_FILENO) {
//                 close(input_fd);
//                 if (saved_stdin != -1) {
//                     dup2(saved_stdin, STDIN_FILENO);
//                     close(saved_stdin);
//                 }
//             }
//             return -1;
//         }
//         saved_stdout = dup(STDOUT_FILENO);
//         dup2(output_fd, STDOUT_FILENO);
//     }
    
//     if (strcmp(builtin_name, "reveal") == 0) {
//         builtin_reveal(args);
//     } else if (strcmp(builtin_name, "log") == 0) {
//         builtin_log(args);
//     } else if (strcmp(builtin_name, "hop") == 0) {
//         builtin_hop(args);
//     }
    
//     // Ensure output is flushed before restoring file descriptors
//     if (saved_stdout != -1) {
//         fflush(stdout);
//     }
    
//     if (saved_stdout != -1) {
//         dup2(saved_stdout, STDOUT_FILENO);
//         close(saved_stdout);
//         close(output_fd);
//     }
//     if (saved_stdin != -1) {
//         dup2(saved_stdin, STDIN_FILENO);
//         close(saved_stdin);
//         close(input_fd);
//     }
    
//     return 0;
// }

// // int execute_single_command(char **args, char *input_file, char *output_file, int append_output) {
// //     if (!args || !args[0]) return -1;
    
// //     // Check if it's a builtin command
// //     if (is_builtin_command(args[0])) {
// //         return execute_builtin_with_redirection(args[0], args, input_file, output_file, append_output);
// //     }
    
// //     // External command
// //     int input_fd = STDIN_FILENO;
// //     int output_fd = STDOUT_FILENO;
    
// //     if (input_file) {
// //         input_fd = open(input_file, O_RDONLY);
// //         if (input_fd == -1) {
// //             perror(input_file);
// //             return -1;
// //         }
// //     }
    
// //     if (output_file) {
// //         int flags = O_WRONLY | O_CREAT;
// //         flags |= append_output ? O_APPEND : O_TRUNC;
// //         output_fd = open(output_file, flags, 0644);
// //         if (output_fd == -1) {
// //             printf("Unable to create file for writing\n");
// //             if (input_fd != STDIN_FILENO) close(input_fd);
// //             return -1;
// //         }
// //     }
    
// //     pid_t pid = create_process(args, input_fd, output_fd, NULL, 0);
    
// //     if (input_fd != STDIN_FILENO) close(input_fd);
// //     if (output_fd != STDOUT_FILENO) close(output_fd);
    
// //     // Wait for the child process
// //     int status;
// //     waitpid(pid, &status, WUNTRACED);
    
// //     return WEXITSTATUS(status);
// // }

// // ... existing code ...

// int execute_single_command(char **args, char *input_file, char *output_file, int append_output) {
//     if (!args || !args[0]) return -1;
    
//     // Check if it's a builtin command
//     if (is_builtin_command(args[0])) {
//         return execute_builtin_with_redirection(args[0], args, input_file, output_file, append_output);
//     }
    
//     // External command
//     int input_fd = STDIN_FILENO;
//     int output_fd = STDOUT_FILENO;
    
//     if (input_file) {
//         input_fd = open(input_file, O_RDONLY);
//         if (input_fd == -1) {
//             perror(input_file);
//             return -1;
//         }
//     }
    
//     if (output_file) {
//         int flags = O_WRONLY | O_CREAT;
//         flags |= append_output ? O_APPEND : O_TRUNC;
//         output_fd = open(output_file, flags, 0644);
//         if (output_fd == -1) {
//             printf("Unable to create file for writing\n");
//             if (input_fd != STDIN_FILENO) close(input_fd);
//             return -1;
//         }
//     }
    
//     pid_t pid = fork();
    
//     if (pid == 0) {
//         // Child process
//         setpgid(0, 0);  // Create new process group
        
//         if (input_fd != STDIN_FILENO) {
//             dup2(input_fd, STDIN_FILENO);
//             close(input_fd);
//         }
        
//         if (output_fd != STDOUT_FILENO) {
//             dup2(output_fd, STDOUT_FILENO);
//             close(output_fd);
//         }
        
//         execvp(args[0], args);
//         // If execvp returns, the command could not be executed
//         fprintf(stderr, "Command not found!\n");
//         _exit(127);
//     } else if (pid > 0) {
//         // Parent process
//         if (input_fd != STDIN_FILENO) close(input_fd);
//         if (output_fd != STDOUT_FILENO) close(output_fd);
        
//         // Set as foreground process group
//         set_foreground_process_group(pid);
        
//         // Wait for the child process
//         int status;
//         waitpid(pid, &status, WUNTRACED);
        
//         // Reset foreground process group
//         reset_foreground_process_group();
        
//         return WEXITSTATUS(status);
//     } else {
//         perror("fork");
//         if (input_fd != STDIN_FILENO) close(input_fd);
//         if (output_fd != STDOUT_FILENO) close(output_fd);
//         return -1;
//     }
// }

#include "shell.h"
#include "builtins.h"
#include "input.h"
#include "fg_bg.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>

pid_t create_process(char **args, int input_fd, int output_fd, int pipe_fds[][2], int num_pipes) {
    pid_t pid = fork();
    
    if (pid == 0) {
        // Child process
        if (input_fd != STDIN_FILENO) {
            dup2(input_fd, STDIN_FILENO);
            close(input_fd);
        }
        
        if (output_fd != STDOUT_FILENO) {
            dup2(output_fd, STDOUT_FILENO);
            close(output_fd);
        }
        
        // Close all pipe file descriptors in child
        for (int i = 0; i < num_pipes; i++) {
            close(pipe_fds[i][0]);
            close(pipe_fds[i][1]);
        }
        
        execvp(args[0], args);
        // If execvp returns, the command could not be executed
        fprintf(stderr, "Command not found!\n");
        _exit(127);
    }
    
    return pid;
}

pid_t create_builtin_process(char **args, int input_fd, int output_fd, int pipe_fds[][2], int num_pipes) {
    pid_t pid = fork();
    
    if (pid == 0) {
        // Child process - redirect I/O
        if (input_fd != STDIN_FILENO) {
            dup2(input_fd, STDIN_FILENO);
            close(input_fd);
        }
        
        if (output_fd != STDOUT_FILENO) {
            dup2(output_fd, STDOUT_FILENO);
            close(output_fd);
        }
        
        // Close all pipe file descriptors in child
        for (int i = 0; i < num_pipes; i++) {
            close(pipe_fds[i][0]);
            close(pipe_fds[i][1]);
        }
        
        // Execute builtin command
        if (strcmp(args[0], "reveal") == 0) {
            builtin_reveal(args);
        } else if (strcmp(args[0], "log") == 0) {
            builtin_log(args);
        } else if (strcmp(args[0], "hop") == 0) {
            builtin_hop(args);
        }
        
        exit(0);
    }
    
    return pid;
}

int execute_builtin_with_redirection(const char *builtin_name, char **args, char *input_file, char *output_file, int append_output) {
    int input_fd = STDIN_FILENO;
    int output_fd = STDOUT_FILENO;
    int saved_stdout = -1;
    int saved_stdin = -1;
    
    if (input_file) {
        input_fd = open(input_file, O_RDONLY);
        if (input_fd == -1) {
            perror(input_file);
            return -1;
        }
        saved_stdin = dup(STDIN_FILENO);
        if (dup2(input_fd, STDIN_FILENO) == -1) {
            perror("dup2");
            close(input_fd);
            if (saved_stdin != -1) {
                close(saved_stdin);
            }
            return -1;
        }
    }
    
    if (output_file) {
        int flags = O_WRONLY | O_CREAT;
        flags |= append_output ? O_APPEND : O_TRUNC;
        output_fd = open(output_file, flags, 0644);
        if (output_fd == -1) {
            printf("Unable to create file for writing\n");
            if (input_fd != STDIN_FILENO) {
                close(input_fd);
                if (saved_stdin != -1) {
                    dup2(saved_stdin, STDIN_FILENO);
                    close(saved_stdin);
                }
            }
            return -1;
        }
        saved_stdout = dup(STDOUT_FILENO);
        dup2(output_fd, STDOUT_FILENO);
    }
    
    if (strcmp(builtin_name, "reveal") == 0) {
        builtin_reveal(args);
    } else if (strcmp(builtin_name, "log") == 0) {
        builtin_log(args);
    } else if (strcmp(builtin_name, "hop") == 0) {
        builtin_hop(args);
    }
    
    // Ensure output is flushed before restoring file descriptors
    if (saved_stdout != -1) {
        fflush(stdout);
    }
    
    if (saved_stdout != -1) {
        dup2(saved_stdout, STDOUT_FILENO);
        close(saved_stdout);
        close(output_fd);
    }
    if (saved_stdin != -1) {
        dup2(saved_stdin, STDIN_FILENO);
        close(saved_stdin);
        close(input_fd);
    }
    
    return 0;
}

int execute_single_command(char **args, char *input_file, char *output_file, int append_output) {
    if (!args || !args[0]) return -1;
    
    // Check if it's a builtin command
    if (is_builtin_command(args[0])) {
        return execute_builtin_with_redirection(args[0], args, input_file, output_file, append_output);
    }
    
    // External command
    int input_fd = STDIN_FILENO;
    int output_fd = STDOUT_FILENO;
    
    if (input_file) {
        input_fd = open(input_file, O_RDONLY);
        if (input_fd == -1) {
            perror(input_file);
            return -1;
        }
    }
    
    if (output_file) {
        int flags = O_WRONLY | O_CREAT;
        flags |= append_output ? O_APPEND : O_TRUNC;
        output_fd = open(output_file, flags, 0644);
        if (output_fd == -1) {
            printf("Unable to create file for writing\n");
            if (input_fd != STDIN_FILENO) close(input_fd);
            return -1;
        }
    }
    
    pid_t pid = fork();
    
    if (pid == 0) {
        // Child process - like reference implementation
        signal(SIGTSTP, SIG_DFL);  // Reset SIGTSTP to default in child
        
        if (input_fd != STDIN_FILENO) {
            dup2(input_fd, STDIN_FILENO);
            close(input_fd);
        }
        
        if (output_fd != STDOUT_FILENO) {
            dup2(output_fd, STDOUT_FILENO);
            close(output_fd);
        }
        
        execvp(args[0], args);
        // If execvp returns, the command could not be executed
        printf("Command not found!\n");
        exit(0);
    } else if (pid > 0) {
        // Parent process - following reference implementation pattern
        if (input_fd != STDIN_FILENO) close(input_fd);
        if (output_fd != STDOUT_FILENO) close(output_fd);
        
        // Set process group like reference implementation
        setpgid(pid, 0);
        
        // Ignore SIGTTOU before terminal control operations
        signal(SIGTTOU, SIG_IGN);
        
        // Set foreground process group to the child process
        tcsetpgrp(STDIN_FILENO, pid);
        foreground_pgid = pid;
        
        // Wait for the child process
        int status;
        waitpid(pid, &status, WUNTRACED);
        
        // Reset terminal control back to shell
        tcsetpgrp(STDIN_FILENO, getpgrp());
        foreground_pgid = 0;
        
        // Handle stopped processes (Ctrl-Z)
        if (WIFSTOPPED(status)) {
            // Add to background jobs
            add_background_job(pid, pid, args[0]);
            // Find the job that was just added
            for (int i = 0; i < MAX_BACKGROUND_JOBS; i++) {
                if (background_jobs[i].active && background_jobs[i].pid == pid) {
                    background_jobs[i].status = JOB_STOPPED;
                    printf("[%d] Stopped %s\n", background_jobs[i].job_id, args[0]);
                    fflush(stdout);
                    break;
                }
            }
            return 0; // Return 0 for stopped processes
        }
        
        return WEXITSTATUS(status);
    } else {
        perror("fork");
        if (input_fd != STDIN_FILENO) close(input_fd);
        if (output_fd != STDOUT_FILENO) close(output_fd);
        return -1;
    }
}

int execute_pipeline(char ***commands, int command_count, char *input_file, char *output_file, int append_output) {
    if (command_count == 1) {
        return execute_single_command(commands[0], input_file, output_file, append_output);
    }
    
    int pipe_fds[command_count - 1][2];
    pid_t pids[command_count];
    
    // Create all pipes
    for (int i = 0; i < command_count - 1; i++) {
        if (pipe(pipe_fds[i]) == -1) {
            perror("pipe");
            return -1;
        }
    }
    
    // Execute each command in the pipeline
    for (int i = 0; i < command_count; i++) {
        int input_fd = STDIN_FILENO;
        int output_fd = STDOUT_FILENO;
        
        // Set up pipe connections
        if (i > 0) {
            input_fd = pipe_fds[i-1][0];
        }
        if (i < command_count - 1) {
            output_fd = pipe_fds[i][1];
        }
        
        // Handle file redirections
        if (i == 0 && input_file) {
            int file_fd = open(input_file, O_RDONLY);
            if (file_fd == -1) {
                printf("No such file or directory\n");
                // Close all created pipes
                for (int j = 0; j < command_count - 1; j++) {
                    close(pipe_fds[j][0]);
                    close(pipe_fds[j][1]);
                }
                return -1;
            }
            input_fd = file_fd;
        }
        
        if (i == command_count - 1 && output_file) {
            int flags = O_WRONLY | O_CREAT;
            flags |= append_output ? O_APPEND : O_TRUNC;
            int file_fd = open(output_file, flags, 0644);
            if (file_fd == -1) {
                printf("Unable to create file for writing\n");
                // Close all created pipes
                for (int j = 0; j < command_count - 1; j++) {
                    close(pipe_fds[j][0]);
                    close(pipe_fds[j][1]);
                }
                return -1;
            }
            output_fd = file_fd;
        }
        
        // Create process for this command
        if (is_builtin_command(commands[i][0])) {
            pids[i] = create_builtin_process(commands[i], input_fd, output_fd, pipe_fds, command_count - 1);
        } else {
            pids[i] = create_process(commands[i], input_fd, output_fd, pipe_fds, command_count - 1);
        }
        
        if (pids[i] < 0) {
            perror("fork");
            // Close all created pipes
            for (int j = 0; j < command_count - 1; j++) {
                close(pipe_fds[j][0]);
                close(pipe_fds[j][1]);
            }
            return -1;
        }
    }
    
    // Close all pipe file descriptors in parent
    for (int i = 0; i < command_count - 1; i++) {
        close(pipe_fds[i][0]);
        close(pipe_fds[i][1]);
    }
    
    // Wait for all processes and collect exit statuses
    int last_status = 0;
    int command_not_found = 0;
    
    for (int i = 0; i < command_count; i++) {
        int status;
        pid_t waited_pid = waitpid(pids[i], &status, 0);
        
        if (waited_pid == pids[i]) {
            int exit_status = WEXITSTATUS(status);
            if (exit_status == 127) {
                command_not_found = 1;
            }
            if (i == command_count - 1) {
                last_status = exit_status;
            }
        }
    }
    
    // Print error message if any command was not found
    if (command_not_found) {
        printf("Command not found!\n");
        fflush(stdout);
    }
    
    return last_status;
}

// Function to execute a single command in background
void execute_background_command(char **args, char *input_file, char *output_file, int append_output) {
    pid_t pid = fork();
    
    if (pid == 0) {
        // Child process
        setpgid(0, 0);
        
        // Handle file redirections
        if (input_file) {
            int input_fd = open(input_file, O_RDONLY);
            if (input_fd == -1) {
                printf("No such file or directory\n");
                _exit(EXIT_FAILURE);
            }
            if (dup2(input_fd, STDIN_FILENO) == -1) {
                perror("dup2");
                _exit(EXIT_FAILURE);
            }
            close(input_fd);
        }
        
        if (output_file) {
            int flags = O_WRONLY | O_CREAT;
            flags |= append_output ? O_APPEND : O_TRUNC;
            int output_fd = open(output_file, flags, 0644);
            if (output_fd == -1) {
                printf("Unable to create file for writing\n");
                _exit(EXIT_FAILURE);
            }
            if (dup2(output_fd, STDOUT_FILENO) == -1) {
                perror("dup2");
                _exit(EXIT_FAILURE);
            }
            close(output_fd);
        }
        
        // Execute the command
        if (is_builtin_command(args[0])) {
            if (strcmp(args[0], "reveal") == 0) {
                builtin_reveal(args);
            } else if (strcmp(args[0], "log") == 0) {
                builtin_log(args);
            } else if (strcmp(args[0], "hop") == 0) {
                builtin_hop(args);
            } else if (strcmp(args[0], "activities") == 0) {
                builtin_activities(args);
            } else if (strcmp(args[0], "ping") == 0) {
                builtin_ping(args);
            }
            _exit(0);
        } else {
            execvp(args[0], args);
            _exit(127);
        }
    } else if (pid > 0) {
        // Parent process - add to job control system
        int job_index = add_background_job(pid, pid, args[0]);
        if (job_index >= 0) {
            printf("[%d] %d\n", background_jobs[job_index].job_id, pid);
            fflush(stdout);
        }
    } else {
        perror("fork");
    }
}

// Function to execute a pipeline in background
void execute_background_pipeline(char ***commands, int command_count, char *input_file, char *output_file, int append_output) {
    if (command_count == 1) {
        execute_background_command(commands[0], input_file, output_file, append_output);
        return;
    }
    
    int pipe_fds[command_count - 1][2];
    pid_t pids[command_count];
    
    // Create all pipes
    for (int i = 0; i < command_count - 1; i++) {
        if (pipe(pipe_fds[i]) == -1) {
            perror("pipe");
            return;
        }
    }
    
    // Execute each command in the pipeline
    for (int i = 0; i < command_count; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            setpgid(0, 0);
            
            // Set up pipe connections
            if (i > 0) {
                if (dup2(pipe_fds[i-1][0], STDIN_FILENO) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }
            if (i < command_count - 1) {
                if (dup2(pipe_fds[i][1], STDOUT_FILENO) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }
            
            // Close all pipe file descriptors in child
            for (int j = 0; j < command_count - 1; j++) {
                close(pipe_fds[j][0]);
                close(pipe_fds[j][1]);
            }
            
            // Handle file redirections
            if (i == 0 && input_file) {
                int input_fd = open(input_file, O_RDONLY);
                if (input_fd == -1) {
                    printf("No such file or directory\n");
                    exit(EXIT_FAILURE);
                }
                if (dup2(input_fd, STDIN_FILENO) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
                close(input_fd);
            }
            
            if (i == command_count - 1 && output_file) {
                int flags = O_WRONLY | O_CREAT;
                flags |= append_output ? O_APPEND : O_TRUNC;
                int output_fd = open(output_file, flags, 0644);
                if (output_fd == -1) {
                    printf("Unable to create file for writing\n");
                    exit(EXIT_FAILURE);
                }
                if (dup2(output_fd, STDOUT_FILENO) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
                close(output_fd);
            }
            
            // Execute the command
            if (is_builtin_command(commands[i][0])) {
                if (strcmp(commands[i][0], "reveal") == 0) {
                    builtin_reveal(commands[i]);
                } else if (strcmp(commands[i][0], "log") == 0) {
                    builtin_log(commands[i]);
                } else if (strcmp(commands[i][0], "hop") == 0) {
                    builtin_hop(commands[i]);
                } else if (strcmp(commands[i][0], "activities") == 0) {
                    builtin_activities(commands[i]);
                } else if (strcmp(commands[i][0], "ping") == 0) {
                    builtin_ping(commands[i]);
                }
                exit(0);
            } else {
                execvp(commands[i][0], commands[i]);
                _exit(127);
            }
        } else if (pid < 0) {
            perror("fork");
            return;
        } else {
            pids[i] = pid;
        }
    }
    
    // Close all pipe file descriptors in parent
    for (int i = 0; i < command_count - 1; i++) {
        close(pipe_fds[i][0]);
        close(pipe_fds[i][1]);
    }
    
    // Store the last process as the background job
    int job_index = add_background_job(pids[command_count - 1], pids[command_count - 1], commands[command_count - 1][0]);
    if (job_index >= 0) {
        printf("[%d] %d\n", background_jobs[job_index].job_id, pids[command_count - 1]);
        fflush(stdout);
    }
}

void check_background_jobs(void) {
    cleanup_finished_jobs();
}