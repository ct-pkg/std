/*
 * LocaleHandle.shim.h - the <locale.h> surface behind std::locale.
 *
 * `std_locale_duplicate` is the only allocation: the factory owns a copy of the
 * name so the caller's string may die immediately. `free` is called by the
 * Drop implementation in LocaleHandle.h.ct.
 *
 * `std_locale_from_name` exists for the mapping unit: a `.m.ct` cannot call a
 * `.h.ct` symbol directly, so the string factory goes through this inline
 * wrapper, which is visible in the consumer's translation unit once Locale.h
 * has been included. The forward declaration is enough because only the
 * pointer crosses the boundary.
 *
 * `std_locale_apply` changes process-global state (`setlocale`), and
 * `std_locale_conventions` copies out of the static `struct lconv`.
 *
 * License: MIT (see LICENSE).
 */

#ifndef CT_STD_LOCALE_LOCALE_SHIM_H
#define CT_STD_LOCALE_LOCALE_SHIM_H

#include <locale.h>
#include <stdlib.h>
#include <string.h>

struct std_locale_LocaleHandle;
struct std_locale_LocaleHandle *std_locale_make_locale(const char *name);

static inline char *std_locale_duplicate(const char *name)
{
    size_t len;
    char *copy;

    if (!name || !*name) return NULL;
    len = strlen(name);
    copy = (char *)malloc(len + 1);
    if (!copy) return NULL;
    memcpy(copy, name, len + 1);
    return copy;
}

static inline int std_locale_copy(const char *name, char *out, int cap)
{
    size_t len;

    if (!name || !out || cap <= 0) return -1;
    len = strlen(name);
    if ((int)len + 1 > cap) {
        out[0] = 0;
        return -1;
    }
    memcpy(out, name, len + 1);
    return (int)len;
}

/* The string-start factory used by Locale.m.ct. */
static inline struct std_locale_LocaleHandle *std_locale_from_name(const char *name)
{
    return std_locale_make_locale(name);
}

static inline int std_locale_apply(const char *name, int category)
{
    if (!name || !*name) return -1;
    return setlocale(category, name) ? 0 : -1;
}

/* Read the current process locale name; 0 on success, -1 when it does not fit. */
static inline int std_locale_current(char *out, int cap)
{
    const char *name;

    if (!out || cap <= 0) return -1;
    name = setlocale(LC_ALL, NULL);
    return std_locale_copy(name, out, cap);
}

static inline int std_locale_conventions(char *decimal_point, int dp_cap,
                                         char *thousands_separator, int ts_cap)
{
    struct lconv *conv;

    if (!decimal_point || dp_cap <= 0 || !thousands_separator || ts_cap <= 0) {
        return -1;
    }
    conv = localeconv();
    if (!conv) return -1;
    if (std_locale_copy(conv->decimal_point, decimal_point, dp_cap) < 0) return -1;
    if (std_locale_copy(conv->thousands_sep, thousands_separator, ts_cap) < 0) {
        decimal_point[0] = 0;
        return -1;
    }
    return 0;
}

/* Categories as functions: `std::locale` has no portable numeric constants and
   the shim is the one place that includes <locale.h>. */
static inline int std_locale_category_all(void) { return LC_ALL; }
static inline int std_locale_category_numeric(void) { return LC_NUMERIC; }
static inline int std_locale_category_time(void) { return LC_TIME; }
static inline int std_locale_category_collate(void) { return LC_COLLATE; }
static inline int std_locale_category_ctype(void) { return LC_CTYPE; }
static inline int std_locale_category_monetary(void) { return LC_MONETARY; }

#endif
