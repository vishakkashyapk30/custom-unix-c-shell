#ifndef PING_H
#define PING_H

#include "shell.h"

// Function to send signals to processes
void builtin_ping(char **args);

// Helper function to validate numeric input
int is_valid_number(const char *str);

#endif