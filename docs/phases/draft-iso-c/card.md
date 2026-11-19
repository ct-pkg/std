applies-to: v0.11.0

# draft-iso-c：ISO C17 补全

| 项 | 值 |
| --- | --- |
| phase-id | `draft-iso-c` |
| status | draft |
| 目标 | 把 ISO C17 标准库中尚未覆盖的头文件/函数补进 std，按 CT 形态呈现 |
| 规范版本 | 1.17.0 |
| 依赖 | `v0-PH-8` 收口；D-002 |

## 范围

- 先产出“ISO C17 头文件 × 现状 × 缺口”的逐项表（含已覆盖/不适用/待补）。
- 待补族：`fenv` / `complex` / `uchar` / `wctype` / `stdatomic` / `threads`（C11）/
  `inttypes`+`stdint`；`string.h` / `stdio.h` / `stdlib.h` / `time.h` / `math.h` 的缺口。
- `setjmp` / `stdarg` / `tgmath` 只做直调说明，不做 CT 封装。

## 不做什么

- 不封装变参函数；不引入 CT 值类型；不处理 Unicode 规范化。

## 已知风险

| 风险 | 触发信号 |
| --- | --- |
| `complex`/`fenv` 在 CT 侧没有对应标量 | 探针编译失败或只能 `void *` |
| C11 `threads.h` 与 POSIX pthread 重复 | 平台缺 `<threads.h>` 或语义冲突 |
| `setjmp` 是宏 | 无法映射为函数 |
