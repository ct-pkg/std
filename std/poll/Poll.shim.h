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
#endif
