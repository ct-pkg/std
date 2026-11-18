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

#ifndef CT_STD_LOCALE_LOCALEFACTORY_SHIM_H
#define CT_STD_LOCALE_LOCALEFACTORY_SHIM_H

#include "LocaleHandle.shim.h"

#endif
