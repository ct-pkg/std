/* uname / rlimit / statvfs / fnmatch / dlfcn. License: MIT (see LICENSE). */
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
#ifndef CT_STD_POSIX_SHIM_H
#define CT_STD_POSIX_SHIM_H
#include <sys/utsname.h>
#include <sys/resource.h>
#include <sys/statvfs.h>
#include <fnmatch.h>
#include <dlfcn.h>
#include <string.h>
#include <stddef.h>

static inline int std_posix_uname_field(int field, char *out, int cap)
{
    struct utsname u;
    const char *p;
    if (!out || cap <= 0 || uname(&u) != 0) return -1;
    switch (field) {
    case 0: p = u.sysname; break;
    case 1: p = u.nodename; break;
    case 2: p = u.release; break;
    case 3: p = u.version; break;
    default: p = u.machine; break;
    }
    if ((int)strlen(p) + 1 > cap) return -1;
    strcpy(out, p);
    return 0;
}
static inline int std_posix_rlimit_get(int resource, int which, long *out)
{
    struct rlimit r;
    if (!out || getrlimit(resource, &r) != 0) return -1;
    *out = (long)(which == 0 ? r.rlim_cur : r.rlim_max);
    return 0;
}
static inline int std_posix_rlimit_set(int resource, long cur, long max)
{
    struct rlimit r;
    r.rlim_cur = (rlim_t)cur;
    r.rlim_max = (rlim_t)max;
    return setrlimit(resource, &r) == 0 ? 0 : -1;
}
static inline int std_posix_rlimit_cpu(void) { return RLIMIT_CPU; }
static inline int std_posix_rlimit_fsize(void) { return RLIMIT_FSIZE; }
static inline int std_posix_rlimit_data(void) { return RLIMIT_DATA; }
static inline int std_posix_rlimit_stack(void) { return RLIMIT_STACK; }
static inline int std_posix_rlimit_nofile(void) { return RLIMIT_NOFILE; }
static inline int std_posix_statvfs(const char *path, long *bsize, long *blocks, long *bfree, long *bavail)
{
    struct statvfs v;
    if (!path || statvfs(path, &v) != 0) return -1;
    if (bsize) *bsize = (long)v.f_bsize;
    if (blocks) *blocks = (long)v.f_blocks;
    if (bfree) *bfree = (long)v.f_bfree;
    if (bavail) *bavail = (long)v.f_bavail;
    return 0;
}
static inline int std_posix_fnmatch(const char *pattern, const char *name, int flags)
{
    if (!pattern || !name) return -1;
    return fnmatch(pattern, name, flags);
}
static inline int std_posix_fnmatch_nomatch(void) { return FNM_NOMATCH; }
static inline void *std_posix_dlopen(const char *path, int flags)
{
    return dlopen(path, flags);
}
static inline void *std_posix_dlsym(void *handle, const char *name)
{
    return dlsym(handle, name);
}
static inline int std_posix_dlclose(void *handle)
{
    return handle && dlclose(handle) == 0 ? 0 : -1;
}
static inline int std_posix_dlerror(char *out, int cap)
{
    const char *e = dlerror();
    if (!e) { if (out && cap > 0) out[0] = 0; return 0; }
    if (!out || cap <= 0 || (int)strlen(e) + 1 > cap) return -1;
    strcpy(out, e);
    return 0;
}
static inline int std_posix_rtld_now(void) { return RTLD_NOW; }
static inline int std_posix_rtld_lazy(void) { return RTLD_LAZY; }
#endif
