#include "builtins.h"
#include "parser.h"
#include "input.h"
#include "fg_bg.h"
#include <ctype.h>
#include <sys/stat.h>

// Log storage - circular buffer
static char command_history[15][MAX_INPUT_SIZE];  // Max 15 commands
static int history_count = 0;  // Total commands added
static int history_start = 0;  // Start index in circular buffer

void initialize_log(void) {
    load_log_from_file();
}

void add_to_log(const char *command) {
    if (!command || strlen(command) == 0) return;
    
    // Don't add if identical to last command
    if (history_count > 0) {
        int last_idx = (history_start + (history_count - 1)) % 15;
        if (strcmp(command_history[last_idx], command) == 0) {
            return;
        }
    }
    
    // Add to circular buffer
    if (history_count < 15) {
        // Buffer not full yet
        int idx = (history_start + history_count) % 15;
        strncpy(command_history[idx], command, MAX_INPUT_SIZE - 1);
        command_history[idx][MAX_INPUT_SIZE - 1] = '\0';
        history_count++;
    } else {
        // Buffer full, overwrite oldest
        strncpy(command_history[history_start], command, MAX_INPUT_SIZE - 1);
        command_history[history_start][MAX_INPUT_SIZE - 1] = '\0';
        history_start = (history_start + 1) % 15;
    }
    
    save_log_to_file();
}

void print_log(void) {
    if (history_count == 0) {
        return;
    }
    
    // Print in order: oldest to newest
    for (int i = 0; i < history_count; i++) {
        int idx = (history_start + i) % 15;
        printf("%s\n", command_history[idx]);
    }
}

void purge_log(void) {
    history_count = 0;
    history_start = 0;
    save_log_to_file();
}

void execute_command_no_history(char **args) {
    // This function executes a command without adding it to history
    if (!args || !args[0]) return;
    
    if (strcmp(args[0], "hop") == 0) {
        builtin_hop(args);
    } else if (strcmp(args[0], "reveal") == 0) {
        builtin_reveal(args);
    } else if (strcmp(args[0], "log") == 0) {
        builtin_log(args);
    } else if (strcmp(args[0], "exit") == 0 || strcmp(args[0], "logout") == 0) {
        builtin_exit(args);
    } else if (strcmp(args[0], "fg") == 0) {
        builtin_fg(args);
    } else if (strcmp(args[0], "bg") == 0) {
        builtin_bg(args);
    } else {
        // External command
        execute_single_command(args, NULL, NULL, 0);
    }
}

void builtin_exit(char **args) {
    (void)args; // Suppress unused parameter warning
    save_log_to_file();
    printf("logout\n");
    fflush(stdout);
    exit(0);
}

void builtin_hop(char **args) {
    if (!args[1]) {
        // No argument, go to home directory
        if (chdir(home_directory) == 0) {
            strncpy(previous_directory, current_display_directory, MAX_PATH_SIZE - 1);
            previous_directory[MAX_PATH_SIZE - 1] = '\0';
            strcpy(current_display_directory, "~");
        } else {
            printf("No such directory!\n");
        }
    } else if (strcmp(args[1], "~") == 0) {
        // Explicit home
        if (chdir(home_directory) == 0) {
            strncpy(previous_directory, current_display_directory, MAX_PATH_SIZE - 1);
            previous_directory[MAX_PATH_SIZE - 1] = '\0';
            strcpy(current_display_directory, "~");
        } else {
            printf("No such directory!\n");
        }
    } else if (strcmp(args[1], "-") == 0) {
        // Previous directory
        if (strlen(previous_directory) == 0) {
            printf("No previous directory\n");
            return;
        }
        
        // Convert display path to actual path if needed
        char actual_prev_path[MAX_PATH_SIZE];
        if (strncmp(previous_directory, "~", 1) == 0) {
            if (strcmp(previous_directory, "~") == 0) {
                strcpy(actual_prev_path, shell_start_directory);  // Use shell start directory, not home
            } else if (strncmp(previous_directory, "~/", 2) == 0) {
                snprintf(actual_prev_path, sizeof(actual_prev_path), "%s%s", 
                        shell_start_directory, previous_directory + 1);
            } else {
                strcpy(actual_prev_path, previous_directory);
            }
        } else {
            strcpy(actual_prev_path, previous_directory);
        }
        
        if (chdir(actual_prev_path) == 0) {
            char temp_display[MAX_PATH_SIZE];
            strcpy(temp_display, current_display_directory);
            strcpy(current_display_directory, previous_directory);
            strcpy(previous_directory, temp_display);
        } else {
            printf("No such directory!\n");
        }
    } else if (args[1][0] == '~' && args[1][1] == '/') {
        // ~/path - relative to shell start directory
        char new_path[MAX_PATH_SIZE];
        int ret = snprintf(new_path, sizeof(new_path), "%s%s", shell_start_directory, args[1] + 1);
        if (ret >= (int)sizeof(new_path)) {
            printf("Path too long\n");
            return;
        }
        
        if (chdir(new_path) == 0) {
            strncpy(previous_directory, current_display_directory, MAX_PATH_SIZE - 1);
            previous_directory[MAX_PATH_SIZE - 1] = '\0';
            strcpy(current_display_directory, args[1]);
        } else {
            printf("No such directory!\n");
        }
    } else {
        // Regular path (relative or absolute)
        if (chdir(args[1]) == 0) {
            strncpy(previous_directory, current_display_directory, MAX_PATH_SIZE - 1);
            previous_directory[MAX_PATH_SIZE - 1] = '\0';
            
            // Update display directory
            char new_actual_path[MAX_PATH_SIZE];
            if (getcwd(new_actual_path, sizeof(new_actual_path)) != NULL) {
                // Check if new path is under shell start directory
                if (strncmp(new_actual_path, shell_start_directory, strlen(shell_start_directory)) == 0) {
                    if (strcmp(new_actual_path, shell_start_directory) == 0) {
                        strcpy(current_display_directory, "~");
                    } else {
                        snprintf(current_display_directory, sizeof(current_display_directory), 
                                "~%s", new_actual_path + strlen(shell_start_directory));
                    }
                } else {
                    strcpy(current_display_directory, new_actual_path);
                }
            }
        } else {
            printf("No such directory!\n");
        }
    }
    
    fflush(stdout);
    fflush(stderr);
}

int compare_strings(const void *a, const void *b) {
    return strcmp(*(const char**)a, *(const char**)b);
}

int is_output_redirected_or_piped(void) {
    return !isatty(STDOUT_FILENO);
}

// Helper function to reveal a single directory
void reveal_directory(const char *path, int show_hidden, int long_format) {
    // Handle empty path (when previous_directory is empty)
    if (!path || strlen(path) == 0) {
        printf("No such directory!\n");
        return;
    }
    
    struct stat path_stat;
    if (stat(path, &path_stat) != 0) {
        printf("No such directory!\n");
        return;
    }
    
    if (S_ISREG(path_stat.st_mode)) {
        // It's a file - just print the filename
        printf("%s\n", path);
        return;
    }
    
    if (!S_ISDIR(path_stat.st_mode)) {
        printf("No such directory!\n");
        return;
    }
    
    // It's a directory
    DIR *dir = opendir(path);
    if (!dir) {
        printf("No such directory!\n");
        return;
    }
    
    // Collect entries
    char **entries = malloc(1000 * sizeof(char*));
    if (!entries) {
        closedir(dir);
        return;
    }
    int count = 0;
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && count < 1000) {
        if (!show_hidden && entry->d_name[0] == '.') {
            continue;
        }
        entries[count] = strdup(entry->d_name);
        if (entries[count]) {
            count++;
        }
    }
    closedir(dir);
    
    // Sort entries lexicographically (by ASCII values)
    qsort(entries, count, sizeof(char*), compare_strings);
    
    // Print entries
    if (long_format) {
        // Line by line format (one entry per line)
        for (int i = 0; i < count; i++) {
            printf("%s\n", entries[i]);
            free(entries[i]);
        }
    } else {
        // Default format - like ls (space separated, but newline when piped)
        for (int i = 0; i < count; i++) {
            printf("%s", entries[i]);
            if (i < count - 1) {
                if (is_output_redirected_or_piped()) {
                    printf("\n");  // One per line when piped/redirected
                } else {
                    printf("  ");  // Space separated when to terminal
                }
            }
            free(entries[i]);
        }
        if (!is_output_redirected_or_piped()) {
            printf("\n");  // Final newline for terminal output
        } else if (count > 0) {
            printf("\n");  // Final newline for last item when piped
        }
    }
    
    free(entries);
}

void builtin_reveal(char **args) {
    char *path = ".";  // Default to current directory
    int show_hidden = 0;
    int long_format = 0;
    int arg_index = 1;
    static char expanded_path[MAX_PATH_SIZE];
    
    // Parse flags (treat a lone "-" as a path, not flags)
    while (args[arg_index] && args[arg_index][0] == '-' && args[arg_index][1] != '\0') {
        char *flag = args[arg_index];
        for (int i = 1; flag[i]; i++) {
            if (flag[i] == 'a') {
                show_hidden = 1;
            } else if (flag[i] == 'l') {
                long_format = 1;
            }
        }
        arg_index++;
    }
    
    // Get path if provided
    if (args[arg_index]) {
        path = args[arg_index];
        if (path[0] == '~') {
            if (path[1] == '\0') {
                // "~" → home_directory
                strncpy(expanded_path, home_directory, MAX_PATH_SIZE - 1);
                expanded_path[MAX_PATH_SIZE - 1] = '\0';
            } else if (path[1] == '/') {
                // "~/..." → home_directory + "/..."
                int ret = snprintf(expanded_path, sizeof(expanded_path), "%s%s", home_directory, path + 1);
                if (ret >= (int)sizeof(expanded_path)) {
                    printf("No such directory!\n");
                    return;
                }
            }
            path = expanded_path;
        } else if (strcmp(path, "-") == 0) {
            // "-" → previous_directory, with proper path expansion
            if (strlen(previous_directory) == 0) {
                printf("No such directory!\n");
                return;
            }
            
            // Convert display path to actual path (same logic as hop -)
            if (strncmp(previous_directory, "~", 1) == 0) {
                if (strcmp(previous_directory, "~") == 0) {
                    strcpy(expanded_path, shell_start_directory);
                } else if (strncmp(previous_directory, "~/", 2) == 0) {
                    snprintf(expanded_path, sizeof(expanded_path), "%s%s", 
                            shell_start_directory, previous_directory + 1);
                } else {
                    strcpy(expanded_path, previous_directory);
                }
            } else {
                strcpy(expanded_path, previous_directory);
            }
            path = expanded_path;
        }
        arg_index++;
    }
    
    // Check if too many arguments
    if (args[arg_index]) {
        printf("reveal: Invalid Syntax!\n");
        return;
    }
    
    reveal_directory(path, show_hidden, long_format);
}

void builtin_log(char **args) {
    if (!args[1]) {
        // No arguments - print log
        print_log();
    } else if (strcmp(args[1], "purge") == 0) {
        // Purge log
        purge_log();
    } else if (strcmp(args[1], "execute") == 0) {
        // Execute command at index
        if (!args[2]) {
            printf("Usage: log execute <index>\n");
            return;
        }
        
        int index = atoi(args[2]);
        if (index < 1 || index > history_count) {
            printf("Invalid index\n");
            return;
        }
        
        // Convert to 0-based index (newest to oldest)
        // Index 1 = newest, so actual_index = history_count - 1
        int actual_index = (history_start + (history_count - index)) % 15;
        
        // Parse and execute the command without adding to history
        char *command_to_execute = strdup(command_history[actual_index]);
        
        // Only print "Executing:" message if output is not redirected/piped
        if (!is_output_redirected_or_piped()) {
            printf("Executing: %s\n", command_to_execute);
        }
        
        // Parse the command
        ParsedCommand parsed;
        if (parse_command(command_to_execute, &parsed) == 0) {
            if (parsed.command_count == 1) {
                execute_command_no_history(parsed.commands[0]);
            } else if (parsed.command_count > 1) {
                execute_pipeline(parsed.commands, parsed.command_count, 
                               parsed.input_file, parsed.output_file, parsed.append_output);
            }
            free_parsed_command(&parsed);
        }
        
        free(command_to_execute);
    } else {
        printf("Usage: log [purge | execute <index>]\n");
    }
}

void save_log_to_file(void) {
    char log_file_path[MAX_PATH_SIZE + 20]; // Extra space for "/.shell_history"
    
    // Use shell_start_directory instead of home_directory
    if (strlen(shell_start_directory) + 16 >= sizeof(log_file_path)) {
        return; // Path too long
    }
    
    int ret = snprintf(log_file_path, sizeof(log_file_path), "%s/.shell_history", shell_start_directory);
    if (ret >= (int)sizeof(log_file_path)) {
        return; // Path was truncated
    }
    
    FILE *file = fopen(log_file_path, "w");
    if (!file) return;
    
    fprintf(file, "%d\n%d\n", history_count, history_start);
    
    for (int i = 0; i < history_count; i++) {
        int idx = (history_start + i) % 15;
        fprintf(file, "%s\n", command_history[idx]);
    }
    
    fclose(file);
}

void load_log_from_file(void) {
    char log_file_path[MAX_PATH_SIZE + 20]; // Extra space for "/.shell_history"
    
    // Use shell_start_directory instead of home_directory
    if (strlen(shell_start_directory) + 16 >= sizeof(log_file_path)) {
        return; // Path too long
    }
    
    int ret = snprintf(log_file_path, sizeof(log_file_path), "%s/.shell_history", shell_start_directory);
    if (ret >= (int)sizeof(log_file_path)) {
        return; // Path was truncated
    }
    
    FILE *file = fopen(log_file_path, "r");
    if (!file) return;
    
    if (fscanf(file, "%d\n%d\n", &history_count, &history_start) != 2) {
        fclose(file);
        return;
    }
    
    if (history_count > 15) history_count = 15;
    if (history_start >= 15) history_start = 0;
    
    char line[MAX_INPUT_SIZE];
    for (int i = 0; i < history_count; i++) {
        if (fgets(line, sizeof(line), file)) {
            // Remove newline
            size_t len = strlen(line);
            if (len > 0 && line[len-1] == '\n') {
                line[len-1] = '\0';
            }
            int idx = (history_start + i) % 15;
            strncpy(command_history[idx], line, MAX_INPUT_SIZE - 1);
            command_history[idx][MAX_INPUT_SIZE - 1] = '\0';
        }
    }
    
    fclose(file);
}