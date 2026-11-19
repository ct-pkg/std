/* sys/mman surface. License: MIT (see LICENSE). */
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
#ifndef CT_STD_MMAP_SHIM_H
#define CT_STD_MMAP_SHIM_H
#include <sys/mman.h>
#include <stddef.h>
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

static inline void *std_mmap_map(int fd, long length, int prot, int flags, long offset)
{
    if (length <= 0 || offset < 0) return 0;
    return mmap(0, (size_t)length, prot, flags, fd, (off_t)offset);
}
static inline int std_mmap_unmap(void *addr, long length)
{
    if (!addr || length <= 0) return -1;
    return munmap(addr, (size_t)length) == 0 ? 0 : -1;
}
static inline int std_mmap_protect(void *addr, long length, int prot)
{
    if (!addr || length <= 0) return -1;
    return mprotect(addr, (size_t)length, prot) == 0 ? 0 : -1;
}
static inline int std_mmap_sync(void *addr, long length, int flags)
{
    if (!addr || length <= 0) return -1;
    return msync(addr, (size_t)length, flags) == 0 ? 0 : -1;
}
static inline int std_mmap_prot_read(void) { return PROT_READ; }
static inline int std_mmap_prot_write(void) { return PROT_WRITE; }
static inline int std_mmap_prot_exec(void) { return PROT_EXEC; }
static inline int std_mmap_prot_none(void) { return PROT_NONE; }
static inline int std_mmap_shared(void) { return MAP_SHARED; }
static inline int std_mmap_private(void) { return MAP_PRIVATE; }
static inline int std_mmap_anonymous(void) { return MAP_ANONYMOUS; }
static inline int std_mmap_sync_sync(void) { return MS_SYNC; }
static inline int std_mmap_sync_async(void) { return MS_ASYNC; }
#endif
