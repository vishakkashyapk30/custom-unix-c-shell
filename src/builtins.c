#include "../include/shell.h"
#include "../include/prompt.h"
#include "../include/parser.h"

// Global variables
char *previous_directory = NULL;
static char *command_history[15];
static int history_count = 0;
static int history_start = 0;

// Helper function to tokenize input into arguments using strtok_r for safety
char** tokenize_input(const char* input) {
    if (!input) return NULL;

    char** tokens = malloc(MAX_ARGS * sizeof(char*));
    if (!tokens) return NULL;

    char* input_copy = strdup(input);
    if (!input_copy) {
        free(tokens);
        return NULL;
    }

    int token_count = 0;
    char *saveptr; // For strtok_r
    char* token = strtok_r(input_copy, " \t\n\r", &saveptr);

    while (token != NULL && token_count < MAX_ARGS - 1) {
        tokens[token_count] = strdup(token);
        token_count++;
        token = strtok_r(NULL, " \t\n\r", &saveptr);
    }

    tokens[token_count] = NULL;
    free(input_copy);
    return tokens;
}

void free_tokens(char** tokens) {
    if (!tokens) return;
    for (int i = 0; tokens[i] != NULL; i++) {
        free(tokens[i]);
    }
    free(tokens);
}

// Helper function to add command to history
void add_to_history(const char *command) {
    if (!command || strlen(command) == 0) return;

    // Don't add if it's a log command
    if (strncmp(command, "log", 3) == 0 && (command[3] == ' ' || command[3] == '\0')) {
        return;
    }

    // Don't add if identical to the last command
    if (history_count > 0) {
        int last_index = (history_start + history_count - 1) % 15;
        if (command_history[last_index] && strcmp(command_history[last_index], command) == 0) {
            return;
        }
    }

    int index_to_add;
    if (history_count < 15) {
        index_to_add = (history_start + history_count) % 15;
        history_count++;
    } else {
        index_to_add = history_start;
        free(command_history[index_to_add]);
        history_start = (history_start + 1) % 15;
    }

    command_history[index_to_add] = strdup(command);
}

// Helper function to execute a command without adding to history
void execute_command_no_history(char **args) {
    if (args == NULL || args[0] == NULL) {
        return;
    }
    if (strcmp(args[0], "hop") == 0) {
        builtin_hop(args);
    } else if (strcmp(args[0], "reveal") == 0) {
        builtin_reveal(args);
    } else if (strcmp(args[0], "log") == 0) {
        // Don't execute log from log execute to prevent recursion
        printf("Cannot execute log command from history\n");
    } else {
        printf("Command not found: %s\n", args[0]);
    }
}

// B.1: hop implementation
int builtin_hop(char **args) {
    char current_dir[MAX_PATH_SIZE];
    if (getcwd(current_dir, sizeof(current_dir)) == NULL) {
        perror("hop: getcwd");
        return 1;
    }

    if (args[1] == NULL) { // hop with no arguments
        char *old_dir = strdup(current_dir);
        if (chdir(home_directory) == 0) {
            free(previous_directory);
            previous_directory = old_dir;
        } else {
            perror("hop");
            free(old_dir);
            return 1;
        }
        return 0;
    }

    int i = 1;
    while (args[i] != NULL) {
        char *target_dir = NULL;
        if (strcmp(args[i], "~") == 0) {
            target_dir = home_directory;
        } else if (strcmp(args[i], ".") == 0) {
            i++;
            continue;
        } else if (strcmp(args[i], "..") == 0) {
            target_dir = "..";
        } else if (strcmp(args[i], "-") == 0) {
            if (previous_directory == NULL) {
                fprintf(stderr, "hop: previous directory not set\n");
                i++;
                continue;
            }
            target_dir = previous_directory;
        } else {
            target_dir = args[i];
        }

        char *old_dir = strdup(current_dir);
        if (chdir(target_dir) != 0) {
            perror("hop");
            free(old_dir);
            return 1; // Stop on first error
        }
        
        free(previous_directory);
        previous_directory = old_dir;
        
        if (getcwd(current_dir, sizeof(current_dir)) == NULL) {
            perror("hop: getcwd");
            return 1;
        }
        i++;
    }
    return 0;
}

// Helper function to compare strings for qsort
int compare_strings(const void *a, const void *b) {
    return strcmp(*(const char **)a, *(const char **)b);
}

// B.2: reveal implementation
int builtin_reveal(char **args) {
    int show_all = 0, long_format = 0;
    char *target_path = ".";
    char path_buffer[MAX_PATH_SIZE];

    int i = 1;
    while (args[i] != NULL) {
        if (args[i][0] == '-') {
            for (int j = 1; args[i][j] != '\0'; j++) {
                if (args[i][j] == 'a') show_all = 1;
                else if (args[i][j] == 'l') long_format = 1;
            }
        } else {
            target_path = args[i];
            break; // First non-flag is the path
        }
        i++;
    }

    if (strcmp(target_path, "~") == 0) {
        target_path = home_directory;
    } else if (strcmp(target_path, "-") == 0) {
        if (previous_directory) {
            target_path = previous_directory;
        } else {
            fprintf(stderr, "reveal: previous directory not set\n");
            return 1;
        }
    }

    DIR *dir = opendir(target_path);
    if (dir == NULL) {
        snprintf(path_buffer, sizeof(path_buffer), "reveal: %s", target_path);
        perror(path_buffer);
        return 1;
    }

    char **entries = NULL;
    int count = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (!show_all && entry->d_name[0] == '.') continue;
        entries = realloc(entries, (count + 1) * sizeof(char *));
        if (!entries) {
            perror("realloc");
            closedir(dir);
            return 1;
        }
        entries[count] = strdup(entry->d_name);
        if (!entries[count]) {
            perror("strdup");
            closedir(dir);
            return 1;
        }
        count++;
    }
    closedir(dir);

    qsort(entries, count, sizeof(char *), compare_strings);

    for (i = 0; i < count; i++) {
        printf("%s%s", entries[i], long_format ? "\n" : (i == count - 1 ? "" : " "));
    }
    if (!long_format && count > 0) printf("\n");

    for (i = 0; i < count; i++) free(entries[i]);
    free(entries);
    return 0;
}

// B.3: log implementation
int builtin_log(char **args) {
    if (args[1] == NULL) {
        // Display history from oldest to newest
        for (int i = 0; i < history_count; i++) {
            int index = (history_start + i) % 15;
            printf("%s\n", command_history[index]);
        }
    } else if (strcmp(args[1], "purge") == 0) {
        // Clear history
        for (int i = 0; i < 15; i++) {
            free(command_history[i]);
            command_history[i] = NULL;
        }
        history_count = 0;
        history_start = 0;
    } else if (strcmp(args[1], "execute") == 0 && args[2] != NULL) {
        // Execute command at index (1-indexed, newest to oldest)
        int index = atoi(args[2]);
        if (index < 1 || index > history_count) {
            fprintf(stderr, "log: invalid index\n");
            return 1;
        }
        
        // Convert from 1-indexed newest-to-oldest to actual array index
        int actual_index = (history_start + history_count - index) % 15;
        char *command_to_run = command_history[actual_index];
        
        // Validate syntax first
        if (!parse_shell_command(command_to_run)) {
            printf("Invalid Syntax!\n");
            return 1;
        }
        
        // Execute the command without adding to history
        char **exec_args = tokenize_input(command_to_run);
        if (exec_args && exec_args[0]) {
            execute_command_no_history(exec_args);
        }
        free_tokens(exec_args);
    } else {
        fprintf(stderr, "log: invalid arguments\n");
        return 1;
    }
    return 0;
}

void save_log_to_file(void) {
    if (!home_directory) return;
    char log_file[MAX_PATH_SIZE];
    snprintf(log_file, sizeof(log_file), "%s/.shell_history", home_directory);
    FILE *file = fopen(log_file, "w");
    if (!file) return;
    for (int i = 0; i < history_count; i++) {
        int index = (history_start + i) % 15;
        fprintf(file, "%s\n", command_history[index]);
    }
    fclose(file);
}

void load_log_from_file(void) {
    if (!home_directory) return;
    char log_file[MAX_PATH_SIZE];
    snprintf(log_file, sizeof(log_file), "%s/.shell_history", home_directory);
    FILE *file = fopen(log_file, "r");
    if (!file) return;
    char line[MAX_INPUT_SIZE];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = 0;
        if (strlen(line) > 0) {
            add_to_history(line);
        }
    }
    fclose(file);
}