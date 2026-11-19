applies-to: v0.11.0

# draft-posix-core：POSIX 线程 / 同步 / 内存映射 / 多路复用

| 项 | 值 |
| --- | --- |
| phase-id | `draft-posix-core` |
| status | draft |
| 目标 | 把 pthread、semaphore、mmap、poll/select 按 ① 直连 + 不透明句柄纳入 std |
| 规范版本 | 1.17.0 |
| 依赖 | `v0-PH-8` 收口；D-002 |

## 范围

- `pthread`：线程、mutex、cond、rwlock、once、key；线程启动回调走 `void *`。
- `semaphore`：`sem_init` / `sem_wait` / `sem_post` / `sem_destroy`。
- `sys/mman`：`mmap` / `munmap` / `mprotect` / `msync` / `madvise`。
- `poll` / `sys/select`：`pollfd` 数组与 `fd_set` 由 shim 分配，CT 侧拿 `void *` 句柄。

## 不做什么

- 不建 `Mutex` / `Cond` / `PollSet` 等 CT 值类型；不承诺跨平台统一布局。
- 不封装 `pthread_attr_t` 的全部字段；只给常用设置器。

## 已知风险

| 风险 | 触发信号 |
| --- | --- |
| 结构体大小/布局 CT 不可见 | 必须 malloc；栈上不可用 |
| 线程回调无法捕获闭包 | 只能顶层函数 + `void *` 参数 |
| `select` 的 `fd_set` 上限 | `FD_SETSIZE` 平台差异 |
