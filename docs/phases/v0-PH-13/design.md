applies-to: v0.17.0

# v0-PH-13 设计

- `aiocb` 由 shim `calloc`，CT 侧只拿 `void *`；设置 fd/offset/buf/nbytes 后调用 `aio_read`/`aio_write`；
  `aio_error`/`aio_return`/`aio_cancel`/`aio_suspend` 直接转发。
- `posix_spawn` 只接受单路径：shim 构造 `argv = {path, 0}`，`spawn_wait` 等待并返回状态，`spawn_pid` 返回 pid。
- mq 在 macOS 不可用，归 `v0-PH-14`。
