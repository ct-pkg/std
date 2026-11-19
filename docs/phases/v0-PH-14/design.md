applies-to: v0.18.0

# v0-PH-14 设计

- Linux：`mqd_t` 是 fd，直接返回 `int`；`mq_open` 用 `mq_attr` 设置 maxmsg/msgsize。
- macOS：没有 `<mqueue.h>`，shim 编译为 `mq_supported() == 0`，五个函数返回 -1。
- 不暴露 `mq_attr` / `mqd_t`；调用方只拿 int 句柄与缓冲区。
