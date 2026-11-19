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
#include <pwd.h>
#include <grp.h>
#include <sched.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <glob.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>

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
static inline int std_posix_pwd_field(int uid, int field, char *out, int cap)
{
    struct passwd *p = getpwuid((uid_t)uid);
    const char *v;
    if (!p || !out || cap <= 0) return -1;
    switch (field) {
    case 0: v = p->pw_name; break;
    case 1: v = p->pw_dir; break;
    default: v = p->pw_shell; break;
    }
    if (!v) v = "";
    if ((int)strlen(v) + 1 > cap) return -1;
    strcpy(out, v);
    return 0;
}
static inline int std_posix_grp_name(int gid, char *out, int cap)
{
    struct group *g = getgrgid((gid_t)gid);
    if (!g || !out || cap <= 0) return -1;
    if ((int)strlen(g->gr_name) + 1 > cap) return -1;
    strcpy(out, g->gr_name);
    return 0;
}
static inline int std_posix_sched_yield(void) { return sched_yield() == 0 ? 0 : -1; }
static inline int std_posix_sched_max(int policy) { return sched_get_priority_max(policy); }
static inline int std_posix_sched_min(int policy) { return sched_get_priority_min(policy); }
static inline int std_posix_sched_other(void) { return SCHED_OTHER; }
static inline int std_posix_sched_fifo(void) { return SCHED_FIFO; }
static inline int std_posix_sched_rr(void) { return SCHED_RR; }


static inline int std_posix_shm_open(const char *name, int flags, int mode)
{
    if (!name) return -1;
    return shm_open(name, flags, (mode_t)mode);
}
static inline int std_posix_shm_unlink(const char *name)
{
    return name && shm_unlink(name) == 0 ? 0 : -1;
}
static inline int std_posix_shm_truncate(int fd, long length)
{
    return fd >= 0 && length >= 0 && ftruncate(fd, (off_t)length) == 0 ? 0 : -1;
}
static inline int std_posix_shm_close(int fd) { return fd >= 0 && close(fd) == 0 ? 0 : -1; }
static inline int std_posix_o_creat(void) { return O_CREAT; }
static inline int std_posix_o_rdwr(void) { return O_RDWR; }

static inline void *std_posix_glob_new(void) { return calloc(1, sizeof(glob_t)); }
static inline int std_posix_glob_run(void *handle, const char *pattern)
{
    if (!handle || !pattern) return -1;
    return glob(pattern, 0, 0, (glob_t *)handle) == 0 ? 0 : -1;
}
static inline int std_posix_glob_count(void *handle)
{
    return handle ? (int)((glob_t *)handle)->gl_pathc : -1;
}
static inline const char *std_posix_glob_path(void *handle, int index)
{
    glob_t *g = (glob_t *)handle;
    if (!g || index < 0 || index >= (int)g->gl_pathc) return 0;
    return g->gl_pathv[index];
}
static inline void std_posix_glob_free(void *handle)
{
    if (!handle) return;
    globfree((glob_t *)handle);
    free(handle);
}

static inline int std_posix_isatty(int fd) { return isatty(fd) ? 1 : 0; }
static inline void *std_posix_tty_new(void) { return calloc(1, sizeof(struct termios)); }
static inline int std_posix_tty_get(int fd, void *handle)
{
    if (fd < 0 || !handle) return -1;
    return tcgetattr(fd, (struct termios *)handle) == 0 ? 0 : -1;
}
static inline int std_posix_tty_set(int fd, void *handle)
{
    if (fd < 0 || !handle) return -1;
    return tcsetattr(fd, TCSANOW, (struct termios *)handle) == 0 ? 0 : -1;
}
static inline int std_posix_tty_echo(void *handle)
{
    return handle && (((struct termios *)handle)->c_lflag & ECHO) ? 1 : 0;
}
static inline int std_posix_tty_set_echo(void *handle, int on)
{
    struct termios *t = (struct termios *)handle;
    if (!t) return -1;
    if (on) t->c_lflag |= ECHO; else t->c_lflag &= ~ECHO;
    return 0;
}
static inline void std_posix_tty_free(void *handle) { free(handle); }


#endif
