/* ISO C stdlib functions that do not belong to a type host.
   License: MIT (see LICENSE). */
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef CT_STD_NUM_STDLIB_SHIM_H
#define CT_STD_NUM_STDLIB_SHIM_H
#include <stdlib.h>

static inline int std_stdlib_rand(void) { return rand(); }
static inline void std_stdlib_srand(unsigned int seed) { srand(seed); }
static inline int std_stdlib_div_quot(int a, int b) { return b ? a / b : 0; }
static inline int std_stdlib_div_rem(int a, int b) { return b ? a % b : 0; }
static inline void *std_stdlib_aligned_alloc(int alignment, int size)
{
    void *p = 0;
    if (alignment <= 0 || size < 0) return 0;
    if (posix_memalign(&p, (size_t)alignment, (size_t)size) != 0) return 0;
    return p;
}
#endif
