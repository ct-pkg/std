/*
 * Stat.shim.h - the struct stat field extraction behind std::fs::Stat.
 *
 * A `.h.ct` unit cannot include <sys/stat.h>, and this shim cannot include the
 * generated Stat.h (it is included from there, so the guard is already set when
 * the struct is still undefined). The fill function therefore takes one out
 * pointer per field, and the CT body does the assignment.
 *
 * `follow` is 1 for stat(2) and 0 for lstat(2). `kind` is 0 regular, 1
 * directory, 2 symlink, 3 other, so the CT predicates do not need the S_IS*
 * macros.
 *
 * License: MIT (see LICENSE).
 */

/* The POSIX.1-2008 feature set: shims are the first headers in a generated
   consumer, so defining it here makes mkdir/readlink/telldir/fileno/setenv and
   friends visible under strict -std=c99 on glibc. */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#ifndef CT_STD_FS_STAT_SHIM_H
#define CT_STD_FS_STAT_SHIM_H

#include <sys/stat.h>
#include <errno.h>

static inline int std_fs_stat_fill(const char *path, int follow,
                                   long *size, int *mode, int *kind,
                                   long *mtime, long *atime, long *ctime,
                                   int *uid, int *gid, int *device,
                                   long *inode, int *links,
                                   long *block_size, long *blocks)
{
    struct stat st;

    if (!path || !*path || !size || !mode || !kind) {
        errno = EINVAL;
        return -1;
    }
    if ((follow ? stat(path, &st) : lstat(path, &st)) != 0) return -1;
    *size = (long)st.st_size;
    *mode = (int)(st.st_mode & 07777);
    *mtime = (long)st.st_mtime;
    *atime = (long)st.st_atime;
    *ctime = (long)st.st_ctime;
    *uid = (int)st.st_uid;
    *gid = (int)st.st_gid;
    *device = (int)st.st_dev;
    *inode = (long)st.st_ino;
    *links = (int)st.st_nlink;
    *block_size = (long)st.st_blksize;
    *blocks = (long)st.st_blocks;
    if (S_ISREG(st.st_mode)) *kind = 0;
    else if (S_ISDIR(st.st_mode)) *kind = 1;
    else if (S_ISLNK(st.st_mode)) *kind = 2;
    else *kind = 3;
    return 0;
}

#endif
