#ifndef _H_TEST_LOG
#define _H_TEST_LOG

extern int log_open(const char *file, int append);
extern void log_close(void);

extern void log_write(const char *fmt, ...);
extern void log_writeln(const char *str);

#endif /* _H_TEST_LOG */
