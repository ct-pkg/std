applies-to: v0.17.0

# v0-PH-14：POSIX mqueue

| 项 | 值 |
| --- | --- |
| 编号 | `v0-PH-14` |
| 目标 | 纳入 mq_open / mq_send / mq_receive / mq_close / mq_unlink；macOS 无此接口时返回 -1 |
| 规范版本 | 1.17.0 |
| version-minor | 14（收口 `0.18.0`，含永久偏移 `+4`） |
| 依赖 | `v0-PH-13` 已收口；D-002 |
| 状态 | 已收口（2026-11-19，`v0.18.0`） |

## 范围

**做**：Linux 的真实 mqueue；macOS 编译为返回 -1 的 stub，并用 `mq_supported()` 暴露能力。
**不做**：不建 CT 值类型；mq_attr 用出参。

## 任务清单

| id | deliverable | verify | effort |
| --- | --- | --- | --- |
| T1 | mq 五函数 + supported | `tests/posix/mq.ct` | M |

## 进度

| 任务 | 状态 | 证据 |
| --- | --- | --- |
| T1 | done | `tests/posix/mq.ct` 1 check（macOS stub；Linux 真实） |

## 退出判据

- [x] `tests/posix/mq.ct` 通过（macOS 走 stub 分支，Linux 走真实分支）。
- [x] `bash tools/check.sh all` 退出码 0；`ct pkg check .` 通过。
- [x] 版本落点全部为 `0.18.0`；annotated tag `v0.18.0` 指向收口提交并推送。
- [x] **任务清单每项可独立验收（6.6 T-6）**：收口对照表贴 verify 输出。
