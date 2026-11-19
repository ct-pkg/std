applies-to: v0.13.0

# v0-PH-10：POSIX 核心

| 项 | 值 |
| --- | --- |
| 编号 | `v0-PH-10` |
| 目标 | 以不透明句柄 + 显式释放的方式纳入 pthread / semaphore / mmap / poll / select |
| 规范版本 | 1.17.0 |
| version-minor | 10（收口 `0.14.0`，含永久偏移 `+4`） |
| 依赖 | `v0-PH-9` 已收口；D-002 |
| 状态 | 待开工 |

## 范围

**做**：

- `sys/mman`：`mmap` / `munmap` / `mprotect` / `msync` 与 prot/flags 常量；`madvise` 平台缺失时 deferred。
- `poll`：`pollfd` 数组用不透明 `void *` 句柄，`poll` 返回就绪数。
- `sys/select`：`fd_set` 用不透明句柄，`select` 返回就绪数。
- `pthread`：线程、mutex、cond、rwlock、once、key；句柄全部 `void *` + `*_destroy`。
- `semaphore`：`sem_init` / `sem_wait` / `sem_post` / `sem_destroy`。
- PH-9 deferred 的 `complex` / `stdatomic` / `threads` / `uchar` 重新评估；不能做则维持 deferred 并写目标。

**不做**：不建 `Mutex` / `Cond` / `PollSet` 等 CT 值类型；不承诺跨平台结构布局；不做 Windows。

## 交付物

| 交付物 | 路径 |
| --- | --- |
| mmap 模块 | `std/mmap/` |
| poll/select 模块 | `std/poll/` |
| pthread/semaphore 模块 | `std/thread/` |
| 测试 | `tests/mmap/`、`tests/poll/`、`tests/thread/` |
| deferred 复评 | `docs/posix-deferred-review.md` |

## 任务清单

| id | deliverable | verify | effort |
| --- | --- | --- | --- |
| T1 | PH-10 设计：句柄生命周期、平台差异 | `docs/phases/v0-PH-10/design.md` + `g1` | S |
| T2 | `mmap` 四函数 + 常量 | `tests/mmap/main.ct` 通过 | M |
| T3 | `poll` 句柄与 `poll` | `tests/poll/main.ct` 通过 | M |
| T4 | `select` 句柄与 `select` | `tests/poll/select.ct` 通过 | M |
| T5 | pthread 线程 + mutex + cond | `tests/thread/main.ct` 通过 | L（拆 T5a/T5b/T5c） |
| T6 | semaphore | `tests/thread/sem.ct` 通过 | M |
| T7 | deferred 复评 | `docs/posix-deferred-review.md` | S |

## 退出判据

- [ ] 每个模块的夹具通过；`bash tests/run.sh` 全绿。
- [ ] `bash tools/check.sh all` 退出码 0；`ct pkg check .` 通过。
- [ ] 版本落点全部为 `0.14.0`；annotated tag `v0.14.0` 指向收口提交并推送。
- [ ] **任务清单每项可独立验收（6.6 T-6）**：收口对照表逐项贴 verify 输出。
- [ ] deferred 复评有结论；不能做的项写目标 phase，不静默跳过。

## 已知风险

| 风险 | 触发信号 | 处置 |
| --- | --- | --- |
| 结构体布局不可见 | 必须 malloc 句柄 | 文档写清 `*_destroy` |
| 线程回调无闭包 | 只能顶层函数 + `void *` | 示例写死 |
| 平台差异（macOS `sem_*` 已废弃） | 编译警告/错误 | 用 `dispatch_semaphore` 或 `sem_open` 回退 |
