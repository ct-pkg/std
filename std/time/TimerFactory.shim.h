/*
 * TimerFactory.shim.h - the mapping unit's shim for std::time::IntTimer.
 *
 * Includes Timer.h for the CT factory prototypes, then chooses the periodic or
 * the one-shot factory. The `void *` callback channel is not involved here.
 *
 * License: MIT (see LICENSE).
 */

#ifndef CT_STD_TIME_TIMERFACTORY_SHIM_H
#define CT_STD_TIME_TIMERFACTORY_SHIM_H

#include "Timer.h"

static inline struct std_time_Timer *std_time_timer_from_int(int milliseconds,
                                                             int signal_number,
                                                             int once)
{
    if (once) return std_time_make_timer_once(milliseconds, signal_number);
    return std_time_make_timer(milliseconds, signal_number);
}

#endif
