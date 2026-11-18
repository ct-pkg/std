/*
 * Dir.shim.h - opendir / readdir / closedir / telldir / seekdir / chdir behind
 * std::fs::Dir.
 *
 * The `DIR *` crosses the boundary as `void *`. `std_fs_dir_next` skips `.` and
 * `..` and copies the name; it never hands out `struct dirent`.
 *
 * `std_fs_dir_close` is idempotent (NULL is success); the CT layer nulls the
 * field so a second close does not touch a freed DIR.
 *
 * License: MIT (see LICENSE).
 */

/* The POSIX.1-2008 feature set: shims are the first headers in a generated
   consumer, so defining it here makes mkdir/readlink/telldir/fileno/setenv and
   friends visible under strict -std=c99 on glibc. */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#ifndef CT_STD_FS_DIR_SHIM_H
#define CT_STD_FS_DIR_SHIM_H

#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

static inline void *std_fs_dir_open(const char *path)
{
    if (!path || !*path) return NULL;
    return (void *)opendir(path);
}

static inline int std_fs_dir_close(void *handle)
{
    DIR *d = (DIR *)handle;

    if (!d) return 0;
    return closedir(d) == 0 ? 0 : -1;
}

static inline int std_fs_dir_next(void *handle, char *out, int cap)
{
    DIR *d = (DIR *)handle;
    struct dirent *e;
    size_t len;

    if (!d || !out || cap <= 0) return -1;
    for (;;) {
        e = readdir(d);
        if (!e) return 0;
        if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) continue;
        len = strlen(e->d_name);
        if ((int)len + 1 > cap) {
            out[0] = 0;
            return -1;
        }
        memcpy(out, e->d_name, len + 1);
        return 1;
    }
}

static inline void std_fs_dir_rewind(void *handle)
{
    DIR *d = (DIR *)handle;

    if (!d) return;
    rewinddir(d);
}

static inline long std_fs_dir_tell(void *handle)
{
    DIR *d = (DIR *)handle;

    if (!d) return -1;
    return (long)telldir(d);
}

static inline void std_fs_dir_seek(void *handle, long position)
{
    DIR *d = (DIR *)handle;

    if (!d) return;
    seekdir(d, position);
}

static inline int std_fs_dir_enter(const char *path)
{
    if (!path || !*path) return -1;
    return chdir(path) == 0 ? 0 : -1;
}

static inline int std_fs_getcwd(char *out, int cap)
{
    if (!out || cap <= 0) return -1;
    if (!getcwd(out, (size_t)cap)) {
        out[0] = 0;
        return -1;
    }
    return (int)strlen(out);
}

static inline char *std_fs_duplicate(const char *s)
{
    size_t len;
    char *copy;

    if (!s) return NULL;
    len = strlen(s);
    copy = (char *)malloc(len + 1);
    if (!copy) return NULL;
    memcpy(copy, s, len + 1);
    return copy;
}

static inline int std_fs_copy_string(const char *s, char *out, int cap)
{
    size_t len;

    if (!s || !out || cap <= 0) return -1;
    len = strlen(s);
    if ((int)len + 1 > cap) {
        out[0] = 0;
        return -1;
    }
    memcpy(out, s, len + 1);
    return (int)len;
}

#endif
