#include <ctype.h>
#include <limits.h>
#include <stdarg.h>
#include <stddef.h>

#include "tap_parser.h"
#include "tap_utils.h"
#include "tap_constants.h"


#define cast(tp) (struct _tap_parser *)tp
#define ret_call0(tp, fn) do { \
						      if ((tp)->fn == NULL) \
								  return tap_default_##fn(cast(tp)); \
							  return (tp)->fn(cast(tp)); \
					      } while (0)
#define ret_call1(tp, fn, a) do { \
							     if ((tp)->fn == NULL) \
								     return tap_default_##fn(cast(tp), a); \
								 return (tp)->fn(cast(tp), a); \
							 } while (0)
#define ret_call2(tp, fn, a, b) do { \
							        if ((tp)->fn == NULL) \
							            return tap_default_##fn(cast(tp), a, b); \
									return (tp)->fn(cast(tp), a, b); \
								} while (0)

static int invalid(struct _tap_parser *tp, const char *fmt, ...);

/* Default callback functions */

int
tap_default_invalid_callback(struct _tap_parser *tp, const char *msg)
{
	/* increment our parse errors */
	tp->parse_errors++;

	/* useless, mainly to stop a compile warning
	 * for unused data. */
	if (msg == NULL)
		return 0;

	return 0;
}

int
tap_default_unknown_callback(struct _tap_parser *tp)
{
	/* technically a parse error */
	tp->parse_errors++;
	return 0;
}

int
tap_default_version_callback(struct _tap_parser *tp, long tap_version)
{
	if (tap_version > MAX_TAP_VERSION) {
		return invalid(tp, "TAP Version %ld is greater than "
						   "the maximum of %ld",
					   tap_version, MAX_TAP_VERSION);
	}

	if (tap_version < MIN_TAP_VERSION) {
		return invalid(tp, "TAP Version %ld is less than "
						   "the minimum of %ld",
					   tap_version, MIN_TAP_VERSION);
	}

	tp->version = tap_version;
	return 0;
}

int
tap_default_comment_callback(struct _tap_parser *tp)
{
	(void)tp; /* stop compiler warning */
	return 0;
}

int
tap_default_bailout_callback(struct _tap_parser *tp, char *msg)
{
	tp->bailed = 1;

	if (msg == NULL)
		return 1;

	return 1;
}

int
tap_default_pragma_callback(struct _tap_parser *tp, int state, char *pragma)
{
	if (strncmp(pragma, "strict", sizeof("strict") - 1) == 0) {
		tp->strict = state;
		return 0;
	}

	/* always report invalid pragmas */
	return invalid(tp, "Invalid pragma: %s", pragma);
}

int
tap_default_plan_callback(struct _tap_parser *tp, long upper, char *skip)
{
	/* Already encountered a plan?? */
	if (tp->plan != -1)
		return invalid(tp, "More than one plan given");

	tp->plan = upper;

	if (upper == 0 && skip) {
		tp->skip_all = 1;
		tp->skip_all_reason = strdup(skip);
		if (tp->skip_all_reason == NULL)
			return invalid(tp, "strdup failed: %s", strerror(errno));

		return 0;
	}

	if (upper == 0) {
		tp->skip_all = 1;
		tp->skip_all_reason = NULL;
		return 0;
	}

	return 0;
}

int
tap_default_test_callback(struct _tap_parser *tp, tap_test_result *ttr)
{
	if (tp->plan != -1 && ttr->test_num > tp->plan) {
		return invalid(tp, "Test %ld outside of plan bounds 1..%ld",
					   ttr->test_num, tp->plan);
	}

	switch (ttr->type) {
	case TTT_TODO_PASSED:
		tp->todo++;
		tp->todo_passed++;
		tp->passed++;
		return invalid(tp, "TODO test passed: %s", tp->buffer);

	case TTT_SKIP_FAILED:
		tp->failed++;
		tp->skipped++;
		return invalid(tp, "SKIP test failed: %s", tp->buffer);

	case TTT_OK:
		tp->passed++;
		tp->actual_passed++;
		return 0;

	case TTT_NOT_OK:
		tp->failed++;
		tp->actual_failed++;
		return 0;

	case TTT_TODO:
		tp->failed++;
		tp->todo++;
		return 0;

	case TTT_SKIP:
		tp->passed++;
		tp->skipped++;
		return 0;

	default:
		break;
	}

	/* Should never happen... EVER */
	return invalid(tp, "%s: Invalid tap_test_result?!", __func__);
}


/* Actual parsing */

static int
parse_version(tap_parser *tp)
{
	long version;
	char *end;
	char *buf;

#define len(s) (sizeof(s) - 1)

	if (strncmp("TAP", tp->buffer, len("TAP")) != 0)
		return -1;

	if (!isspace(*(tp->buffer + len("TAP"))))
		return -1;

	/* unfortunantly there can be as much whitespace as the
	 * user wants between TAP and version... */
	buf = strip(tp->buffer + 3);

	if (strncmp("version", buf, len("version")) != 0)
		return -1;

	buf += len("version");

	if (!isspace(*buf))
		return -1;

	/* strtol doesn't set errno to 0 for us */
	errno = 0;
	/* strtol skips leading whitespace,
	 * don't bother stripping */
	version = strtol(buf, &end, 10);

	if (version < 0)
		return -1;

	if (*end != '\0' && !isspace(*end))
		return -1;

	if (version == 0 && errno == EINVAL)
		return -1;

	if (version == LONG_MAX && errno == ERANGE)
		return invalid(tp, "TAP version too large");

	buf = strip(end);
	if (*buf != '\0')
		return -1;

#undef len

	ret_call1(tp, version_callback, version);
}

static int
parse_pragma(tap_parser *tp)
{
	int state;
	size_t len;
	char *c;
	char *pragma;
	char *buf;

	if (strncmp(tp->buffer, "pragma", sizeof("pragma") - 1) != 0)
		return -1;

	buf = tp->buffer + sizeof("pragma") - 1;
	buf = strip(buf);

	while (*buf != '\0') {
		switch (*buf) {
		case '+':
			state = 1;
			break;
		case '-':
			state = 0;
			break;
		default:
			return invalid(tp, "Invalid pragma");
		}

		++buf;

		c = strchr(buf, ',');
		/* last in list, eval and return */
		if (c == NULL)
			ret_call2(tp, pragma_callback, state, chomp(buf));

		/* end buf to pass to pragma callback */
		*c = '\0';
		if (tp->pragma_callback == NULL)
			tap_default_pragma_callback(cast(tp), state, buf);
		else
			tp->pragma_callback(cast(tp), state, buf);


		/* more than one in list, skip past , */
		buf = strip(c + 1);
		if (*buf == '\0')
			return invalid(tp, "Trailing comma in pragma list");
	}

	/* XXX: invalid? */
	return 0;
}

static int
parse_plan(tap_parser *tp)
{
	long upper;
	char *end;
	char *buf;

	if (strncmp(tp->buffer, "1..", 3) != 0)
		return -1;

	if (!isdigit(*(tp->buffer + 3)))
		return -1;

	errno = 0;
	upper = strtol(tp->buffer + 3, &end, 10);

	if (upper < 0)
		return -1;

	if (*end != '\0' && !isspace(*end))
		return -1;

	if (upper == 0 && errno == EINVAL)
		return -1;

	if (upper == LONG_MAX && errno == ERANGE)
		return invalid(tp, "Test plan upper bound is too large");

	if (*end == '\0')
		ret_call2(tp, plan_callback, upper, NULL);

	if (!isspace(*end) && *end != '#')
		return invalid(tp, "Trailing characters in test plan");

	buf = strip(end);

	if (*buf == '\0')
		ret_call2(tp, plan_callback, upper, NULL);

	if (*buf != '#')
		return invalid(tp, "Trailing characters after test plan");

	buf = strip(buf + 1);
	if (strncasecmp(buf, "skip", 4) != 0)
		ret_call2(tp, plan_callback, upper, NULL);

	buf = strip(buf + 4);

	/* if there isn't a skip reason, just give it the
	 * buffer at the NUL character (e.g. give it the empty string) */
	if (*buf == '\0')
		ret_call2(tp, plan_callback, upper, buf);

	ret_call2(tp, plan_callback, upper, chomp(buf));
}

static int
parse_test(tap_parser *tp)
{
	enum tap_test_type type;
	long test_num;
	char *end;
	char *buf;
	tap_test_result ttr;

	type = TTT_OK;

	buf = tp->buffer;
	if (strncmp(buf, "not ", 4) == 0) {
		type = TTT_NOT_OK;
		buf = strip(buf + 4);
	}

	if (strncmp(buf, "ok", 2) != 0)
		return -1;

	buf = strip(buf + 2);

	if (!isdigit(*buf)) {
		test_num = tp->test_num + 1;
		goto rasons;
	}

	errno = 0;
	/* strtol strips for us */
	test_num = strtol(buf, &end, 10);

	if (*end == '\0') {
		/* not reason or directive */
		memset(&ttr, 0, sizeof(ttr));
		ttr.test_num = test_num;
		ret_call1(tp, test_callback, &ttr);
	}

	if (!isspace(*end) && *end != '#') {
		/* text touching the digit?
		 * back off the number and assume
		 * it's part of the test description */
		test_num = tp->test_num + 1;
		goto rasons;
	}

	buf = end;

	if (test_num == 0 && errno == EINVAL) {
		/* not starting with a digit?
		 * must be a test description */
		test_num = tp->test_num + 1;
		goto rasons;
	}

	if (test_num == LONG_MAX && errno == ERANGE)
		return invalid(tp, "Test number is too large");

rasons:
	/* From this point we have a test_num */
	if (test_num != tp->test_num + 1) {
		if (test_num == tp->test_num) {
			return invalid(tp, "Duplicate test number %ld",
						   test_num);
		}
		return invalid(tp, "Tests out of order?!");
	}

	memset(&ttr, 0, sizeof(ttr));
	ttr.test_num = test_num;

	tp->test_num++;

	/* Now skip to the description or directive */
	buf = strip(buf);

	if (*buf != '#') {
		/* description! */
		char *c;
		c = strchr(buf, '#');
		if (c == NULL) {
			/* We only have a description */
			ttr.reason = chomp(buf);
			ret_call1(tp, test_callback, &ttr);
		}

		/* toss a \0 on the reason */
		*c = '\0';
		/* save off the reason */
		ttr.reason = chomp(buf);

		buf = c;
	}

	/* There is a comment of some sort... */

	/* skip passed the '#' */
	buf = strip(buf + 1);

	if (strncasecmp(buf, "skip", 4) == 0) {
		if (type == TTT_NOT_OK)
			type = TTT_SKIP_FAILED;
		else
			type = TTT_SKIP;

		ttr.directive = trim(buf + 4);
	}
	else if (strncasecmp(buf, "todo", 4) == 0) {
		if (type == TTT_OK)
			type = TTT_TODO_PASSED;
		else
			type = TTT_TODO;

		ttr.directive = trim(buf + 4);
	}

	ttr.type = type;
	ret_call1(tp, test_callback, &ttr);
}


static int
tap_eval(tap_parser *tp)
{
	int ret;
	char *bail;

	/* XXX: Should "Bail out!" match:
	 * /^Bail out!\s*(.*)$/
	 * or
	 * /^.*Bail out!\s*(.*)$/ ?
	 */
	/* Check for Bail out before anything else */
	bail = strstr(tp->buffer, "Bail out!");
	if (bail != NULL) {
		bail += sizeof("Bail out!") - 1;
		bail = strip(bail);
		if (*bail != '\0')
			ret_call1(tp, bailout_callback, chomp(bail));
		else
			ret_call1(tp, bailout_callback, NULL);
	}


	/* line too long... or maybe hit eof? */
	if (tp->buffer[tp->buffer_len - 1] != '\n')
		return invalid(tp, "Line too long!");

	/* skip whitespace only lines */
	if (strip(tp->buffer)[0] == '\0')
		return 0;

	/* version only checked iff first line of input */
	if (tp->first_line) {
		tp->first_line = 0;
		ret = parse_version(tp);
		if (ret != -1)
			return ret;
	}


	/* pragma support only available from TAP 13 onward */
	if (tp->version >= 13) {
		ret = parse_pragma(tp);
		if (ret != -1)
			return ret;
	}

	/* check for comment */
	if (tp->buffer[0] == '#')
		ret_call0(tp, comment_callback);

	/* check the plan */
	ret = parse_plan(tp);
	if (ret != -1)
		return ret;

	ret = parse_test(tp);
	if (ret != -1)
		return ret;

	ret_call0(tp, unknown_callback);
}


static int
invalid(struct _tap_parser *tp, const char *fmt, ...)
{
	va_list ap;
	static char msg[1024];

	va_start(ap, fmt);
	vsnprintf(msg, 1024, fmt, ap);
	va_end(ap);

	if (tp->invalid_callback == NULL)
		return tap_default_invalid_callback(tp, msg);

	return tp->invalid_callback(tp, msg);
}


/* Get next line of tap, 0 if good, 1 if no more input */
int
tap_parser_next(tap_parser *tp)
{
	if (get_line(tp) == -1)
		return 1;

	return tap_eval(tp);
}
