/* The POSIX.1-2008 feature set: shims are the first headers in a generated
   consumer, so defining it here makes mkdir/readlink/telldir/fileno/setenv and
   friends visible under strict -std=c99 on glibc. */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#ifndef STD_IO_STDOUT_SHIM_H
#define STD_IO_STDOUT_SHIM_H

#include <stdio.h>

/* A recipe may only name its own binding parameters, so `fflush(stdout)` cannot
   be written inline; the flush target lives here instead. */
static inline int std_io_flush(void) {
    return fflush(stdout);
}

/* fputs without a newline; `write_line` is `puts`, which appends one. */
static inline int std_io_write(const char *s) {
    if (!s) return -1;
    return fputs(s, stdout) < 0 ? -1 : 0;
}

static inline int std_io_write_line(const char *s) {
    if (!s) return -1;
    return puts(s) < 0 ? -1 : 0;
}

#endif
