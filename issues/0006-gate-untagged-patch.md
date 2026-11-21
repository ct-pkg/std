---
id: 0006
status: verified
---

# Issue 0006：`g8` 拒绝未打 tag 的 PATCH，和 V-7 冲突

| 项 | 值 |
| --- | --- |
| 编号 | 0006 |
| 状态 | **verified**（REV 复核通过，2026-11-21；`0.20.1`，fix 无 tag） |
| 类型 | 门禁缺陷 / 版本规则一致性 |
| 发现 | 2026-11-21，处理 0004 后发布记账时 |
| 关联 | V-3、V-4、V-7、g8 |

## 1. 问题

`g8` 对当前版本一律要求 annotated tag `v<version>`：

```text
FAIL  v0.20.1 is not an annotated tag
```

但 V-7 明确“只有 phase tag，fix 不打 tag”。于是：

- 按 V-3 做了一次 fix，PATCH 到 `0.20.1`；
- 按 V-7 不能建 `v0.20.1`；
- `g8` 仍然红。

这不是 fix 做错了，是 `g8` 把“当前版本必须有 tag”写死，漏掉了 PATCH 发行。

同一处还硬编码了 `0.19.x` 身份发行特例；该特例应由 decision 记录，不应写进 gate。

## 2. 复现（红证）

`baseline: 5d3f3f8`

```console
$ bash tools/tests/selftest.sh
FAIL: untagged patch after phase tag: gate g8 unexpectedly failed
  FAIL  v0.20.1 is not an annotated tag
# 退出码 1
```

fixture 做的是：在 `v0.20.0` 之上把版本改为 `0.20.1` 并提交，但不建
`v0.20.1`，直接跑 `g8`。

## 3. 期望

- PATCH > 0 且没有 `v<version>` tag：允许；要求最近的 phase tag
  `v<MAJOR>.<MINOR>.0` 是 annotated tag 且是 HEAD 的祖先；
- PATCH == 0 且没有 `v<version>` tag：仍然失败（phase 发行必须有 tag）；
- 删除 `g6` 的 `0.19.x` 硬编码，改由 `docs/decisions/D-004` 记录；
- 负例：phase 版本无 tag 必须红；PATCH 有 tag 但指向错误提交必须红。

## 4. 处置（DEV，2026-11-21）

- `g8`：当 `v<version>` 不存在且 PATCH > 0 时，要求同 MINOR 的 phase tag
  `v<MAJOR>.<MINOR>.0` 是 annotated tag 且为 HEAD 祖先；PATCH == 0 仍必须有
  自己的 tag。
- `g6`：删除 `0.19.x` 硬编码；0.19 的例外由 D-004 记录。
- 版本到 `0.20.1`（fix，按 V-7 不打 tag）；`ct.pkg` / README / roadmap /
  AGENTS / CHANGELOG 同步。
- `tools/tests/selftest.sh` 新增未打 tag 的 0.20.1 正例，保留 phase 无 tag
  与 code-after-tag 负例。

原始输出：

```console
$ bash tools/tests/selftest.sh
selftest: 9 negative gates ok
# 退出码 0

$ bash tools/check.sh all
gates: 10 ok, 0 failed/blocked
# 退出码 0
```

修复提交见紧随其后的 `fix(gates)` 提交；复核由 REV 在下一步写入。


## 5. 复核（REV，2026-11-21）

修复提交：`d11afa3`（`fix(gates): 0006 accept untagged PATCH releases`）。
版本 `0.20.1`，按 V-7 不建 `v0.20.1`；最近的 phase tag 仍是
`v0.20.0`（annotated）。

| # | 命令 | 原始输出 | 退出码 |
| --- | --- | --- | --- |
| 1 | `bash tools/tests/selftest.sh` | `selftest: 9 negative gates ok` | 0 |
| 2 | `bash tools/check.sh all` | `gates: 10 ok, 0 failed/blocked` | 0 |
| 3 | `bash tests/run.sh` | `std: 25 passed, 0 failed` | 0 |
| 4 | `git cat-file -t v0.20.1` | 失败（无 fix tag，符合 V-7） | 1 |
| 5 | `git cat-file -t v0.20.0` | `tag`（phase tag 存在） | 0 |
| 6 | `git merge-base --is-ancestor v0.20.0^{} HEAD` | 无输出 | 0 |

复核结论：

- `g8` 对 `0.20.1` 走“未打 tag 的 PATCH”分支，并校验 `v0.20.0` 是
  annotated phase tag 且为 HEAD 祖先；不再要求 `v0.20.1`。
- `g6` 的 `0.19.x` 硬编码已删除；0.19 的例外只由 D-004 登记。
- `selftest` 覆盖：未打 tag 的 PATCH 通过、phase 版本无 tag 失败、
  release tag 后的 code change 失败。
- 本 issue 的 fix 是门禁修复，按 V-3 计一次 PATCH；`0.20.1` 无 tag。
