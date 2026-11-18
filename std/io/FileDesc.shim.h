/*
 * FileDesc.shim.h - open/read/write/seek/close/recv/send behind std::io::FileDesc.
 *
 * Every function takes the raw fd and reports -1 on error; the CT layer is what
 * turns -1 into a closed handle. `std_io_desc_open` passes 0644 when the flags
 * create the file (open(2) requires a mode there, and this API has no mode
 * parameter).
 *
 * `send` / `recv` live here rather than in a future net shim because they are
 * plain fd operations; a regular file reports ENOTSOCK and the call returns -1.
 *
 * License: MIT (see LICENSE).
 */

/* The POSIX.1-2008 feature set: shims are the first headers in a generated
   consumer, so defining it here makes mkdir/readlink/telldir/fileno/setenv and
   friends visible under strict -std=c99 on glibc. */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#ifndef CT_STD_IO_FILEDESC_SHIM_H
#define CT_STD_IO_FILEDESC_SHIM_H

#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>

static inline int std_io_desc_open(const char *path, int flags)
{
    if (!path || !*path) return -1;
    return open(path, flags, 0644);
}

static inline int std_io_desc_close(int fd)
{
    if (fd < 0) return 0;
    return close(fd) == 0 ? 0 : -1;
}

static inline int std_io_desc_read(int fd, char *buf, int cap)
{
    if (fd < 0 || !buf || cap < 0) return -1;
    if (cap == 0) return 0;
    return (int)read(fd, buf, (size_t)cap);
}

static inline int std_io_desc_write(int fd, const char *data, int n)
{
    if (fd < 0 || n < 0 || (!data && n > 0)) return -1;
    if (n == 0) return 0;
    return (int)write(fd, data, (size_t)n);
}

static inline long std_io_desc_seek(int fd, long offset, int origin)
{
    if (fd < 0) return -1;
    return (long)lseek(fd, (off_t)offset, origin);
}

static inline int std_io_desc_send(int fd, const char *data, int n)
{
    if (fd < 0 || n < 0 || (!data && n > 0)) return -1;
    return (int)send(fd, data, (size_t)n, 0);
}

static inline int std_io_desc_receive(int fd, char *buf, int cap)
{
    if (fd < 0 || !buf || cap < 0) return -1;
    if (cap == 0) return 0;
    return (int)recv(fd, buf, (size_t)cap, 0);
}

static inline int std_io_desc_flag_read(void) { return O_RDONLY; }
static inline int std_io_desc_flag_write(void) { return O_WRONLY; }
static inline int std_io_desc_flag_read_write(void) { return O_RDWR; }
static inline int std_io_desc_flag_append(void) { return O_WRONLY | O_APPEND | O_CREAT; }
static inline int std_io_desc_flag_create(void) { return O_WRONLY | O_CREAT | O_TRUNC; }
static inline int std_io_desc_flag_update(void) { return O_RDWR | O_CREAT | O_TRUNC; }
static inline int std_io_desc_flag_exclusive(void) { return O_WRONLY | O_CREAT | O_EXCL; }

static inline int std_io_desc_origin_start(void) { return SEEK_SET; }
static inline int std_io_desc_origin_current(void) { return SEEK_CUR; }
static inline int std_io_desc_origin_end(void) { return SEEK_END; }

#endif
