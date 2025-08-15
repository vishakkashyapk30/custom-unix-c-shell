#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/utsname.h>
#include <pwd.h>
#include <errno.h>

#define MAX_INPUT_SIZE 1024
#define MAX_PATH_SIZE 512

extern char *home_directory;
extern char *username;
extern char *system_name;

#endif