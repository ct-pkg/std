# 平台验证记录（0.11.0）

| 项 | 值 |
| --- | --- |
| 日期 | 2026-11-19 |
| 编译器 | **ct v0.67.0**（`requires = ct >= 0.67.0` 的唯一验证版本） |
| macOS | arm64，clang，`bash tests/run.sh` **20/20** |
| Linux | Ubuntu 24.04，gcc 13，Podman 镜像 `localhost/ct-ci:24.04`，`bash tests/run.sh` **20/20，0 warnings** |
| CI | `.github/workflows/test.yml` 的 host 矩阵钉 `CT_REF: v0.67.0`（ubuntu/gcc + macos/clang）；`compiler-head` 仍跟 ct-lang `main` |

## 复现命令

macOS（本机）：

```sh
cd /Users/samxx/trae-ws/std
ct --version        # ct v0.67.0
bash tests/run.sh
```

Linux（Podman，ct 从 ct-lang tag 构建一次后复用 volume `ct67`）：

```sh
podman run --rm -v "$PWD":/w -v ct67:/ct -w /w localhost/ct-ci:24.04 bash -lc '
  test -x /ct/ct || {
    git clone --depth 1 --branch v0.67.0 https://github.com/samxxu/ct-lang /tmp/ct-lang
    bash tools/bootstrap-ct.sh /tmp/ct-lang /ct
  }
  /ct/ct --version
  CC=gcc CT=/ct/ct bash tests/run.sh
'
```

## 这一轮修掉的 Linux 专属问题

| 问题 | 原因 | 修法 |
| --- | --- | --- |
| `popen` / `pclose` / `kill` 隐式声明，`pclose` 段错误 | strict `-std=c99` 下 glibc 不声明这些 POSIX 函数 | 进程核心移入 `Process.shim.c`，先定义 `_POSIX_C_SOURCE` |
| `struct addrinfo` 不完整 | 同上（`getaddrinfo` 需要 feature-test 宏） | 网络核心移入 `Net.shim.c` |
| `timer_create` / `clock_gettime` / `nanosleep` 不可见 | 同上 | 时间核心移入 `Time.shim.c` |
| `mkdir` / `readlink` / `telldir` / `fileno` / `setenv` 等隐式声明 | 各 shim 头被包含时首个系统头已处理完 | 每个 `.shim.h` 顶部定义受保护的 `_POSIX_C_SOURCE` |
| 周期 `Timer` 被 SIGALRM 杀死 | glibc `signal()` 在 `_POSIX_C_SOURCE` 下采用 System V 语义，首次投递后复位 | 改用 `sigaction`，处理器持续有效 |
| `is_digit` / `is_nan` / `signbit` 断言不稳定 | glibc 返回任意非零值，不保证 1 | 测试改为 `!= 0` / `== 0` |
| `math` 链接失败 | Linux 的 libm 不并入 libc | `tests/run.sh` 传 `--ldflags "-lm"` |

## 与 CI 的关系

本地 Podman 只覆盖 **Ubuntu 24.04 / gcc 13**；GitHub CI 的 host 矩阵再覆盖 **ubuntu/gcc + macos/clang**。
CI 的 `compiler-head` 夜间 job 仍跟 `main`，用于提前发现 ct-lang 的语言侧变化。
