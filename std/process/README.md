# `std::process` — 进程身份、控制与信号

`fork` / `execute*` / `system` / `wait*`、`getpid` 一族、`signal` / `alarm` 与 `pipe_open` 的
直映射。所有入口都挂在 namespace 上：`std::process::Process.get_pid()`。

## 1. 定位与边界

- 只做 POSIX / C 的直连，不引入新的进程类型，也不接管子进程生命周期。
- `fork` 返回 `0`（子进程）/ 子进程 pid（父进程）/ `-1`（失败）。
- `execute*` 成功即替换进程镜像，**只在失败时返回 `-1`**；不会返回。
- `system` / `pipe_open` 经过 shell，命令串由调用方负责。
- `pipe_open` 返回 `void *`（内部是 `FILE *`），只能交给 `pipe_close`；它不属于
  `std::io::File`，两者不可混用。
- 信号回调经 `void *` 通道传递（与 `std::search` 的比较器同一机制），只能是顶层
  `void handler(int)` 函数，没有闭包。
- 平台差异：`sigrtmin()` / `sigrtmax()` 在没有实时信号的平台上（macOS）回退为 `SIGUSR2`；
  README 与测试都按返回值使用，不假设具体数值。

## 2. 依赖与安装

依赖 `<unistd.h>` / `<signal.h>` / `<sys/wait.h>` / `<stdio.h>` / `<stdlib.h>`；
`requires ct >= 0.64.0`。

| 文件 | 内容 |
| --- | --- |
| `Process.m.ct` | `Process` trait + `map trait Process for namespace std::process` |
| `Process.shim.h` | 全部 `std_process_*` 的 `static inline` 实现 |

引入方式：`import std::process::Process;`。

## 3. 快速开始

```ct
import std::process::Process;

int main(void)
{
    int status = 0;
    int pid = std::process::Process.fork();

    if (pid == 0) {
        std::process::Process.exit_process(7);
    }
    std::process::Process.wait_pid(pid, &status, 0);
    return std::process::Process.exit_status_code(status) == 7 ? 0 : 1;
}
```

## 4. API 全表

### 4.1 身份

| 方法 | 语义 | 返回 |
| --- | --- | --- |
| `get_pid()` | `getpid` | pid |
| `get_parent_pid()` | `getppid` | ppid |
| `get_uid()` / `get_effective_uid()` | `getuid` / `geteuid` | uid |
| `get_gid()` / `get_effective_gid()` | `getgid` / `getegid` | gid |

### 4.2 进程控制

| 方法 | 签名 | 语义 | 失败 |
| --- | --- | --- | --- |
| `fork()` | `int fork()` | 0 子 / pid 父 | `-1` |
| `execute(path, argv)` | `int execute(const char *path, char **argv)` | `execv` | 只返回 `-1` |
| `execute_search(file, argv)` | `int execute_search(const char *file, char **argv)` | `execvp`（查 `PATH`） | 只返回 `-1` |
| `execute_with_environment(path, argv, envp)` | `int execute_with_environment(const char *, char **, char **)` | `execve` | 只返回 `-1` |
| `system(command)` | `int system(const char *command)` | 经 shell，返回 wait status | `-1` |
| `exit_process(code)` | `void exit_process(int code)` | `exit`，不返回 | — |
| `abort_process()` | `void abort_process()` | `abort`，不返回 | — |

`argv` / `envp` 是 NULL 结尾的 `char *` 数组，与 C 相同。

### 4.3 等待

| 方法 | 签名 | 语义 |
| --- | --- | --- |
| `wait_pid(pid, &status, options)` | `int wait_pid(int pid, int *status, int options)` | `waitpid`；子进程 id 或 `-1` |
| `wait_any(&status)` | `int wait_any(int *status)` | `wait` |
| `wait_no_hang()` | `int wait_no_hang()` | `WNOHANG` |
| `is_exited(status)` / `is_signaled(status)` | `int` | `WIFEXITED` / `WIFSIGNALED` |
| `exit_status_code(status)` | `int` | `WEXITSTATUS`；未正常退出返回 `-1` |
| `termination_signal(status)` | `int` | `WTERMSIG`；未被信号终止返回 `-1` |

### 4.4 信号与管道

| 方法 | 签名 | 语义 | 失败 |
| --- | --- | --- | --- |
| `signal(signum, handler)` | `int signal(int signum, void *handler)` | 装 `void (*)(int)` 处理器 | `-1` |
| `raise_signal(signum)` | `int raise_signal(int signum)` | `raise` | `-1` |
| `kill(pid, signum)` | `int kill(int pid, int signum)` | `kill`；`signum == 0` 是存在性探测 | `-1` |
| `alarm(seconds)` | `int alarm(int seconds)` | 返回上次剩余秒数 | `-1`（负参数） |
| `pause()` | `int pause()` | 挂起至信号 | `-1` |
| `pipe_open(command, mode)` | `void *pipe_open(const char *, const char *)` | `popen` | `0` |
| `pipe_close(pipe)` | `int pipe_close(void *pipe)` | `pclose`，返回 wait status | `-1`（`pclose` 自身失败） |

信号编号以函数给出：`sigint` / `sigterm` / `sigkill` / `sigalrm` / `sigchld` / `sighup` /
`sigusr1` / `sigusr2` / `sigrtmin` / `sigrtmax`。

## 5. 与相邻模块的关系

- `std::env` 管当前进程的环境变量；`execute_with_environment` 的 `envp` 是子进程镜像的数组。
- `std::io::File` 拥有 `FILE *`；`pipe_open` 的 `void *` 不归它管，关法只有 `pipe_close`。
- `std::time::Timer` 到期后发的就是本模块给出的信号；没有回调层。

## 6. 限制与未支持

- **`fork` 后的运行时状态不可假定安全**：子进程里只应调用 async-signal-safe 的操作，
  或立刻 `execute*` / `exit_process`。
- **`execute*` 不解析 `argv`**：数组由调用方构造，且必须在调用期间有效。
- **`system` / `pipe_open` 经 shell**：命令注入与引用规则由调用方负责。
- **回调只支持顶层函数**：CT 没有跨 C 边界的闭包；处理器运行在信号上下文，只能调用
  async-signal-safe 函数。
- **没有实时信号的平台**：`sigrtmin()` / `sigrtmax()` 返回 `SIGUSR2`。
- **不等待进程组**：`wait_pid` 只映射 `waitpid`，没有 `waitid` / `wait4`。

## 7. 版本与变更记录

| 版本 | 状态 | 变更 |
| --- | --- | --- |
| `0.6.0` | 本轮新增 | 新建模块：身份、`fork` / `execute*` / `system`、`wait*`、信号、`pipe_open`。 |

编译器下限 `ct >= 0.64.0`。
