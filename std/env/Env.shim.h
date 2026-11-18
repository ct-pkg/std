/*
 * Env.shim.h - getenv / setenv / unsetenv behind std::env.
 *
 * The copy-out policy lives here rather than in the mapping unit: a `.m.ct`
 * accepts only `#include` and `#define` in its prologue, so a function body has
 * to be in the shim.
 *
 * `std_env_get` returns -1 when the name is missing or the value (plus its NUL)
 * does not fit `cap`; the buffer is cleared in both cases. A present but empty
 * value returns 0. `std_env_set` always overwrites (`setenv` with overwrite=1).
 *
 * License: MIT (see LICENSE).
 */

#ifndef CT_STD_ENV_ENV_SHIM_H
#define CT_STD_ENV_ENV_SHIM_H

#include <stdlib.h>
#include <string.h>

static inline int std_env_get(const char *name, char *out, int cap)
{
    const char *value;
    size_t len;

    if (!name || !*name || !out || cap <= 0) return -1;
    value = getenv(name);
    if (!value) {
        out[0] = 0;
        return -1;
    }
    len = strlen(value);
    if ((int)len + 1 > cap) {
        out[0] = 0;
        return -1;
    }
    memcpy(out, value, len + 1);
    return (int)len;
}

static inline int std_env_set(const char *name, const char *value)
{
    if (!name || !*name || !value) return -1;
    return setenv(name, value, 1) == 0 ? 0 : -1;
}

static inline int std_env_unset(const char *name)
{
    if (!name || !*name) return -1;
    return unsetenv(name) == 0 ? 0 : -1;
}

#endif
