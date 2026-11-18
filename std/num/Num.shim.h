/*
 * Num.shim.h - the C surface behind std::num's Int / Long / LLong / Char.
 *
 * The file must share the base name of `Num.m.ct` and sit beside it; the
 * compiler includes it in every consumer's generated `.c` and adds this
 * directory as an include root. Everything is `static inline`, so an unused
 * helper is dropped instead of triggering `-Wunused-function`.
 *
 * `to_str` writes into the caller's buffer and truncates silently.
 *
 * A negative argument to the ctype helpers is cast to `unsigned char`, which is
 * what the C library requires; that is the safe spelling compared with
 * forwarding the raw `int`/`char`.
 *
 * License: MIT (see LICENSE).
 */
/* The POSIX.1-2008 feature set: shims are the first headers in a generated
   consumer, so defining it here makes mkdir/readlink/telldir/fileno/setenv and
   friends visible under strict -std=c99 on glibc. */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#ifndef CT_STD_NUM_NUM_SHIM_H
#define CT_STD_NUM_NUM_SHIM_H

#include <ctype.h>
#include <stdio.h>

/* ---- arithmetic helpers ------------------------------------------------- */

static inline long long std_num_clamp(long long v, long long lo, long long hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

static inline long long std_num_max(long long a, long long b)
{
    return a > b ? a : b;
}

static inline long long std_num_min(long long a, long long b)
{
    return a < b ? a : b;
}

/* ---- stringification ---------------------------------------------------- */

/* Silently truncates to `cap - 1` characters; `out == 0` / `cap <= 0` leaves
   the buffer untouched and returns `out`. */
static inline char *std_num_to_str(long long v, char *out, int cap)
{
    if (out == 0 || cap <= 0) return out;
    snprintf(out, (size_t)cap, "%lld", v);
    return out;
}

/* 2-36, lower-case digits, leading `-` for negatives. Never truncates: when the
   sign and every digit do not fit in `cap`, nothing is written and NULL is
   returned. An out-of-range base yields the empty string. */
static inline char *std_num_to_base(long long v, int base, char *out, int cap)
{
    char tmp[70];
    int i = 0, j = 0, neg = 0;
    unsigned long long u;

    if (out == 0 || cap <= 0) return out;
    out[0] = 0;
    if (base < 2 || base > 36) return out;
    if (v < 0) {
        neg = 1;
        u = (unsigned long long)(-(v + 1)) + 1ULL;
    } else {
        u = (unsigned long long)v;
    }
    do {
        int d = (int)(u % (unsigned long long)base);
        tmp[i++] = (char)(d < 10 ? '0' + d : 'a' + d - 10);
        u /= (unsigned long long)base;
    } while (u != 0 && i < (int)sizeof tmp - 2);
    if (neg) tmp[i++] = '-';
    if (i + 1 > cap) return 0;
    while (i > 0) out[j++] = tmp[--i];
    out[j] = 0;
    return out;
}

/* ---- ctype, with the cast the C library requires ------------------------ */

static inline int std_num_char_upper(int v)
{
    return toupper((unsigned char)v);
}

static inline int std_num_char_lower(int v)
{
    return tolower((unsigned char)v);
}

static inline int std_num_is_digit(int v)
{
    return isdigit((unsigned char)v);
}

static inline int std_num_is_alpha(int v)
{
    return isalpha((unsigned char)v);
}

static inline int std_num_is_space(int v)
{
    return isspace((unsigned char)v);
}

static inline int std_num_is_upper(int v)
{
    return isupper((unsigned char)v);
}

static inline int std_num_is_lower(int v)
{
    return islower((unsigned char)v);
}

static inline int std_num_is_alnum(int v)
{
    return isalnum((unsigned char)v);
}

static inline int std_num_is_punct(int v)
{
    return ispunct((unsigned char)v);
}

static inline int std_num_is_print(int v)
{
    return isprint((unsigned char)v);
}

static inline int std_num_is_graph(int v)
{
    return isgraph((unsigned char)v);
}

static inline int std_num_is_cntrl(int v)
{
    return iscntrl((unsigned char)v);
}

static inline int std_num_is_blank(int v)
{
    return isblank((unsigned char)v);
}

static inline int std_num_is_xdigit(int v)
{
    return isxdigit((unsigned char)v);
}

#endif
