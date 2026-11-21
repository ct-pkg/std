applies-to: v0.20.0

# v0-PH-16：`CharStr` 改名与内建层边界对齐

| 项 | 值 |
| --- | --- |
| 编号 | `v0-PH-16` |
| 目标 | `std::str` 的单元与 trait 统一为 `CharStr`；按「逃生层 + 用户面工具」记录内建层边界与 `lib/std` 路径清单 |
| 规范版本 | 1.17.0 |
| version-minor | 16（收口 `0.20.0`，含永久偏移 `+4`） |
| 依赖 | `v0-PH-14` 已收口；D-003；issue 0004 / 0005 |
| 状态 | 已收口（2026-11-21，`v0.20.0`） |

## 范围

**做**：

- `std/str/String.m.ct` → `CharStr.m.ct`，`String.shim.h` → `CharStr.shim.h`；`pub trait CStrOps` → `CharStr`。
- 全仓 `import std::str::String` → `import std::str::CharStr`，共 43 个文件。
- 记录内建层边界：只删语言级类型重复，C 资源句柄保留；`lib/std` 路径清单 9 行待改名触发。
- 版本位从 `0.19.0` 到 `0.20.0`，发 annotated tag `v0.20.0`。

**不做**：

- 不新建 `std::char` 模块；不删 `io` / `time` / `net` / `locale` 等 C 资源句柄。
- 不复制 `Drop` / `AutoDrop` / `Handback` 声明；不预写 `lib/core` 路径。
- 不改 `str` / `fs` / `utf8` / `arena` / `test` 的方法名与行为；不新增模块。

## 任务清单

| id | deliverable | verify | effort |
| --- | --- | --- | --- |
| T1 | 单元与 trait 改名为 `CharStr`，import 全量更新（issue 0004） | `bash tests/run.sh` | M |
| T2 | 内建层边界规划与 `lib/std` 清单（issue 0005） | `grep -ri "std::vec\|std::map" --include="*.ct" .` 为空 | S |
| T3 | `0.20.0` 版本位、CHANGELOG、annotated tag 与推送 | `bash tools/check.sh all` | S |

## 进度

| 任务 | 状态 | 证据 |
| --- | --- | --- |
| T1 | done | `e159e88`；`tests/run.sh` 25 passed 0 failed |
| T2 | done | `296c47b`、`e613880`；`grep` 0 业务命中 |
| T3 | done | 本卡所在提交；tag `v0.20.0` |

## 退出判据

- [x] `bash tests/run.sh` 25 passed, 0 failed。
- [x] `bash tools/check.sh all` 退出码 0；`ct pkg check .` 通过。
- [x] 版本落点全部为 `0.20.0`；annotated tag `v0.20.0` 指向收口提交并推送。
- [x] **任务清单每项可独立验收（6.6 T-6）**：下表贴 verify 输出。

## 收口对照

| 任务 | verify 命令 | 原始输出 |
| --- | --- | --- |
| T1 | `bash tests/run.sh \| tail -1` | `std: 25 passed, 0 failed` |
| T2 | `grep -ri "std::vec\|std::map" --include="*.ct" .` | 无输出（0 命中） |
| T3 | `ct pkg check .` | `ok: 4 criteria` |
