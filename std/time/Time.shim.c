/*
 * Time.shim.c - the one translation unit that sees the POSIX timer types.
 *
 * Feature-test macros must be defined before the first system header, and a
 * .h.ct shim header is included after other units' headers; putting the timer
 * code in this .shim.c is what makes timer_create visible under strict
 * -std=c99 on glibc. On macOS (no timer_t) the code falls back to setitimer.
 *
 * License: MIT (see LICENSE).
 */

#define _POSIX_C_SOURCE 200809L

#include "Time.shim.h"
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>


long std_time_now_seconds(void)
{
    return (long)time(NULL);
}

long std_time_now_nanoseconds(void)
{
    struct timespec ts;

    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) return -1;
    return (long)ts.tv_sec * 1000000000L + (long)ts.tv_nsec;
}

double std_time_diff_seconds(long a, long b)
{
    return difftime((time_t)a, (time_t)b);
}

int std_time_sleep_milliseconds(int milliseconds)
{
    struct timespec req;
    struct timespec rem;

    if (milliseconds < 0) return -1;
    req.tv_sec = milliseconds / 1000;
    req.tv_nsec = (long)(milliseconds % 1000) * 1000000L;
    rem.tv_sec = 0;
    rem.tv_nsec = 0;
    while (nanosleep(&req, &rem) != 0) {
        if (errno != EINTR) return -1;
        req = rem;
    }
    return 0;
}

long std_time_cpu_ticks(void)
{
    return (long)clock();
}

static int std_time_field(long when, int which)
{
    struct tm parts;
    time_t t = (time_t)when;

    if (!localtime_r(&t, &parts)) return -1;
    switch (which) {
    case 0: return parts.tm_year + 1900;
    case 1: return parts.tm_mon + 1;
    case 2: return parts.tm_mday;
    case 3: return parts.tm_hour;
    case 4: return parts.tm_min;
    case 5: return parts.tm_sec;
    default: return parts.tm_wday;
    }
}

int std_time_year(long when) { return std_time_field(when, 0); }
int std_time_month(long when) { return std_time_field(when, 1); }
int std_time_day(long when) { return std_time_field(when, 2); }
int std_time_hour(long when) { return std_time_field(when, 3); }
int std_time_minute(long when) { return std_time_field(when, 4); }
int std_time_second(long when) { return std_time_field(when, 5); }
int std_time_day_of_week(long when) { return std_time_field(when, 6); }

int std_time_format(long when, const char *pattern, char *out, int cap)
{
    struct tm parts;
    time_t t = (time_t)when;
    char tmp[1024];
    size_t need;

    if (!pattern || !out || cap <= 0) return -1;
    if (!localtime_r(&t, &parts)) return -1;
    need = strftime(tmp, sizeof tmp, pattern, &parts);
    if (need == 0) {
        out[0] = 0;
        return 0;
    }
    if ((int)need + 1 > cap) {
        out[0] = 0;
        return -1;
    }
    memcpy(out, tmp, need + 1);
    return (int)need;
}

long std_time_from_parts(int year, int month, int day,
                         int hour, int minute, int second)
{
    struct tm parts;
    time_t t;

    parts.tm_year = year - 1900;
    parts.tm_mon = month - 1;
    parts.tm_mday = day;
    parts.tm_hour = hour;
    parts.tm_min = minute;
    parts.tm_sec = second;
    parts.tm_isdst = -1;
    t = mktime(&parts);
    if (t == (time_t)-1) return -1;
    return (long)t;
}

#if defined(_POSIX_TIMERS) && _POSIX_TIMERS > 0 && !defined(__APPLE__)

struct std_time_timer_native {
    timer_t id;
    int milliseconds;
    int signal_number;
    int once;
};

void *std_time_timer_new(int milliseconds, int once, int signal_number)
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

int std_time_timer_start(void *handle)
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

int std_time_timer_stop(void *handle)
{
    struct std_time_timer_native *n = (struct std_time_timer_native *)handle;
    struct itimerspec spec;

    if (!n) return -1;
    memset(&spec, 0, sizeof spec);
    return timer_settime(n->id, 0, &spec, NULL) == 0 ? 0 : -1;
}

int std_time_timer_remaining(void *handle)
{
    struct std_time_timer_native *n = (struct std_time_timer_native *)handle;
    struct itimerspec spec;

    if (!n) return -1;
    if (timer_gettime(n->id, &spec) != 0) return -1;
    return (int)(spec.it_value.tv_sec * 1000 + spec.it_value.tv_nsec / 1000000L);
}

void std_time_timer_delete(void *handle)
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

void *std_time_timer_new(int milliseconds, int once, int signal_number)
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

int std_time_timer_start(void *handle)
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

int std_time_timer_stop(void *handle)
{
    struct std_time_timer_native *n = (struct std_time_timer_native *)handle;
    struct itimerval value;

    if (!n) return -1;
    memset(&value, 0, sizeof value);
    if (setitimer(ITIMER_REAL, &value, NULL) != 0) return -1;
    n->active = 0;
    return 0;
}

int std_time_timer_remaining(void *handle)
{
    struct std_time_timer_native *n = (struct std_time_timer_native *)handle;
    struct itimerval value;

    if (!n) return -1;
    if (getitimer(ITIMER_REAL, &value) != 0) return -1;
    return (int)(value.it_value.tv_sec * 1000 + value.it_value.tv_usec / 1000);
}

void std_time_timer_delete(void *handle)
{
    struct std_time_timer_native *n = (struct std_time_timer_native *)handle;

    if (!n) return;
    if (n->active) std_time_timer_stop(n);
    free(n);
}

#endif


