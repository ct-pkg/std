/*
 * Timer.shim.h - the Timer unit's shim.
 *
 * The native implementation lives in Time.shim.c: POSIX timer types and the
 * feature-test macro (_POSIX_C_SOURCE) must be defined before the first system
 * header, and a .shim.h is included after other units' headers in a consumer.
 * Moving the code into a .shim.c keeps the macro effective on Linux while the
 * macOS fallback stays in the same file.
 *
 * License: MIT (see LICENSE).
 */

#ifndef CT_STD_TIME_TIMER_SHIM_H
#define CT_STD_TIME_TIMER_SHIM_H

#include <stdlib.h>
#include "Time.shim.h"

#endif
