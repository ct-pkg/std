/*
 * Process.shim.h - the POSIX process surface behind std::process.
 *
 * `fork` / `execute*` / `system` are POSIX or C89; the `execute*` trio takes a
 * NULL-terminated `char *argv[]`, exactly like C. They only return on failure.
 *
 * `pipe_open` hides a `FILE *` behind `void *` because a mapping unit cannot
 * name a type from another unit; it is the only borrowed handle here and
 * `std_process_pipe_close` must be used to release it.
 *
 * `signal` receives a function pointer through `void *` (CT passes a top-level
 * function name); the cast back to `void (*)(int)` is the same contract
 * std::search uses for its comparator.
 *
 * The wait-status helpers are the `W*` macros turned into functions so a CT
 * caller does not need the macros. `sigrtmin()` / `sigrtmax()` are functions
 * because `SIGRTMIN` is a function call on glibc and a constant elsewhere.
 *
 * License: MIT (see LICENSE).
 */

#ifndef CT_STD_PROCESS_PROCESS_SHIM_H
#define CT_STD_PROCESS_PROCESS_SHIM_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

static inline int std_process_get_pid(void) { return (int)getpid(); }
static inline int std_process_get_parent_pid(void) { return (int)getppid(); }
static inline int std_process_get_uid(void) { return (int)getuid(); }
static inline int std_process_get_effective_uid(void) { return (int)geteuid(); }
static inline int std_process_get_gid(void) { return (int)getgid(); }
static inline int std_process_get_effective_gid(void) { return (int)getegid(); }

static inline int std_process_fork(void) { return (int)fork(); }

static inline int std_process_execute(const char *path, char **argv)
{
    if (!path || !argv) return -1;
    execv(path, argv);
    return -1;
}

static inline int std_process_execute_search(const char *file, char **argv)
{
    if (!file || !argv) return -1;
    execvp(file, argv);
    return -1;
}

static inline int std_process_execute_environment(const char *path, char **argv, char **envp)
{
    if (!path || !argv || !envp) return -1;
    execve(path, argv, envp);
    return -1;
}

static inline int std_process_system(const char *command)
{
    if (!command) return -1;
    return system(command);
}

static inline void std_process_exit(int code)
{
    exit(code);
}

static inline void std_process_abort(void)
{
    abort();
}

static inline void *std_process_pipe_open(const char *command, const char *mode)
{
    if (!command || !mode) return NULL;
    return (void *)popen(command, mode);
}

static inline int std_process_pipe_close(void *pipe)
{
    if (!pipe) return -1;
    /* pclose returns the child's wait status (not just 0/-1), so callers
       decode it with the wait-status helpers; -1 means pclose itself failed. */
    return (int)pclose((FILE *)pipe);
}

static inline int std_process_wait_pid(int pid, int *status, int options)
{
    if (!status) return -1;
    return (int)waitpid((pid_t)pid, status, options);
}

static inline int std_process_wait_any(int *status)
{
    if (!status) return -1;
    return (int)wait(status);
}

static inline int std_process_wait_no_hang(void) { return WNOHANG; }
static inline int std_process_is_exited(int status) { return WIFEXITED(status) ? 1 : 0; }
static inline int std_process_is_signaled(int status) { return WIFSIGNALED(status) ? 1 : 0; }
static inline int std_process_exit_status_code(int status)
{
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}
static inline int std_process_termination_signal(int status)
{
    return WIFSIGNALED(status) ? WTERMSIG(status) : -1;
}

static inline int std_process_signal(int signum, void *handler)
{
    if (signum <= 0) return -1;
    if (signal(signum, (void (*)(int))handler) == SIG_ERR) return -1;
    return 0;
}

static inline int std_process_raise_signal(int signum)
{
    return raise(signum) == 0 ? 0 : -1;
}

static inline int std_process_kill(int pid, int signum)
{
    return kill((pid_t)pid, signum) == 0 ? 0 : -1;
}

static inline int std_process_alarm(int seconds)
{
    if (seconds < 0) return -1;
    return (int)alarm((unsigned int)seconds);
}

static inline int std_process_pause(void) { return pause(); }

static inline int std_process_sigint(void) { return SIGINT; }
static inline int std_process_sigterm(void) { return SIGTERM; }
static inline int std_process_sigkill(void) { return SIGKILL; }
static inline int std_process_sigalrm(void) { return SIGALRM; }
static inline int std_process_sigchld(void) { return SIGCHLD; }
static inline int std_process_sighup(void) { return SIGHUP; }
static inline int std_process_sigusr1(void) { return SIGUSR1; }
static inline int std_process_sigusr2(void) { return SIGUSR2; }
/* macOS has no real-time signals. The fallback keeps the factory contract
   (caller asks for "the real-time band") without inventing a new API; the
   README states the substitution. */
#ifdef SIGRTMIN
static inline int std_process_sigrtmin(void) { return SIGRTMIN; }
static inline int std_process_sigrtmax(void) { return SIGRTMAX; }
#else
static inline int std_process_sigrtmin(void) { return SIGUSR2; }
static inline int std_process_sigrtmax(void) { return SIGUSR2; }
#endif

#endif
