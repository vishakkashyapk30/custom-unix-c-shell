#ifndef PARSER_H
#define PARSER_H

#include "shell.h"

typedef struct {
    char **commands[MAX_COMMANDS];
    int command_count;
    char *input_file;
    char *output_file;
    int append_output;
    int background;
} ParsedCommand;

// Parser functions
int parse_command(char *input, ParsedCommand *parsed);
void free_parsed_command(ParsedCommand *parsed);
int is_valid_syntax(char *input);
int parse_shell_command(const char *command);
char **tokenize_command(char *command);
void trim_whitespace(char *str);

#endif