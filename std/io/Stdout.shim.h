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
