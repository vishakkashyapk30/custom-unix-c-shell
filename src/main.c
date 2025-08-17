#include "../include/shell.h"
#include "../include/prompt.h"
#include "../include/input.h"
#include "../include/parser.h"

int init_shell_globals(void) {
    initialize_shell_info();
    load_log_from_file();
    return 0;
}

void execute_command(char **args) {
    if (args == NULL || args[0] == NULL) {
        return;
    }
    if (strcmp(args[0], "hop") == 0) {
        builtin_hop(args);
    } else if (strcmp(args[0], "reveal") == 0) {
        builtin_reveal(args);
    } else if (strcmp(args[0], "log") == 0) {
        builtin_log(args);
    } else {
        printf("Command not found: %s\n", args[0]);
    }
}

int main() {
    if (init_shell_globals() != 0) {
        fprintf(stderr, "Failed to initialize shell\n");
        return 1;
    }
    
    int should_continue = 1;
    while (should_continue) {
        display_prompt();
        char *input = read_input();
        
        if (input == NULL) { // EOF (Ctrl+D)
            printf("\n");
            break;
        }
        
        if (strlen(input) == 0) {
            free(input);
            continue;
        }
        
        // First, validate syntax using the parser
        if (!parse_shell_command(input)) {
            printf("Invalid Syntax!\n");
            free(input);
            continue;
        }
        
        // Tokenize input to check what kind of command it is
        char **args = tokenize_input(input);
        
        if (args && args[0]) {
            if (strcmp(args[0], "exit") == 0) 
            {
                should_continue = 0;
            } 
            else
            {
                // For built-in commands, add to history first (except log)
                if (strcmp(args[0], "log") != 0) 
                {
                    add_to_history(input);
                }
                execute_command(args);
            }
        }
        free_tokens(args);
        free(input);
    }
    
    save_log_to_file(); // Save history on exit
    return 0;
}