#ifndef _H_TEST_RESULTS
#define _H_TEST_RESULTS

#include "tap_parser.h"

struct _ttr_node {
    char *file;  /* filename of the test */
    char *path;  /* path to the test */
    int aborted; /* did we abort? */
    int status;  /* status after running the test */
    int child_status; /* child status from waitpid */
    tap_results *tr;
    struct _ttr_node *next;
};
typedef struct _ttr_node ttr_node;

typedef struct {
    long total_tests_run;
    long total_failed;
    long total_skipped;
    long total_todo;
    long total_aborted;
    long total_parse_errors;

    tap_parser *tp;
    ttr_node *root;
    ttr_node *top;
} test_results;

extern ttr_node* ttr_node_new(void);
extern void ttr_node_delete(ttr_node *n);

extern void test_results_init(test_results *tsr);
extern void test_results_fini(test_results *tsr);
extern void test_results_push(test_results *tsr, ttr_node *n);

#endif /* _H_TEST_RESULTS */
/* vim: set ts=4 sw=4 sws=4 expandtab: */
