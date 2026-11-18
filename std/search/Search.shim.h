/*
 * Search.shim.h - qsort / bsearch behind std::search.
 *
 * The comparator is a C function pointer. CT passes a function name through a
 * `void *` parameter; the shim casts it back here. That is the whole callback
 * contract: a comparator with the C `qsort` signature, no closures, no context
 * argument.
 *
 * License: MIT (see LICENSE).
 */
#ifndef CT_STD_SEARCH_SHIM_H
#define CT_STD_SEARCH_SHIM_H

#include <stdlib.h>

typedef int (*std_search_cmp_fn)(const void *, const void *);

static inline void std_search_sort(char *base, int count, int elem_size, void *cmp)
{
    if (!base || count <= 1 || elem_size <= 0 || !cmp) return;
    qsort(base, (size_t)count, (size_t)elem_size, (std_search_cmp_fn)cmp);
}

/* Byte index of the matching element, or -1 when absent. The array must already
   be sorted by the same comparator. */
static inline int std_search_binary_search(char *base, int count, int elem_size,
                                           void *key, void *cmp)
{
    void *hit;

    if (!base || count <= 0 || elem_size <= 0 || !key || !cmp) return -1;
    hit = bsearch(key, base, (size_t)count, (size_t)elem_size, (std_search_cmp_fn)cmp);
    if (!hit) return -1;
    return (int)(((char *)hit - base) / elem_size);
}

#endif
