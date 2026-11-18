/*
 * Timer.shim.h - timer_create / timer_settime on POSIX, setitimer on macOS.
 *
 * The native object is malloc'd so the CT side holds one stable pointer. On
 * POSIX the object stores the `timer_t`; on the fallback it stores the interval
 * and signal because `setitimer` has no per-timer handle (there is exactly one
 * ITIMER_REAL per process). The fallback always delivers SIGALRM; the signal
 * argument is kept for the API but cannot be honoured.
 *
 * `remaining` returns milliseconds, 0 when disarmed, -1 on error.
 *
 * License: MIT (see LICENSE).
 */

#ifndef CT_STD_TIME_TIMER_SHIM_H
#define CT_STD_TIME_TIMER_SHIM_H

#include <time.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#if defined(_POSIX_TIMERS) && _POSIX_TIMERS > 0 && !defined(__APPLE__)

struct std_time_timer_native {
    timer_t id;
    int milliseconds;
    int signal_number;
    int once;
};

static inline void *std_time_timer_new(int milliseconds, int once, int signal_number)
{
    struct std_time_timer_native *n;
    struct sigevent event;

    if (milliseconds <= 0 || signal_number <= 0) return NULL;
    n = (struct std_time_timer_native *)malloc(sizeof *n);
    if (!n) return NULL;
    memset(&event, 0, sizeof event);
    event.sigev_notify = SIGEV_SIGNAL;
    event.sigev_signo = signal_number;
    if (timer_create(CLOCK_REALTIME, &event, &n->id) != 0) {
        free(n);
        return NULL;
    }
    n->milliseconds = milliseconds;
    n->signal_number = signal_number;
    n->once = once;
    return n;
}

static inline int std_time_timer_start(void *handle)
{
    struct std_time_timer_native *n = (struct std_time_timer_native *)handle;
    struct itimerspec spec;

    if (!n) return -1;
    memset(&spec, 0, sizeof spec);
    spec.it_value.tv_sec = n->milliseconds / 1000;
    spec.it_value.tv_nsec = (long)(n->milliseconds % 1000) * 1000000L;
    if (n->once) {
        spec.it_interval.tv_sec = 0;
        spec.it_interval.tv_nsec = 0;
    } else {
        spec.it_interval = spec.it_value;
    }
    return timer_settime(n->id, 0, &spec, NULL) == 0 ? 0 : -1;
}

static inline int std_time_timer_stop(void *handle)
{
    struct std_time_timer_native *n = (struct std_time_timer_native *)handle;
    struct itimerspec spec;

    if (!n) return -1;
    memset(&spec, 0, sizeof spec);
    return timer_settime(n->id, 0, &spec, NULL) == 0 ? 0 : -1;
}

static inline int std_time_timer_remaining(void *handle)
{
    struct std_time_timer_native *n = (struct std_time_timer_native *)handle;
    struct itimerspec spec;

    if (!n) return -1;
    if (timer_gettime(n->id, &spec) != 0) return -1;
    return (int)(spec.it_value.tv_sec * 1000 + spec.it_value.tv_nsec / 1000000L);
}

static inline void std_time_timer_delete(void *handle)
{
    struct std_time_timer_native *n = (struct std_time_timer_native *)handle;

    if (!n) return;
    timer_delete(n->id);
    free(n);
}

#else /* fallback: one ITIMER_REAL per process */

struct std_time_timer_native {
    int milliseconds;
    int signal_number;
    int once;
    int active;
};

static inline void *std_time_timer_new(int milliseconds, int once, int signal_number)
{
    struct std_time_timer_native *n;

    if (milliseconds <= 0 || signal_number <= 0) return NULL;
    n = (struct std_time_timer_native *)malloc(sizeof *n);
    if (!n) return NULL;
    n->milliseconds = milliseconds;
    n->signal_number = signal_number;
    n->once = once;
    n->active = 0;
    return n;
}

static inline int std_time_timer_start(void *handle)
{
    struct std_time_timer_native *n = (struct std_time_timer_native *)handle;
    struct itimerval value;

    if (!n) return -1;
    memset(&value, 0, sizeof value);
    value.it_value.tv_sec = n->milliseconds / 1000;
    value.it_value.tv_usec = (n->milliseconds % 1000) * 1000;
    if (!n->once) value.it_interval = value.it_value;
    if (setitimer(ITIMER_REAL, &value, NULL) != 0) return -1;
    n->active = 1;
    return 0;
}

static inline int std_time_timer_stop(void *handle)
{
    struct std_time_timer_native *n = (struct std_time_timer_native *)handle;
    struct itimerval value;

    if (!n) return -1;
    memset(&value, 0, sizeof value);
    if (setitimer(ITIMER_REAL, &value, NULL) != 0) return -1;
    n->active = 0;
    return 0;
}

static inline int std_time_timer_remaining(void *handle)
{
    struct std_time_timer_native *n = (struct std_time_timer_native *)handle;
    struct itimerval value;

    if (!n) return -1;
    if (getitimer(ITIMER_REAL, &value) != 0) return -1;
    return (int)(value.it_value.tv_sec * 1000 + value.it_value.tv_usec / 1000);
}

static inline void std_time_timer_delete(void *handle)
{
    struct std_time_timer_native *n = (struct std_time_timer_native *)handle;

    if (!n) return;
    if (n->active) std_time_timer_stop(n);
    free(n);
}

#endif

#endif
