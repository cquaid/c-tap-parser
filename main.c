#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "tap_parser.h"

static int
invalid(struct _tap_parser *tp, const char *msg)
{
	fprintf(stderr,"Error: %s\n", msg);
	return tap_default_invalid_callback(tp, msg);
}

static int
unknown(struct _tap_parser *tp)
{
	fprintf(stderr, "Unknown: %s\n", tp->buffer);
	return tap_default_unknown_callback(tp);
}

static int
version(struct _tap_parser *tp, long tap_version)
{
	fprintf(stderr, "Version: %ld\n", tap_version);
	return tap_default_version_callback(tp, tap_version);
}

static int
comment(struct _tap_parser *tp)
{
	fprintf(stderr, "Comment: %s\n", tp->buffer);
	return tap_default_comment_callback(tp);
}

static int
bailout(struct _tap_parser *tp, char *msg)
{
	fprintf(stderr, "Bailout: %s\n", msg);
	return tap_default_bailout_callback(tp, msg);
}

static int
pragma(struct _tap_parser *tp, int state, char *pragma)
{
	fprintf(stderr, "Pragma: %c%s\n", (state)?'+':'-', pragma);
	return tap_default_pragma_callback(tp, state, pragma);
}

static int
plan(struct _tap_parser *tp, long upper, char *skip)
{
	fprintf(stderr, "Plan: 1..%ld %s\n", upper, skip);
	return tap_default_plan_callback(tp, upper, skip);
}

static int
test(struct _tap_parser *tp, tap_test_result *ttr)
{
	fprintf(stderr, "Test: ");
	switch (ttr->type) {
	case TTT_OK:
		fprintf(stderr, "ok ");
		break;
	case TTT_NOT_OK:
		fprintf(stderr, "not ok ");
		break;
	case TTT_TODO:
		fprintf(stderr, "todo ");
		break;
	case TTT_TODO_PASSED:
		fprintf(stderr, "ok_todo ");
		break;
	case TTT_SKIP:
		fprintf(stderr, "skip ");
		break;
	case TTT_SKIP_FAILED:
		fprintf(stderr, "not_ok_skip ");
		break;
	}

	fprintf(stderr, "%ld %s # %s\n", ttr->test_num, ttr->reason, ttr->directive);
	return tap_default_test_callback(tp, ttr);
}



int main(int argc, char *argv[])
{
	tap_parser tp;

	tap_parser_init(&tp, 512);

	tp.fd = open(argv[1], O_RDONLY);

	tp.test_callback = test;
	tp.plan_callback = plan;
	tp.pragma_callback = pragma;
	tp.bailout_callback = bailout;
	tp.comment_callback = comment;
	tp.version_callback = version;
	tp.unknown_callback = unknown;
	tp.invalid_callback = invalid;

	while (tap_parser_next(&tp) == 0) {
		/* do nothing */
	}

	printf("Aborted or end of input?\n");

	tap_parser_fini(&tp);
	close(tp.fd);
	return 0;
}
