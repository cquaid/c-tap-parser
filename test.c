#include <sys/types.h>

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "tap_parser.h"

/* Types */

enum analyze_ret {
    AR_SUCCESS = 0, /* All tests passed     */
    AR_ABORTED = 1, /* Something went wrong */
    AR_FAILED  = 2  /* Some tests failed    */
};


/* Globals */
static int debug = 0;
static int verbosity = 0;

/* Callback Functions */
static int invalid(tap_parser *tp, int err, const char *msg);
static int unknown(tap_parser *tp);
static int version(tap_parser *tp, long tap_version);
static int comment(tap_parser *tp);
static int bailout(tap_parser *tp, char *msg);
static int pragma(tap_parser *tp, int state, char *pragma);
static int plan(tap_parser *tp, long upper, char *skip);
static int test(tap_parser *tp, tap_test_result *ttr);

/* Helpers */
static void dump_results_array(const tap_parser *tp);
static void dump_tap_stats(const tap_parser *tp);
static int analyze_results(const tap_parser *tp);
static pid_t exec_test(tap_parser *tp, const char *path);


static void
usage(FILE *file, const char *name)
{
    fprintf(file, "usage: %s [options] filename\n", name);
    fprintf(file, " -h    display this message\n");
    fprintf(file, " -v    increase verbose output\n");
    fprintf(file, " -d    debug information, implies -vv\n");
    fflush(file);
}

int
main(int argc, char *argv[])
{
    int ret;
    int opt;
    pid_t pid;
    tap_parser tp;

    const char *name;
    const char *filename;

    name = argv[0];

    while ((opt = getopt(argc, argv, "vhd")) != EOF) {
        switch (opt) {
        case 'v':
            verbosity++;
            break;
        case 'd':
            debug = 1;
            if (verbosity < 2)
                verbosity = 2;
            break;
        case 'h':
            usage(stdout, name);
            exit(EXIT_SUCCESS);
        default:
            fprintf(stderr, "Invalid option: %c\n", (char)(opt & 0xff));
            usage(stderr, name);
            exit(EXIT_FAILURE);
        }
    }

    argv += optind;
    argc -= optind;

    if (argc != 1) {
        fprintf(stderr, "Missing filename!");
        usage(stderr, name);
        exit(EXIT_FAILURE);
    }

    filename = argv[0];
    if (verbosity >= 3) {
        printf("Running %s\n", filename);
        fflush(stdout);
    }

    ret = tap_parser_init(&tp, 512);
    if (ret != 0) {
        fprintf(stderr, "tap_parser_init(): %s\n", strerror(ret));
        fflush(stderr);
        exit(EXIT_FAILURE);
    }

    pid = exec_test(&tp, filename);
    if (verbosity >= 3) {
        printf("Child pid: %lu\n", (unsigned long)pid);
        fflush(stdout);
    }

    tap_parser_set_test_callback(&tp, test);
    tap_parser_set_plan_callback(&tp, plan);
    tap_parser_set_pragma_callback(&tp, pragma);
    tap_parser_set_bailout_callback(&tp, bailout);
    tap_parser_set_comment_callback(&tp, comment);
    tap_parser_set_version_callback(&tp, version);
    tap_parser_set_unknown_callback(&tp, unknown);
    tap_parser_set_invalid_callback(&tp, invalid);

    while (tap_parser_next(&tp) == 0)
        ;

    if (debug)
        dump_tap_stats(&tp);

    ret = analyze_results(&tp);

    if (verbosity >= 1)
        dump_results_array(&tp);

    close(tp.fd);
    tap_parser_fini(&tp);
    return ret;
}

static void
dump_tap_stats(const tap_parser *tp)
{
    putchar('\n');
    if (tp->bailed)
        printf("Bailed out.\n");

    if (tp->skip_all) {
        if (tp->skip_all_reason && tp->skip_all_reason[0] != '\0')
            printf("All Skipped (%s)\n\n", tp->skip_all_reason);
        else
            printf("All Skipped.\n\n");
    }

    if (tp->plan == -1)
        printf("Ran %ld of 0 tests.\n", tp->tests_run);
    else
        printf("Ran %ld of %ld tests.\n", tp->tests_run, tp->plan);
    printf("    Tap Version: %ld\n", tp->version);
    printf("     Tap Errors: %ld\n", tp->parse_errors);
    printf("         Failed: %ld\n", tp->failed);
    printf("  Actual Failed: %ld\n", tp->failed - (tp->todo_passed + tp->skip_failed));
    printf("         Passed: %ld\n", tp->passed);
    printf("  Actual Passed: %ld\n", tp->passed - tp->skipped);
    printf("        Skipped: %ld\n", tp->skipped);
    printf("  Dubious Skips: %ld\n", tp->skip_failed);
    printf("          Todos: %ld\n", tp->todo);
    printf("  Dubious Todos: %ld\n", tp->todo_passed);

    fflush(stdout);
}

/* This function was ripped from the c-tap-harness */
static size_t
print_range(size_t first, size_t last, size_t chars, size_t limit)
{
    size_t i;
    size_t needed = 0;

    /* Count radii needed for first number:
     *
     * 12 has 2 radii
     * 256 has 3 radii
     * etc.
     **/
    for (i = first; i > 0; i /= 10)
        ++needed;

    if (last > first) {
        /* Count radii needed for last number*/
        for (i = last; i > 0; i /= 10)
            ++needed;
        /* +1 for the '-' */
        ++needed;
    }

    /* +2 for ', ' */
    if (chars > 0)
        needed += 2;

    /* Since we can have a limit, make sure we don't
     * print too many characters */
    if (limit > 0 && (chars + needed) > limit) {
        needed = 0;
        if (chars <= limit) {
            if (chars > 0) {
                printf(", ");
                needed += 2;
            }
            printf("...");
            needed += 3;
        }
    }
    else {
        if (chars > 0)
            printf(", ");
        if (last > first)
            printf("%lu-", (unsigned long)first);
        printf("%lu", (unsigned long)last);
    }

    return needed;
}

static void
summarize_results(const tap_parser *tp)
{
    size_t i, first, last;
    size_t failed, missing;

    first = last = 0;
    failed = missing = 0;
    /* First loop to check if we're missing something */
    for (i = 1; i < tp->results_len; ++i) {
        /* Skip anything that's not invalid */
        if (tp->results[i] != TTT_INVALID)
            continue;

        if (missing == 0)
            printf("MISSED ");

        if (first && i == last)
            ++last;
        else {
            if (first)
                print_range(first, last, missing - 1, 0);
            ++missing;
            first = last = i;
        }
    }

    /* Final range if the loop ended */
    if (first)
        print_range(first, last, missing - 1, 0);



    first = last = 0;
    /* Print the Failed tests */
    for (i = 1; i < tp->results_len; ++i) {
        /* Skip anything that's not a failure */
        if (tp->results[i] != TTT_NOT_OK)
            continue;

        /* seperate the fields if we have more than one */
        if (missing && !failed)
            printf("; ");

        if (failed == 0)
            printf("FAILED ");

        if (first && i == last)
            ++last;
        else {
            if (first)
                print_range(first, last, failed - 1, 0);
            ++failed;
            first = last = i;
        }
    }

    /* Final range for failed */
    if (first)
        print_range(first, last, failed - 1, 0);


    if (missing == 0 && failed == 0) {
        if (tp->todo_passed || tp->skip_failed)
            printf("dubious");
        else
            printf("ok");

        if (tp->skipped > 0) {
            if (tp->skipped == 1)
                printf(" (skipped 1 test)");
            else
                printf(" (skipped %ld tests)", tp->skipped);
        }
    }

    putchar('\n');
    fflush(stdout);
}

static int
analyze_results(const tap_parser *tp)
{
    putchar('\n');

    if (tp->skip_all) {
        if (tp->skip_all_reason == NULL)
            printf("skipped\n");
        else
            printf("skipped (%s)\n", tp->skip_all_reason);
        return AR_SUCCESS;
    }

    if (tp->bailed) {
        printf("Aborted (Bailed Out)\n");
        return AR_ABORTED;
    }

    if (tp->plan == -1) {
        printf("Aborted (No Plan)\n");
        return AR_ABORTED;
    }

    if (tp->tests_run > tp->plan) {
        printf("Aborted (Extra Tests)\n");
        return AR_ABORTED;
    }

#if 0
    if (tp->tests_run < tp->plan) {
        printf("Aborted (Missing Tests)\n");
        return AR_ABORTED;
    }
#endif

    summarize_results(tp);

    if (tp->failed)
        return AR_FAILED;

    return AR_SUCCESS;
}

static void
dump_results_array(const tap_parser *tp)
{
    long i, t;
    long passed, failed;
    long todo, skipped;
    long dubious, missing;
    struct tt {
        enum tap_test_type type;
        const char *str;
    } const static normal[] = {
        { TTT_OK,     " passed" },
        { TTT_NOT_OK, " failed" },
        { TTT_TODO,   "   todo" },
        { TTT_SKIP,   "skipped" }
    };
#define normal_len (sizeof(normal)/sizeof(struct tt))


    if (tp->results == NULL || tp->results == 0)
        return;

    passed = failed = 0;
    todo = skipped = 0;
    dubious = missing = 0;
    for (i = 1; i < tp->results_len; ++ i) {
        switch (tp->results[i]) {
        case TTT_OK:
            ++passed;
            break;
        case TTT_NOT_OK:
            ++failed;
            break;
        case TTT_TODO:
            ++todo;
            break;
        case TTT_SKIP:
            ++skipped;
            break;
        case TTT_TODO_PASSED:
            ++dubious;
            break;
        case TTT_SKIP_FAILED:
            ++dubious;
            break;
        case TTT_INVALID:
            ++missing;
            break;
        }
    }

    for (t = 0; t < normal_len; ++t) {
        switch (normal[t].type) {
        case TTT_OK:
            if (!passed) continue;
            break;
        case TTT_NOT_OK:
            if (!failed) continue;
            break;
        case TTT_TODO:
            if (!todo) continue;
            break;
        case TTT_SKIP:
            if (!skipped) continue;
            break;
        case TTT_TODO_PASSED:
        case TTT_SKIP_FAILED:
        case TTT_INVALID:
            continue;
        }
        printf("%s: ", normal[t].str);
        for (i = 1; i < tp->results_len; ++i) {
            if (tp->results[i] == normal[t].type)
                printf("%ld, ", i);
        }
        putchar('\n');
    }

    if (dubious) {
        printf("dubious: ");
        for (i = 1; i < tp->results_len; ++i) {
            if (tp->results[i] == TTT_TODO_PASSED ||
                tp->results[i] == TTT_SKIP_FAILED ) {
                printf("%ld, ", i);
            }
        }
        putchar('\n');
    }

    if (missing) {
        printf("missing: ");
        for (i = 1; i < tp->results_len; ++i) {
            if (tp->results[i] == TTT_INVALID)
                printf("%ld, ", i);
        }
        putchar('\n');
    }

    fflush(stdout);

#undef normal_len
}

static pid_t
exec_test(tap_parser *tp, const char *path)
{
    pid_t child;
    int pipes[2];

#define READ_PIPE  0
#define WRITE_PIPE 1
    if (pipe(pipes) == -1) {
        perror("pipe()");
        exit(EXIT_FAILURE);
    }

    child = fork();
    if (child == (pid_t)-1) {
        perror("fork()");
        exit(EXIT_FAILURE);
    }

    if (child == 0) {
        /* child proc */
        int fd = open("/dev/null", O_WRONLY);
        if (fd == -1)
            exit(EXIT_FAILURE);
        if (dup2(fd, STDERR_FILENO) == -1)
            exit(EXIT_FAILURE);

        if (dup2(pipes[WRITE_PIPE], STDOUT_FILENO) == -1)
            exit(EXIT_FAILURE);

        close(fd);
        close(pipes[READ_PIPE]);
        close(pipes[WRITE_PIPE]);

        if (execl(path, path, (char *)NULL) == -1)
            exit(EXIT_FAILURE);
    }
    else {
        /* parent, close write end */
        close(pipes[WRITE_PIPE]);
    }

    tp->fd = pipes[READ_PIPE];
    return child;
#undef READ_PIPE
#undef WRITE_PIPE
}


/* TAP Functions */

static int
invalid(tap_parser *tp, int err, const char *msg)
{
    if (verbosity >= 3) {
        printf("Error: [%d] %s\n", err, msg);
        fflush(stdout);
    }

    return tap_default_invalid_callback(tp, err, msg);
}

static int
unknown(tap_parser *tp)
{
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
}

static int
version(tap_parser *tp, long tap_version)
{
    if (verbosity >= 3) {
        printf("Version: %ld\n", tap_version);
        fflush(stdout);
    }

    return tap_default_version_callback(tp, tap_version);
}

static int
comment(tap_parser *tp)
{
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
}

static int
bailout(tap_parser *tp, char *msg)
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
pragma(tap_parser *tp, int state, char *pragma)
{
    if (verbosity >= 3) {
        printf("Pragma: %c%s\n", (state) ? '+' : '-', pragma);
        fflush(stdout);
    }

    return tap_default_pragma_callback(tp, state, pragma);
}

static int
plan(tap_parser *tp, long upper, char *skip)
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
test(tap_parser *tp, tap_test_result *ttr)
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

/* vim: set ts=4 sw=4 sws=4 expandtab: */
