---
id: 0005
status: deferred
---

# Issue 0005：内建 trait 显性化与 `lib/std` → `lib/core` 的包内接应

| 项 | 值 |
| --- | --- |
| 编号 | 0005 |
| 状态 | **deferred**（等语言仓库两次合并：① 内建 trait 声明收归内建层；② `lib/std` → `lib/core` 改名） |
| 类型 | 跨仓接应（本仓只改文档与索引，不复制声明、不抢跑路径） |
| 触发条件 | 语言仓库合并后，按 [docs/planning/builtin-layer-boundary.md](../planning/builtin-layer-boundary.md) §3、§4 执行 |
| 提出日期 | 2026-11-21 |

## 1. 范围

语言仓库把 `Drop` / `AutoDrop` / `Handback` 的正式声明收归内建层，并把内建层目录
`lib/std/` 更名为 `lib/core/`。本仓不做声明、不做改名，只做两件跟随：

1. **指针**：包内凡提到这三个 trait 的地方，确认表述为「内建 trait」，并指向内建层声明文件
   （只给路径，不复制声明）。
2. **路径替换**：内建层路径引用按清单一次性机械替换。

## 2. 现状核查（2026-11-21，只读）

| 检查 | 结果 |
| --- | --- |
| 把 `Drop` 写成「用户自定义 trait」的段落 | **0 处**；`README.md:421` 已写「语言内建的析构钩子」 |
| `AutoDrop` 引用 | 0 |
| `Handback` 引用 | 0 |
| `trait Drop` 等声明复制 | 0 |
| `lib/std` 路径引用 | 9 处，全部在 `README.md` 与 `std/README.md`，清单见规划文档 §4.1 |

## 3. 触发后要做的

| 步 | 动作 | 判据 |
| --- | --- | --- |
| 1 | 重新 grep `lib/std`（行号已漂移，按内容定位） | 清单 9 处逐一确认 |
| 2 | 替换指向当前内建层的 8 处；历史句保留 | 只剩历史句含 `lib/std`，且有「现为 `lib/core/`」注 |
| 3 | `README.md:421` 与 `decision-resource-ownership.md` 补声明指针 | 两处各一行路径引用，无声明副本 |
| 4 | 确认 `CT_STD_DIR` 是否随目录改名 | 以语言仓回复为准；未确认不改 |
| 5 | 跑门禁 | `bash tools/check.sh all` 绿、`ct pkg check .` 绿 |
| 6 | 发版并刷新索引 | 新 MINOR 的 tag 合格；`ct index verify` 输出 `OK` |

## 4. 现在明确**不做**的事

- 不按 `lib/core` 预写任何文档（改名批次未定，避免二次返工）。
- 不在本仓声明或复制 `Drop` / `AutoDrop` / `Handback`。
- 不做旧 `import` 的兼容别名或 lint 指引：本项目尚未正式发布，改名（如 `std::str::CharStr`）
  不留兼容窗口，旧路径直接硬错即可（负责人 2026-11-21 裁定）。
- 不改 `arena` / `test` / `str` / `fs` / `utf8` 的功能。
- 不新增模块。

## 5. 关联

- 规划与清单：[builtin-layer-boundary.md](../planning/builtin-layer-boundary.md)
- 相邻改名：issue [0004](0004-str-unit-name-charstr.md)（`std::str::String` → `CharStr`，落 `0.20.0`）
