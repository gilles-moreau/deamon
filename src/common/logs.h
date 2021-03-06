#ifndef _LOGS_H
#define _LOGS_H

#include <syslog.h>

typedef enum {
	LOG_LEVEL_QUIET = 0,
	LOG_LEVEL_FATAL,
	LOG_LEVEL_ERROR,
	LOG_LEVEL_INFO,
	LOG_LEVEL_WARNING,
	LOG_LEVEL_VERBOSE,
	LOG_LEVEL_DEBUG
} log_level_t;

typedef struct {
	log_level_t stderr_level;
	log_level_t syslog_level;
	log_level_t logfile_level;
} log_option_t;

extern void fatal(const char *, ...)
	__attribute__((format (printf, 1, 2))) __attribute__((noreturn));

extern int error(const char *, ...)
	__attribute__((format (printf, 1, 2)));

extern void info(const char *, ...)
	__attribute__((format (printf, 1, 2)));

extern void debug(const char *, ...)
	__attribute__((format (printf, 1, 2)));

int init_log(char *prog, log_option_t opts, char *logfile);

#endif
