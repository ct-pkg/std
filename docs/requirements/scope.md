# std 主张层：范围与目标

| 项 | 值 |
| --- | --- |
| 性质 | 主张层；回答“为什么做、不做什么、受什么约束” |
| 锚点 | `v0.11.0` / `8689082`（历史）；新 phase 从 `0.12.0` 起 |
| 决策 | [D-001](../decisions/D-001-spec-adoption-legacy-exemption.md)、[D-002](../decisions/D-002-scope-iso-c-posix.md) |

## 1. 目标

让 CT 程序员用 CT 的写法直接使用宿主 libc 的 **ISO C17 标准库 + POSIX.1-2017** 能力，不需要记 C 函数名、不需要手写 shim，也不牺牲类型/所有权边界。

## 2. 要求

| id | 要求 | 验收 |
| --- | --- | --- |
| REQ-SCOPE-1 | 范围包含 ISO C17 标准库 | `ACC-SCOPE-1`、roadmap PH-9 |
| REQ-SCOPE-2 | 范围包含 POSIX.1-2017 | `ACC-SCOPE-2`、roadmap PH-10/PH-11 |
| REQ-SCOPE-3 | 变参函数、Windows 原生 API、C++ 不在范围 | `ACC-SCOPE-3` |
| REQ-REG-1 | 本仓按 1.17.0 规范纳管，G0–G9 可执行 | `ACC-REG-1`、PH-8 |
| REQ-REG-2 | 存量文档/issue/历史提交按 D-001 豁免，新对象按规范 | `ACC-REG-2` |
| REQ-REG-3 | 版本从 `v0-PH-8` 起按偏移 `+4` 收口 | `ACC-REG-3` |

## 3. 非目标

- 不做堆字符串类型、Unicode 规范化 / 字素簇、正则的 CT 语法糖（`regex` 只做不透明句柄）。
- 不封装 `scanf` / `printf` / `sprintf` / `v*` 变参族。
- 不追求 Windows 原生；macOS/Linux 的差异按 phase 卡片的风险表处理。
- 不为 POSIX 结构体建 CT 值类型；需要时用不透明句柄 + 显式释放。

## 4. 约束

- `requires = ct >= 0.67.0`；平台验证见 [platform-validation](../design/platform-validation.md)。
- 新增能力必须有可运行夹具；文档中的可执行代码块从 `docs/snippets/` 来（新文档）。
- 一个 phase 一个 MINOR；fix 只走 PATCH；纯文档不 bump。
