#include "parser.h"
#include <ctype.h>
#include "builtins.h"
#include "input.h"

void trim_whitespace(char *str) {
    if (!str) return;
    
    char *start = str;
    char *end;
    
    // Trim leading space
    while (isspace((unsigned char)*start)) start++;
    
    // All spaces?
    if (*start == 0) {
        str[0] = '\0';
        return;
    }
    
    // Trim trailing space
    end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end)) end--;
    
    // Write new null terminator
    end[1] = '\0';
    
    // Move trimmed string to beginning if necessary
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
}

int is_valid_syntax(char *input) {
    if (!input || strlen(input) == 0) return 1;
    
    char *temp = strdup(input);
    trim_whitespace(temp);
    
    // Check for invalid sequences
    if (strstr(temp, "||") || strstr(temp, "&&") || strstr(temp, ";;") ||
        strstr(temp, "|;") || strstr(temp, ";|") ||
        strstr(temp, "|&") || strstr(temp, "&|")) {
        free(temp);
        return 0;
    }
    
    // Check for invalid starting/ending characters
    if (temp[0] == '|' || temp[0] == '&' || temp[0] == ';') {
        free(temp);
        return 0;
    }
    
    size_t len = strlen(temp);
    if (len > 0 && temp[len-1] == '|') {
        free(temp);
        return 0;
    }
    
    // More comprehensive CFG validation
    // Check that | is followed by valid tokens (not ; or &)
    for (size_t i = 0; i < len - 1; i++) {
        if (temp[i] == '|') {
            // Skip whitespace after |
            size_t j = i + 1;
            while (j < len && (temp[j] == ' ' || temp[j] == '\t' || temp[j] == '\n' || temp[j] == '\r')) {
                j++;
            }
            // After | and whitespace, next char must not be ; or &
            if (j < len && (temp[j] == ';' || temp[j] == '&')) {
                free(temp);
                return 0;
            }
        }
    }
    
    // Check that ; is followed by valid tokens (not | or &)
    for (size_t i = 0; i < len - 1; i++) {
        if (temp[i] == ';') {
            // Skip whitespace after ;
            size_t j = i + 1;
            while (j < len && (temp[j] == ' ' || temp[j] == '\t' || temp[j] == '\n' || temp[j] == '\r')) {
                j++;
            }
            // After ; and whitespace, next char must not be | or &
            if (j < len && (temp[j] == '|' || temp[j] == '&')) {
                free(temp);
                return 0;
            }
        }
    }
    
    // Check that & is only at the end or followed by whitespace and end
    for (size_t i = 0; i < len - 1; i++) {
        if (temp[i] == '&') {
            // Skip whitespace after &
            size_t j = i + 1;
            while (j < len && (temp[j] == ' ' || temp[j] == '\t' || temp[j] == '\n' || temp[j] == '\r')) {
                j++;
            }
            // After & and whitespace, must be at end of string
            if (j < len) {
                free(temp);
                return 0;
            }
        }
    }
    
    free(temp);
    return 1;
}

char **tokenize_command(char *command) {
    char **tokens = malloc(MAX_ARGS * sizeof(char*));
    if (!tokens) return NULL;
    
    int count = 0;
    char *command_copy = strdup(command);
    char *token = strtok(command_copy, " \t\n\r");
    
    while (token != NULL && count < MAX_ARGS - 1) {
        tokens[count] = strdup(token);
        count++;
        token = strtok(NULL, " \t\n\r");
    }
    
    tokens[count] = NULL;
    free(command_copy);
    return tokens;
}

int parse_command(char *input, ParsedCommand *parsed) {
    memset(parsed, 0, sizeof(ParsedCommand));
    
    char *input_copy = strdup(input);
    trim_whitespace(input_copy);
    
    // Handle background execution
    size_t len = strlen(input_copy);
    if (len > 0 && input_copy[len-1] == '&') {
        parsed->background = 1;
        input_copy[len-1] = '\0';
        trim_whitespace(input_copy);
    }
    
    // Split by semicolon to handle sequential execution
    char **semicolon_parts = malloc(MAX_COMMANDS * sizeof(char*));
    int semicolon_count = 0;
    
    char *semicolon_token = strtok(input_copy, ";");
    while (semicolon_token && semicolon_count < MAX_COMMANDS) {
        trim_whitespace(semicolon_token);
        semicolon_parts[semicolon_count] = strdup(semicolon_token);
        semicolon_count++;
        semicolon_token = strtok(NULL, ";");
    }
    
    // For now, just process the first command group (sequential execution will be handled in input.c)
    char *input_working = semicolon_parts[0];
    
    // Free the semicolon parts array (we'll handle sequential execution in input.c)
    for (int i = 1; i < semicolon_count; i++) {
        free(semicolon_parts[i]);
    }
    free(semicolon_parts);
    
    // Parse pipes - split the entire command by |
    char **pipe_parts = malloc(MAX_COMMANDS * sizeof(char*));
    int pipe_count = 0;
    char *pipe_token = strtok(input_working, "|");
    
    while (pipe_token && pipe_count < MAX_COMMANDS) {
        trim_whitespace(pipe_token);
        pipe_parts[pipe_count] = strdup(pipe_token);
        pipe_count++;
        pipe_token = strtok(NULL, "|");
    }
    free(input_working);
    
    // Now process each pipe part for redirections
    for (int i = 0; i < pipe_count; i++) {
        char *cmd_part = pipe_parts[i];
        char *input_redir = NULL, *output_redir = NULL;
        int append = 0;
        
        // Parse redirections for this command using tokenization approach
        char *cmd_copy = strdup(cmd_part);
        char **tokens = malloc(MAX_ARGS * sizeof(char*));
        int token_count = 0;
        
        // Tokenize the command
        char *token = strtok(cmd_copy, " \t\n\r");
        while (token != NULL && token_count < MAX_ARGS - 1) {
            tokens[token_count] = strdup(token);
            token_count++;
            token = strtok(NULL, " \t\n\r");
        }
        tokens[token_count] = NULL;
        
        // Parse redirections from tokens
        char **clean_tokens = malloc(MAX_ARGS * sizeof(char*));
        int clean_count = 0;
        
        for (int j = 0; j < token_count; j++) {
            if (strcmp(tokens[j], "<") == 0) {
                // Input redirection (only for first command)
                if (i == 0 && j + 1 < token_count) {
                    // For multiple input redirects, check each file in order
                    // and fail on the first error
                    int fd = open(tokens[j + 1], O_RDONLY);
                    if (fd == -1) {
                        // File doesn't exist or can't be opened - return special error code
                        // Clean up allocated memory first
                        for (int k = 0; k < token_count; k++) {
                            free(tokens[k]);
                        }
                        free(tokens);
                        for (int k = 0; k < clean_count; k++) {
                            free(clean_tokens[k]);
                        }
                        free(clean_tokens);
                        if (input_redir) free(input_redir);
                        if (output_redir) free(output_redir);
                        return -2; // Special error code for file not found
                    }
                    close(fd);
                    
                    // Free previous input_redir if it exists (use last valid one)
                    if (input_redir) {
                        free(input_redir);
                    }
                    input_redir = strdup(tokens[j + 1]);
                    j++; // Skip the filename token
                }
            } else if (strcmp(tokens[j], ">") == 0) {
                // Output redirection (only for last command)
                if (i == pipe_count - 1 && j + 1 < token_count) {
                    // Free previous output_redir if it exists (multiple output redirects)
                    if (output_redir) {
                        free(output_redir);
                    }
                    output_redir = strdup(tokens[j + 1]);
                    append = 0;
                    j++; // Skip the filename token
                }
            } else if (strcmp(tokens[j], ">>") == 0) {
                // Append redirection (only for last command)
                if (i == pipe_count - 1 && j + 1 < token_count) {
                    // Free previous output_redir if it exists (multiple output redirects)
                    if (output_redir) {
                        free(output_redir);
                    }
                    output_redir = strdup(tokens[j + 1]);
                    append = 1;
                    j++; // Skip the filename token
                }
            } else {
                // Regular argument
                clean_tokens[clean_count] = strdup(tokens[j]);
                clean_count++;
            }
        }
        clean_tokens[clean_count] = NULL;
        
        // Free token array
        for (int j = 0; j < token_count; j++) {
            free(tokens[j]);
        }
        free(tokens);
        
        // Reconstruct clean command
        free(cmd_copy);
        cmd_copy = malloc(MAX_INPUT_SIZE);
        cmd_copy[0] = '\0';
        
        for (int j = 0; j < clean_count; j++) {
            if (j > 0) strcat(cmd_copy, " ");
            strcat(cmd_copy, clean_tokens[j]);
            free(clean_tokens[j]);
        }
        free(clean_tokens);
        
        // Tokenize the command
        parsed->commands[i] = tokenize_command(cmd_copy);
        
        // Store redirections
        if (i == 0 && input_redir) {
            parsed->input_file = input_redir;
        }
        if (i == pipe_count - 1 && output_redir) {
            parsed->output_file = output_redir;
            parsed->append_output = append;
        }
        
        free(cmd_copy);
        free(pipe_parts[i]);
    }
    
    parsed->command_count = pipe_count;
    free(pipe_parts);
    free(input_copy);
    
    return 0;
}

void free_parsed_command(ParsedCommand *parsed) {
    for (int i = 0; i < parsed->command_count; i++) {
        if (parsed->commands[i]) {
            for (int j = 0; parsed->commands[i][j]; j++) {
                free(parsed->commands[i][j]);
            }
            free(parsed->commands[i]);
        }
    }
    
    free(parsed->input_file);
    free(parsed->output_file);
    memset(parsed, 0, sizeof(ParsedCommand));
}

int parse_shell_command(const char *command) {
    if (!command) return 0;
    return is_valid_syntax((char*)command);
}

// Function to parse and execute sequential commands
int execute_sequential_commands(char *input) {
    if (!input || strlen(input) == 0) return 0;
    
    char *input_copy = strdup(input);
    trim_whitespace(input_copy);
    
    // Handle background execution for the entire sequence
    int is_background = 0;
    size_t len = strlen(input_copy);
    if (len > 0 && input_copy[len-1] == '&') {
        is_background = 1;
        input_copy[len-1] = '\0';
        trim_whitespace(input_copy);
    }
    
    // Split by semicolon
    char **semicolon_parts = malloc(MAX_COMMANDS * sizeof(char*));
    int semicolon_count = 0;
    
    char *semicolon_token = strtok(input_copy, ";");
    while (semicolon_token && semicolon_count < MAX_COMMANDS) {
        trim_whitespace(semicolon_token);
        semicolon_parts[semicolon_count] = strdup(semicolon_token);
        semicolon_count++;
        semicolon_token = strtok(NULL, ";");
    }
    
    // Execute each command sequentially
    for (int i = 0; i < semicolon_count; i++) {
        ParsedCommand parsed;
        if (parse_command(semicolon_parts[i], &parsed) != 0) {
            printf("Invalid Syntax!\n");
            free(semicolon_parts[i]);
            continue;
        }
        
        if (parsed.command_count == 0) {
            free_parsed_command(&parsed);
            free(semicolon_parts[i]);
            continue;
        }
        
        // Execute the command (single or pipeline)
        if (parsed.command_count == 1 && parsed.commands[0] && parsed.commands[0][0]) {
            char *cmd = parsed.commands[0][0];
            
            // Check if it's a built-in command
            if (strcmp(cmd, "exit") == 0) {
                add_to_log(semicolon_parts[i]);
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
                add_to_log(semicolon_parts[i]);
                builtin_hop(parsed.commands[0]);
            } else if (strcmp(cmd, "reveal") == 0) {
                add_to_log(semicolon_parts[i]);
                if (parsed.input_file || parsed.output_file) {
                    execute_builtin_with_redirection("reveal", parsed.commands[0], 
                                                    parsed.input_file, parsed.output_file, parsed.append_output);
                } else {
                    builtin_reveal(parsed.commands[0]);
                }
            } else if (strcmp(cmd, "activities") == 0) {
                add_to_log(semicolon_parts[i]);
                builtin_activities(parsed.commands[0]);
            } else if (strcmp(cmd, "ping") == 0) {
                add_to_log(semicolon_parts[i]);
                builtin_ping(parsed.commands[0]);
            } else {
                // External command
                add_to_log(semicolon_parts[i]);
                if (parsed.background || is_background) {
                    // Execute in background
                    execute_background_command(parsed.commands[0], parsed.input_file, 
                                             parsed.output_file, parsed.append_output);
                } else {
                    // Execute in foreground
                    int result = execute_single_command(parsed.commands[0], parsed.input_file, 
                                                      parsed.output_file, parsed.append_output);
                    if (result == 127) {
                        printf("Invalid Command (Valid Syntax)\n");
                    }
                }
            }
        } else if (parsed.command_count > 1) {
            // Pipeline case
            add_to_log(semicolon_parts[i]);
            if (parsed.background || is_background) {
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
        free(semicolon_parts[i]);
    }
    
    free(semicolon_parts);
    free(input_copy);
    return 0;
}