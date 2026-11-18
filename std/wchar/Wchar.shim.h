/*
 * Wchar.shim.h - mbstowcs / wcstombs behind std::wchar.
 *
 * The wide string is `wchar_t *` in C and `int *` at the CT boundary. This
 * translation unit refuses to build when the two widths differ, so the cast is
 * a checked platform assumption rather than an unchecked one.
 *
 * The two conversions run a counting pass with a NULL destination first. That
 * is the only portable way to tell "fits" from "truncated": the converting
 * call itself has no overflow indication. A buffer without room for the
 * terminator is -1 and is cleared, never partially filled.
 *
 * License: MIT (see LICENSE).
 */

/* The POSIX.1-2008 feature set: shims are the first headers in a generated
   consumer, so defining it here makes mkdir/readlink/telldir/fileno/setenv and
   friends visible under strict -std=c99 on glibc. */
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#ifndef CT_STD_WCHAR_WCHAR_SHIM_H
#define CT_STD_WCHAR_WCHAR_SHIM_H

#include <stddef.h>
#include <stdlib.h>
#include <wchar.h>

typedef char std_wchar_size_must_match_int[(sizeof(wchar_t) == sizeof(int)) ? 1 : -1];

static inline int std_wchar_wide_length(const int *ws)
{
    if (!ws) return -1;
    return (int)wcslen((const wchar_t *)ws);
}

static inline int std_wchar_mb_to_wide(const char *text, int *out, int cap)
{
    size_t need;

    if (!text || !out || cap <= 0) return -1;
    need = mbstowcs(NULL, text, 0);
    if (need == (size_t)-1 || (int)need + 1 > cap) {
        out[0] = 0;
        return -1;
    }
    mbstowcs((wchar_t *)out, text, (size_t)cap);
    return (int)need;
}

static inline int std_wchar_wide_to_mb(const int *ws, char *out, int cap)
{
    size_t need;

    if (!ws || !out || cap <= 0) return -1;
    need = wcstombs(NULL, (const wchar_t *)ws, 0);
    if (need == (size_t)-1 || (int)need + 1 > cap) {
        out[0] = 0;
        return -1;
    }
    wcstombs(out, (const wchar_t *)ws, (size_t)cap);
    return (int)need;
}

#endif
