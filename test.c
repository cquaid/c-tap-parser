#include <sys/types.h>

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "tap_parser.h"

/* Globals */
static int verbose = 0;

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
static void dump_tap_stats(tap_parser *tp);
static pid_t exec_test(tap_parser *tp, const char *path);

static void vprint(const char *fmt, ...);


static void
usage(FILE *file, const char *name)
{
    fprintf(file, "usage: %s [options] filename\n", name);
    fprintf(file, " -h    display this message\n");
    fprintf(file, " -v    increase verbose output\n");
    fflush(file);
}

int
main(int argc, char *argv[])
{
    int opt;
    pid_t pid;
    tap_parser tp;

    const char *name;
    const char *filename;

    name = argv[0];

    while ((opt = getopt(argc, argv, "vh")) != EOF) {
        switch (opt) {
        case 'v':
            verbose++;
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
    vprint("Running %s\n", filename);

    tap_parser_init(&tp, 512);

    pid = exec_test(&tp, filename);
    vprint("Child pid: %lu\n", (unsigned long)pid);

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

    dump_tap_stats(&tp);

    tap_parser_fini(&tp);
    close(tp.fd);
    return 0;
}

static void
dump_tap_stats(tap_parser *tp)
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
        printf("Ran %ld of 0 tests.\n", tp->test_num);
    else
        printf("Ran %ld of %ld tests.\n", tp->test_num, tp->plan);
    printf("    Tap Version: %ld\n", tp->version);
    printf("     Tap Errors: %ld\n", tp->parse_errors);
    printf("         Failed: %ld\n", tp->failed);
    printf("  Actual Failed: %ld\n", tp->actual_failed);
    printf("         Passed: %ld\n", tp->passed);
    printf("  Actual Passed: %ld\n", tp->actual_passed);
    printf("        Skipped: %ld\n", tp->skipped);
    printf("          Todos: %ld\n", tp->todo);
    printf("  Dubious Todos: %ld\n", tp->todo_passed);
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

static void
vprint(const char *fmt, ...)
{
    va_list ap;

    if (!verbose) return;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);

    fflush(stderr);
}


/* TAP Functions */

static int
invalid(tap_parser *tp, int err, const char *msg)
{
    vprint("Error: [%d] %s\n", err, msg);

    return tap_default_invalid_callback(tp, err, msg);
}

static int
unknown(tap_parser *tp)
{
    size_t len;

    len = strlen(tp->buffer);
    /* remove the trailing newline */
    if (tp->buffer[len - 1] == '\n')
        tp->buffer[len - 1] = '\0';

    vprint("Unknown: %s\n", tp->buffer);

    return tap_default_unknown_callback(tp);
}

static int
version(tap_parser *tp, long tap_version)
{
    vprint("Version: %ld\n", tap_version);

    return tap_default_version_callback(tp, tap_version);
}

static int
comment(tap_parser *tp)
{
    size_t len;

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
    vprint("Bail out!");
    if (msg)
        vprint(" %s\n", msg);
    else
        vprint("\n");

    return tap_default_bailout_callback(tp, msg);
}

static int
pragma(tap_parser *tp, int state, char *pragma)
{
    vprint("Pragma: %c%s\n", (state) ? '+' : '-', pragma);

    return tap_default_pragma_callback(tp, state, pragma);
}

static int
plan(tap_parser *tp, long upper, char *skip)
{
    vprint("Plan: 1..%ld", upper);
    if (skip)
        vprint(" # skip %s\n", skip);
    else
        vprint("\n");

    return tap_default_plan_callback(tp, upper, skip);
}

static int
test(tap_parser *tp, tap_test_result *ttr)
{
    vprint("Test: %ld ", ttr->test_num);
    switch (ttr->type) {
    case TTT_OK:
        vprint("ok");
        break;
    case TTT_NOT_OK:
        vprint("not ok");
        break;
    case TTT_TODO:
        vprint("todo");
        break;
    case TTT_TODO_PASSED:
        vprint("ok todo");
        break;
    case TTT_SKIP:
        vprint("skip");
        break;
    case TTT_SKIP_FAILED:
        vprint("not ok skip");
        break;
    }

    if (ttr->reason) {
        if (ttr->directive)
            vprint(": %s (%s)\n", ttr->reason, ttr->directive);
        else
            vprint(": %s\n", ttr->reason);
    }
    else if (ttr->directive)
        vprint(" (%s)\n", ttr->directive);
    else
        vprint("\n");

    return tap_default_test_callback(tp, ttr);
}

/* vim: set ts=4 sw=4 sws=4 expandtab: */
