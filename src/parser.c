#include "parser.h"
#include <ctype.h>

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
    
    // Split by semicolon (for now, just take the first command group)
    char *semicolon = strchr(input_copy, ';');
    if (semicolon) {
        *semicolon = '\0';
    }
    
    // Parse pipes - split the entire command by |
    char **pipe_parts = malloc(MAX_COMMANDS * sizeof(char*));
    int pipe_count = 0;
    
    char *input_working = strdup(input_copy);
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
                    input_redir = strdup(tokens[j + 1]);
                    j++; // Skip the filename token
                }
            } else if (strcmp(tokens[j], ">") == 0) {
                // Output redirection (only for last command)
                if (i == pipe_count - 1 && j + 1 < token_count) {
                    output_redir = strdup(tokens[j + 1]);
                    append = 0;
                    j++; // Skip the filename token
                }
            } else if (strcmp(tokens[j], ">>") == 0) {
                // Append redirection (only for last command)
                if (i == pipe_count - 1 && j + 1 < token_count) {
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