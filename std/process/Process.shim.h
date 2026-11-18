/*
 * Process.shim.h - declarations for the std::process C core.
 *
 * The implementation is in Process.shim.c so _POSIX_C_SOURCE precedes every
 * system header; this header exposes no system types.
 *
 * License: MIT (see LICENSE).
 */

#ifndef CT_STD_PROCESS_PROCESS_SHIM_H
#define CT_STD_PROCESS_PROCESS_SHIM_H

int std_process_get_pid(void);
int std_process_get_parent_pid(void);
int std_process_get_uid(void);
int std_process_get_effective_uid(void);
int std_process_get_gid(void);
int std_process_get_effective_gid(void);
int std_process_fork(void);
int std_process_execute(const char *path, char **argv);
int std_process_execute_search(const char *file, char **argv);
int std_process_execute_environment(const char *path, char **argv, char **envp);
int std_process_system(const char *command);
void std_process_exit(int code);
void std_process_abort(void);
void * std_process_pipe_open(const char *command, const char *mode);
int std_process_pipe_close(void *pipe);
int std_process_wait_pid(int pid, int *status, int options);
int std_process_wait_any(int *status);
int std_process_wait_no_hang(void);
int std_process_is_exited(int status);
int std_process_is_signaled(int status);
int std_process_exit_status_code(int status);
int std_process_termination_signal(int status);
int std_process_signal(int signum, void *handler);
int std_process_raise_signal(int signum);
int std_process_kill(int pid, int signum);
int std_process_alarm(int seconds);
int std_process_pause(void);
int std_process_sigint(void);
int std_process_sigterm(void);
int std_process_sigkill(void);
int std_process_sigalrm(void);
int std_process_sigchld(void);
int std_process_sighup(void);
int std_process_sigusr1(void);
int std_process_sigusr2(void);
int std_process_sigrtmin(void);
int std_process_sigrtmax(void);

#endif
