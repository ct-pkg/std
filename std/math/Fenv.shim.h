/* fenv.h surface for std::math::Fenv. License: MIT (see LICENSE). */
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef CT_STD_MATH_FENV_SHIM_H
#define CT_STD_MATH_FENV_SHIM_H
#include <fenv.h>

static inline int std_fenv_round(void) { return fegetround(); }
static inline int std_fenv_set_round(int mode) { return fesetround(mode); }
static inline int std_fenv_clear(int excepts) { return feclearexcept(excepts); }
static inline int std_fenv_test(int excepts) { return fetestexcept(excepts); }
static inline int std_fenv_raise(int excepts) { return feraiseexcept(excepts); }
static inline int std_fenv_nearest(void) { return FE_TONEAREST; }
static inline int std_fenv_upward(void) { return FE_UPWARD; }
static inline int std_fenv_downward(void) { return FE_DOWNWARD; }
static inline int std_fenv_toward_zero(void) { return FE_TOWARDZERO; }
static inline int std_fenv_inexact(void) { return FE_INEXACT; }
static inline int std_fenv_invalid(void) { return FE_INVALID; }
static inline int std_fenv_divide_by_zero(void) { return FE_DIVBYZERO; }
static inline int std_fenv_overflow(void) { return FE_OVERFLOW; }
static inline int std_fenv_underflow(void) { return FE_UNDERFLOW; }
#endif
