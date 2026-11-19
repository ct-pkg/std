# Session map

| 对话名 | 角色 | 职责 | 作用域 |
| --- | --- | --- | --- |
| `lead` | EDITOR | 排期、拆任务、收口判定、发布清单 | 常驻 |
| `verify` | VERIFY | 复现、红证认定、独立验收 | 常驻 |
| `review` | REVIEW | 评审报告、issue 立项、P0/P1 清零 | 常驻 |
| `v0-PH-8-dev` | DEV | 实现 PH-8 任务、先红后绿 | `v0-PH-8` |
| `v0-PH-8-verify` | VERIFY | PH-8 红证与退出判据 | `v0-PH-8` |
| `v0-PH-8-review` | REVIEW | PH-8 评审 | `v0-PH-8` |

## 规则

- phase 对话命名：`v<M>-PH-<n>-dev` / `-verify` / `-review`；草案（`draft-*`）不得进入本表。
- 当前可执行 phase：`v0-PH-8`。草案 `draft-iso-c` / `draft-posix-core` / `draft-posix-services` 不排期、不派工。
- 角色的读写边界见 `AGENTS.md` 生成块 §3 与 §5；`lead` 不写实现代码。
