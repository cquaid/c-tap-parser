#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_utils.h"
#include "test_results.h"
#include "tap_parser.h"

ttr_node*
ttr_node_new(void)
{
    ttr_node *ret = (ttr_node *)malloc(sizeof(*ret));
    if (ret == NULL)
        die(errno, "malloc(ttr_node)");

    memset(ret, 0, sizeof(*ret));

    return ret;
}

void
ttr_node_delete(ttr_node *n)
{
    if (n->file)
        free(n->file);

    if (n->path)
        free(n->path);

    if (n->tr)
        free(n->tr);

    free(n);
}

static inline ttr_node*
ttr_node_walk(ttr_node *n)
{
    ttr_node *cur;

    cur = n;
    n = n->next;
    while (n != NULL) {
        cur = n;
        n = n->next;
    }

    return cur;
}


void
test_results_init(test_results *tsr)
{
    memset(tsr, 0, sizeof(*tsr));
}

void
test_results_fini(test_results *tsr)
{
    if (tsr->root != NULL) {
        ttr_node *next, *cur;
        cur = tsr->root;
        next = cur->next;
        while (next != NULL) {
            ttr_node_delete(cur);
            cur = next;
            next = cur->next;
        }
        ttr_node_delete(cur);
    }
}

void
test_results_push(test_results *tsr, ttr_node *n)
{
    if (tsr->root == NULL) {
        tsr->root = n;
        tsr->top = ttr_node_walk(n);
        return;
    }

    if (tsr->top != NULL) {
        tsr->top->next = n;
        tsr->top = ttr_node_walk(n);
        return;
    }

    /* If we here someone was stupid... (probably me) */
    tsr->top = ttr_node_walk(tsr->root);
    tsr->top-> next = n;
    tsr->top = ttr_node_walk(n);
}


/* vim: set ts=4 sw=4 sws=4 expandtab: */
