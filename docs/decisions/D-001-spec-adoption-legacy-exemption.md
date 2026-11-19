applies-to: v0.12.0

# D-001：纳入 1.17.0 规范，存量豁免与版本偏移

| 项 | 值 |
| --- | --- |
| id | D-001 |
| date | 2026-11-19 |
| anchor | `v0.11.0` / `8689082` |
| status | accepted |
| trigger | 用户要求“工程规范要遵守，正规化吧”，并选择“存量豁免” |
| judge | USER |
| signer | USER |
| seats | REQ / EDITOR |
| scope | 本仓全部文档、issue 台账、历史提交与版本号 |
| target | `v0-PH-8` |
| criteria | G0 可执行；存量文档/issue/历史提交不追溯；新文档/新 issue/新 phase 起执行；版本偏移可机械核对 |
| verdict | accepted：采用规范 1.17.0；`v0.11.0` 及之前的内容登记为 legacy；新 phase 从 `v0-PH-8` 开始；版本永久偏移 `d = -4`（`MINOR = n + 4`） |
| rationale | 存量文档共 20+ 份，其中 README/设计正文/多个模块 README 超过 300 行，全部按 G4 拆分会把一次“纳管”变成大规模重写；用户明确选择“存量豁免”。版本号 0.4.0–0.11.0 与 P0–P7 已公开，重编号会破坏 tag 与消费者，规范本身也允许“登记偏离，不重新编号” |
| invalidation | 用户要求撤销存量豁免，或要求重新编号版本；或 G4 对存量的引用造成实际验收困难 |
| affected | `AGENTS.md` 0.3、`docs/planning/roadmap.md`、`docs/phases/v0-PH-8/card.md`、`tools/check.sh`、后续所有新文档/issue |

## 1. 存量豁免的范围

以下对象在 2026-11-19 之前已存在，豁免以下条款，直到它们被**实质性改动**（新增章节/重写超过
30% 或新增可执行代码块）为止：

| 对象 | 豁免条款 | 说明 |
| --- | --- | --- |
| `README.md`、`std/*/README.md`、`docs/design/*.md`、`docs/review/*.md`、`docs/libc-inventory.md`、`docs/backlog.md` | G4 `g4-size`（≤300 行）、`g4-snippets`、`applies-to` | 保留现有教程与证据；拆分/迁移作为后续独立任务，不在 PH-8 强推 |
| `issues/*.md`（4 份） | 5.4 issue front-matter `status`/证据字段 | 已用中文状态收口；新 issue 必须按 5.4 |
| `P0`–`P7` 历史提交 | G8 的 T-7 红/绿提交、一提交一事、phase 冻结 | 无法追溯；PH-8 起的新任务执行 |
| `P0`–`P7` 编号 | PN-3 的 `v0-PH-0` 必须存在、PN-4 的 `v0-PH-<n>` 迁移 | 保留 `P0`–`P7` 标签；新 phase 从 `v0-PH-8` 起；不建 `v0-PH-0…7` 的卡片 |

## 2. 版本偏移

- 历史映射：`P0`→`0.4.0` … `P7`→`0.11.0`。
- 新编号：`P<n>` 视为 `v0-PH-<n>`；`n=7` 时 `MINOR=11`，故 `d = 7 − 11 = −4`，即
  `MINOR = n − d = n + 4`。
- `v0-PH-8` 收口 → `0.12.0`；`v0-PH-9` → `0.13.0`；依此类推。
- 收敛方式：**永久偏移**，不重编号、不回填 tag。

## 3. 签字

- 用户：2026-11-19 会话指令“存量豁免”与“工程规范要遵守，正规化吧”。
- 登记位置：`AGENTS.md` 0.3 偏离项（两条 `deviation:`）。
