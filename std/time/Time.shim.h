/*
 * Time.shim.h - time / clock_gettime / nanosleep / localtime_r / strftime
 * behind std::time.
 *
 * `struct tm` stays inside this header: the CT side only ever receives ints and
 * a formatted string. `localtime_r` is used instead of `localtime` so the
 * shim does not share libc's static buffer between callers.
 *
 * `format` counts first with `strftime(out, 0, ...)` (portable on POSIX) and
 * refuses (-1, buffer cleared) when the result plus NUL does not fit; it never
 * returns a truncated time. A pattern that produces an empty result is 0.
 *
 * `sleep_milliseconds` returns 0 only when the whole interval elapsed; a signal
 * waking nanosleep early is -1 (the caller can retry).
 *
 * License: MIT (see LICENSE).
 */

#ifndef CT_STD_TIME_TIME_SHIM_H
#define CT_STD_TIME_TIME_SHIM_H

#include <time.h>
#include <errno.h>
#include <string.h>

static inline long std_time_now_seconds(void)
{
    return (long)time(NULL);
}

static inline long std_time_now_nanoseconds(void)
{
    struct timespec ts;

    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) return -1;
    return (long)ts.tv_sec * 1000000000L + (long)ts.tv_nsec;
}

static inline double std_time_diff_seconds(long a, long b)
{
    return difftime((time_t)a, (time_t)b);
}

static inline int std_time_sleep_milliseconds(int milliseconds)
{
    struct timespec req;
    struct timespec rem;

    if (milliseconds < 0) return -1;
    req.tv_sec = milliseconds / 1000;
    req.tv_nsec = (long)(milliseconds % 1000) * 1000000L;
    rem.tv_sec = 0;
    rem.tv_nsec = 0;
    /* A signal interrupting nanosleep must not shorten the requested sleep;
       retry with the remaining time. */
    while (nanosleep(&req, &rem) != 0) {
        if (errno != EINTR) return -1;
        req = rem;
    }
    return 0;
}

static inline long std_time_cpu_ticks(void)
{
    return (long)clock();
}

/* One localtime_r per field keeps the API simple; the call is cheap and the
   struct stays in this translation unit. */
static inline int std_time_field(long when, int which)
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

static inline int std_time_year(long when) { return std_time_field(when, 0); }
static inline int std_time_month(long when) { return std_time_field(when, 1); }
static inline int std_time_day(long when) { return std_time_field(when, 2); }
static inline int std_time_hour(long when) { return std_time_field(when, 3); }
static inline int std_time_minute(long when) { return std_time_field(when, 4); }
static inline int std_time_second(long when) { return std_time_field(when, 5); }
static inline int std_time_day_of_week(long when) { return std_time_field(when, 6); }

static inline int std_time_format(long when, const char *pattern, char *out, int cap)
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

static inline long std_time_from_parts(int year, int month, int day,
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

#endif
