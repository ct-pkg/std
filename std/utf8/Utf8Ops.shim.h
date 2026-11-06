/*
 * Utf8Ops.shim.h - the C surface behind std::utf8::Utf8Ops.
 *
 * `Utf8Ops.m.ct` maps the OO spellings (`s.decode(&cp)`, `s.codepointCount()`,
 * `buf.encode(cp, cap)`) onto the `std_utf8_*` functions below. A mapping unit
 * cannot declare functions or carry `#ct-raw` code (a `.m.ct` prologue allows
 * only `#include` / `#define`), so the definitions live here: the compiler
 * auto-detects `<base>.shim.h` next to a `.m.ct`, includes it in every
 * consumer's generated `.c` before the call sites, and adds this directory as
 * an include root.
 *
 * There is no `.shim.c`: the whole module is two files. That is the floor for a
 * mapping unit, and it costs one copy of this code per consumer -- header-only
 * is a compile-time trade, not a free lunch. Keeping the functions `static
 * inline` is what makes that copy quiet: a plain `static` helper that a given
 * consumer never calls triggers `-Wunused-function`, while `static inline` does
 * not (and an unused `inline` is simply dropped by the compiler).
 *
 * Nothing allocates here: `encode` writes into the caller's buffer and reports
 * -1 when it does not fit, and `decode` reports the byte count it consumed.
 *
 * Strict means a decoder rejects everything that is not a well-formed sequence:
 * a bad continuation byte, an overlong encoding, a surrogate and anything past
 * U+10FFFF. A decoder that accepted those would let two different byte strings
 * mean the same text, which is exactly what the standard forbids.
 *
 * These are the same C symbols the module always exported, minus `length`:
 * `std_utf8_length` is now `std_utf8_codepoint_count`, because `String.len()`
 * already means bytes and one word cannot mean both.
 *
 * License: same as the rest of the compiler (see LICENSE / LICENSE.EXCEPTION).
 */
#ifndef CT_STD_UTF8_UTF8OPS_SHIM_H
#define CT_STD_UTF8_UTF8OPS_SHIM_H

/* Decode the first code point of `s` into `cp`. Returns the number of bytes
   consumed (1..4), or -1 when `s` does not start with a valid sequence. */
static inline int std_utf8_decode(const char *s, int *cp)
{
    unsigned char c;
    unsigned int v;
    int n;
    int i;

    if (!s || !s[0]) return -1;
    c = (unsigned char)s[0];
    if (c < 0x80) {
        if (cp) *cp = (int)c;
        return 1;
    }
    if ((c & 0xE0) == 0xC0) {
        n = 2;
        v = c & 0x1F;
    } else if ((c & 0xF0) == 0xE0) {
        n = 3;
        v = c & 0x0F;
    } else if ((c & 0xF8) == 0xF0) {
        n = 4;
        v = c & 0x07;
    } else {
        return -1;
    }
    for (i = 1; i < n; i++) {
        unsigned char x = (unsigned char)s[i];
        if ((x & 0xC0) != 0x80) return -1;
        v = (v << 6) | (x & 0x3F);
    }
    if (n == 2 && v < 0x80) return -1;
    if (n == 3 && (v < 0x800 || (v >= 0xD800 && v <= 0xDFFF))) return -1;
    if (n == 4 && (v < 0x10000 || v > 0x10FFFF)) return -1;
    if (cp) *cp = (int)v;
    return n;
}

/* Number of code points in `s` (0 when `s` is NULL), or -1 when it is not
   valid UTF-8. This counts code points; `std_str_len` counts bytes. */
static inline int std_utf8_codepoint_count(const char *s)
{
    int n;
    int i;
    int step;

    if (!s) return 0;
    n = 0;
    i = 0;
    while (s[i]) {
        step = std_utf8_decode(s + i, 0);
        if (step < 0) return -1;
        i += step;
        n++;
    }
    return n;
}

/* Encode `cp` into `out`. Returns the byte count (1..4), or -1 for a surrogate,
   an out-of-range code point or a buffer that is too small. */
static inline int std_utf8_encode(int cp, char *out, int cap)
{
    int need;

    if (!out || cap <= 0) return -1;
    if (cp < 0 || cp > 0x10FFFF) return -1;
    if (cp >= 0xD800 && cp <= 0xDFFF) return -1;
    if (cp < 0x80) need = 1;
    else if (cp < 0x800) need = 2;
    else if (cp < 0x10000) need = 3;
    else need = 4;
    if (cap < need) return -1;
    if (need == 1) {
        out[0] = (char)cp;
    } else if (need == 2) {
        out[0] = (char)(0xC0 | (cp >> 6));
        out[1] = (char)(0x80 | (cp & 0x3F));
    } else if (need == 3) {
        out[0] = (char)(0xE0 | (cp >> 12));
        out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        out[2] = (char)(0x80 | (cp & 0x3F));
    } else {
        out[0] = (char)(0xF0 | (cp >> 18));
        out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
        out[2] = (char)(0x80 | ((cp >> 6) & 0x3F));
        out[3] = (char)(0x80 | (cp & 0x3F));
    }
    return need;
}

#endif
