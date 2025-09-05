#ifndef INPUT_H
#define INPUT_H

#include "shell.h"
#include "parser.h"

// Input handling functions
char *read_input(void);
void handle_input(char *input);
int is_builtin_command(const char *cmd);

#endif