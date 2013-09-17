#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "tap_parser.h"

#include "test_log.h"
#include "test_utils.h"
#include "test_results.h"
#include "test_callbacks.h"

#define TP_BUFFER_SZ 512

/* Types */

enum analyze_ret {
    AR_SUCCESS = 0, /* All tests passed     */
    AR_ABORTED = 1, /* Something went wrong */
    AR_FAILED  = 2  /* Some tests failed    */
};

/* Globals */
static int debug = 0;
static int capture_stderr = 0;

/* Non-static, used by test_callbacks */
int verbosity = 0;

static int child_exited = 0;
static int child_status = 0;
static pid_t current_child = -1;

static const char *build = NULL;
static const char *source = NULL;

/* Helpers */
#if 0
static void dump_results_array(const tap_results *tr);
static void dump_tap_stats(const tap_parser *tp);
static int analyze_results(const tap_parser *tp);
#endif
static pid_t exec_test(tap_parser *tp, const char *path);
static void unset_envars(void);
static void handle_sigchld(int sig);
static inline int init_parser(tap_parser *tp);
static inline char* find_test(const char *base);
static int run_list(tap_parser *tp, const char *list);
static int run_single(tap_parser *tp, const char *test);
static inline void cook_test_results(test_results *tsr, ttr_node *node, tap_parser *tp);

static void
usage(FILE *file, const char *name)
{
    fprintf(file, "usage: %s [options] filename\n", name);
    fprintf(file, "       %s [options] -l filename\n", name);
    fprintf(file, " -h            display this message\n");
    fprintf(file, " -v            increase verbose output\n");
    fprintf(file, " -d            debug information, implies -vv\n");
    fprintf(file, " -L file       log the test output to a file\n");
    fprintf(file, " -a            open the log with append\n");
    fprintf(file, " -l            filename is a list of tests to run\n");
    fprintf(file, " -s src_dir    test source directory\n");
    fprintf(file, " -b build_dir  test build directory\n");
    fprintf(file, " -e            capture test stderr\n");
    fflush(file);
}

int
main(int argc, char *argv[])
{
    int ret;
    int opt;
    int list = 0;
    int append = 0;
    tap_parser tp;

    const char *name;
    const char *logname = NULL;
    const char *filename = NULL;

    name = argv[0];

    while ((opt = getopt(argc, argv, "vhdaL:ls:b:e")) != EOF) {
        switch (opt) {
        case 'v':
            verbosity++;
            break;
        case 'd':
            debug = 1;
            if (verbosity < 2)
                verbosity = 2;
            break;
        case 'a':
            append = 1;
            break;
        case 'L':
            logname = optarg;
            break;
        case 'l':
            list = 1;
            break;
        case 's':
            source = optarg;
            break;
        case 'b':
            build = optarg;
            break;
        case 'e':
            capture_stderr = 1;
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
        fprintf(stderr, "Missing filename!\n");
        usage(stderr, name);
        exit(EXIT_FAILURE);
    }

    filename = argv[0];

    if (logname != NULL) {
        if (log_open(logname, append))
            die(errno, "Failed to open %s", logname);
    }

    if (source != NULL) {
        /* Set SOURCE and TAP_SOURCE */
        ret = setenv("SOURCE", source, 1);
        if (ret != 0)
            die(errno, "setenv(SOURCE)");

        ret = setenv("TAP_SOURCE", source, 1);
        if (ret != 0)
            die(errno, "setenv(TAP_SOURCE)");
    }


    /* The parser has to be initialized before a
     * tap_parser_reset call */
    ret = tap_parser_init(&tp, TP_BUFFER_SZ);
    if (ret != 0)
        die(ret, "tap_parser_init()");

    /* Handle SIGCHLD */
    signal(SIGCHLD, &handle_sigchld);

    /* Cleanup the environment at exit */
    atexit(unset_envars);

    if (source != NULL) {
        /* Set SOURCE and TAP_SOURCE */
        ret = setenv("SOURCE", source, 1);
        if (ret != 0)
            die(errno, "setenv(SOURCE)");

        ret = setenv("TAP_SOURCE", source, 1);
        if (ret != 0)
            die(errno, "setenv(TAP_SOURCE)");
    }

    if (build != NULL) {
        /* Set BUILD and TAP_BUILD */
        ret = setenv("BUILD", build, 1);
        if (ret != 0)
            die(errno, "setenv(BUILD)");

        ret = setenv("TAP_BUILD", build, 1);
        if (ret != 0)
            die(errno, "setenv(TAP_BUILD)");
    }

    if (list)
        ret = run_list(&tp, filename);
    else
        ret = run_single(&tp, filename);

    tap_parser_fini(&tp);

    return ret;
#if 0
    if (verbosity >= 3) {
        printf("Child pid: %lu\n", (unsigned long)current_child);
        fflush(stdout);
    }

    if (debug)
        dump_tap_stats(&tp);

    ret = analyze_results(&tp);

    if (verbosity >= 2) {
        tap_results *tr = tap_parser_steal_results(&tp);
        dump_results_array(tr);
        tap_results_fini(tr);
    }
#endif
}

#if 0
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
    for (i = 1; i < tp->tr->results_len; ++i) {
        /* Skip anything that's not invalid */
        if (tp->tr->results[i] != TTT_INVALID)
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
    for (i = 1; i < tp->tr->results_len; ++i) {
        /* Skip anything that's not a failure */
        if (tp->tr->results[i] != TTT_NOT_OK)
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
dump_results_array(const tap_results *tr)
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


    if (tr == NULL)
        return;

    if (tr->results == NULL || tr->results_len == 0)
        return;

    passed = failed = 0;
    todo = skipped = 0;
    dubious = missing = 0;
    for (i = 1; i < tr->results_len; ++ i) {
        switch (tr->results[i]) {
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
        for (i = 1; i < tr->results_len; ++i) {
            if (tr->results[i] == normal[t].type)
                printf("%ld, ", i);
        }
        putchar('\n');
    }

    if (dubious) {
        printf("dubious: ");
        for (i = 1; i < tr->results_len; ++i) {
            if (tr->results[i] == TTT_TODO_PASSED ||
                tr->results[i] == TTT_SKIP_FAILED ) {
                printf("%ld, ", i);
            }
        }
        putchar('\n');
    }

    if (missing) {
        printf("missing: ");
        for (i = 1; i < tr->results_len; ++i) {
            if (tr->results[i] == TTT_INVALID)
                printf("%ld, ", i);
        }
        putchar('\n');
    }

    fflush(stdout);

#undef normal_len
}
#endif

static pid_t
exec_test(tap_parser *tp, const char *path)
{
    pid_t child;
    int pipes[2];

#define READ_PIPE  0
#define WRITE_PIPE 1
    if (pipe(pipes) == -1)
        die(errno, "pipe()");

    child = fork();
    if (child == (pid_t)-1)
        die(errno, "fork()");

    if (child == 0) {
        /* child proc */

        if (capture_stderr) {
            if (dup2(pipes[WRITE_PIPE], STDERR_FILENO) == -1)
                exit(EXIT_FAILURE);
        }
        else {
            int fd = open("/dev/null", O_WRONLY);
            if (fd == -1)
                exit(EXIT_FAILURE);
            if (dup2(fd, STDERR_FILENO) == -1)
                exit(EXIT_FAILURE);
            close(fd);
        }

        if (dup2(pipes[WRITE_PIPE], STDOUT_FILENO) == -1)
            exit(EXIT_FAILURE);

        close(pipes[READ_PIPE]);
        close(pipes[WRITE_PIPE]);

        /* log gets cloned for the child, close it */
        log_close();

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

/* Signal Handler */
static void
handle_sigchld(int sig)
{
    pid_t child;

    /* sig unused */
    (void)sig;

    child = waitpid(current_child, &child_status, WNOHANG);
    if (child > 0 && WIFEXITED(child_status))
        child_exited = 1;
}

/* registered atexit to cleanup the environment */
static void
unset_envars(void)
{
    unsetenv("SOURCE");
    unsetenv("TAP_SOURCE");
    unsetenv("BUILD");
    unsetenv("TAP_BUILD");
}

/* Initialize all the parser and set the callbacks. */
static inline int
init_parser(tap_parser *tp)
{
    int ret;

    /* reset so we don't wipe out the buffer */
    ret = tap_parser_reset(tp);
    if (ret != 0)
        return ret;

    /* Set the callbacks */
    tap_parser_set_test_callback(tp, test_cb);
    tap_parser_set_plan_callback(tp, plan_cb);
    tap_parser_set_pragma_callback(tp, pragma_cb);
    tap_parser_set_bailout_callback(tp, bailout_cb);
    tap_parser_set_comment_callback(tp, comment_cb);
    tap_parser_set_version_callback(tp, version_cb);
    tap_parser_set_unknown_callback(tp, unknown_cb);
    tap_parser_set_invalid_callback(tp, invalid_cb);
    tap_parser_set_preparse_callback(tp, preparse_cb);

    return 0;
}

static inline char*
find_test(const char *base)
{
    char *ret;

    const char **dptr;
    const char *dirs[4];

    size_t pos;
    size_t len;
    size_t dir_len;
    size_t base_len;

    struct stat sb;

    dirs[0] = ".";
    dirs[1] = build;
    dirs[2] = source;
    dirs[3] = NULL;

    ret = NULL;
    dir_len = 0;
    base_len = strlen(base);

    for (dptr = &dirs[0]; dptr != NULL; ++dptr) {
        len = strlen(*dptr);

        if (ret != NULL) {
            if (dir_len < len) {
                char *n;
                n = (char *)realloc(ret, len + base_len + 4);
                if (n == NULL) {
                    free(ret);
                    die(errno, "realloc()");
                }

                ret = n;
            }

            dir_len = len;
        }
        else {
            ret = (char *)malloc(len + base_len + 4);
            if (ret == NULL)
                die(errno, "malloc()");
        }

        dir_len = len;

        strncpy(ret, *dptr, dir_len);
        ret[dir_len] = '/';
        pos = dir_len + 1;

        strncpy(&ret[pos], base, base_len);
        pos += base_len;
        ret[pos + 1] = 't';
        ret[pos + 2] = '\0';

        /* First try -t */
        ret[pos] = '-';
        if (stat(ret, &sb) != -1) {
            if (!S_ISREG(sb.st_mode))
                die(0, "%s is not a regular file!\n", ret);
            return ret;
        }

        /* Next is .t */
        ret[pos] = '.';
        if (stat(ret, &sb) != -1) {
            if (!S_ISREG(sb.st_mode))
                die(0, "%s is not a regular file!\n");
            return ret;
        }
    }

    free(ret);
    return NULL;
}

static inline void
make_test_list(test_results *tsr, const char *list)
{
    FILE *file;

    size_t line;
    size_t length;

    char *test;
    char buffer[TP_BUFFER_SZ];

    ttr_node *node;

    file = fopen(list, "r");
    if (file == NULL)
        die(errno, "Cannot open list %s", list);

    line = 0;
    while (fgets(buffer, TP_BUFFER_SZ-1, file)) {
        ++line;

        /* Skip comments */
        if (buffer[0] == '#')
            continue;

        length = strlen(buffer) - 1;
        if (buffer[length] != '\n') {
            die(0, "%s: %lu: line too long\n",
                list, (unsigned long)line);
        }

        buffer[length] = '\0';

        test = find_test(buffer);
        if (test == NULL)
            die(0, "Failed to find test: %s\n", buffer);

        /* Set up the new node */
        node = ttr_node_new();
        node->path = test;
        node->file = strdup(buffer);
        if (node->file == NULL)
            die(errno, "strdup(test_name)");

        test_results_push(tsr, node);
    }

    fclose(file);
}

static int
run_list(tap_parser *tp, const char *list)
{
    size_t length;
    size_t longest;

    ttr_node *node;
    test_results tsr;

    /* Initialize the test results */
    test_results_init(&tsr);

    /* Grap the test list */
    make_test_list(&tsr, list);

    /* Find the longest test name */
    longest = 0;
    node = tsr.root;
    while (node != NULL) {
        length = strlen(node->file);
        if (length > longest)
            longest = length;
        node = node->next;
    }

    /* Run the tests */
    node = tsr.root;
    while (node != NULL) {
        if (verbosity >= 1) {
            printf("%s...", node->file);
            length = longest - strlen(node->file);
            while (length--)
                putchar('.');
        }

        /* Run the test */
        node->status = run_single(tp, node->path);

        /* Detatch and store off the test results */
        node->tr = tap_parser_steal_results(tp);
        node->child_status = child_status;

        cook_test_results(&tsr, node, tp);
        fflush(stdout);

        node = node->next;
    }

    /* Cleanup test results */
    test_results_fini(&tsr);

    return 0;
}

static int
run_single(tap_parser *tp, const char *test)
{
    int ret;

    ret = init_parser(tp);
    if (ret != 0)
        die(ret, "tap_parser_reset()");

    child_exited = 0;
    child_status = 0;
    current_child = -1;

    /* Kick off the test */
    current_child = exec_test(tp, test);

    /* Loop over all output */
    while (tap_parser_next(tp) == 0)
        ;

    if (!child_exited) {
        /* Give the child some time, then kill it! */
        usleep(10);
        if (!child_exited) {
            if (verbosity >= 2) {
                fprintf(stderr, "Killing child (%lu)\n",
                        (unsigned long)current_child);
                fflush(stderr);
            }
            kill(current_child, SIGKILL);
        }
    }

    if (WIFEXITED(child_status))
        ret = WEXITSTATUS(child_status);
    else if (WIFSIGNALED(child_status))
        ret = -WTERMSIG(child_status);

    /* Close the fd */
    close(tp->fd);

    if (ret != 0)
        return ret;

    return !!tp->failed;
}

static inline void
cook_test_results(test_results *tsr, ttr_node *node, tap_parser *tp)
{
    int reported = 0;

    tsr->total_tests_run += tp->tests_run;
    tsr->total_failed += tp->failed;
    tsr->total_skipped += tp->skipped;
    tsr->total_todo += tp->todo;
    tsr->total_parse_errors += tp->parse_errors;

    /* XXX: This function needs to dump test results each pass */
    if (tp->bailed) {
        if (verbosity >= 1) {
            if (tp->bailed_reason == NULL)
                printf("bailed\n");
            else
                printf("bailed (%s)\n", tp->bailed_reason);
            reported = 1;
        }
        node->aborted = 1;
    }
    else if (tp->plan == -1) {
        if (verbosity >= 1) {
            printf("aborted (No Plan)\n");
            reported = 1;
        }
        node->aborted = 1;
    }
    else if (tp->tests_run > tp->plan) {
        if (verbosity >= 1) {
            printf("aborted (Extra Tests)\n");
            reported = 1;
        }
        node->aborted = 1;
    }
    else if (node->status < 0) {
        if (verbosity >= 1) {
            printf("aborted (Killed by signal %d)\n", node->status);
            reported = 1;
        }
        node->aborted = 1;
    }

    tsr->total_aborted += node->aborted;

    if (verbosity >= 1 && !reported) {
        if (tp->skip_all) {
            if (tp->skip_all_reason == NULL)
                printf("skipped\n");
            else
                printf("skipped (%s)\n", tp->skip_all_reason);
            return;
        }

        if (tp->failed)
            printf("not ok\n");
        else
            printf("ok\n");
    }

}

/* vim: set ts=4 sw=4 sws=4 expandtab: */
