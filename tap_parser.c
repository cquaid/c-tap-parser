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

    /* Need to allocate a new tap_results struct
     * to store everything in. */
    tp->tr = (tap_results *)malloc(sizeof(tap_results));
    if (tp->tr == NULL)
        return errno;

    memset(tp->tr, 0, sizeof(tap_results));

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
    if (tp->buffer == NULL) {
        tap_results_fini(tp->tr);
        return errno;
    }

    /* don't bother memsetting the buffer, waste of time */

    return 0;
}

int
tap_parser_reset(tap_parser *tp)
{
    char *buffer;
    size_t buffer_len;
    tap_results *results;

    if (tp->buffer == NULL) {
        /* This case doesn't usually happen
         * But if the results aren't stolen free them */
        if (tp->tr != NULL)
            tap_results_fini(tp->tr);

        /* No buffer? re-init */
        tap_parser_fini(tp);
        return tap_parser_init(tp, DEFAULT_BUFFER_LEN);
    }

    /* Store the buffer addr so we don't
     * have to realloc it. */
    buffer = tp->buffer;
    buffer_len = tp->buffer_len;

    /* If it's not stolen wipe it out */
    if (tp->tr) {
        if (tp->tr->results)
            free(tp->tr->results);
        memset(tp->tr, 0, sizeof(tap_results));
        results = tp->tr;
    }
    else {
        /* Otherwise create a new one */
        results = (tap_results *)malloc(sizeof(tap_results));
        if (results == NULL)
            return errno;

        memset(results, 0, sizeof(tap_results));
    }

    memset(tp, 0, sizeof(*tp));

    tp->tr = results;

    tp->fd = -1;
    tp->plan = -1;

    tp->strict = 1;
    tp->first_line = 1;

    tp->version = DEFAULT_TAP_VERSION;
    tp->blocking_time = DEFAULT_BLOCKING_TIME;

    tp->buffer = buffer;
    tp->buffer_len = buffer_len;

    return 0;
}

void
tap_parser_fini(tap_parser *tp)
{
    if (tp->buffer)
        free(tp->buffer);

    if (tp->tr)
        tap_results_fini(tp->tr);
}

tap_results*
tap_parser_steal_results(tap_parser *tp)
{
    tap_results *ret;

    ret = tp->tr;
    tp->tr = NULL;

    return ret;
}

void
tap_results_fini(tap_results *tr)
{
    if (tr == NULL)
        return;

    if (tr->results != NULL)
        free(tr->results);

    free(tr);
}


/* vim: set ts=4 sw=4 sts=4 expandtab: */
