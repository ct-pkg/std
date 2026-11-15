# 包外移入事项（`std` 侧）认领与状态登记

| 项 | 值 |
| --- | --- |
| 性质 | **只做认领与状态登记**：不改 `ct.pkg`、不动版本号、不打 tag、不跨仓 |
| 来源 | CT 仓移出的包外事项清单中 `EXT-STD-01`…`EXT-STD-05`（按移交文本认领） |
| 责任 | 自本文件起，这些事项由本仓负责推进与验收，不再回写 CT 仓 |
| 基线 | `0.3.1`；默认分支 `main` |
| 日期 | 2026-11-15 |

## 1. 前置能力（只记能力，不记版本）

| 代号 | 能力 | 现状 |
| --- | --- | --- |
| C1 | `ct get` 落 store，并能读取包的 `publisher` | **未落地** |
| C2 | `ns-root` 进入 `-I` 解析 | **未落地** |
| C3 | `ct build` 默认读取 lock | **未落地** |

在 C1/C2 落地前，`ct.pkg` 里的 `publisher` / `ns-root` 属于「已声明但不生效」；此时改它们没有
验收手段，只会制造中间态。本仓因此**保持现状不动**。

## 2. 逐条认领

### EXT-STD-01 `ct.pkg` 补 `publisher` / `ns-root`

| 项 | 内容 |
| --- | --- |
| 来源 | 原 CT `roadmap.md` S 表 S4 行；提案 §10 S4 |
| 要做什么 | `ct.pkg` 声明包身份：`publisher = ct`、`ns-root = .` |
| 前置 | C1（读 `publisher`）、C2（`ns-root` 进 `-I`） |
| 本仓现状 | **声明已具备**：`ct.pkg:39` `publisher = ct`、`ct.pkg:40` `ns-root = .`（连同 `:38` `name`、`:41` `version`）。注释已说明当前机制下这两个键被接受但不生效 |
| 验收 | 前置具备后 `ct pkg check .` 返回 `RC=0` |
| 本次动作 | 仅登记；**不改 `ct.pkg`**、不重打 tag |

### EXT-STD-02 处理 `v0.2.0` 游离 tag

| 项 | 内容 |
| --- | --- |
| 来源 | 同上 §10 S4 |
| 要做什么 | 让 `v0.2.0` 并回默认分支，或按新版本重打 |
| 前置 | 无（纯 git 状态） |
| 本仓现状 | **已满足**：`git merge-base --is-ancestor v0.2.0 main` → `RC=0`；`git rev-parse v0.2.0^{}` = `b04b7d6`，在 `main` 历史内。`v0.2.0` 是 annotated tag，指向 0.2.0 后的文档提交，不是游离对象 |
| 验收 | `git merge-base --is-ancestor v0.2.0 <default>` 返回 `RC=0` |
| 本次动作 | 仅登记；不重打、不删除 tag |

### EXT-STD-03 依赖一律 `<publisher>/<name>@<range>`

| 项 | 内容 |
| --- | --- |
| 来源 | 同上 §10 S4 |
| 要做什么 | 包依赖不得使用裸名；一律 `<publisher>/<name>@<range>` |
| 前置 | C1/C2（依赖解析与 store 身份） |
| 本仓现状 | **无可执行对象**：`ct.pkg` 当前没有任何包依赖，只有 `ct.pkg:43` `requires = ct >= 0.30.0`（编译器下限，不是包依赖）。规则本身成立，等首个依赖引入时适用 |
| 验收 | 引入依赖后，依赖图内包名唯一；`ct pkg check .` `RC=0` |
| 本次动作 | 登记为**长期规则**；不新增/不修改任何依赖行 |

### EXT-STD-04 应用层能力缺口评估

| 子项 | 本仓结论（评估，不排期） | 依据 |
| --- | --- | --- |
| `String` 增量 | **有缺口，建议补**：`rfind_char`、`compare_ignore_case`、`parse_long` / `parse_llong` / `parse_ulong` / `parse_real` | [issue 0001](../issues/0001-str-string-trait-rename.md)、[设计提案 §2.1](design/std-scope-and-presentation.md) |
| `fs` 原子写 + `rename` | `rename` **已有**（`Fs.rename`）；**原子写缺**：现有 `write_file` 是整文件替换（`std/fs/README.md:10`），不是 tmp + `rename` 的原子路径。建议评估 `write_file_atomic` | `std/fs/README.md:10`、[设计提案 §2.5](design/std-scope-and-presentation.md) |
| 公开 `stat` 优先 | **已有替代方案**：不建 `FileStat` 值类型，先提供 `file_size()` / `file_mode()` / `file_mtime()` 标量查询；多字段需求出现后再评估 | [设计提案 §2.5、§2.8](design/std-scope-and-presentation.md) |
| `Map` 按需 | **不做**：`Map` / `MapEntry` 是编译器内置层，本包不提供 `std::map`（0.3.0 已删） | `ct.pkg:18-21` 的说明、`README.md` 版本表 |
| `Vec` 泛化与 UTF-8 宽度 | `Vec` 属内置层，**不归 std、不向 CT 另立任务**；UTF-8 **宽度缺口真实存在**：现只有 `decode` / `count_codepoints` / `encode`（`std/utf8/README.md:3`），没有「显示宽度」能力，建议评估 `count_columns`（含东亚宽字符规则） | `std/utf8/README.md:80`、[设计提案 §2.6](design/std-scope-and-presentation.md) |

原结论「0.x 不做」**需本仓重新确认**：本仓不直接推翻，拆成上表五项，待 owner 逐项拍板（§4 Q1）。

### EXT-STD-05 五包与回调契约评估

| 包 | 本仓结论 | 理由 |
| --- | --- | --- |
| `arena` | **已在 std，不新建包** | `std::arena` 的 `Arena` + `ArenaOps` 已存在（`std/arena/Arena.h.ct`） |
| `string` | **不新建包** | `std::str` 已覆盖 `char *` 借用层（拟改 `CStrOps`），拥有型字符串由编译器内置 `Str` 提供；再建包会形成第三套写法 |
| `result` | **暂缓，依赖语言能力** | 需要语言级「可判错的结果/联合类型」能力；std 不能在方法层自造一套错误联合体 |
| `sync` | **不归 std** | 并发/同步属语言与运行时能力，落在本仓边界外（[设计提案 §1.4](design/std-scope-and-presentation.md)） |
| `callback` | **暂缓，依赖语言能力** | 需要语言级函数指针/回调能力；回调契约应由语言层定义后再谈包装 |
| 回调契约 | **待前置** | 与 `callback` 同源：语言能力未定，契约无法定稿；本仓不预设签名 |

结论：五个包里 **0 个新包**；`arena` 已在，`string` 与 `sync` 明确不做，`result` / `callback`
等语言能力。是否作为独立提案立项见 §4 Q2。

## 3. 本仓实际状态

**已具备**

- `publisher` / `ns-root` 两个键已在清单中（声明层，等 C1/C2 生效）。
- `v0.2.0` 已在默认分支历史内（EXT-STD-02 验收已过）。
- `Fs.rename`、`path.exists`、`file_size` 类查询有落地路径（现有或提案内）。

**缺失 / 阻塞**

- C1 / C2 / C3 全部未落地 → EXT-STD-01 无法验收，EXT-STD-03 无依赖可查。
- `fs` 原子写（tmp + rename）不存在。
- UTF-8 显示宽度不存在。
- 无任何包依赖（因此 EXT-STD-03 目前只是规则，不是待修项）。

**工作区未提交改动登记**

| 文件 | 现状 | 建议 |
| --- | --- | --- |
| `ct.pkg` | **无未提交改动** | 保持不动（前置未满足） |
| `issues/0001-path-file-traits.md` | 有 1 处未提交改动：把样例命令从绝对路径改成 `ct build -I .` | 与本次无关，**未提交也未丢弃**；建议 owner 决定单独提交或回退 |

## 4. 需要 owner 拍板

- **Q1**：EXT-STD-04 五个子项——`String` 增量、`fs` 原子写、`stat` 标量查询、`Map` 不做、
  `Vec`/UTF-8 宽度——是否按上表结论推进？其中原子写与宽度是否进入下一版候选？
- **Q2**：EXT-STD-05 是否作为独立提案立项？`result` / `callback` 是否等语言能力，`sync` 是否
  明确排除，`arena` / `string` 是否确认不建包？
- **Q3**：EXT-STD-01 的验收时机——C1/C2 落地后是否由本仓重跑 `ct pkg check .` 并补 tag？
- **Q4**：`issues/0001-path-file-traits.md` 的未提交改动由谁提交（本仓不代为处置）。

## 5. 执行纪律

- 前置能力未满足前：不改 `ct.pkg` 的 `publisher` / `ns-root`，不动版本号，不打 tag。
- 不跨仓操作，不读取本仓以外的项目目录；CT 不再承担这些事项。
- 认领后的推进、验收、文档维护都在本仓完成。
