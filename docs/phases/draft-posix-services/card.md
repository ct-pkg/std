# draft-posix-services：POSIX 服务

| 项 | 值 |
| --- | --- |
| phase-id | `draft-posix-services` |
| status | draft |
| 目标 | 补 POSIX 的 aio / mq / shm / sched / 资源与信息 / glob / dlfcn / spawn / termios 等服务 |
| 规范版本 | 1.17.0 |
| 依赖 | `v0-PH-10` 收口；D-002 |

## 范围

- `aio`：`aiocb` 不透明句柄 + 访问器；read/write/error/return/suspend/cancel/fsync。
- `mqueue` / `shm` / `semaphore`（若未在 PH-10）/ `sched`。
- `sys/resource` / `sys/utsname` / `unistd` 的 `sysconf` / `getrusage` / `times`。
- `glob` / `fnmatch` / `dlfcn` / `spawn` / `pwd` / `grp` / `termios` / `statvfs`。
- `regex` 作为可选子项，按真实调用点决定。

## 不做什么

- 不建 CT 值类型；`aiocb` / `glob_t` / `termios` 只给不透明句柄。
- 不做 Windows 兼容层；不封装 `pthread`（在 PH-10）。

## 已知风险

| 风险 | 触发信号 |
| --- | --- |
| `aio` 在 macOS 不完整 | 编译/链接失败 |
| `regex` 语义与 CT 字符串边界冲突 | 需要额外所有权规则 |
| `dlfcn` 的符号生命周期 | 句柄关闭后函数指针悬垂 |
