#ifndef _H_TAP_UTILS
#define _H_TAP_UTILS

#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "tap_parser.h"

static inline char*
strip(char *p)
{
	while (isspace(*p))
		++p;

	return p;
}

static char*
chomp(char *str)
{
	char *tmp;
	size_t len;

	len = strlen(str);
	if (!len) return str;

	tmp = (str + len - 1);

	while (isspace(*tmp))
		--tmp;

	*(tmp + 1) = '\0';
	return str;
}

static inline char*
trim(char *str)
{
	/* str = strip(str);
	 * return chomp(str); */
	return chomp(strip(str));
}

/* Returns:
 *  0 - pipe closes/end of read, or blocking too long
 * -1 - error
 *  1 - successfully read a line and more to read */
static int
get_line(tap_parser *tp)
{
	char cbuf[1];
	int count;
	int line_done;
	int iter;
	ssize_t ret;

	/* from *tp */
	char *buffer;
	size_t buffer_len;

	iter = 0;
	count = 0;
	line_done = 0;

	buffer = tp->buffer;
	/* len - 1 to leave room for a null terminator */
	buffer_len = tp->buffer_len - 1;

	while ((count < buffer_len) && (!line_done)) {
		ret = read(tp->fd, cbuf, 1);
		switch (ret) {
		case -1:
			/* According to POSIX, both of these can be returned
			 * when read would normally block iff O_NOBLOCK is set */
			 if (errno == EAGAIN || errno == EWOULDBLOCK) {
				if (iter < tp->blocking_time) {
					++iter;
					sleep(1);
					continue;
				}

				/* if we've looped enough, just return */
				buffer[count] = '\0';
				return 0;
			 }

			 /* For all other errors, return -1 */
			 buffer[count] = '\0';
			 return -1;
		case 0:
			/* EOF reached */
			buffer[count] = '\0';
			return -1;
		default:
			break;
		}

		buffer[count++] = cbuf[0];
		if (cbuf[0] == '\n') {
			buffer[count] = '\0';
			return 1;
		}

		/* Reset the counter since we've read something */
		iter = 0;
	}

	buffer[count] = '\0';

	return 1;
}

/* Just wrap the get_line call */
static inline char*
next_raw(tap_parser *tp)
{
	if (get_line(tp) == -1)
		return NULL;

	return tp->buffer;
}

#endif /* _H_TAP_UTILS */
