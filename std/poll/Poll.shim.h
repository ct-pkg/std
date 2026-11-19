/* poll(2) with an opaque pollfd array. License: MIT (see LICENSE). */
#ifndef _DARWIN_C_SOURCE
#define _DARWIN_C_SOURCE 1
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE 1
#endif
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef CT_STD_POLL_SHIM_H
#define CT_STD_POLL_SHIM_H
#include <poll.h>
#include <sys/select.h>
#include <sys/time.h>
#include <stdlib.h>

static inline void *std_poll_new(int count)
{
    if (count <= 0) return 0;
    return calloc((size_t)count, sizeof(struct pollfd));
}
static inline int std_poll_set_fd(void *set, int index, int fd, int events)
{
    struct pollfd *p = (struct pollfd *)set;
    if (!p || index < 0) return -1;
    p[index].fd = fd; p[index].events = (short)events;
    return 0;
}
static inline int std_poll_events(void *set, int index)
{
    struct pollfd *p = (struct pollfd *)set;
    if (!p || index < 0) return -1;
    return p[index].revents;
}
static inline int std_poll_wait(void *set, int count, int timeout)
{
    if (!set || count <= 0) return -1;
    return poll((struct pollfd *)set, (nfds_t)count, timeout);
}
static inline void std_poll_free(void *set) { free(set); }
static inline int std_poll_in(void) { return POLLIN; }
static inline int std_poll_out(void) { return POLLOUT; }
static inline int std_poll_err(void) { return POLLERR; }
static inline int std_poll_hup(void) { return POLLHUP; }
static inline int std_poll_nval(void) { return POLLNVAL; }
static inline void *std_select_new_set(void)
{
    return calloc(1, sizeof(fd_set));
}
static inline int std_select_add(void *set, int fd)
{
    if (!set || fd < 0 || fd >= FD_SETSIZE) return -1;
    FD_SET(fd, (fd_set *)set);
    return 0;
}
static inline int std_select_remove(void *set, int fd)
{
    if (!set || fd < 0 || fd >= FD_SETSIZE) return -1;
    FD_CLR(fd, (fd_set *)set);
    return 0;
}
static inline int std_select_isset(void *set, int fd)
{
    if (!set || fd < 0 || fd >= FD_SETSIZE) return 0;
    return FD_ISSET(fd, (fd_set *)set) ? 1 : 0;
}
static inline int std_select_wait(void *read_set, void *write_set, void *except_set, int timeout_ms)
{
    struct timeval tv;
    struct timeval *tvp = 0;
    if (timeout_ms >= 0) {
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        tvp = &tv;
    }
    return select(FD_SETSIZE, (fd_set *)read_set, (fd_set *)write_set, (fd_set *)except_set, tvp);
}
static inline void std_select_free(void *set) { free(set); }


#endif
