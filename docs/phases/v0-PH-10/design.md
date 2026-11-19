applies-to: v0.14.0

# v0-PH-10 设计：不透明句柄

| 类型 | 表示 | 释放 | 说明 |
| --- | --- | --- | --- |
| `pthread_mutex_t` / `pthread_cond_t` | `void *` | `mutex_free` / `cond_free` | shim 分配，CT 不见布局 |
| `pthread_t` | `void *` | `join` 内部释放 | `create` 返回句柄，回调 `void *(*)(void *)` |
| 匿名 semaphore | `void *` | `sem_free` | mutex + cond 计数，避开 macOS `sem_init` |
| `pollfd` 数组 | `void *` | `free_set` | `set_fd` / `events` 访问器 |
| `fd_set` | `void *` | `fd_set_free` | `select_wait` 接收三个集合 |
| `mmap` 区 | `void *` | `unmap` | 匿名映射用 `MAP_ANON` 回退 |

平台差异：macOS 没有 `MAP_ANONYMOUS`（用 `MAP_ANON`）、`MADV_*` 常量不全，`madvise` 记 deferred；
`sem_init` 在 macOS 不可用，因此用 pthread 自建信号量。
