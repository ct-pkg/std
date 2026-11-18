/*
 * DirFactory.shim.h - the mapping unit's shim for std::fs::PathDir.
 *
 * It includes Dir.h so the CT factory prototype and the struct definition are
 * visible, then forwards under a separate C name.
 *
 * License: MIT (see LICENSE).
 */

/* The POSIX.1-2008 feature set: shims are the first headers in a generated
   consumer, so defining it here makes mkdir/readlink/telldir/fileno/setenv and
   friends visible under strict -std=c99 on glibc. */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#ifndef CT_STD_FS_DIRFACTORY_SHIM_H
#define CT_STD_FS_DIRFACTORY_SHIM_H

#include "Dir.h"

static inline struct std_fs_Dir *std_fs_open_dir_path(const char *path)
{
    return std_fs_open_dir(path);
}

#endif
