applies-to: v0.12.0

# 依赖清单

| 依赖 | 版本 | 说明 |
| --- | --- | --- |
| ct | `>= 0.67.0` | 唯一编译器依赖 |
| libc / libm | 宿主 | macOS libSystem；Linux glibc + libm |
| Podman | `localhost/ct-ci:24.04` | 仅验证用，不进入包 |
