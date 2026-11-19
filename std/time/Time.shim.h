/*
 * Time.shim.h - the declarations behind std::time.
 *
 * The implementations live in Time.shim.c, not in this header: the POSIX time
 * APIs (clock_gettime, nanosleep, localtime_r, timer_create) need
 * _POSIX_C_SOURCE before the first system header, and a .h.ct shim header is
 * included after other units' headers in a consumer. Keeping the bodies in
 * the .shim.c is what makes them visible under strict -std=c99 on glibc.
 *
 * The Timer prototypes are here too so Timer.h.ct can call them without
 * seeing timer_t / struct itimerspec.
 *
 * License: MIT (see LICENSE).
 */

#ifndef CT_STD_TIME_TIME_SHIM_H
#define CT_STD_TIME_TIME_SHIM_H

long std_time_now_seconds(void);
long std_time_now_nanoseconds(void);
double std_time_diff_seconds(long a, long b);
int std_time_sleep_milliseconds(int milliseconds);
long std_time_cpu_ticks(void);

int std_time_year(long when);
int std_time_utc_year(long when);
int std_time_utc_month(long when);
int std_time_utc_day(long when);
int std_time_utc_hour(long when);
int std_time_utc_minute(long when);
int std_time_utc_second(long when);
int std_time_utc_day_of_week(long when);
long std_time_from_utc_parts(int year, int month, int day, int hour, int minute, int second);
int std_time_now_timespec(long *seconds, long *nanoseconds);
int std_time_month(long when);
int std_time_day(long when);
int std_time_hour(long when);
int std_time_minute(long when);
int std_time_second(long when);
int std_time_day_of_week(long when);

int std_time_format(long when, const char *pattern, char *out, int cap);
long std_time_from_parts(int year, int month, int day, int hour, int minute, int second);

/* Timer native handle, implemented in Time.shim.c. */
void *std_time_timer_new(int milliseconds, int once, int signal_number);
int std_time_timer_start(void *handle);
int std_time_timer_stop(void *handle);
int std_time_timer_remaining(void *handle);
void std_time_timer_delete(void *handle);

#endif
