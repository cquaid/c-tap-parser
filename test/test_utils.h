#ifndef _H_TEST_UTILS
#define _H_TEST_UTILS

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
die(int err, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	if (err)
		fprintf(stderr, ": %s\n", strerror(err));

	fflush(stderr);
	exit(EXIT_FAILURE);
}

#endif /* _H_TEST_UTILS */
