#include "../include/shell.h"
#include "../include/prompt.h"

char *home_directory = NULL;
char *username = NULL;
char *system_name = NULL;

void initialize_shell_info(void) 
{
    // Get username
    struct passwd *pw = getpwuid(getuid());
    if (pw) 
    {
        size_t name_len = strlen(pw->pw_name);
        username = malloc(name_len + 1);
        if (username != NULL) {
            strcpy(username, pw->pw_name);
        }
    }
    
    // Get system name using uname (POSIX compliant)
    struct utsname sys_info;
    system_name = malloc(256);
    if (system_name != NULL) 
    {
        if (uname(&sys_info) == 0) 
        {
            strncpy(system_name, sys_info.nodename, 255);
            system_name[255] = '\0';
        } 
        else 
        {
            strcpy(system_name, "unknown");
        }
    }
    
    // Get home directory (current working directory at startup)
    home_directory = malloc(MAX_PATH_SIZE);
    if (home_directory != NULL && getcwd(home_directory, MAX_PATH_SIZE) == NULL) 
    {
        perror("getcwd");
        exit(1);
    }
}

char* get_current_path_display(void) 
{
    char *current_path = malloc(MAX_PATH_SIZE);
    if (current_path == NULL) 
    {
        return NULL;
    }
    
    if (getcwd(current_path, MAX_PATH_SIZE) == NULL)
    {
        perror("getcwd");
        free(current_path);
        return NULL;
    }
    
    // Check if current path starts with home directory
    size_t home_len = strlen(home_directory);
    if (strncmp(current_path, home_directory, home_len) == 0) 
    {
        // Replace home directory with ~
        char *display_path = malloc(MAX_PATH_SIZE);
        if (display_path == NULL)
        {
            free(current_path);
            return NULL;
        }

        // We are exactly in home directory
        if (current_path[home_len] == '\0')
            strcpy(display_path, "~");
        
        // We are in a subdirectory of home
        else if (current_path[home_len] == '/')
            snprintf(display_path, MAX_PATH_SIZE, "~%s", current_path + home_len);
        
        // Current path has home as prefix but not as ancestor
        else 
            strncpy(display_path, current_path, MAX_PATH_SIZE - 1);
            
        display_path[MAX_PATH_SIZE - 1] = '\0';
        free(current_path);
        return display_path;
    } 
    // Current path doesn't have home as ancestor
    else 
        return current_path;
}

void display_prompt(void) 
{
    char *display_path = get_current_path_display();
    if (display_path != NULL) 
    {
        printf("<%s@%s:%s> ", 
               username ? username : "unknown", 
               system_name ? system_name : "unknown", 
               display_path);
        fflush(stdout);
        free(display_path);
    }
}