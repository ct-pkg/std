applies-to: v0.13.0

# ISO C 探针与 deferred（v0-PH-9 T8）

| 项 | 探针命令 | 结果 | 处置 |
| --- | --- | --- | --- |
| `complex.h` | `cc -std=c99` 编译 `double _Complex z;` + CT 探针 `double _Complex` | CT 无复数类型；C 侧可编译但不能映射成 CT 值 | `deferred: v0-PH-10`（与 POSIX 数值扩展同批） |
| `stdatomic.h` | `cc -std=c11` 编译 `_Atomic int a;`；CT 无泛型原子类型 | C 侧可用；CT 无 `_Atomic` / 泛型 | `deferred: v0-PH-10` |
| `threads.h` | `cc -std=c11` 编译 `thrd_t t;`；平台可选 | 与 pthread 重叠；C11 可选 | `deferred: v0-PH-10` |
| `uchar.h` | `cc -std=c11` 编译 `char16_t c; mbstate_t s;` | CT 无 `char16_t` / `mbstate_t` | `deferred: v0-PH-10` |

`deferred` 不是“以后再说”：目标 phase 与触发条件已写死；PH-10 定稿时必须重新评估。
