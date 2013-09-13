#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "test_log.h"

static FILE* logfile = NULL;

int
log_open(const char *filename, int append)
{
	if (logfile != NULL)
		log_close();

	if (strcmp(filename, "stdout") == 0) {
		logfile = stdout;
		return 0;
	}

	if (strcmp(filename, "stderr") == 0) {
		logfile = stderr;
		return 0;
	}

	if (append)
		logfile = fopen(filename, "a");
	else
		logfile = fopen(filename, "w");

	/* 0 if ! NULL */
	return (logfile == NULL);
}

void
log_close(void)
{
	if (logfile == NULL)
		return;

	if (logfile == stdout)
		return;

	if (logfile == stderr)
		return;

	fclose(logfile);
	logfile = NULL;
}

void
log_write(const char *fmt, ...)
{
	va_list vargs;

	if (logfile == NULL)
		return;

	va_start(vargs, fmt);
	vfprintf(logfile, fmt, vargs);
	va_end(vargs);

	fflush(logfile);
}

void
log_writeln(const char *str)
{
	if (logfile == NULL)
		return;

	fprintf(logfile, "%s\n", str);
	fflush(logfile);
}
