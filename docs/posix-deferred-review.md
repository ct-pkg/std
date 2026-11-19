applies-to: v0.14.0

# POSIX / ISO C deferred 复评（v0-PH-10 T7）

| 项 | 结论 | 理由 / 目标 |
| --- | --- | --- |
| `complex.h` | `blocked: CT language type` | CT 没有复数标量；需要语言侧类型或两 double 约定 |
| `stdatomic.h` | `blocked: CT language type` | CT 没有泛型 `_Atomic`；`pthread` 互斥已覆盖同步用例 |
| `threads.h` | `blocked: CT language type` | C11 可选；`std::thread` 已提供 pthread 形态，避免第二套 |
| `uchar.h` | `blocked: CT language type` | CT 没有 `char16_t` / `mbstate_t`；需要编码层设计 |
| `madvise` | `blocked: CT language type` | macOS 常量不全；`mmap`/`msync` 已覆盖主路径 |

复评触发条件：CT 增加对应类型/泛型，或 PH-11 定稿时重新评估。不得静默跳过。
