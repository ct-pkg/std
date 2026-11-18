/*
 * LocaleFactory.shim.h - the mapping unit's shim.
 *
 * The string factory forwards to the CT factory in Locale.h.ct, so this shim
 * only has to make LocaleHandle.shim.h available in the consumer's translation unit
 * (and, as a side effect, give the driver the unit directory to add to the
 * include path).
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

#ifndef CT_STD_LOCALE_LOCALEFACTORY_SHIM_H
#define CT_STD_LOCALE_LOCALEFACTORY_SHIM_H

#include "LocaleHandle.shim.h"

#endif
