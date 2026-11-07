/*
 * Real.shim.h - the C surface behind std::math::Real's `toStr` / `toString`.
 *
 * The file must share the base name of `Real.m.ct` and sit beside it. The
 * text is round-trip exact: `%g` is used when it already reads back as the same
 * value (so `2.6` stays `2.6`), and `%.17g` -- C99 `DBL_DECIMAL_DIG` -- when six
 * significant digits are not enough (`1.23456789` prints as
 * `1.2345678899999999`). `strtod` therefore always reproduces the input, as long
 * as the whole spelling fits: `toString`'s 32-byte slots always do, but a
 * `toStr` buffer that is too small truncates silently and loses the guarantee.
 * `toString` has its own four-slot ring, separate from the integer ring in
 * `std::num`. Both rings are **not thread-safe** and a fifth live result reuses
 * the first slot.
 *
 * License: same as the rest of the compiler (see LICENSE / LICENSE.EXCEPTION).
 */
#ifndef CT_STD_MATH_REAL_SHIM_H
#define CT_STD_MATH_REAL_SHIM_H

#include <stdio.h>
#include <stdlib.h>

/* `%g` first: when six significant digits already read back exactly, the short
   form is kept. Only a value that `strtod` cannot reproduce from `%g` costs the
   full `%.17g` spelling, so the text is always lossless. */
static inline void std_math_fmt_double(double v, char *out, int cap)
{
    snprintf(out, (size_t)cap, "%g", v);
    if (strtod(out, 0) != v)
        snprintf(out, (size_t)cap, "%.17g", v);
}

static inline char *std_math_double_to_str(double v, char *out, int cap)
{
    if (out == 0 || cap <= 0) return out;
    std_math_fmt_double(v, out, cap);
    return out;
}

static inline char *std_math_double_to_string(double v)
{
    static char ring[4][32];
    static int slot = 0;
    char *out = ring[slot];
    slot = (slot + 1) & 3;
    std_math_fmt_double(v, out, (int)sizeof ring[0]);
    return out;
}

#endif
