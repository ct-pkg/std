/*
 * Fs.shim.h - the C surface behind std::fs::FsOps.
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
 * (`String.shim.h` declares `std_str_*`) or with user code.
 *
 * Allocation: nothing here allocates. `readFile` and `list` write into the
 * caller's buffer and take its capacity; the caller owns the buffer and there is
 * nothing to free. `list` joins entries with '\n', skips `.` and `..`, and fails
 * (-1) rather than returning a partial listing.
 *
 * License: same as the rest of the compiler (see LICENSE / LICENSE.EXCEPTION).
 */

#ifndef CT_STD_FS_FS_SHIM_H
#define CT_STD_FS_FS_SHIM_H

#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

/* 1 when `path` names something `stat` can see, 0 otherwise. */
static inline int std_fs_exists(const char *path)
{
    struct stat st;

    if (!path || !*path) return 0;
    return stat(path, &st) == 0;
}

/* Read the whole file into `out`. Returns the byte count, or -1 when the file
   cannot be opened or does not fit `cap` (including the terminating NUL). */
static inline int std_fs_read_file(const char *path, char *out, int cap)
{
    FILE *f;
    int c;
    int n;

    if (!path || !out || cap <= 0) return -1;
    f = fopen(path, "rb");
    if (!f) return -1;
    n = 0;
    for (;;) {
        c = fgetc(f);
        if (c == EOF) break;
        if (n + 1 >= cap) {
            fclose(f);
            return -1;
        }
        out[n] = (char)c;
        n++;
    }
    out[n] = 0;
    fclose(f);
    return n;
}

/* Replace or create `path` with `data`; 0 on success, -1 on failure. */
static inline int std_fs_write_file(const char *path, const char *data)
{
    FILE *f;
    size_t len;

    if (!path || !data) return -1;
    f = fopen(path, "wb");
    if (!f) return -1;
    len = strlen(data);
    if (fwrite(data, 1, len, f) != len) {
        fclose(f);
        return -1;
    }
    if (fclose(f) != 0) return -1;
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

    if (!dir || !out || cap <= 0) return -1;
    d = opendir(dir);
    if (!d) return -1;
    n = 0;
    count = 0;
    while ((e = readdir(d)) != 0) {
        if (strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0) continue;
        len = (int)strlen(e->d_name);
        if (n + len + 2 > cap) {
            closedir(d);
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
    return count;
}

#endif
