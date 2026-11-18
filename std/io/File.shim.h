/*
 * File.shim.h - the stdio surface behind std::io::File.
 *
 * The CT struct stores the `FILE *` as `void *`; that is the only reason this
 * header exists. Every function accepts NULL and reports the module's failure
 * value, so a method on a NULL handle is safe without a C-side branch.
 *
 * `read_line` reads byte by byte instead of calling `fgets`: `fgets` truncates
 * a long line without saying so, and the contract here is -1 (no partial
 * result). The bytes already consumed stay consumed, which the README states.
 *
 * `close_stream` is idempotent at the CT level (the field is nulled); here a
 * NULL pointer is a no-op success.
 *
 * License: MIT (see LICENSE).
 */

/* The POSIX.1-2008 feature set: shims are the first headers in a generated
   consumer, so defining it here makes mkdir/readlink/telldir/fileno/setenv and
   friends visible under strict -std=c99 on glibc. */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#ifndef CT_STD_IO_FILE_SHIM_H
#define CT_STD_IO_FILE_SHIM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline void *std_io_open_stream(const char *path, const char *mode)
{
    FILE *fp;

    if (!path || !mode) return NULL;
    fp = fopen(path, mode);
    return (void *)fp;
}

static inline int std_io_close_stream(void *handle)
{
    FILE *fp = (FILE *)handle;

    if (!fp) return 0;
    return fclose(fp) == 0 ? 0 : -1;
}

static inline int std_io_stream_read(void *handle, char *buf, int n)
{
    FILE *fp = (FILE *)handle;

    if (!fp || !buf || n < 0) return -1;
    if (n == 0) return 0;
    return (int)fread(buf, 1, (size_t)n, fp);
}

static inline int std_io_stream_write(void *handle, const char *buf, int n)
{
    FILE *fp = (FILE *)handle;

    if (!fp || n < 0 || (!buf && n > 0)) return -1;
    if (n == 0) return 0;
    return (int)fwrite(buf, 1, (size_t)n, fp);
}

static inline int std_io_stream_seek(void *handle, long offset, int origin)
{
    FILE *fp = (FILE *)handle;

    if (!fp) return -1;
    return fseek(fp, offset, origin) == 0 ? 0 : -1;
}

static inline long std_io_stream_tell(void *handle)
{
    FILE *fp = (FILE *)handle;

    if (!fp) return -1;
    return ftell(fp);
}

static inline int std_io_stream_at_end(void *handle)
{
    FILE *fp = (FILE *)handle;

    if (!fp) return 0;
    return feof(fp) ? 1 : 0;
}

static inline int std_io_stream_has_error(void *handle)
{
    FILE *fp = (FILE *)handle;

    if (!fp) return 1;
    return ferror(fp) ? 1 : 0;
}

static inline void std_io_stream_clear_error(void *handle)
{
    FILE *fp = (FILE *)handle;

    if (!fp) return;
    clearerr(fp);
}

static inline int std_io_stream_read_char(void *handle)
{
    FILE *fp = (FILE *)handle;

    if (!fp) return -1;
    return fgetc(fp);
}

static inline int std_io_stream_write_char(void *handle, int c)
{
    FILE *fp = (FILE *)handle;

    if (!fp) return -1;
    return fputc(c, fp);
}

static inline int std_io_stream_unread_char(void *handle, int c)
{
    FILE *fp = (FILE *)handle;

    if (!fp) return -1;
    return ungetc(c, fp);
}

static inline int std_io_stream_read_line(void *handle, char *out, int cap)
{
    FILE *fp = (FILE *)handle;
    int i;
    int c;

    if (!fp || !out || cap <= 0) return -1;
    i = 0;
    for (;;) {
        c = fgetc(fp);
        if (c == EOF) {
            out[i] = 0;
            return i == 0 ? 0 : i;
        }
        if (c == '\n') {
            out[i] = 0;
            return i;
        }
        if (i + 1 >= cap) {
            out[i] = 0;
            return -1;
        }
        out[i] = (char)c;
        i++;
    }
}

static inline int std_io_stream_write_line(void *handle, const char *text)
{
    FILE *fp = (FILE *)handle;
    size_t len;

    if (!fp || !text) return -1;
    len = strlen(text);
    if (fwrite(text, 1, len, fp) != len) return -1;
    if (fputc('\n', fp) == EOF) return -1;
    return 0;
}

static inline int std_io_stream_flush(void *handle)
{
    FILE *fp = (FILE *)handle;

    if (!fp) return -1;
    return fflush(fp) == 0 ? 0 : -1;
}

static inline int std_io_origin_start(void) { return SEEK_SET; }
static inline int std_io_origin_current(void) { return SEEK_CUR; }
static inline int std_io_origin_end(void) { return SEEK_END; }

#endif
