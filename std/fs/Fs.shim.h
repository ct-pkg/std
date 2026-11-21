/*
 * Fs.shim.h - the C surface behind std::fs::FsOps / std::fs::FsEnv.
 *
 * std::fs is a mapping module: it extends `char *` with file-system verbs, so
 * its whole shape is one mapping unit (`Fs.m.ct`) plus this header. There is no
 * `Fs.h.ct` (no new type) and no `.shim.c` (the functions are `static inline`
 * here, so each consumer gets its own copy and the program needs no extra
 * translation unit).
 *
 * Why the code lives here instead of in the `.m.ct`: a mapping unit refuses
 * anything but `#include` and `#define` in its prologue (`Parser.h.ct`, the
 * "a .m.ct unit allows only #include and #define" check), so `#ct-raw` with a
 * function body cannot be written there. The shim header is the closest thing
 * to a single file that the compiler accepts.
 *
 * `static inline` is deliberate: a plain `static` function that a consumer does
 * not call trips `-Wunused-function` under `-Wall -Wextra`; `static inline` does
 * not. Internal linkage also means no duplicate symbols when several programs
 * (or several translation units) include this header.
 *
 * Every name is prefixed so it cannot collide with another module's shim
 * (`CharStr.shim.h` declares `std_str_*`) or with user code.
 *
 * Allocation: nothing here allocates. `read_file` and `list` write into the
 * caller's buffer and take its capacity; the caller owns the buffer and there is
 * nothing to free. `list` joins entries with '\n', skips `.` and `..`, and fails
 * (-1) rather than returning a partial listing.
 *
 * Errors: every call refreshes the errno snapshot returned by
 * `std::fs::FsEnv.get_last_error()` -- 0 after success, the captured `errno`
 * after a failed system call, and `EINVAL` / `ERANGE` / `EISDIR` for the
 * rejections this module makes itself (bad argument, buffer too small, read of a
 * directory). The snapshot is one `static` int per consumer translation unit:
 * not thread-safe and not shared across translation units, same caveat as the
 * static slots in `std::num`.
 *
 * License: MIT (see LICENSE).
 */

/* The POSIX.1-2008 feature set: shims are the first headers in a generated
   consumer, so defining it here makes mkdir/readlink/telldir/fileno/setenv and
   friends visible under strict -std=c99 on glibc. */
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#ifndef CT_STD_FS_FS_SHIM_H
#define CT_STD_FS_FS_SHIM_H

#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>

/* errno snapshot of the last std::fs call in this translation unit. */
static int std_fs_errno_cache = 0;

static inline void std_fs_note(int e)
{
    std_fs_errno_cache = e;
}

/* The value `get_last_error()` returns. Never clears itself. */
static inline int std_fs_get_last_error(void)
{
    return std_fs_errno_cache;
}

/* 1 when `path` names something `stat` can see, 0 otherwise. */
static inline int std_fs_exists(const char *path)
{
    struct stat st;

    if (!path || !*path) {
        std_fs_note(EINVAL);
        return 0;
    }
    if (stat(path, &st) == 0) {
        std_fs_note(0);
        return 1;
    }
    std_fs_note(errno);
    return 0;
}

/* Read the whole file into `out`. Returns the byte count, or -1 when `path` is
   a directory, cannot be opened or does not fit `cap` (including the
   terminating NUL). */
static inline int std_fs_read_file(const char *path, char *out, int cap)
{
    struct stat st;
    FILE *f;
    int c;
    int n;

    if (!path || !out || cap <= 0) {
        std_fs_note(EINVAL);
        return -1;
    }
    f = fopen(path, "rb");
    if (!f) {
        std_fs_note(errno);
        return -1;
    }
    /* POSIX opens a directory read-only without complaint and then reports
       EOF on the first read, so a directory would come back as a successful
       0-byte read -- a silent wrong value. Reject it here. The check runs on
       the open descriptor, so it cannot race with the path. */
    if (fstat(fileno(f), &st) == 0 && S_ISDIR(st.st_mode)) {
        fclose(f);
        std_fs_note(EISDIR);
        return -1;
    }
    n = 0;
    for (;;) {
        c = fgetc(f);
        if (c == EOF) break;
        if (n + 1 >= cap) {
            fclose(f);
            std_fs_note(ERANGE);
            return -1;
        }
        out[n] = (char)c;
        n++;
    }
    out[n] = 0;
    fclose(f);
    std_fs_note(0);
    return n;
}

/* Replace or create `path` with `data`; 0 on success, -1 on failure. */
static inline int std_fs_write_file(const char *path, const char *data)
{
    FILE *f;
    size_t len;

    if (!path || !data) {
        std_fs_note(EINVAL);
        return -1;
    }
    f = fopen(path, "wb");
    if (!f) {
        std_fs_note(errno);
        return -1;
    }
    len = strlen(data);
    if (fwrite(data, 1, len, f) != len) {
        std_fs_note(errno);
        fclose(f);
        return -1;
    }
    if (fclose(f) != 0) {
        std_fs_note(errno);
        return -1;
    }
    std_fs_note(0);
    return 0;
}

/* Join the entries of `dir` with '\n' into `out` (no trailing newline) and
   return the entry count, or -1 when `dir` cannot be opened or the listing does
   not fit `cap`. `.` and `..` are skipped. */
static inline int std_fs_list(const char *dir, char *out, int cap)
{
    DIR *d;
    struct dirent *e;
    int n;
    int count;
    int len;
    int i;

    if (!dir || !out || cap <= 0) {
        std_fs_note(EINVAL);
        return -1;
    }
    d = opendir(dir);
    if (!d) {
        std_fs_note(errno);
        return -1;
    }
    n = 0;
    count = 0;
    while ((e = readdir(d)) != 0) {
        if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) continue;
        len = (int)strlen(e->d_name);
        if (n + len + 2 > cap) {
            closedir(d);
            std_fs_note(ERANGE);
            return -1;
        }
        if (n > 0) {
            out[n] = '\n';
            n++;
        }
        for (i = 0; i < len; i++) {
            out[n] = e->d_name[i];
            n++;
        }
        count++;
    }
    out[n] = 0;
    closedir(d);
    std_fs_note(0);
    return count;
}

/* Create `path` as a directory with `mode` (e.g. 0755). 0 on success, -1 on
   failure -- an already existing path fails too (EEXIST), it is never treated
   as success. */
static inline int std_fs_mkdir(const char *path, int mode)
{
    if (!path || !*path) {
        std_fs_note(EINVAL);
        return -1;
    }
    if (mkdir(path, (mode_t)mode) != 0) {
        std_fs_note(errno);
        return -1;
    }
    std_fs_note(0);
    return 0;
}

/* `remove(3)`: unlink a file or rmdir an empty directory. 0 on success, -1 on
   failure (a non-empty directory reports ENOTEMPTY). No recursive delete. */
static inline int std_fs_remove(const char *path)
{
    if (!path || !*path) {
        std_fs_note(EINVAL);
        return -1;
    }
    if (remove(path) != 0) {
        std_fs_note(errno);
        return -1;
    }
    std_fs_note(0);
    return 0;
}

/* `rename(2)`: 0 on success, -1 on failure (EXDEV across file systems). POSIX
   replaces an existing target; other platforms differ. */
static inline int std_fs_rename(const char *path, const char *to)
{
    if (!path || !*path || !to || !*to) {
        std_fs_note(EINVAL);
        return -1;
    }
    if (rename(path, to) != 0) {
        std_fs_note(errno);
        return -1;
    }
    std_fs_note(0);
    return 0;
}


/* ---- metadata, links, permissions ------------------------------------- */

static inline int std_fs_access(const char *path, int mode)
{
    if (!path || !*path) {
        std_fs_note(EINVAL);
        return -1;
    }
    if (access(path, mode) != 0) {
        std_fs_note(errno);
        return -1;
    }
    std_fs_note(0);
    return 0;
}

static inline int std_fs_read_link(const char *path, char *out, int cap)
{
    ssize_t n;

    if (!path || !*path || !out || cap <= 0) {
        std_fs_note(EINVAL);
        return -1;
    }
    n = readlink(path, out, (size_t)cap);
    if (n < 0) {
        std_fs_note(errno);
        return -1;
    }
    if (n >= cap) {
        out[0] = 0;
        std_fs_note(ERANGE);
        return -1;
    }
    out[n] = 0;
    std_fs_note(0);
    return (int)n;
}

static inline int std_fs_resize(const char *path, long length)
{
    if (!path || !*path || length < 0) {
        std_fs_note(EINVAL);
        return -1;
    }
    if (truncate(path, (off_t)length) != 0) {
        std_fs_note(errno);
        return -1;
    }
    std_fs_note(0);
    return 0;
}

static inline int std_fs_set_mode(const char *path, int mode)
{
    if (!path || !*path) {
        std_fs_note(EINVAL);
        return -1;
    }
    if (chmod(path, (mode_t)mode) != 0) {
        std_fs_note(errno);
        return -1;
    }
    std_fs_note(0);
    return 0;
}

static inline int std_fs_size(const char *path)
{
    struct stat st;

    if (!path || !*path || stat(path, &st) != 0) {
        std_fs_note(path && *path ? errno : EINVAL);
        return -1;
    }
    if (st.st_size > (off_t)INT_MAX) {
        std_fs_note(ERANGE);
        return -1;
    }
    std_fs_note(0);
    return (int)st.st_size;
}

static inline int std_fs_mode(const char *path)
{
    struct stat st;

    if (!path || !*path || stat(path, &st) != 0) {
        std_fs_note(path && *path ? errno : EINVAL);
        return -1;
    }
    std_fs_note(0);
    return (int)(st.st_mode & 07777);
}

static inline int std_fs_modification_time(const char *path)
{
    struct stat st;

    if (!path || !*path || stat(path, &st) != 0) {
        std_fs_note(path && *path ? errno : EINVAL);
        return -1;
    }
    if (st.st_mtime > (time_t)INT_MAX) {
        std_fs_note(ERANGE);
        return -1;
    }
    std_fs_note(0);
    return (int)st.st_mtime;
}

static inline int std_fs_is_readable(const char *path)
{
    return (path && *path && access(path, R_OK) == 0) ? 1 : 0;
}

static inline int std_fs_is_writable(const char *path)
{
    return (path && *path && access(path, W_OK) == 0) ? 1 : 0;
}

/* Lexical absolute path: prepend getcwd() for relative input, then collapse
   "//", "." and "..". Symlinks are not resolved (that is what readlink is for)
   and the path does not have to exist. Kept lexical because realpath() is not
   declared under strict -std=c99 on macOS and its NULL-destination extension
   would need a feature macro this header cannot set after <stdlib.h>. */
static inline int std_fs_resolve_path(const char *path, char *out, int cap)
{
    char joined[4096];
    char norm[4096];
    int jn;
    int n;
    int i;
    int start;
    int len;

    if (!path || !*path || !out || cap <= 0) {
        std_fs_note(EINVAL);
        return -1;
    }
    if (path[0] == '/') {
        jn = (int)strlen(path);
        if (jn >= (int)sizeof joined) {
            std_fs_note(ERANGE);
            return -1;
        }
        memcpy(joined, path, (size_t)jn + 1);
    } else {
        if (!getcwd(joined, sizeof joined)) {
            std_fs_note(errno);
            return -1;
        }
        jn = (int)strlen(joined);
        if (jn + 1 + (int)strlen(path) >= (int)sizeof joined) {
            std_fs_note(ERANGE);
            return -1;
        }
        joined[jn] = '/';
        strcpy(joined + jn + 1, path);
        jn = (int)strlen(joined);
    }

    n = 0;
    norm[n++] = '/';
    i = 0;
    while (i < jn) {
        while (i < jn && joined[i] == '/') i++;
        start = i;
        while (i < jn && joined[i] != '/') i++;
        len = i - start;
        if (len == 0) continue;
        if (len == 1 && joined[start] == '.') continue;
        if (len == 2 && joined[start] == '.' && joined[start + 1] == '.') {
            if (n > 1) {
                n--;
                while (n > 1 && norm[n - 1] != '/') n--;
                if (n > 1) n--;
            }
            continue;
        }
        if (n > 1) norm[n++] = '/';
        if (n + len >= (int)sizeof norm) {
            std_fs_note(ERANGE);
            return -1;
        }
        memcpy(norm + n, joined + start, (size_t)len);
        n += len;
    }
    norm[n] = 0;
    if (n + 1 > cap) {
        out[0] = 0;
        std_fs_note(ERANGE);
        return -1;
    }
    memcpy(out, norm, (size_t)n + 1);
    std_fs_note(0);
    return n;
}

static inline int std_fs_access_read(void) { return R_OK; }
static inline int std_fs_access_write(void) { return W_OK; }
static inline int std_fs_access_execute(void) { return X_OK; }
static inline int std_fs_access_exists(void) { return F_OK; }

/* A streaming scan: the callback sees each name (`.` and `..` skipped) and a
   non-zero return stops the walk. No array is allocated; the count is the
   number of callbacks that ran. */
typedef int (*std_fs_scan_fn)(const char *, void *);

static inline int std_fs_scan_directory(const char *path, void *callback, void *context)
{
    DIR *d;
    struct dirent *e;
    std_fs_scan_fn cb;
    int count;

    if (!path || !*path || !callback) {
        std_fs_note(EINVAL);
        return -1;
    }
    cb = (std_fs_scan_fn)callback;
    d = opendir(path);
    if (!d) {
        std_fs_note(errno);
        return -1;
    }
    count = 0;
    while ((e = readdir(d)) != 0) {
        if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) continue;
        count++;
        if (cb(e->d_name, context) != 0) break;
    }
    closedir(d);
    std_fs_note(0);
    return count;
}

#endif
