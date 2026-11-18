/*
 * Errno.shim.h - the C surface behind std::errno.
 *
 * Every function is `static inline`, so it is compiled into the consumer's
 * translation unit and `errno` here is the caller's `errno`. That is the whole
 * reason the module is a mapping unit: a value cached inside a separate
 * translation unit would read 0 from the caller's side.
 *
 * `std_errno_message` copies instead of returning `strerror`'s pointer. The
 * pointer is borrowed (a static buffer shared by every caller) and the package
 * does not expose borrowed libc storage; `cap` includes the terminating NUL.
 *
 * License: MIT (see LICENSE).
 */

/* The POSIX.1-2008 feature set: shims are the first headers in a generated
   consumer, so defining it here makes mkdir/readlink/telldir/fileno/setenv and
   friends visible under strict -std=c99 on glibc. */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#ifndef CT_STD_ERRNO_ERRNO_SHIM_H
#define CT_STD_ERRNO_ERRNO_SHIM_H

#include <errno.h>
#include <string.h>

static inline int std_errno_get_last(void)
{
    return errno;
}

/* Test hook and helper for shims that need to put back a captured value. */
static inline int std_errno_set_last(int code)
{
    errno = code;
    return 0;
}

/* Copy `strerror(code)` into `out`. Returns the byte count, or -1 when the
   buffer cannot hold the message plus the terminator. */
static inline int std_errno_message(int code, char *out, int cap)
{
    const char *msg;
    size_t len;

    if (!out || cap <= 0) return -1;
    msg = strerror(code);
    if (!msg) {
        if (cap < 1) return -1;
        out[0] = 0;
        return 0;
    }
    len = strlen(msg);
    if ((int)len + 1 > cap) {
        out[0] = 0;
        return -1;
    }
    memcpy(out, msg, len + 1);
    return (int)len;
}

#endif
