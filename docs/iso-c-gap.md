applies-to: v0.12.0

# ISO C17 缺口表（v0-PH-9 T1）

状态词：`covered`（已有 CT 接口）、`direct`（宏/类型，直接调 C）、`partial`（部分覆盖）、
`missing`（待补）、`na`（变参/语言机制，不封装）。

| 头文件 | 现状 | 缺口 / 处置 |
| --- | --- | --- |
| `assert.h` | `direct` | `assert` 是宏；文档写直调 |
| `complex.h` | `blocked` | 探针见 `docs/iso-c-probes.md`；CT 无复数类型，等语言侧 |
| `ctype.h` | `covered` | 14 个方法；无缺口 |
| `errno.h` | `covered` | `errno` / `strerror` / `perror`（perror 在 T3） |
| `fenv.h` | `missing` | T7：舍入模式、异常标志 |
| `float.h` | `direct` | 宏；不封装 |
| `inttypes.h` | `direct` | 类型与格式宏；不封装 |
| `iso646.h` | `direct` | 宏；不封装 |
| `limits.h` | `direct` | 宏；不封装 |
| `locale.h` | `covered` | `setlocale` / `localeconv` |
| `math.h` | `partial` | 35/39 方法；缺 `erf` / `erfc` / `tgamma` / `lgamma` / `remainder` / `nextafter` / `fdim` / `fma`；T2–T7 后按需补 |
| `setjmp.h` | `na` | 跳转语义无法映射；文档写直调风险 |
| `signal.h` | `covered` | `signal` / `raise` / `alarm` 在 process |
| `stdalign.h` | `direct` | 宏；不封装 |
| `stdarg.h` | `na` | 变参；不封装 |
| `stdatomic.h` | `blocked` | CT 无泛型原子类型，等语言侧 |
| `stdbool.h` | `direct` | 宏；不封装 |
| `stddef.h` | `direct` | `size_t` / `NULL` 等；不封装 |
| `stdint.h` | `direct` | 定长整数；不封装 |
| `stdio.h` | `partial` | 缺 `tmpfile` / `tmpnam` / `setvbuf` / `freopen` / `perror` / `putc` / `getc` / `fgets` / `fputs`（T3） |
| `stdlib.h` | `partial` | 缺 `div` / `ldiv` / `lldiv` / `aligned_alloc` / `at_quick_exit` / `quick_exit` / `rand` / `srand`（T4） |
| `stdnoreturn.h` | `direct` | 宏；不封装 |
| `string.h` | `covered` | T2 已补 `strspn` / `strcspn` / `strpbrk` / `strcoll` / `strxfrm`；`strtok_r` 是 POSIX，归 PH-10 |
| `tgmath.h` | `na` | 泛型宏；只写直调说明 |
| `threads.h` | `blocked` | C11 可选，与 pthread 重叠；等语言侧 |
| `time.h` | `partial` | T5 已补 UTC 字段、`timegm`、`now_timespec`；`asctime` / `ctime` 仍缺 |
| `uchar.h` | `blocked` | CT 无 `char16_t` / `mbstate_t`，等语言侧 |
| `wchar.h` | `partial` | 有转换；缺宽字符 I/O（T6 视成本） |
| `wctype.h` | `covered` | T6 已补 7 个 `isw_*` 与 `towupper` / `towlower` |

## 优先级

1. `string` / `stdio` / `stdlib` / `time`：日常使用频率最高，先做。
2. `uchar` / `wctype` / `fenv`：平台差异小，可做。
3. `complex` / `stdatomic` / `threads`：先探针，必要时 defer 到 PH-10（POSIX 线程与内存模型同批）。
