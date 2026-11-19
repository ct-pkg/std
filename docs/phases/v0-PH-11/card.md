applies-to: v0.14.0

# v0-PH-11：POSIX 服务

| 项 | 值 |
| --- | --- |
| 编号 | `v0-PH-11` |
| 目标 | 纳入 uname / rlimit / statvfs / fnmatch / dlfcn 五个高频 POSIX 服务 |
| 规范版本 | 1.17.0 |
| version-minor | 11（收口 `0.15.0`，含永久偏移 `+4`） |
| 依赖 | `v0-PH-10` 已收口；D-002 |
| 状态 | 已收口（2026-11-19，`v0.15.0`） |

## 范围

**做**：`sys/utsname`、`sys/resource`、`sys/statvfs`、`fnmatch`、`dlfcn`。
**不做**：aio / mq / shm / sched / posix_spawn / termios / pwd / grp / glob 归 `v0-PH-12`（draft）。

## 交付物

| 交付物 | 路径 |
| --- | --- |
| posix 模块 | `std/posix/` |
| 测试 | `tests/posix/main.ct` |
| 设计 | `docs/phases/v0-PH-11/design.md` |

## 任务清单

| id | deliverable | verify | effort |
| --- | --- | --- | --- |
| T1 | 设计：字段拷贝与句柄 | `docs/phases/v0-PH-11/design.md` + `g1` | S |
| T2 | uname 五字段 | `tests/posix/main.ct` | M |
| T3 | rlimit 读/写 | 同上 | M |
| T4 | statvfs | 同上 | M |
| T5 | fnmatch | 同上 | M |
| T6 | dlopen/dlsym/dlclose/dlerror | 同上 | M |

## 退出判据

- [x] `tests/posix/main.ct` 通过；`bash tests/run.sh` 全绿。
- [x] `bash tools/check.sh all` 退出码 0；`ct pkg check .` 通过。
- [x] 版本落点全部为 `0.15.0`；annotated tag `v0.15.0` 指向收口提交并推送。
- [x] **任务清单每项可独立验收（6.6 T-6）**：收口对照表逐项贴 verify 输出。
- [x] 剩余 POSIX 服务在 `draft-posix-remaining` 有明确目标，不静默跳过。

## 进度

| 任务 | 状态 | 证据 |
| --- | --- | --- |
| T1 | done | `docs/phases/v0-PH-11/design.md` |
| T2–T6 | done | `tests/posix/main.ct` 10 checks |

## 已知风险

| 风险 | 触发信号 | 处置 |
| --- | --- | --- |
| dlfcn 符号生命周期 | close 后函数指针悬垂 | 文档写清 |
| statvfs 字段宽度 | 32/64 位差异 | 统一 long 出参 |
