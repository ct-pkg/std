applies-to: v0.15.0

# v0-PH-12：POSIX 服务续

| 项 | 值 |
| --- | --- |
| 编号 | `v0-PH-12` |
| 目标 | 补 pwd / grp / sched / shm / termios / glob / aio / mq / spawn |
| 规范版本 | 1.17.0 |
| version-minor | 12（收口 `0.16.0`，含永久偏移 `+4`） |
| 依赖 | `v0-PH-11` 已收口；D-002 |
| 状态 | 进行中（T1–T3 完成） |

## 范围

**做**：`pwd` / `grp` 字段拷贝；`sched_yield` 与优先级范围；`shm_open`/`shm_unlink`；`termios` 的 `isatty`/`tcgetattr`/`tcsetattr` 基本项；`glob` 句柄；`aio` / `mq` / `posix_spawn` 按探针结果做或 deferred。
**不做**：不建 CT 值类型；不承诺跨平台结构布局。

## 交付物

| 交付物 | 路径 |
| --- | --- |
| posix 扩展 | `std/posix/` |
| 测试 | `tests/posix/` |
| 设计 | `docs/phases/v0-PH-12/design.md` |

## 任务清单

| id | deliverable | verify | effort |
| --- | --- | --- | --- |
| T1 | 设计 | `docs/phases/v0-PH-12/design.md` | S |
| T2 | pwd / grp 字段 | `tests/posix/pwd.ct` | M |
| T3 | sched yield/priority | `tests/posix/sched.ct` | M |
| T4 | shm_open/unlink | `tests/posix/shm.ct` | M |
| T5 | termios / isatty | `tests/posix/termios.ct` | M |
| T6 | glob 句柄 | `tests/posix/glob.ct` | M |
| T7 | aio / mq / spawn 探针 | `docs/phases/v0-PH-12/design.md` | M |

## 退出判据

- [ ] T2–T7 的测试/结论齐全；`bash tests/run.sh` 全绿。
- [ ] `bash tools/check.sh all` 退出码 0；`ct pkg check .` 通过。
- [ ] 版本落点全部为 `0.16.0`；annotated tag `v0.16.0` 指向收口提交并推送。
- [ ] **任务清单每项可独立验收（6.6 T-6）**：收口对照表逐项贴 verify 输出。
- [ ] 无法做的项在 `design.md` 标 `deferred: <phase>` 或拒绝理由。

## 进度

| 任务 | 状态 | 证据 |
| --- | --- | --- |
| T1 | done | `docs/phases/v0-PH-12/design.md` |
| T2 | done | `tests/posix/pwd.ct` 3 checks |
| T3 | done | `tests/posix/sched.ct` 3 checks |
| T4–T7 | pending | 18:05 继续 |

## 已知风险

| 风险 | 触发信号 | 处置 |
| --- | --- | --- |
| aio 平台不完整 | 编译/链接失败 | deferred |
| termios 布局 | 只能句柄 | 不建值类型 |
