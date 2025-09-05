#include "input.h"
#include "parser.h"
#include "builtins.h"

char *read_input(void) {
    char *input = malloc(MAX_INPUT_SIZE);
    if (!input) {
        perror("malloc");
        return NULL;
    }
    
    if (fgets(input, MAX_INPUT_SIZE, stdin) == NULL) {
        free(input);
        return NULL;
    }
    
    // Remove newline
    size_t len = strlen(input);
    if (len > 0 && input[len-1] == '\n') {
        input[len-1] = '\0';
    }
    
    return input;
}

int is_builtin_command(const char *cmd) {
    return (strcmp(cmd, "hop") == 0 || 
            strcmp(cmd, "reveal") == 0 || 
            strcmp(cmd, "log") == 0 ||
            strcmp(cmd, "exit") == 0);
}

void handle_input(char *input) {
    ParsedCommand parsed;
    
    // Check syntax first
    if (!is_valid_syntax(input)) {
        printf("Invalid Syntax!\n");
        return;
    }
    
    if (parse_command(input, &parsed) != 0) {
        printf("Invalid Syntax!\n");
        return;
    }
    
    if (parsed.command_count == 0) {
        free_parsed_command(&parsed);
        return;
    }
    
    // Single command case
    if (parsed.command_count == 1 && parsed.commands[0] && parsed.commands[0][0]) {
        char *cmd = parsed.commands[0][0];
        
        // Check if it's a built-in command
        if (strcmp(cmd, "exit") == 0) {
            add_to_log(input);
            builtin_exit(parsed.commands[0]);
        } else if (strcmp(cmd, "log") == 0) {
            // Log commands don't get added to history
            if (parsed.input_file || parsed.output_file) {
                execute_builtin_with_redirection("log", parsed.commands[0], 
                                                parsed.input_file, parsed.output_file, parsed.append_output);
            } else {
                builtin_log(parsed.commands[0]);
            }
        } else if (strcmp(cmd, "hop") == 0) {
            add_to_log(input);
            builtin_hop(parsed.commands[0]);
        } else if (strcmp(cmd, "reveal") == 0) {
            add_to_log(input);
            if (parsed.input_file || parsed.output_file) {
                execute_builtin_with_redirection("reveal", parsed.commands[0], 
                                                parsed.input_file, parsed.output_file, parsed.append_output);
            } else {
                builtin_reveal(parsed.commands[0]);
            }
        } else {
            // External command - add to log and try to execute
            add_to_log(input);
            int result = execute_single_command(parsed.commands[0], parsed.input_file, 
                                              parsed.output_file, parsed.append_output);
            if (result == -2) {
                printf("Command not found\n");
            }
        }
    } else if (parsed.command_count > 1) {
        // Pipeline case - add to log and execute
        add_to_log(input);
        int result = execute_pipeline(parsed.commands, parsed.command_count, 
                                    parsed.input_file, parsed.output_file, parsed.append_output);
        if (result == -2) {
            printf("Command not found\n");
        }
    }
    
    free_parsed_command(&parsed);
}