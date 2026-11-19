applies-to: v0.15.0

# v0-PH-11 设计

- `uname` / `statvfs` 的结构体只在 shim 内出现，字段逐个拷贝到调用方缓冲区/出参。
- `getrlimit` / `setrlimit` 用 `long` 出参与入参（`rlim_t` 在 32/64 位均能装下常用值）。
- `fnmatch` 直接返回 C 值（0 匹配；`FNM_NOMATCH` 不匹配；其他为错误）。
- `dlopen` 返回 `void *` 句柄；`dlsym` 返回 `void *`；`dlerror` 拷贝到调用方缓冲区。
- 不建 CT 值类型，不暴露 `struct utsname` / `struct statvfs` / `struct rlimit`。
