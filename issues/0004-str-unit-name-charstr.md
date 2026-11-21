---
id: 0004
status: open
---

# Issue 0004：`std::str` 的单元名与 import 路径改为 `CharStr`

| 项 | 值 |
| --- | --- |
| 编号 | 0004 |
| 状态 | **open**（已实现，待提交与 `0.20.0` 发版；复核由 REV 认定） |
| 类型 | 破坏性改名（import 路径与 trait 名） |
| 版本落点 | `0.20.0` + 新 tag `v0.20.0` |
| 涉及文件 | `std/str/String.m.ct` → `std/str/CharStr.m.ct`；`std/str/String.shim.h` → `std/str/CharStr.shim.h` |
| 承接 | [0001 §8 的遗留项](0001-str-string-trait-rename.md)：当时只改 trait 名，未动单元文件 |
| 裁定 | 负责人 2026-11-21 选方案 A：保留 `std::str` 模块，单元与 trait 统一叫 `CharStr` |
| 提出日期 | 2026-11-21 |

## 1. 问题

`0.4.0` 只把 trait 从 `String` 改成了 `CStrOps`，单元文件名与 import 路径仍是 `String`，
读代码时两套名字并存：

- `std/str/String.m.ct:52` 声明 `pub trait CStrOps`；
- 消费者却要写 `import std::str::String;`（单元名取自文件名，不是 trait 名）；
- 同一文件第 1 行自述仍是「`String.m.ct` - … (`std::str::String`)」。

结果与 0001 要消除的混淆同源：名字 `String` 让人以为这是一个拥有文本的值类型，
实际拿到的是 `char *` 上的零成本方法层；真正的拥有型字符串是编译器内置的 `Str`。

## 2. 证据（改名前的状态）

```console
$ grep -n "pub trait" std/str/String.m.ct
52:pub trait CStrOps {

$ grep -rn "import std::str::String" tests std | wc -l
48

$ ls std/str
README.md  String.m.ct  String.shim.h
```

- 48 处 `import std::str::String` 分布在 **21 个 fixture** 与 **9 份模块 README**。
- `tests/run.sh` 的 layout 闸门只校验 `.h.ct` 的 namespace 与 struct 名，不校验 `.m.ct` 的
  文件名与 trait 名，所以现状不会被门禁拦住。

## 3. 方案选择

| 方案 | 内容 | 结论 |
| --- | --- | --- |
| A | `std::str::CharStr`：保留模块，单元与 trait 同时改名 | **采用**（负责人 2026-11-21） |
| B | 保持 `std::str::CStrOps` | 否决：负责人认为观感不佳 |
| C | `std::char::CharStr`：新建 `std::char` 模块 | 否决：与 `std::num::Char`（单字符谓词）语义重叠；需删掉 `std::str` 模块，牵动 116 处引用、45 个文件、`tests/run.sh` 的模块列表与 `tests/str` 目录 |

`CharStr` 的含义是「C 形状的 char 字符串方法层」：既不是内置拥有型 `Str`，也不是值类型
`String`。

## 4. 改动清单（已执行）

1. `git mv std/str/String.m.ct std/str/CharStr.m.ct`、`git mv std/str/String.shim.h std/str/CharStr.shim.h`。
2. `pub trait CStrOps` / `map trait CStrOps for type char *` → `CharStr`。
3. 全仓 `std::str::String` → `std::str::CharStr`（21 个 fixture、9 份模块 README、根 README）。
4. 文件路径引用 `String.m.ct` / `String.shim.h` → `CharStr.*`。
5. 历史表述保留史实：`0.4.0` 记作 `String` → `CStrOps`，`0.20.0` 记作 `CStrOps` → `CharStr`
   （README 版本表、`docs/roadmap.md`、设计文档）。

C 函数前缀 `std_str_*` 与全部方法名不变；shim 按 `<base>.shim.h` 自动包含，只改文件名。

## 5. 版本落点与发布耦合

- 破坏性改名按 README 的版本策略走 minor，落 **`0.20.0`**。
- **ct 0.77 起的官方 pin 表**：发 tag 后 `ct get ct/std@0.20.0` 仍会报
  `no official version of 'std' satisfies '0.20.0' (the built-in pin is 0.19.0)`，
  直到 `ct` 侧 pin 表更新；发版清单要包含这一步。

## 6. 验收判据

| # | 判据 | 命令 | 阈值 |
| --- | --- | --- | --- |
| 1 | 旧 import 路径零命中 | `grep -rn "std::str::String" . \| wc -l` | `0` |
| 2 | 旧文件名零命中 | `grep -rn "String\.m\.ct\|String\.shim\.h" . \| wc -l` | `0` |
| 3 | 新单元存在 | `ls std/str` | 含 `CharStr.m.ct`、`CharStr.shim.h` |
| 4 | 测试门全绿 | `bash tests/run.sh` | `25 passed, 0 failed` |
| 5 | 包清单通过 | `ct pkg check .` | `ok: 4 criteria` |
| 6 | 全门禁通过 | `bash tools/check.sh all` | `gates: 10 ok` |
| 7 | 消费者路径可用 | 隔离 store 下 `ct get <本仓路径>` + `ct build` | 通过，`import std::str::CharStr` 可解析 |

## 7. 非目标

- 不改任何方法名（`length` / `find` / `copy` / `parse_long` …）。
- 不改 C 侧函数名（`std_str_*`）与语义。
- 不顺带改 `Path` / `File` 等同类 trait 名（[0001 §6](0001-str-string-trait-rename.md) 已登记）。

## 8. 处置（DEV，2026-11-21）

已按方案 A 实现，工作区改动 43 个文件（含 2 个文件改名）。原始输出：

```console
$ bash tests/run.sh | tail -1
std: 25 passed, 0 failed

$ ct pkg check .
ok: 4 criteria

$ bash tools/check.sh all | tail -1
gates: 10 ok, 0 failed/blocked
```

消费者路径（隔离 `CT_HOME`，避免污染真实 store）：

```console
$ CT_HOME=<tmp> ct get /Users/samxx/trae-ws/std
resolved   ct/std 0.19.0
  store    <tmp>/pkg/ct/std/0.19.0
  action   installed
$ CT_HOME=<tmp> ct build main.ct -o main --cflags "-Wall -Wextra"
'cc' -std=c99 -Wall -Wextra -I '…/std/str' … '-o' 'main'
$ ./main; echo $?
0
```

未做：提交、bump `0.20.0`、打 tag、推送（等负责人指令）；复核签字由 REV 认定。
