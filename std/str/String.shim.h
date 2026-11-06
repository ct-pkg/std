/*
 * String.shim.h - the C surface behind std::str::String.
 *
 * `String.m.ct` maps the OO spellings (`s.len()`, `s.toLower()`) onto the
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
 * License: same as the rest of the compiler (see LICENSE / LICENSE.EXCEPTION).
 */
#ifndef CT_STD_STR_STRING_SHIM_H
#define CT_STD_STR_STRING_SHIM_H

#include <string.h>

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

/* Lower-case `s` in place; -1 when `s` is NULL. */
static inline int std_str_lower(char *s)
{
    int i;

    if (!s) return -1;
    for (i = 0; s[i]; i++) {
        if (s[i] >= 'A' && s[i] <= 'Z') s[i] = (char)(s[i] - 'A' + 'a');
    }
    return 0;
}

/* Upper-case `s` in place; -1 when `s` is NULL. */
static inline int std_str_upper(char *s)
{
    int i;

    if (!s) return -1;
    for (i = 0; s[i]; i++) {
        if (s[i] >= 'a' && s[i] <= 'z') s[i] = (char)(s[i] - 'a' + 'A');
    }
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

#endif
