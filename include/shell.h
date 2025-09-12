#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/utsname.h>
#include <pwd.h>
#include <sys/stat.h>

#define MAX_INPUT_SIZE 1024
#define MAX_ARGS 64
#define MAX_COMMANDS 16
#define MAX_PATH_SIZE 1024
#define MAX_LOG_SIZE 20

// Global variables
extern char home_directory[MAX_PATH_SIZE];
extern char previous_directory[MAX_PATH_SIZE];
extern char shell_start_directory[MAX_PATH_SIZE];  // Add this line

// Core shell functions
void initialize_shell(void);
void shell_loop(void);

// Command execution
int execute_single_command(char **args, char *input_file, char *output_file, int append_output);
int execute_pipeline(char ***commands, int command_count, char *input_file, char *output_file, int append_output);
int execute_builtin_with_redirection(const char *builtin_name, char **args, char *input_file, char *output_file, int append_output);

// Background execution
void execute_background_command(char **args, char *input_file, char *output_file, int append_output);
void execute_background_pipeline(char ***commands, int command_count, char *input_file, char *output_file, int append_output);
void check_background_jobs(void);

// Additional builtin commands
void builtin_activities(char **args);
void builtin_ping(char **args);

// Process management
pid_t create_process(char **args, int input_fd, int output_fd, int pipe_fds[][2], int num_pipes);
pid_t create_builtin_process(char **args, int input_fd, int output_fd, int pipe_fds[][2], int num_pipes);

#endif