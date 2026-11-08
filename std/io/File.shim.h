#ifndef STD_IO_FILE_SHIM_H
#define STD_IO_FILE_SHIM_H

#include <stdio.h>
#include <stdlib.h>

/* The handle is opaque to CT: callers only ever hold an `IoFile *`, and
   File.m.ct declares `pub type IoFile = struct ct_io_file;`. `ct_io_file`
   wraps `FILE *` instead of exposing it and stays private to this shim, so no
   cast appears at the call site and no `-Wincompatible-pointer-types` warning
   is raised.

   The C tag and every `std_io_*` symbol stay as they are. */
typedef struct ct_io_file ct_io_file;

struct ct_io_file {
    FILE *fp;
};

static inline ct_io_file *std_io_open(const char *path, const char *mode) {
    ct_io_file *f;
    if (!path || !mode) return NULL;
    f = (ct_io_file *)malloc(sizeof *f);
    if (!f) return NULL;
    f->fp = fopen(path, mode);
    if (!f->fp) {
        free(f);
        return NULL;
    }
    return f;
}

static inline int std_io_read(ct_io_file *f, char *buf, int n) {
    if (!f || !f->fp || !buf || n < 0) return -1;
    return (int)fread(buf, 1, (size_t)n, f->fp);
}

static inline int std_io_write(ct_io_file *f, const char *buf, int n) {
    if (!f || !f->fp || n < 0 || (!buf && n > 0)) return -1;
    return (int)fwrite(buf, 1, (size_t)n, f->fp);
}

/* Idempotent by design: a NULL or already-closed handle is a no-op success.
   The wrapper itself is deliberately not freed, so the second call never
   touches released memory. See README.md, "内存与所有权". */
static inline int std_io_close(ct_io_file *f) {
    int rc;
    if (!f) return 0;
    if (!f->fp) return 0;
    rc = fclose(f->fp);
    f->fp = NULL;
    return rc == 0 ? 0 : -1;
}

#endif
