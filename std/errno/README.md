# `std::errno` — `errno` 与 `strerror` 的两条读法

`std::errno` 把 `errno` 和错误文本放在一个模块里：`std::errno::Errno.get_last()` 读当前
翻译单元的 `errno`，`code.error_message(out, cap)` 把 `strerror(code)` 拷进调用方缓冲。
不分配、不缓存、不返回 libc 的静态指针。

## 1. 定位与边界

- `errno` 是宏，没有可用的接收者；读取走 namespace 入口，写入（测试与库内回填）走
  `set_last(code)`。
- `strerror` 返回的是**借用**指针（下一次同类调用可能覆盖），所以本模块只提供拷贝式
  `error_message`：容量不够返回 `-1` 并清空缓冲，不截断。
- 不提供 `E*` 常量表：常量值随平台变化，本包不在 CT 侧复制一份；需要判断具体错误时
  用 `std::errno::Errno.get_last()` 与平台常量比较，或在调用方直接 `#define`。
- 不做 `perror`：它直接写 `stderr`，输出目标不归这个模块管。
- **跨翻译单元**：shim 是 `static inline`，`get_last()` 在调用方 TU 里读 `errno`，与失败
  调用处在同一 TU。这是本包不使用“包内错误缓存”的原因——缓存跨 TU 会读到 0。

## 2. 依赖与安装

依赖 C 的 `<errno.h>` / `<string.h>`；`requires ct >= 0.30.0`。

| 文件 | 内容 |
| --- | --- |
| `Errno.m.ct` | `Errno`（namespace 宿主）与 `ErrnoCode`（`int` 宿主）两个 trait + 两个 `map trait` |
| `Errno.shim.h` | `std_errno_get_last` / `set_last` / `message` 的 `static inline` 实现 |

引入方式：`import std::errno::Errno;`。

## 3. 快速开始

```ct
import std::errno::Errno;
import std::str::String;
import std::test::Test;

int main(void)
{
    char msg[64];
    int code = 2;                 /* ENOENT on Linux and macOS */

    std::errno::Errno.set_last(code);
    if (std::errno::Errno.get_last() == code) {
        code.error_message(msg, 64);
        /* msg is now a private copy of "No such file or directory" */
    }
    return 0;
}
```

## 4. API 全表

| 方法 | 签名 | 语义 | 层 | 失败 |
| --- | --- | --- | --- | --- |
| `get_last()` | `int std::errno::Errno.get_last()` | 读本翻译单元的 `errno` | ① | 无 |
| `set_last(code)` | `int std::errno::Errno.set_last(int code)` | 写 `errno`（测试与 shim 回填用） | ① | 无 |
| `error_message(code, out, cap)` | `int std::errno::Errno.error_message(int code, char *out, int cap)` | 拷贝 `strerror(code)` | ② | 缓冲不足 `-1`，缓冲清空 |
| `error_message(out, cap)` | `int code.error_message(char *out, int cap)` | 同上，`int` 接收者形式 | ② | 同上 |

**返回值**：`error_message` 成功时返回文本字节数（不含 NUL）；`cap` 必须容纳文本与 NUL，
否则返回 `-1`，`out[0]` 置 0。

## 5. 与相邻模块的关系

- `std::fs` 的 `FsEnv.get_last_error()` 是 `std::fs` 自己的 per-TU 快照；两者不共享存储，
  但都在调用方 TU 内，能读到同一次失败调用的 `errno`。
- `std::locale` 与 `std::process` 的失败也走 `errno`，可直接用本模块读。
- 不与 `strerror` 的静态缓冲共享生命周期：拿到的是私有副本。

## 6. 限制与未支持

- **没有常量表**：`ENOENT` 一类值不在 CT 侧提供，`.ct` 文件不能 `#include <errno.h>`。
- **不保证线程语义**：`errno` 本身是线程局部的；本模块只做直读，不另建存储。
- **文本随 locale 变化**：`strerror` 的文案受 `LC_MESSAGES` 影响，测试只校验长度与拷贝。
- **不接受空缓冲**：`out == NULL` 或 `cap <= 0` 一律 `-1`。

## 7. 版本与变更记录

| 版本 | 状态 | 变更 |
| --- | --- | --- |
| `0.6.0` | 本轮新增 | 新建模块：`get_last` / `set_last` / `error_message`（namespace 与 `int` 两种形式）。 |

编译器下限 `ct >= 0.30.0`。
