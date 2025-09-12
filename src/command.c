#include "shell.h"
#include "builtins.h"
#include "input.h"

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
        _exit(127);
    }
    
    return pid;
}

// New function to handle builtins in pipelines
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
            printf("No such file or directory\n");
            return -1;
        }
        saved_stdin = dup(STDIN_FILENO);
        dup2(input_fd, STDIN_FILENO);
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
            printf("No such file or directory\n");
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
    
    pid_t pid = create_process(args, input_fd, output_fd, NULL, 0);
    
    if (input_fd != STDIN_FILENO) close(input_fd);
    if (output_fd != STDOUT_FILENO) close(output_fd);
    
    int status;
    waitpid(pid, &status, 0);
    return WEXITSTATUS(status);
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
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            
            // Set up pipe connections
            if (i > 0) {
                // Not the first command - read from previous pipe
                if (dup2(pipe_fds[i-1][0], STDIN_FILENO) == -1) {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }
            }
            if (i < command_count - 1) {
                // Not the last command - write to next pipe
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
            
            // Handle file redirections for this specific command
            int input_fd = STDIN_FILENO;
            int output_fd = STDOUT_FILENO;
            
            // Check for input redirection in this command
            if (i == 0 && input_file) {
                input_fd = open(input_file, O_RDONLY);
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
            
            // Check for output redirection in this command
            if (i == command_count - 1 && output_file) {
                int flags = O_WRONLY | O_CREAT;
                flags |= append_output ? O_APPEND : O_TRUNC;
                output_fd = open(output_file, flags, 0644);
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
                }
                exit(0);
            } else {
                execvp(commands[i][0], commands[i]);
                // If execvp returns, the command could not be executed
                _exit(127);
            }
        } else if (pid < 0) {
            perror("fork");
            return -1;
        } else {
            pids[i] = pid;
        }
    }
    
    // Close all pipe file descriptors in parent
    for (int i = 0; i < command_count - 1; i++) {
        close(pipe_fds[i][0]);
        close(pipe_fds[i][1]);
    }
    
    // Wait for all processes to complete
    int last_status = 0;
    for (int i = 0; i < command_count; i++) {
        int status;
        waitpid(pids[i], &status, 0);
        int exit_status = WEXITSTATUS(status);
        
        // Check if any command failed with exit status 127 (command not found)
        if (exit_status == 127) {
            printf("Invalid Command (Valid Syntax)\n");
        }
        
        if (i == command_count - 1) {
            last_status = exit_status;
        }
    }
    
    return last_status;
}