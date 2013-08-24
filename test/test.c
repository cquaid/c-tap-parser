#include <sys/types.h>

#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../tap_parser.h"

/* Callback Functions */
static int invalid(tap_parser *tp, const char *msg);
static int unknown(tap_parser *tp);
static int version(tap_parser *tp, long tap_version);
static int comment(tap_parser *tp);
static int bailout(tap_parser *tp, char *msg);
static int pragma(tap_parser *tp, int state, char *pragma);
static int plan(tap_parser *tp, long upper, char *skip);
static int test(tap_parser *tp, tap_test_result *ttr);

/* Helpers */
static pid_t exec_test(tap_parser *tp, const char *path);

int
main(int argc, char *argv[])
{
    pid_t pid;
    tap_parser tp;


    if (argc != 2) {
        fprintf(stderr, "usage: %s <filename>\n", argv[0]);
        return 1;
    }

    tap_parser_init(&tp, 512);

    pid = exec_test(&tp, argv[1]);
    printf("Child pid: %lu\n", (unsigned long)pid);

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

    if (tp.bailed)
        printf("Bailed out.\n");

    printf("\nFailed %ld of %ld tests\n", tp.actual_failed, tp.plan);

    tap_parser_fini(&tp);
    close(tp.fd);
    return 0;
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


static int
invalid(tap_parser *tp, const char *msg)
{
    fprintf(stderr, "Error: %s\n", msg);
    fflush(stderr);

    return tap_default_invalid_callback(tp, msg);
}

static int
unknown(tap_parser *tp)
{
    size_t len;

    len = strlen(tp->buffer);
    /* remove the trailing newline */
    if (tp->buffer[len - 1] == '\n')
        tp->buffer[len - 1] = '\0';

    fprintf(stderr, "Unknown: %s\n", tp->buffer);
    fflush(stderr);

    return tap_default_unknown_callback(tp);
}

static int
version(tap_parser *tp, long tap_version)
{
    printf("Version: %ld\n", tap_version);
    fflush(stdout);

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
    printf("Pragma: %c%s\n", (state) ? '+' : '-', pragma);
    fflush(stdout);

    return tap_default_pragma_callback(tp, state, pragma);
}

static int
plan(tap_parser *tp, long upper, char *skip)
{
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
