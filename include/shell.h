#ifndef SHELL_H
#define SHELL_H

#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700
#define MAX_INPUT_SIZE 1024
#define MAX_PATH_SIZE 512
#define MAX_ARGS 64

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <errno.h>

extern char *home_directory;
extern char *username;
extern char *system_name;
extern char *previous_directory;

// Function declarations
int init_shell_globals(void);
void execute_command(char **args);
void execute_command_no_history(char **args);
char** tokenize_input(const char* input);
void free_tokens(char** tokens);

// Built-in commands
int builtin_hop(char **args);
int builtin_reveal(char **args);
int builtin_log(char **args);

// History functions
void add_to_history(const char *command);
void save_log_to_file(void);
void load_log_from_file(void);

#endif