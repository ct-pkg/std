#ifndef STD_IO_STDOUT_SHIM_H
#define STD_IO_STDOUT_SHIM_H

#include <stdio.h>

/* A recipe may only name its own binding parameters, so `fflush(stdout)` cannot
   be written inline; the flush target lives here instead. */
static inline int std_io_flush(void) {
    return fflush(stdout);
}

#endif
