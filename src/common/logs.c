#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stddef.h>
#include <pthread.h>
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "logs.h"
#include "macros.h"
#include "skrum_errno.h"

typedef struct {
	char *argv0;
	FILE *logfp;
	log_option_t opts;
	unsigned initialized:1;
} log_t;

static log_t *log = NULL;
static pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;

static int _log_init(char *prog, log_option_t opts, char *logfile)
{
	int rc = 0;

	if (!log) {
		log = malloc(sizeof(log_t));
		log->argv0 = NULL;
		log->logfp = NULL;
	}

	if (prog) {
		if (log->argv0)
			free(log->argv0);
		log->argv0 = strdup(basename(prog));
	}

	log->opts = opts;

	if (logfile && (log->opts.logfile_level > LOG_LEVEL_QUIET)) {
		int mode = O_CREAT | O_WRONLY | O_APPEND | O_CLOEXEC;
		int fd;
		FILE *fp;

		fd = open(logfile, mode, S_IRUSR | S_IWUSR);
		if (fd >= 0)
			fp = fdopen(fd, "a");

		if ((fd < 0) || !fp) {
			char *errmsg = skrum_strerror(errno);
			fprintf(stderr,
					"%s: %s: Unable to open logfile `%s': %s\n",
					prog, __func__, logfile, errmsg);
			if (fd >= 0)
				close(fd);

			rc = errno;
			goto out;
		}

		if (log->logfp)
			fclose(log->logfp); /* Ignore errors */

		log->logfp = fp;

	}

	if (log->logfp && (fileno(log->logfp) < 0))
		log->logfp = NULL;

	log->initialized = 1;

out: 
	return rc;
}

int init_log(char *prog, log_option_t opts, char *logfile)
{
	int rc;

	skrum_mutex_lock(&log_lock);
	rc = _log_init(prog, opts, logfile);
	skrum_mutex_unlock(&log_lock);
	return rc;	
}

static void _log_printf(log_t *log, FILE *stream,
		const char *fmt, va_list ap)
{
	int fd = -1;

	if (stream)
		fd = fileno(stream);
	if (fd < 0)
		return;

	vfprintf(stream, fmt, ap);
}

static void _log_msg(log_level_t level, const char *fmt, va_list args)
{
	char *pfx = "";
	char *pretty_fmt;
	const char *fmt_template = "%s %s\n";
	int len;

	skrum_mutex_lock(&log_lock);

	if (log->opts.stderr_level > level) {
		switch(level) {
			case LOG_LEVEL_FATAL:
				pfx = "fatal:";
				break;

			case LOG_LEVEL_ERROR:
				pfx = "error:";
				break;

			case LOG_LEVEL_INFO:
				pfx = "info:";
				break;

			case LOG_LEVEL_WARNING:
				pfx = "warning:";
				break;

			case LOG_LEVEL_VERBOSE:
				pfx = "";
				break;

			case LOG_LEVEL_DEBUG:
				pfx = "debug:";
				break;

			default:
				pfx = "internal error:";
				break;
		}
	}

	len = snprintf(NULL, 0, fmt_template, pfx, fmt); 
	pretty_fmt = malloc(len+1);
	if (!pretty_fmt)
		perror("malloc");	

	snprintf(pretty_fmt, len+1, fmt_template, pfx, fmt);

	if (level < log->opts.stderr_level) {
		fflush(stdout);
		_log_printf(log, stderr, pretty_fmt, args);
		fflush(stderr);	
	}

	if (level < log->opts.logfile_level) {
		_log_printf(log, log->logfp, pretty_fmt, args);
		fflush(log->logfp);
	}

	free(pretty_fmt);
	skrum_mutex_unlock(&log_lock);
}

void fatal(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	_log_msg(LOG_LEVEL_FATAL, fmt, ap);
	va_end(ap);	

	exit(1);
}

int error(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	_log_msg(LOG_LEVEL_ERROR, fmt, ap);
	va_end(ap);

	return SKRUM_ERROR;
}

void info(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	_log_msg(LOG_LEVEL_INFO, fmt, ap);
	va_end(ap);	
}

void debug(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	_log_msg(LOG_LEVEL_DEBUG, fmt, ap);
	va_end(ap);	
}
