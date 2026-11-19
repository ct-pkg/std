applies-to: v0.15.0

# v0-PH-12 设计（进行中）

- pwd/grp：`struct passwd` / `struct group` 只在 shim 内出现，字段拷贝到调用方缓冲区。
- sched：直接返回 `sched_get_priority_max/min` 与 `sched_yield`。
- shm：`shm_open` 返回 fd，交给 `std::mmap` 映射。
- termios：`struct termios` 用 `void *` 句柄，`tcgetattr`/`tcsetattr` 拷贝。
- glob：`glob_t` 用 `void *` 句柄，访问器读出路径与数量。
- aio/mq/spawn：先探针，平台不完整则 deferred 到 PH-13。
