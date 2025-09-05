#ifndef PROMPT_H
#define PROMPT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <pwd.h>

#define MAX_PATH_SIZE 1024

// Global variables for prompt
extern char username[256];
extern char system_name[256];
extern char current_directory[MAX_PATH_SIZE];

// Function declarations
void initialize_shell_info(void);
char *get_current_path_display(void);
void display_prompt(void);

#endif