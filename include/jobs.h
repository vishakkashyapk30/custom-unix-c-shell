#ifndef JOBS_H
#define JOBS_H

#include "shell.h"

// Job control builtin commands
void builtin_fg(char **args);
void builtin_bg(char **args);
void builtin_activities(char **args);
void builtin_ping(char **args);

// Job management utilities
int find_job_by_id(int job_id);
int find_most_recent_job(void);
void print_job_status(void);
void cleanup_finished_jobs(void);

#endif