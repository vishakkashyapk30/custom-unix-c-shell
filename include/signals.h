#ifndef SIGNALS_H
#define SIGNALS_H

#include "shell.h"

// Signal handling functions
void setup_signal_handlers(void);
void sigint_handler(int sig);
void sigtstp_handler(int sig);
void sigchld_handler(int sig);

// Signal utilities
void ignore_terminal_signals(void);
void restore_terminal_signals(void);

#endif