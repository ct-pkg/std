/*
 * String.shim.h - the C surface behind std::str::String.
 *
 * `String.m.ct` maps the OO spellings (`s.len()`, `s.to_lower(out, cap)`) onto the
 * `std_str_*` functions below. A mapping unit cannot declare functions or carry
 * `#ct-raw` code (a `.m.ct` prologue allows only `#include` / `#define`), so the
 * definitions live here: the compiler auto-detects `<base>.shim.h` next to a
 * `.m.ct`, includes it in every consumer's generated `.c` before the call
 * sites, and adds this directory as an include root.
 *
 * There is no `.shim.c`: the whole module is two files. That is the floor for a
 * mapping unit, and it costs one copy of this code per consumer -- header-only
 * is a compile-time trade, not a free lunch. Keeping the functions `static
 * inline` is what makes that copy quiet: a plain `static` helper that a given
 * consumer never calls triggers `-Wunused-function`, while `static inline` does
 * not (and an unused `inline` is simply dropped by the compiler).
 *
 * Nothing allocates here: an output buffer and its capacity are always passed
 * in, and a buffer that is too small is a failure (-1), never a silent
 * truncation. A missing needle or index is -1, a boolean is 0 or 1.
 *
 * These are the same C symbols the module always exported; adding an ability
 * means adding it here, never going back to a `.h.ct`.
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

#ifndef CT_STD_STR_STRING_SHIM_H
#define CT_STD_STR_STRING_SHIM_H

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <errno.h>

/* Length in bytes (0 when `s` is NULL). */
static inline int std_str_len(const char *s)
{
    int n;

    if (!s) return 0;
    for (n = 0; s[n]; n++) {
    }
    return n;
}

/* Byte offset of `needle` in `s`, or -1 when absent. */
static inline int std_str_find(const char *s, const char *needle)
{
    const char *p;

    if (!s || !needle) return -1;
    p = strstr(s, needle);
    if (!p) return -1;
    return (int)(p - s);
}

/* Byte offset of `c` in `s`, or -1 when absent. */
static inline int std_str_find_char(const char *s, int c)
{
    const char *p;

    if (!s) return -1;
    p = strchr(s, (char)c);
    if (!p) return -1;
    return (int)(p - s);
}

/* 1 when `s` starts with `prefix`, 0 otherwise. */
static inline int std_str_starts_with(const char *s, const char *prefix)
{
    int i;

    if (!s || !prefix) return 0;
    for (i = 0; prefix[i]; i++) {
        if (s[i] != prefix[i]) return 0;
    }
    return 1;
}

/* 1 when `s` ends with `suffix`, 0 otherwise. */
static inline int std_str_ends_with(const char *s, const char *suffix)
{
    int n;
    int m;
    int i;

    if (!s || !suffix) return 0;
    n = std_str_len(s);
    m = std_str_len(suffix);
    if (m > n) return 0;
    for (i = 0; i < m; i++) {
        if (s[n - m + i] != suffix[i]) return 0;
    }
    return 1;
}

/* Copy `s` into `out`; -1 when the buffer cannot hold it plus the NUL. */
static inline int std_str_copy(const char *s, char *out, int cap)
{
    int n;
    int i;

    if (!s || !out || cap <= 0) return -1;
    n = std_str_len(s);
    if (n + 1 > cap) return -1;
    for (i = 0; i < n; i++) {
        out[i] = s[i];
    }
    out[n] = 0;
    return 0;
}

/* Copy the half-open byte range [start, end) of `s` into `out`; -1 when the
   range is out of bounds or the buffer is too small. */
static inline int std_str_slice(const char *s, int start, int end, char *out, int cap)
{
    int n;
    int i;

    if (!s || !out || cap <= 0) return -1;
    n = std_str_len(s);
    if (start < 0 || end < start || end > n) return -1;
    if (end - start + 1 > cap) return -1;
    for (i = start; i < end; i++) {
        out[i - start] = s[i];
    }
    out[end - start] = 0;
    return 0;
}

/* Join `a` and `b` into `out`; -1 when the buffer is too small. */
static inline int std_str_concat(const char *a, const char *b, char *out, int cap)
{
    int na;
    int nb;
    int i;

    if (!a || !b || !out || cap <= 0) return -1;
    na = std_str_len(a);
    nb = std_str_len(b);
    if (na + nb + 1 > cap) return -1;
    for (i = 0; i < na; i++) {
        out[i] = a[i];
    }
    for (i = 0; i < nb; i++) {
        out[na + i] = b[i];
    }
    out[na + nb] = 0;
    return 0;
}

/* Lower-case `s` into `out`; -1 when a pointer is missing or `out` cannot hold
   `s` plus the NUL. The receiver is only read. */
static inline int std_str_lower(const char *s, char *out, int cap)
{
    int n;
    int i;

    if (!s || !out || cap <= 0) return -1;
    n = std_str_len(s);
    if (n + 1 > cap) return -1;
    for (i = 0; i < n; i++) {
        out[i] = (s[i] >= 'A' && s[i] <= 'Z') ? (char)(s[i] - 'A' + 'a') : s[i];
    }
    out[n] = 0;
    return 0;
}

/* Upper-case `s` into `out`; -1 when a pointer is missing or `out` cannot hold
   `s` plus the NUL. The receiver is only read. */
static inline int std_str_upper(const char *s, char *out, int cap)
{
    int n;
    int i;

    if (!s || !out || cap <= 0) return -1;
    n = std_str_len(s);
    if (n + 1 > cap) return -1;
    for (i = 0; i < n; i++) {
        out[i] = (s[i] >= 'a' && s[i] <= 'z') ? (char)(s[i] - 'a' + 'A') : s[i];
    }
    out[n] = 0;
    return 0;
}

/* Lexicographic comparison, the `strcmp` shape: negative, zero or positive. */
static inline int std_str_compare(const char *a, const char *b)
{
    int i;

    if (!a) a = "";
    if (!b) b = "";
    for (i = 0; a[i] && b[i]; i++) {
        if (a[i] != b[i]) return (a[i] < b[i]) ? -1 : 1;
    }
    if (a[i] == b[i]) return 0;
    return (a[i] == 0) ? -1 : 1;
}

/* The `memcpy` shape behind `dst.copy_from(src, n)`: exactly `n` bytes.
   Source and destination must not overlap; use `move_from` when they may. */
static inline void std_str_copy_from(char *dst, const char *src, int n)
{
    if (!src || !dst || n <= 0) return;
    memcpy(dst, src, (size_t)n);
}

/* The `memmove` shape behind `dst.move_from(src, n)`: exactly `n` bytes,
   overlapping ranges allowed. */
static inline void std_str_move_from(char *dst, const char *src, int n)
{
    if (!src || !dst || n <= 0) return;
    memmove(dst, src, (size_t)n);
}

/* The `memset` shape behind `dst.fill(c, n)`: exactly `n` bytes set to `c`. */
static inline void std_str_fill(char *dst, int c, int n)
{
    if (!dst || n <= 0) return;
    memset(dst, c, (size_t)n);
}

/* The `memcmp` shape behind `a.compare_with(b, n)`: three-state, stops after
   `n` bytes. */
static inline int std_str_compare_with(const char *a, const char *b, int n)
{
    if (!a || !b || n <= 0) return 0;
    return memcmp(a, b, (size_t)n);
}

/* The `memchr` shape behind `s.find_byte(c, n)`: pointer to the byte or NULL.
   No NUL is implied -- `n` is the whole search window. */
static inline char *std_str_find_byte(const char *s, int c, int n)
{
    if (!s || n <= 0) return 0;
    return (char *)memchr(s, c, (size_t)n);
}

/* The `strncmp` shape behind `a.compare_prefix(b, n)`: three-state, stops at a
   NUL in either string or after `n` bytes. */
static inline int std_str_compare_prefix(const char *a, const char *b, int n)
{
    if (!a || !b || n <= 0) return 0;
    return strncmp(a, b, (size_t)n);
}

/* Split the receiver on every byte that appears in `sep` (an empty or missing
   `sep` keeps the whole string as one field). Fields are copied NUL-terminated
   into `out`, one slot of `slot_size` bytes per field. Empty fields are kept:
   `"a,,b"` yields three fields and a trailing separator yields a trailing empty
   field. Returns the required field count; when `cap` or `slot_size` cannot
   hold every field, nothing is written. The receiver is never modified. */
static inline int std_str_split_into(const char *s, const char *sep,
                                     char *out, int cap, int slot_size)
{
    int n;
    int fields = 1;
    int maxlen = 0;
    int curlen = 0;
    int i;
    int f = 0;
    int pos = 0;

    if (!s) return 0;
    n = std_str_len(s);
    for (i = 0; i < n; i++) {
        if (sep && sep[0] && strchr(sep, s[i])) {
            if (curlen > maxlen) maxlen = curlen;
            curlen = 0;
            fields++;
        } else {
            curlen++;
        }
    }
    if (curlen > maxlen) maxlen = curlen;
    if (!out || slot_size <= 0 || cap < fields || maxlen + 1 > slot_size) {
        return fields;
    }
    for (i = 0; i < n; i++) {
        if (sep && sep[0] && strchr(sep, s[i])) {
            out[f * slot_size + pos] = 0;
            f++;
            pos = 0;
        } else {
            out[f * slot_size + pos] = s[i];
            pos++;
        }
    }
    out[f * slot_size + pos] = 0;
    return fields;
}

/* The C parser behind `s.to_int()`: 0 for a missing receiver, otherwise exactly
   `atoi` (no error report; overflow is the C library's undefined behaviour). */
static inline int std_str_to_int(const char *s)
{
    if (!s) return 0;
    return atoi(s);
}

/* The C parser behind `s.to_double()`: `atof`, 0 for a missing receiver. */
static inline double std_str_to_double(const char *s)
{
    if (!s) return 0.0;
    return atof(s);
}


/* Parsing with an error report. Each returns 0 and writes `out` on success;
   -1 on a NULL/empty receiver, a missing out pointer, trailing non-blank text,
   a leading '-' for the unsigned form, or a range error (ERANGE from the
   strto* family). Leading and trailing whitespace are allowed; the value is
   not written on failure. Unlike `to_int`/`to_double`, these never silently
   return 0. */
static inline int std_str_only_blank_after(const char *end)
{
    if (!end) return 0;
    while (*end == ' ' || *end == '\t' || *end == '\n' ||
           *end == '\r' || *end == '\f' || *end == '\v') {
        end++;
    }
    return *end == 0;
}

static inline int std_str_has_leading_minus(const char *s)
{
    while (*s == ' ' || *s == '\t' || *s == '\n' ||
           *s == '\r' || *s == '\f' || *s == '\v') {
        s++;
    }
    return *s == '-';
}
static inline int std_str_parse_long(const char *s, long *out)
{
    char *end;
    long value;

    if (!s || !*s || !out) return -1;
    errno = 0;
    value = strtol(s, &end, 10);
    if (errno == ERANGE || end == s || !std_str_only_blank_after(end)) return -1;
    *out = value;
    return 0;
}

static inline int std_str_parse_long_long(const char *s, long long *out)
{
    char *end;
    long long value;

    if (!s || !*s || !out) return -1;
    errno = 0;
    value = strtoll(s, &end, 10);
    if (errno == ERANGE || end == s || !std_str_only_blank_after(end)) return -1;
    *out = value;
    return 0;
}

static inline int std_str_parse_unsigned_long(const char *s, unsigned long *out)
{
    char *end;
    unsigned long value;

    if (!s || !*s || !out || std_str_has_leading_minus(s)) return -1;
    errno = 0;
    value = strtoul(s, &end, 10);
    if (errno == ERANGE || end == s || !std_str_only_blank_after(end)) return -1;
    *out = value;
    return 0;
}

static inline int std_str_parse_real(const char *s, double *out)
{
    char *end;
    double value;

    if (!s || !*s || !out) return -1;
    errno = 0;
    value = strtod(s, &end);
    if (errno == ERANGE || end == s || !std_str_only_blank_after(end)) return -1;
    *out = value;
    return 0;
}


/* Byte offset of the last `c`, or -1 when absent (strrchr). */
static inline int std_str_reverse_find_char(const char *s, int c)
{
    const char *p;

    if (!s) return -1;
    p = strrchr(s, c);
    if (!p) return -1;
    return (int)(p - s);
}

/* ASCII case-insensitive compare; NULL is treated as the empty string, like
   `compare`. The sign of the result is all that is specified. */
static inline int std_str_compare_ignore_case(const char *a, const char *b)
{
    if (!a) a = "";
    if (!b) b = "";
    return strcasecmp(a, b);
}

#endif
