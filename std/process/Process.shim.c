/*
 * Process.shim.c - the POSIX process surface in its own translation unit.
 *
 * popen / pclose / kill need _POSIX_C_SOURCE before the first system header;
 * a .h.ct/.m.ct shim header is included after other units' headers, so the
 * implementation lives here and Process.shim.h only declares it.
 *
 * License: MIT (see LICENSE).
 */

#define _POSIX_C_SOURCE 200809L

#include "Process.shim.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

int std_process_get_pid(void) { return (int)getpid(); }
int std_process_get_parent_pid(void) { return (int)getppid(); }
int std_process_get_uid(void) { return (int)getuid(); }
int std_process_get_effective_uid(void) { return (int)geteuid(); }
int std_process_get_gid(void) { return (int)getgid(); }
int std_process_get_effective_gid(void) { return (int)getegid(); }

int std_process_fork(void) { return (int)fork(); }

int std_process_execute(const char *path, char **argv)
{
    if (!path || !argv) return -1;
    execv(path, argv);
    return -1;
}

int std_process_execute_search(const char *file, char **argv)
{
    if (!file || !argv) return -1;
    execvp(file, argv);
    return -1;
}

int std_process_execute_environment(const char *path, char **argv, char **envp)
{
    if (!path || !argv || !envp) return -1;
    execve(path, argv, envp);
    return -1;
}

int std_process_system(const char *command)
{
    if (!command) return -1;
    return system(command);
}

void std_process_exit(int code)
{
    exit(code);
}

void std_process_abort(void)
{
    abort();
}

void *std_process_pipe_open(const char *command, const char *mode)
{
    if (!command || !mode) return NULL;
    return (void *)popen(command, mode);
}

int std_process_pipe_close(void *pipe)
{
    if (!pipe) return -1;
    /* pclose returns the child's wait status (not just 0/-1), so callers
       decode it with the wait-status helpers; -1 means pclose itself failed. */
    return (int)pclose((FILE *)pipe);
}

int std_process_wait_pid(int pid, int *status, int options)
{
    if (!status) return -1;
    return (int)waitpid((pid_t)pid, status, options);
}

int std_process_wait_any(int *status)
{
    if (!status) return -1;
    return (int)wait(status);
}

int std_process_wait_no_hang(void) { return WNOHANG; }
int std_process_is_exited(int status) { return WIFEXITED(status) ? 1 : 0; }
int std_process_is_signaled(int status) { return WIFSIGNALED(status) ? 1 : 0; }
int std_process_exit_status_code(int status)
{
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}
int std_process_termination_signal(int status)
{
    return WIFSIGNALED(status) ? WTERMSIG(status) : -1;
}

int std_process_signal(int signum, void *handler)
{
    struct sigaction action;

    if (signum <= 0) return -1;
    memset(&action, 0, sizeof action);
    action.sa_handler = (void (*)(int))handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    return sigaction(signum, &action, NULL) == 0 ? 0 : -1;
}

int std_process_raise_signal(int signum)
{
    return raise(signum) == 0 ? 0 : -1;
}

int std_process_kill(int pid, int signum)
{
    return kill((pid_t)pid, signum) == 0 ? 0 : -1;
}

int std_process_alarm(int seconds)
{
    if (seconds < 0) return -1;
    return (int)alarm((unsigned int)seconds);
}

int std_process_pause(void) { return pause(); }

int std_process_sigint(void) { return SIGINT; }
int std_process_sigterm(void) { return SIGTERM; }
int std_process_sigkill(void) { return SIGKILL; }
int std_process_sigalrm(void) { return SIGALRM; }
int std_process_sigchld(void) { return SIGCHLD; }
int std_process_sighup(void) { return SIGHUP; }
int std_process_sigusr1(void) { return SIGUSR1; }
int std_process_sigusr2(void) { return SIGUSR2; }
/* macOS has no real-time signals. The fallback keeps the factory contract
   (caller asks for "the real-time band") without inventing a new API; the
   README states the substitution. */
#ifdef SIGRTMIN
int std_process_sigrtmin(void) { return SIGRTMIN; }
int std_process_sigrtmax(void) { return SIGRTMAX; }
#else
int std_process_sigrtmin(void) { return SIGUSR2; }
int std_process_sigrtmax(void) { return SIGUSR2; }
#endif

