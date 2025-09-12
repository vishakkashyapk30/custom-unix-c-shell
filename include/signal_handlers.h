#ifndef SIGNAL_HANDLERS_H
#define SIGNAL_HANDLERS_H

#include "shell.h"
#include "fg_bg.h"
#include <signal.h>

// Signal handling functions
void setup_signal_handlers(void);
void sigint_handler(int sig);
void sigtstp_handler(int sig);
void sigchld_handler(int sig);

// Signal utilities
void ignore_terminal_signals(void);
void restore_terminal_signals(void);
void handle_eof(void);

// Process group management
void set_foreground_process_group(pid_t pgid);
void reset_foreground_process_group(void);

#endif
