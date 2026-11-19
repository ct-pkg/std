applies-to: v0.12.0

# v0-PH-8：工程规范纳管

| 项 | 值 |
| --- | --- |
| 编号 | `v0-PH-8` |
| 目标 | 把本仓纳入 1.17.0 规范：G0 落地、G0–G9 可执行、phase/验收/决策/台账可用；不改 std 的 API |
| 规范版本 | 1.17.0 |
| version-minor | 8（收口 `0.12.0`，含永久偏移 `+4`） |
| 依赖 | 无；`v0.11.0` 已发布 |
| 状态 | 已收口（2026-11-19，`v0.12.0`） |

## 范围

**做**：

- `docs/session-map.md`、`AGENTS.md` 的 0.2 话术路由与 0.3 偏离项。
- `tools/check.sh` 的 g0–g9 与负例；`tools/legacy-docs.txt` 存量文档清单。
- `docs/phases/`、`docs/requirements/`、`docs/acceptance.md`、`docs/decisions/`。
- `CHANGELOG.md`、版本落点核对、`v0.12.0` 收口。
- 新文档/新 issue/新提交按规范执行；存量按 D-001 豁免。

**不做**：

- 不修改 std 的任何源码/API/测试断言。
- 不拆分存量 README/设计文档（D-001 豁免；后续单独任务）。
- 不补 P0–P7 的历史 phase 卡片、红证与 tag。

## 交付物

| 交付物 | 路径 |
| --- | --- |
| 会话地图 | `docs/session-map.md` |
| 门禁脚本 | `tools/check.sh` |
| 存量豁免清单 | `tools/legacy-docs.txt` |
| 主张/验收 | `docs/requirements/scope.md`、`docs/acceptance.md` |
| 决策记录 | `docs/decisions/D-001-spec-adoption-legacy-exemption.md`、`D-002-scope-iso-c-posix.md` |
| 变更记录 | `CHANGELOG.md` |
| 版本落点 | `ct.pkg`、`README.md`、`docs/planning/roadmap.md` |

## 任务清单

| id | deliverable | verify | effort | 备注 |
| --- | --- | --- | --- | --- |
| T1 | `docs/session-map.md` + `AGENTS.md` 0.2/0.3 | `bash tools/check.sh g0` | S | 纯文档/配置，`no-test: 文档与配置`；评审签字见 D-001 |
| T2 | `tools/check.sh` 的 g0/g3/g9 | `bash tools/check.sh g0 && bash tools/check.sh g3 && bash tools/check.sh g9` | M | 先红后绿；负例放 `tools/tests/` |
| T3 | `tools/check.sh` 的 g1/g2 | `bash tools/check.sh g1 && bash tools/check.sh g2` | M | 卡片/验收字段检查 |
| T4 | `tools/check.sh` 的 g4 + `tools/legacy-docs.txt` | `bash tools/check.sh g4` | M | 存量清单 + 新文档 ≤300 行/锚点/`applies-to` |
| T5 | `tools/check.sh` 的 g5/g7 | `bash tools/check.sh g5 && bash tools/check.sh g7` | M | 红证索引、密钥/锁文件/sanitizer 入口 |
| T6 | `tools/check.sh` 的 g6 + issue/池台账 | `bash tools/check.sh g6` | M | 存量 issue 走豁免；新 issue 按 5.4 |
| T7 | `tools/check.sh` 的 g8 | `bash tools/check.sh g8` | M | 版本/提交/tag/phase 冻结 |
| T8 | `CHANGELOG.md` + `0.12.0` 收口 | `bash tools/check.sh all && ct pkg check . && git tag -n v0.12.0` | S | 版本移动与 tag 同提交 |

## 退出判据

- [x] `bash tools/check.sh all` 退出码 0。
- [x] `docs/session-map.md` 存在，且 `AGENTS.md` 0.2/0.3 非空。
- [x] `docs/phases/v0-PH-8/card.md` 通过 `tools/check.sh g1`；`docs/acceptance.md` 通过 `g2`。
- [x] `bash tests/run.sh` 20/20；`ct pkg check .` 通过。
- [x] 存量豁免清单存在且有 D-001 签字；新文档在清单外。
- [x] 版本落点全部为 `0.12.0`；annotated tag `v0.12.0` 指向收口提交并推送。
- [x] **任务清单每项可独立验收（6.6 T-6）**：收口对照表逐项贴 verify 输出。
- [x] 无 P0/P1 issue；新 issue 状态合法。

## 已知风险

| 风险 | 触发信号 | 处置 |
| --- | --- | --- |
| G4 存量豁免被误用 | 新文档被加入 `legacy-docs.txt` | 代码评审 + D-001 范围核对 |
| g1/g8 与历史号段冲突 | `g1-version` 报 PH-0 缺失/偏移 | 用 D-001 偏移；不迁移 P0–P7 |
| 门禁脚本自身无红证 | `g5` 报缺红证 | 每个 g* 检查先写失败 fixture，再实现 |
| 规范块漂移 | `g9`/`g0` 失败 | 跑 `sync-agents.sh --check` |
