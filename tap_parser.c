#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "tap_parser.h"
#include "tap_constants.h"

int
tap_parser_init(tap_parser *tp, size_t buffer_len)
{
	memset(tp, 0, sizeof(*tp));

	/* File descriptors should be -1 when unset. */
	tp->fd = -1;
	tp->blocking_time = DEFAULT_BLOCKING_TIME;

	/* Strict by default
	 * Non-strict mode isn't supported anyway */
	tp->strict = 1;

	/* TAP 12 is the default unless input
	 * starts with "TAP version 13" */
	tp->version = DEFAULT_TAP_VERSION;

	/* The TAP plan is -1 when unset */
	tp->plan = -1;

	/* Tell the evaluator that it's the first line.
	 * TAP version has to be the FIRST line of input,
	 * all other cases it's an error */
	tp->first_line = 1;

	/* when buffer_len == 0 assume default */
	if (buffer_len == 0)
		tp->buffer_len = DEFAULT_BUFFER_LEN;
	else
		tp->buffer_len = buffer_len;

	tp->buffer = (char *)malloc(tp->buffer_len);
	if (tp->buffer == NULL)
		return errno;

	/* don't bother memsetting the buffer, waste of time */

	return 0;
}

void
tap_parser_fini(tap_parser *tp)
{
	if (tp->buffer) free(tp->buffer);
	/* Well... we're done here. */
}
