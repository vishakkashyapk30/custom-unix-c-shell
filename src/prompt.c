#include "prompt.h"
#include "shell.h"
#include <sys/utsname.h>
#include <pwd.h>
#include <unistd.h>

// LLM Code begins
// Global variables for prompt
char username[256];
char system_name[256];
char current_directory[MAX_PATH_SIZE];

void initialize_shell_info(void) {
    // Get system name
    struct utsname sys_info;
    if (uname(&sys_info) == 0) {
        strcpy(system_name, sys_info.nodename);
    } else {
        strcpy(system_name, "unknown");
    }
    
    // Get username
    struct passwd *pw = getpwuid(getuid());
    if (pw) {
        strcpy(username, pw->pw_name);
    } else {
        strcpy(username, "user");
    }
}

char *get_current_path_display(void) {
    if (getcwd(current_directory, sizeof(current_directory)) == NULL) {
        return "~";
    }
    
    // Check if current directory is shell start directory or subdirectory
    if (strncmp(current_directory, shell_start_directory, strlen(shell_start_directory)) == 0) {
        if (strcmp(current_directory, shell_start_directory) == 0) {
            return "~";
        } else {
            // Return relative path from shell start directory
            static char relative_path[MAX_PATH_SIZE];
            snprintf(relative_path, sizeof(relative_path), "~%s", 
                     current_directory + strlen(shell_start_directory));
            return relative_path;
        }
    }
    
    return current_directory;
}

void display_prompt(void) {
    printf("<%s@%s:%s> ", username, system_name, current_display_directory);
    fflush(stdout);
}
// LLM Code ends