/*
 * Path.shim.h - the mapping unit's shim for std::io::Path.
 *
 * `std_io_open_path` is a thin wrapper around the CT factory in File.h.ct. It
 * exists because a `.m.ct` recipe can only call C symbols; including File.h
 * gives the generated prototype and the struct definition.
 *
 * License: MIT (see LICENSE).
 */

#ifndef CT_STD_IO_PATH_SHIM_H
#define CT_STD_IO_PATH_SHIM_H

#include "File.h"

static inline struct std_io_File *std_io_open_path(const char *path, const char *mode)
{
    return std_io_open_file(path, mode);
}

#endif
