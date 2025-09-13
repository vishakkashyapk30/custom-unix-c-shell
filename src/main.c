#include "shell.h"
#include "prompt.h"
#include "input.h"
#include "builtins.h"
#include "signal_handlers.h"
#include "fg_bg.h"
#include "parser.h"

// LLM Code begins
// Global variables
char home_directory[MAX_PATH_SIZE];
char previous_directory[MAX_PATH_SIZE];
char shell_start_directory[MAX_PATH_SIZE];
char current_display_directory[MAX_PATH_SIZE];  // Add display directory tracking

void initialize_shell(void) {
    // Get actual home directory
    char *home = getenv("HOME");
    if (home) {
        strcpy(home_directory, home);
    } else {
        strcpy(home_directory, "/");
    }
    
    // Store the directory where shell was started
    if (getcwd(shell_start_directory, sizeof(shell_start_directory)) == NULL) {
        strcpy(shell_start_directory, home_directory);
    }
    
    // Initialize previous directory as empty
    strcpy(previous_directory, "");
    
    // Initialize display directory as ~
    strcpy(current_display_directory, "~");
    
    // Initialize shell info for prompt
    initialize_shell_info();
    
    // Initialize log system
    initialize_log();
    
    // Initialize job control system
    init_job_control_system();
    
    // Setup signal handlers
    setup_signal_handlers();
}

void shell_loop(void) {
    char *input;
    
    while (1) {
        display_prompt();
        input = read_input();
        
        if (input == NULL) {
            // EOF (Ctrl+D) - handle cleanup and exit gracefully
            // Kill all background processes
            kill_all_background_jobs();
            
            // Wait for all background processes to terminate
            wait_for_background_jobs();
            
            // Print logout message
            printf("logout\n");
            fflush(stdout);
            
            // Exit the loop instead of calling exit()
            break;
        }
        
        // Trim whitespace and check if input is empty
        trim_whitespace(input);
        if (strlen(input) == 0) {
            free(input);
            continue;
        }
        
        handle_input(input);
        free(input);
    }
}

int main(void) {
    initialize_shell();
    shell_loop();
    return 0;
}
// LLM Code ends