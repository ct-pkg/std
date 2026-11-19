applies-to: v0.12.0

# Session map

| 对话名 | 角色 | 职责 | 作用域 |
| --- | --- | --- | --- |
| `lead` | EDITOR | 排期、拆任务、收口判定、发布清单 | 常驻 |
| `verify` | VERIFY | 复现、红证认定、独立验收 | 常驻 |
| `review` | REVIEW | 评审报告、issue 立项、P0/P1 清零 | 常驻 |
| `v0-PH-8-dev` | DEV | 实现 PH-8 任务、先红后绿 | `v0-PH-8` |
| `v0-PH-8-verify` | VERIFY | PH-8 红证与退出判据 | `v0-PH-8` |
| `v0-PH-8-review` | REVIEW | PH-8 评审 | `v0-PH-8` |
| `v0-PH-9-dev` | DEV | 实现 PH-9 任务、先红后绿 | `v0-PH-9` |
| `v0-PH-9-verify` | VERIFY | PH-9 红证与退出判据 | `v0-PH-9` |
| `v0-PH-9-review` | REVIEW | PH-9 评审 | `v0-PH-9` |
| `v0-PH-10-dev` | DEV | 实现 PH-10 任务、先红后绿 | `v0-PH-10` |
| `v0-PH-10-verify` | VERIFY | PH-10 红证与退出判据 | `v0-PH-10` |
| `v0-PH-10-review` | REVIEW | PH-10 评审 | `v0-PH-10` |
| `v0-PH-11-dev` | DEV | 实现 PH-11 任务、先红后绿 | `v0-PH-11` |
| `v0-PH-11-verify` | VERIFY | PH-11 红证与退出判据 | `v0-PH-11` |
| `v0-PH-11-review` | REVIEW | PH-11 评审 | `v0-PH-11` |
| `v0-PH-12-dev` | DEV | 实现 PH-12 任务、先红后绿 | `v0-PH-12` |
| `v0-PH-12-verify` | VERIFY | PH-12 红证与退出判据 | `v0-PH-12` |
| `v0-PH-12-review` | REVIEW | PH-12 评审 | `v0-PH-12` |

## 规则

- phase 对话命名：`v<M>-PH-<n>-dev` / `-verify` / `-review`；草案（`draft-*`）不得进入本表。
- 当前可执行 phase：`v0-PH-8`。草案卡不排期、不派工（PN-14）。
- 角色的读写边界见 `AGENTS.md` 生成块 §3 与 §5；`lead` 不写实现代码。
