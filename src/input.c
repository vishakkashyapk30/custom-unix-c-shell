#include "input.h"
#include "parser.h"
#include "builtins.h"
#include "fg_bg.h"

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
            strcmp(cmd, "exit") == 0 ||
            strcmp(cmd, "logout") == 0 ||
            strcmp(cmd, "activities") == 0 ||
            strcmp(cmd, "ping") == 0 ||
            strcmp(cmd, "fg") == 0 ||
            strcmp(cmd, "bg") == 0);
}

void handle_input(char *input) {
    // Check for completed background jobs before processing new input
    check_background_jobs();
    
    // Check if this is a sequential command (contains semicolon)
    if (strchr(input, ';') != NULL) {
        // Handle sequential execution
        execute_sequential_commands(input);
        return;
    }
    
    ParsedCommand parsed;
    
    // Check syntax first
    if (!is_valid_syntax(input)) {
        printf("Invalid Syntax!\n");
        return;
    }
    
    int parse_result = parse_command(input, &parsed);
    if (parse_result == -3) {
        // Output redirection failed
        printf("Unable to create file for writing\n");
        return;
    } else if (parse_result != 0) {
        if (parse_result == -2) {
            printf("No such file or directory\n");
        } else {
            printf("Invalid Syntax!\n");
        }
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
        if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "logout") == 0) {
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
        } else if (strcmp(cmd, "activities") == 0) {
            add_to_log(input);
            builtin_activities(parsed.commands[0]);
        } else if (strcmp(cmd, "ping") == 0) {
            add_to_log(input);
            builtin_ping(parsed.commands[0]);
        } else if (strcmp(cmd, "fg") == 0) {
            add_to_log(input);
            builtin_fg(parsed.commands[0]);
        } else if (strcmp(cmd, "bg") == 0) {
            add_to_log(input);
            builtin_bg(parsed.commands[0]);
        } else {
            // External command - add to log and try to execute
            add_to_log(input);
            if (parsed.background) {
                // Execute in background
                execute_background_command(parsed.commands[0], parsed.input_file, 
                                         parsed.output_file, parsed.append_output);
            } else {
                // Execute in foreground
                int result = execute_single_command(parsed.commands[0], parsed.input_file, 
                                                  parsed.output_file, parsed.append_output);
                if (result == 127) {
                    printf("Command not found!\n");
                }
            }
        }
    } else if (parsed.command_count > 1) {
        // Pipeline case - add to log and execute
        add_to_log(input);
        if (parsed.background) {
            // Execute pipeline in background
            execute_background_pipeline(parsed.commands, parsed.command_count, 
                                      parsed.input_file, parsed.output_file, parsed.append_output);
        } else {
            // Execute pipeline in foreground
            int result = execute_pipeline(parsed.commands, parsed.command_count, 
                                        parsed.input_file, parsed.output_file, parsed.append_output);
            if (result == 127) {
                printf("Command not found!\n");
            }
        }
    }
    
    free_parsed_command(&parsed);
}