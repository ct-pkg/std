applies-to: v0.16.0

# v0-PH-13：POSIX 异步

| 项 | 值 |
| --- | --- |
| 编号 | `v0-PH-13` |
| 目标 | 纳入 aio 与 posix_spawn；mq 归 `v0-PH-14` |
| 规范版本 | 1.17.0 |
| version-minor | 13（收口 `0.17.0`，含永久偏移 `+4`） |
| 依赖 | `v0-PH-12` 已收口；D-002 |
| 状态 | 已收口（2026-11-19，`v0.17.0`） |

## 范围

**做**：`aio` 的 aiocb 句柄与 read/write/error/return/cancel/suspend；`posix_spawn` 的 `spawn_wait(path)` 与 `spawn_pid(path)`。
**不做**：mq 归 `v0-PH-14`（draft）。

## 交付物

| 交付物 | 路径 |
| --- | --- |
| posix 扩展 | `std/posix/` |
| 测试 | `tests/posix/aio.ct`、`tests/posix/spawn.ct` |
| 设计 | `docs/phases/v0-PH-13/design.md` |

## 任务清单

| id | deliverable | verify | effort |
| --- | --- | --- | --- |
| T1 | 设计 | `docs/phases/v0-PH-13/design.md` | S |
| T2 | aio 句柄 | `tests/posix/aio.ct` | M |
| T3 | posix_spawn | `tests/posix/spawn.ct` | M |

## 进度

| 任务 | 状态 | 证据 |
| --- | --- | --- |
| T1 | done | `docs/phases/v0-PH-13/design.md` |
| T2 | done | `tests/posix/aio.ct` 5 checks |
| T3 | done | `tests/posix/spawn.ct` 1 check |

## 退出判据

- [x] T2/T3 测试通过；`bash tests/run.sh` 全绿。
- [x] `bash tools/check.sh all` 退出码 0；`ct pkg check .` 通过。
- [x] 版本落点全部为 `0.17.0`；annotated tag `v0.17.0` 指向收口提交并推送。
- [x] **任务清单每项可独立验收（6.6 T-6）**：收口对照表逐项贴 verify 输出。
- [x] mq 在 `draft-posix-mq` 有目标，不静默跳过。
