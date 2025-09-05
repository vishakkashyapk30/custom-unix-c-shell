#ifndef BUILTINS_H
#define BUILTINS_H

#include "shell.h"

// Built-in command functions
void builtin_hop(char **args);
void builtin_reveal(char **args);
void builtin_log(char **args);
void builtin_exit(char **args);

// Command execution with history
void execute_command_with_history(char *input);
void execute_command_no_history(char **args);

// Log management functions
void initialize_log(void);
void add_to_log(const char *command);
void print_log(void);
void purge_log(void);
void save_log_to_file(void);
void load_log_from_file(void);

// Utility functions for reveal
int compare_strings(const void *a, const void *b);

#endif