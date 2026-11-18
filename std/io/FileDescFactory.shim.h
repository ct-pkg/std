/*
 * FileDescFactory.shim.h - the mapping unit's shim.
 *
 * It includes FileDesc.h so the CT factory prototype and the struct definition
 * are visible, then forwards to it. The separate name (`std_io_open_desc_path`)
 * avoids colliding with the CT factory's own C symbol.
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

#ifndef CT_STD_IO_FILEDESCFACTORY_SHIM_H
#define CT_STD_IO_FILEDESCFACTORY_SHIM_H

#include "FileDesc.h"

static inline struct std_io_FileDesc *std_io_open_desc_path(const char *path, int flags)
{
    return std_io_open_as_file_desc(path, flags);
}

#endif
