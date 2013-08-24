#ifndef _H_TAP_CONSTANTS
#define _H_TAP_CONSTANTS

/* How long to wait for input in non-blocking context
 * on a file descriptor in seconds. */
#define DEFAULT_BLOCKING_TIME 20

/* Default buffer size for tap input */
#define DEFAULT_BUFFER_LEN 512

/* Current default TAP version */
#define DEFAULT_TAP_VERSION 12

/* Minimum supported TAP version (for the TAP version directive) */
#define MIN_TAP_VERSION 13
/* Maximum supported TAP version */
#define MAX_TAP_VERSION 13

#endif /* _H_TAP_CONSTANTS */

/* vim: set ts=4 sw=4 sts=4 expandtab: */
