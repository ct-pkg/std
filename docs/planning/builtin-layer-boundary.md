applies-to: v0.19.0

# 内建层边界与 `lib/std` → `lib/core` 接应

| 项 | 值 |
| --- | --- |
| 性质 | 规划（REQ 维护）；对应语言侧「内建层」收权，本仓只做接应 |
| 定位 | `std` 包收缩为**逃生层 + 用户面工具**：`str` / `fs` / `utf8` 做 `char *` 方法映射，`arena` / `test` 是纯库设施；**语言级类型（`Vec` / `Map` / `Str` / `String` / `Box` / `Option` / `Result`）不在本仓重复出现**。逃生层还包括 C 资源句柄（`io::File` / `FileDesc`、`fs::Dir`、`locale::LocaleHandle`、`time::Timer`、`net::Socket`）与 libc/POSIX 原语映射，它们保留（裁定见 §6） |
| 语言仓动作 | 内建层 `lib/std/` 将更名为 `lib/core/`；`Drop` / `AutoDrop` / `Handback` 的正式声明收归内建层。两项都由语言仓库执行，本仓不抢跑 |
| 当前状态 | 任务一已核查（无需删除）；任务二待语言仓合并后加指针；任务三清单已成文，等改名触发 |

## 1. 模块分类（按定位口径）

| 类别 | 模块 | 判据 |
| --- | --- | --- |
| `char *` 方法映射（保留） | `str`、`fs`、`utf8` | 接收者是 C 已有类型，零分配，写调用方缓冲区 |
| 纯库设施（保留） | `arena`、`test` | CT 自有值类型，不映射语言级语义 |
| C 资源句柄（逃生层，保留） | `io`（`File` / `FileDesc`）、`fs::Dir`、`locale::LocaleHandle`、`time::Timer`、`net::Socket` | 包装 `FILE *` / fd / `DIR *` / `timer_t` 等 C 资源，不是语言级类型 |
| 原语方法层（逃生层） | `num`、`math`、`search`、`env`、`process`、`errno`、`wchar` | 宿主是 C 原语或 namespace，不定义拥有型值 |
| POSIX 直连（逃生层） | `posix`、`mmap`、`poll`、`thread`、`iso_c` | 只做 libc/POSIX 直映射 |
| 已移出 | `vec`、`map` | 0.3.0 删除，见 §2 |
| 语言级类型 | —— | 本仓不定义、不复制；`Str` / `Vec` / `Map` 归语言侧 |

## 2. 任务一：清理被内建层接管的模块

### 2.1 `std::vec` / `std::map`

**结论：0.3.0 已删除，本次无需再删、不发 MINOR。**

```text
$ ls std | grep -E '^(vec|map)$'
（无输出）

$ grep -rn "std::vec\|std::map" --include="*.ct" --include="*.h" .
（无输出）
```

文档中的命中全部是历史说明（`README.md:19`、`README.md:460`、`README.md:466`、
`README.md:491-493`、`std/README.md:28`、`docs/design/std-scope-and-presentation.md:47`），
已注明「0.3.0 删掉」与替代写法，属允许保留的历史段。

消费者若仍写旧 import，编译器给的是硬错，不是静默消失：

```text
$ ct build main.ct          # main.ct 里写 import std::vec::Vec;
./std/vec/Vec.h.ct:47:1: error: 'Vec' is a built-in type and cannot be redeclared
```

### 2.2 owning 类型工具核查

| 检查 | 结果 |
| --- | --- |
| `pub struct String` / `pub type String` | 0 命中 |
| `pub struct Str` / `pub type Str` | 0 命中 |
| `pub struct Box` / `Option` / `Result` | 0 命中 |
| `pub struct Vec` / `Map` | 0 命中 |

`std::str` 的 `CharStr` 是 `char *` 的方法层（trait），不是拥有型值类型；0.20.0 起
单元与 trait 统一叫 `CharStr`（issue 0004），进一步避开与内置 `Str` 的混淆。
`io::File` / `FileDesc`、`fs::Dir`、`locale::LocaleHandle`、`time::Timer`、`net::Socket`
是 C 资源句柄，属于逃生层，不属「语言级类型重复定义」，本次不动。

### 2.3 `Str` 本体残留

**结论：无残留声明。** `std/` 下无 `Str` 的 struct / type 声明；文档中出现的 `Str`
均表述为「语言侧内置、本仓只作消费方」（`README.md:116`、`docs/design/std-scope-and-presentation.md:295`、
`std/str/CharStr.m.ct:11` 附近）。本仓无 PH-80 记录，PH-80 属语言仓编号，不在本仓台账。

## 3. 任务二：内建 trait 显性化的包内跟进

现状（全仓 grep）：

| 名字 | 命中 | 说明 |
| --- | --- | --- |
| `Drop` | 142 行 / 16 份文档 | 全部是用法（`use Drop` + `impl Drop`）或机制描述 |
| `AutoDrop` | 0 | 本仓未出现 |
| `Handback` | 0 | 本仓未出现 |

表述核查：**没有任何一处把 `Drop` 写成「用户自定义 trait」**。`README.md:421` 已经写作
「语言内建的析构钩子」。因此本次不需要改写措辞，只差**一个指针**：指向内建层声明文件。

待语言仓合并后（声明落在 `lib/std/`，改名为 `lib/core/` 后由 §4 统一替换）：

| 落点 | 动作 |
| --- | --- |
| `README.md:421`（术语表 `Drop` 行） | 补「声明见语言仓内建层 `<路径>`」，不复制声明 |
| `docs/design/decision-resource-ownership.md`（结论段） | 同上，补一处指针 |
| 各模块 README 的 `use Drop` 说明 | 不加指针，避免 7 处重复 |

`AutoDrop` / `Handback`：本仓零引用，语言仓合并后无需动作；若届时新增引用，按同一规则
（只指针、不复制声明）处理。

## 4. 任务三：`lib/std` → `lib/core` 路径清单（预登记）

### 4.1 清单

`grep -rn "lib/std" --include="*.md" --include="*.ct" --include="*.sh" --include="*.yml" --include="*.py" .`
共 9 处，全部在文档里，`tools/`、CI 与源码零命中：

| # | 位置 | 原文要点 | 改名前 | 改名后 |
| --- | --- | --- | --- | --- |
| 1 | `README.md:18` | 内置层位置 `lib/std/Vec.h.ct` | 现状 | `lib/core/Vec.h.ct` |
| 2 | `README.md:274` | 内置层提供 `Vec` | 现状 | 同 #1 |
| 3 | `README.md:311` | 编译器查找路径 `CT_STD_DIR`、`lib/std/` | 现状 | 路径改 `lib/core/`；**环境变量名以语言仓为准** |
| 4 | `README.md:312` | 输入文件旁 `../lib/std/` | 现状 | `../lib/core/` |
| 5 | `README.md:436` | 同 #1 的排查提示 | 现状 | 同 #1 |
| 6 | `README.md:468` | 0.30.0 起内置层 | 现状 | 同 #1 |
| 7 | `README.md:499` | 历史句「0.30.0 之前不存在 `lib/std/`」 | **保留**（史实） | 不改，必要时加「现为 `lib/core/`」 |
| 8 | `README.md:598` | 命名规范适用范围「`lib/std` 内置层」 | 现状 | `lib/core` 内置层 |
| 9 | `std/README.md:27` | 内置层位置 | 现状 | 同 #1 |

### 4.2 改名触发后的机械步骤

1. 重新跑 §4.1 的 grep（行号会漂移，以命中内容为准，不用旧行号）。
2. 只替换指向「当前内建层」的 8 处；#7 历史句保留，必要时补注。
3. 向语言仓确认 `CT_STD_DIR` 是否随目录改名（`CT_CORE_DIR`？），未确认前不动该变量名。
4. 跑 `bash tools/check.sh all` 与 `ct pkg check .`。
5. 文档变更需要新 tag：按 issue 0004 的落点并进 `0.20.0`；若改名晚于该版本，则单独开 MINOR。
6. 更新 `ct-pkg/index`（由维护者跑 `tools/refresh-index.sh`），并确认 `ct index verify` 绿。

## 5. 验收判据

| # | 判据 | 命令 | 阈值 |
| --- | --- | --- | --- |
| 1 | 内建类型无业务命中 | `grep -ri "std::vec\|std::map" --include="*.ct" .` | `0` |
| 2 | 无 `Drop` 声明复制 | `grep -rn "trait Drop" .` | `0` |
| 3 | 无 owning 类型残留 | `grep -rn "pub struct \(String\|Str\|Box\|Option\|Result\)" std/` | `0` |
| 4 | 索引可验证 | `ct index verify <index 文件>` | `index verify: OK` |
| 5 | 不留兼容别名 | 旧 `import` 在消费者工程编译 | 非零退出（`cannot resolve import`）；不提供 `String` 别名或转发单元 |
| 6 | 路径清单成文 | 本文 §4.1 | 9 行齐全 |

## 6. 已裁定：方案 B（负责人 2026-11-21）

**只删语言级类型重复模块，保留 C 资源句柄与 libc/POSIX 映射。**

| 项 | 内容 |
| --- | --- |
| 删除对象 | 与内建 `Vec` / `Map` / `Str` / `String` / `Box` / `Option` / `Result` 语义重叠的模块或类型。核查结果 0 处（`vec` / `map` 已于 0.3.0 删除） |
| 保留对象 | `io` / `time` / `net` / `locale` / `env` / `process` / `errno` / `wchar` / `num` / `math` / `search` / `posix` / `mmap` / `poll` / `thread` / `iso_c`：宿主是 C 类型、C 资源或 namespace，不重复定义语言级类型 |
| 本轮动作 | 不删任何模块；`str` / `fs` / `utf8` / `arena` / `test` 的功能不动；不新增模块 |
| 后续触发 | 只有出现新的「语言级类型重复」（例如语言层新增某类型而本仓也有同名定义）时，才另开 issue 处置；C 资源句柄不属此类 |
