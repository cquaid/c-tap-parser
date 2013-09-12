#ifndef _H_TEST_CALLBACKS
#define _H_TEST_CALLBACKS

#include "tap_parser.h"

#include <stdio.h>
#include <string.h>

/* From test.c */
extern int verbosity;

static int
invalid_cb(tap_parser *tp, int err, const char *msg)
{
    if (verbosity >= 3) {
        printf("Error: [%d] %s\n", err, msg);
        fflush(stdout);
    }

    return tap_default_invalid_callback(tp, err, msg);
}

static int
unknown_cb(tap_parser *tp)
{
    /* Ignore unknown lines */
    (void)tp;
    return 0;

#if 0
    size_t len;

    if (verbosity < 3)
        return tap_default_unknown_callback(tp);

    len = strlen(tp->buffer);
    /* remove the trailing newline */
    if (tp->buffer[len - 1] == '\n')
        tp->buffer[len - 1] = '\0';

    printf("Unknown: %s\n", tp->buffer);
    fflush(stdout);

    return tap_default_unknown_callback(tp);
#endif
}

static int
version_cb(tap_parser *tp, long tap_version)
{
    if (verbosity >= 3) {
        printf("Version: %ld\n", tap_version);
        fflush(stdout);
    }

    return tap_default_version_callback(tp, tap_version);
}

static int
comment_cb(tap_parser *tp)
{
    /* Do nothing for comments */
    (void)tp;
    return 0;

#if 0
    size_t len;

    if (verbosity < 3)
        return tap_default_comment_callback(tp);

    len = strlen(tp->buffer);
    /* remove the trailing newline */
    if (tp->buffer[len - 1] == '\n')
        tp->buffer[len - 1] = '\0';

    printf("Comment: %s\n", tp->buffer);
    fflush(stdout);

    return tap_default_comment_callback(tp);
#endif
}

static int
bailout_cb(tap_parser *tp, char *msg)
{
    if (verbosity < 3)
        return tap_default_bailout_callback(tp, msg);

    printf("Bail out!");
    if (msg)
        printf(" %s\n", msg);
    else
        putchar('\n');

    fflush(stdout);

    return tap_default_bailout_callback(tp, msg);
}

static int
pragma_cb(tap_parser *tp, int state, char *pragma)
{
    if (verbosity >= 3) {
        printf("Pragma: %c%s\n", (state) ? '+' : '-', pragma);
        fflush(stdout);
    }

    return tap_default_pragma_callback(tp, state, pragma);
}

static int
plan_cb(tap_parser *tp, long upper, char *skip)
{
    if (verbosity < 3)
            return tap_default_plan_callback(tp, upper, skip);

    printf("Plan: 1..%ld", upper);
    if (skip)
        printf(" # skip %s\n", skip);
    else
        putchar('\n');

    fflush(stdout);

    return tap_default_plan_callback(tp, upper, skip);
}

static int
test_cb(tap_parser *tp, tap_test_result *ttr)
{
    if (verbosity < 3)
        return tap_default_test_callback(tp, ttr);

    printf("Test: %ld ", ttr->test_num);
    switch (ttr->type) {
    case TTT_OK:
        printf("ok");
        break;
    case TTT_NOT_OK:
        printf("not ok");
        break;
    case TTT_TODO:
        printf("todo");
        break;
    case TTT_TODO_PASSED:
        printf("ok todo");
        break;
    case TTT_SKIP:
        printf("skip");
        break;
    case TTT_SKIP_FAILED:
        printf("not ok skip");
        break;
    case TTT_INVALID:
        printf("missing?");
        break;
    }

    if (ttr->reason) {
        if (ttr->directive)
            printf(": %s (%s)\n", ttr->reason, ttr->directive);
        else
            printf(": %s\n", ttr->reason);
    }
    else if (ttr->directive)
        printf(" (%s)\n", ttr->directive);
    else
        putchar('\n');

    fflush(stdout);

    return tap_default_test_callback(tp, ttr);
}

#endif /* _H_TEST_CALLBACKS */

/* vim: set ts=4 sw=4 sws=4 expandtab: */
