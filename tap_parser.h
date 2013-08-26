#ifndef _H_TAP_PARSER
#define _H_TAP_PARSER

#include <stddef.h>

enum tap_test_type {
    TTT_OK,          /* ok ...                */
    TTT_NOT_OK,      /* not ok ...            */
    TTT_TODO,        /* not ok ... # todo ... */
    TTT_TODO_PASSED, /* ok ... # todo ...     */
    TTT_SKIP,        /* ok ... # skip ...     */
    TTT_SKIP_FAILED  /* not ok ... # skip ... */
};

/* easy way to pass around the test result */
typedef struct {
    enum tap_test_type type;
    long test_num;
    char *reason;
    char *directive;
} tap_test_result;

struct _tap_parser;
typedef struct _tap_parser tap_parser;

/* test callback called for every TAP test:
 * Args:
 *  tap_test_result *ttr - test results
 *
 * The ttr->type for TTT_TODO_PASSED and TTT_SKIP_FAILED
 * are actually error cases.
 */
typedef int(*tap_test_callback)(tap_parser*, tap_test_result*);

/* plan callback is called everytime a plan statement is found
 * Args:
 *  long upper_bound - upper_bound of the plan
 *  char *skip_message - if a skip is present, the message
 *
 * Valid Plans:
 *  "1..\d+" At the top of the input, set number of tests expected
 *  "1..\d+" At bottom of input (last line), say we're done here,
 *            and that we expected \d+ number of tests to have already run
 *  "1..0"   At top of input, skip everything.
 *  "1..0 # skip <message>"  At top of input, skip everything, report <message>"
 */
typedef int(*tap_plan_callback)(tap_parser*, long, char*);

/* pragma callback is called for each pragma found in the
 * pragma directive list.
 * Args:
 *  int state - state of the pragma, 0 - off, 1 - on
 *  char *pragma_name - name of the pragma to modify
 */
typedef int(*tap_pragma_callback)(tap_parser*, int, char*);

/* bailout callback called when 'Bail out!' is encountered
 * Args:
 *  char *bailout_reason - why we bailed
 */
typedef int(*tap_bailout_callback)(tap_parser*, char*);

/* comment_callback, called when a comment is found
 *
 * the comment can be found in *tp->buffer
 */
typedef int(*tap_comment_callback)(tap_parser*);

/* version callback, called when a version is found
 * Args:
 *  long tap_version
 *
 * This will only ever be called ONCE, and ONLY if it's the first
 * line of input.  All others call invalid_callback.
 */
typedef int(*tap_version_callback)(tap_parser*, long);


/* unknown is called when the evaluator doesn't know what to
 * do with something.
 *
 * the raw line can be found in *tp->buffer
 */
typedef int(*tap_unknown_callback)(tap_parser*);

/* invalid callback is called when a parse error is thrown
 * Args:
 *  const char *error_message
 */
typedef int(*tap_invalid_callback)(tap_parser*, const char*);

struct _tap_parser {
    /* Parser Callbacks */
    tap_test_callback test_callback;
    tap_plan_callback plan_callback;
    /* tap_yaml_callback yaml_callback; */ /* not supported */
    tap_pragma_callback pragma_callback;
    tap_bailout_callback bailout_callback;
    tap_comment_callback comment_callback;
    tap_version_callback version_callback;
    tap_unknown_callback unknown_callback;
    /* when a parse error is thrown, go here */
    tap_invalid_callback invalid_callback;

    /* Parser Storage */
    int first_line;
    char *buffer;
    size_t buffer_len;

    /* Parser Config */
    int strict;
    int fd;
    int blocking_time;

    /* TAP Specific Members */
    int bailed;   /* have we bailed? */
    long version;
    long plan;
    long test_num;
    long tests_run;
    long skipped;
    long passed;
    long todo;
    long failed;
    long actual_failed;     /* failed + todo */
    long actual_passed;     /* passed + skipped */
    long todo_passed;       /* todo's that unexpectedly succeed */
    long parse_errors;      /* number of parse errors found */
    int skip_all;          /* Skip all tests? */
    char *skip_all_reason; /* Why all tests are skipped */
};

/* Initialize the parser, returns errno from malloc on failure */
extern int tap_parser_init(tap_parser *tp, size_t buffer_len);

/* Cleanup... */
extern void tap_parser_fini(tap_parser *tp);

/* Get next line of tap, 0 if good, 1 if no more input */
extern int tap_parser_next(tap_parser *tp);

/* default callbacks */
extern int tap_default_invalid_callback(tap_parser *tp, const char *msg);
extern int tap_default_unknown_callback(tap_parser *tp);
extern int tap_default_version_callback(tap_parser *tp, long tap_version);
extern int tap_default_comment_callback(tap_parser *tp);
extern int tap_default_bailout_callback(tap_parser *tp, char *msg);
extern int tap_default_pragma_callback(tap_parser *tp, int state, char *pragma);
extern int tap_default_plan_callback(tap_parser *tp, long upper, char *skip);
extern int tap_default_test_callback(tap_parser *tp, tap_test_result *ttr);

/* macros for setting callbacks */
#define tap_parser_set_callback(tp, name, fn) do { (tp)->name##_callback = fn; } while(0)
#define tap_parser_set_invalid_callback(tp, fn) tap_parser_set_callback(tp, invalid, fn)
#define tap_parser_set_unknown_callback(tp, fn) tap_parser_set_callback(tp, unknown, fn)
#define tap_parser_set_version_callback(tp, fn) tap_parser_set_callback(tp, version, fn)
#define tap_parser_set_comment_callback(tp, fn) tap_parser_set_callback(tp, comment, fn)
#define tap_parser_set_bailout_callback(tp, fn) tap_parser_set_callback(tp, bailout, fn)
#define tap_parser_set_pragma_callback(tp, fn) tap_parser_set_callback(tp, pragma, fn)
#define tap_parser_set_plan_callback(tp, fn) tap_parser_set_callback(tp, plan, fn)
#define tap_parser_set_test_callback(tp, fn) tap_parser_set_callback(tp, test, fn)

#endif /* _H_TAP_PARSER */

/* vim: set ts=4 sw=4 sts=4 expandtab: */
