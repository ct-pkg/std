# Issue 0001：`std::str::String` 这个 trait 名应当改掉

| 项 | 值 |
| --- | --- |
| 编号 | 0001 |
| 状态 | **已解决（0.4.0，trait 改名；单元文件名未改，见 §8）** |
| 类型 | 破坏性改名（BREAKING，仅 trait 名，方法名不变） |
| 版本落点 | `0.4.0` + 新 tag `v0.4.0` |
| 涉及单元 | 现 `std/str/String.m.ct` / `String.shim.h` → 目标 `CStrOps.m.ct` / `CStrOps.shim.h` |
| 已定名 | `CStrOps`（负责人 2026-11-15 决定，理由见 §3） |
| 提出日期 | 2026-11-15 |

## 1. 问题

`std::str::String` 这个名字暗示「拥有文本的值」，但它实际是一个**不拥有内存的方法层**：一组
`map trait ... for type char *` 的静态映射，调用点直接展开成 `std_str_*`，没有接口值、没有
vtable、不分配、不持有长度。真正拥有内存、带长度的字符串是编译器内置层的 `Str`
（`char *data; int len;` + `use Drop`）。

两者并存时，名字与所有权语义正好相反：

- 读 `Str` 的人以为「内置的那个是借用视图，`String` 才是拥有的值」——事实相反；
- 一个不拥有任何东西的 trait 占用了生态里最常表示「拥有字符串」的名字，消费者写
  `String` 时无法从名字判断是否需要负责释放。

`std/str/String.m.ct:9-11` 的注释已经承认这是「避免与 C 层 `std_str_*` 重名」的退让选择，
不是命名惯例的推论。

## 2. 证据

| 位置 | 内容 |
| --- | --- |
| `std/str/String.m.ct:49` | `pub trait String {` —— 14 个纯读/写调用方缓冲区的方法，无字段、无 Drop |
| `std/str/String.m.ct:66` | `map trait String for type char * {` —— 宿主是裸 `char *`，零成本静态映射 |
| `std/str/String.m.ct:17-22` | 「Every method here reads the receiver…nothing allocates and nothing mutates the receiver」 |
| 语言侧内置 `Str`（实现不在本仓） | `pub struct Str { char *data; int len; } use Drop;` —— 内置、拥有、带长度、随作用域释放 |

即：同一个包里，`String` 不拥有，`Str` 拥有；名字给出的暗示与所有权相反。

## 3. 建议

**主方案：`String` → `CStrOps`。**

`CStrOps` 明确表达「借用的、NUL 结尾的 C 字符串方法层」，与内置 `Str`（拥有、带长度、
非 NUL 结尾）互补，而不是竞争：

- 模块的方法确实按 NUL 结尾工作：`std_str_len` 逐字节数到 `'\0'`（`std/str/String.shim.h:34-42`），
  `find` / `starts_with` / `ends_with` 都以它为长度源；唯一例外是 `copy_to`
  （`std/str/String.m.ct:61`，显式 `n` 字节、不补 NUL、不检查容量）。
- `C` 前缀把「这是从 C 侧借来的视图」写进名字，`Ops` 后缀说明这是方法层、不是值类型。

同时它维持本包既有的 `xxxOps` 形态（`String` 是唯一例外）：

| 单元 | trait | 宿主 | 形态 |
| --- | --- | --- | --- |
| `std/fs/Fs.m.ct:31` | `FsOps` | `char *`（`:45`） | `xxxOps` |
| `std/fs/Fs.m.ct:41` | `FsEnv` | `namespace std::fs`（`:55`） | 命名空间用 `xxxEnv` |
| `std/arena/Arena.h.ct:31` | `ArenaOps` | 自有 `struct Arena`（`:38`） | `xxxOps` |
| `std/test/Test.h.ct:29` | `TestOps` | 自有 `struct Test`（`:37`） | `xxxOps` |
| `std/utf8/Utf8Ops.m.ct:35` | `Utf8Ops` | `char *`（`:41`） | `xxxOps` |
| **`std/str/String.m.ct:49`** | **`String`** | **`char *`（`:66`）** | **例外** |

**次选：`StrOps`。** 形态同样合规，但 `StrOps` 与内置 `Str` 只差后缀、共享 `Str` 前缀，视觉上
仍可能被读成「`Str` 的方法层」——也就是「拥有值的那个类型的方法」，这正是本 issue 要消除的
误解；不如带 `C` 前缀那样把「借用」讲清楚。

**兜底：`TextOps`。** 形态合规、不暗示拥有；但本模块是**字节**层，码点/字素在 `std::utf8`，
`text` 比 `str` 更容易让人以为这里管编码。

**已否决：`CharOps`。** 在 CT 里 `char` 是**单字符标量**，`std::num` 的 `Char` trait
（`std/num/Num.m.ct:94`）已经承载 `is_digit` / `is_alpha` / `is_space` / `is_upper` / `is_lower` /
`to_upper` / `to_lower` 这批字符判断方法（`:104-112` 映射到 `char` 宿主）。叫 `CharOps` 会把读者
引向「字符判断层」，与真实含义（借用的字符串方法层）相差更远，误解比原名 `String` 更深，故排除。

## 4. 影响面

方法名与调用形态全部不变（`s.to_lower(out, cap)`、`s.len()` 等不受影响），改动只落在
**trait 名及其可见入口**：

1. **`import` 路径（如沿用本包 file = unit 的惯例）**：`README.md:321` 明确
   `import std::str::String` 对应 `std/str/String.m.ct`，所以 trait 改名要连带把单元文件
   改为 `CStrOps.m.ct`（shim 同对改为 `CStrOps.shim.h`），`import` 随之变成
   `std::str::CStrOps`。`tests/run.sh:72-88` 的 layout 闸门只校验 `*.h.ct` 的 namespace/struct，
   不会拦住这件事，属于惯例约束而非门禁约束。
2. **本仓调用点**：10 个 fixture 直接 `import std::str::String`——
   `tests/str/{main,first,guide,coverage,parse}.ct`、`tests/fs/{main,guide}.ct`、
   `tests/utf8/{first,guide}.ct`、`tests/all/main.ct`。
3. **本仓文档**：`README.md`（`:3`、`:57`、`:86`、`:128`、`:270`、`:301`、`:321`）、
   `std/README.md:8`、`std/str/README.md`（8 处）、`std/utf8/README.md:36,142`、
   `std/fs/README.md:144,218`。
4. **源码内自述**：`std/str/String.m.ct:2,33`、`std/str/String.shim.h:2` 的文件头注释。
5. **仓外**：所有 `import std::str::String` 的第三方 ct-pkg 包与各自 README；这些仓库不在本仓
   范围内，改名时要同步发一条迁移说明（旧名 → 新名，无兼容别名，遵循本包 0.2.0 起的
   「不留别名、不留兼容层」口径）。

## 5. 版本落点

按 `README.md:494` 的版本策略「`0.x` 期间：破坏性改动走 minor（`0.1.0` → `0.2.0`），
修复走 patch」，本项属于破坏性改名，落 **`0.4.0`**，并打 `v0.4.0`（`README.md:496`：tag 为
`v<version>`，索引只认 `v*` 且取最高一个）。

另注意 `README.md:499-501`：tag 之后的包摘要覆盖 `README.md`、`tests/` 等目录下所有文件，
任何改动都要新版本号 + 新 tag，所以本次改名**不能**在 `v0.3.1` 的内容上就地修改。

## 6. 同类问题（「看似类型、实为方法层」的 trait）

全仓 `pub trait` 共 16 个，按严重度排列：

| trait | 声明 | 宿主 | 判定 |
| --- | --- | --- | --- |
| `String` | `std/str/String.m.ct:49` | `char *` | **本 issue 对象。** 与内置拥有型 `Str` 语义相反，最严重 |
| `Path` | `std/io/File.m.ct:11` | `char *`（`:15`） | **同类。** 读起来像一个路径类型（别处常是拥有字符串的类型），实际只有 `open` 一个方法；建议 `PathOps`，或并入 `FsOps` 一类的 `xxxOps` |
| `File` | `std/io/File.m.ct:19` | `IoFile *`（`:25`） | **同类（轻）。** 宿主是有名字的句柄 `IoFile`，trait 却叫 `File`，读起来像另一个类型；改名可考虑 `FileOps` |
| `Int` / `Long` / `LLong` / `Char` | `std/num/Num.m.ct:32/53/73/94` | `int` / `long` / `long long` / `char`（`:42/63/83/104`） | **同类（轻）。** 名字就是宿主原始类型，不承诺所有权，不会造成「谁释放」的误解；不属 `xxxOps` 形态，若统一口径可改 `IntOps` 等 |
| `Float` / `Real` | `std/math/Real.m.ct:19/35` | `float` / `double`（`:27/47`） | **同类（轻）。** 同上；另外 `Real` 映射 `double`、`Float` 映射 `float`，两个名字读起来像两个标量类型 |
| `Stdout` | `std/io/Stdout.m.ct:5` | `namespace std::io`（`:12`） | **不同类。** 命名空间级入口，不是值类型方法层，不产生所有权歧义；可不改 |
| `Math` | `std/math/Math.m.ct:18` | `namespace std::math`（`:23`） | **不同类。** 同上 |
| `FsOps` / `FsEnv` / `ArenaOps` / `TestOps` / `Utf8Ops` | 见 §3 | `char *` / namespace / 自有 struct | 已符合惯例，无需处理 |

结论：真正的「名字指向类型、实际是方法层」问题集中在 `String`、`Path`，`File` 次之；
`Int` / `Long` / `LLong` / `Char` / `Float` / `Real` 只是形态不统一，不涉及所有权误导；
`Stdout` / `Math` 是命名空间映射，属于另一类，不计入。

**范围声明**：`Path` / `File` 只在本文档作为同类问题登记，**不**并入本 issue 一起改名；
是否改成 `PathOps` / `FileOps`，要等负责人另行确认后单独开 issue、单独落版本。本 issue 的
唯一改动对象是 `String` → `CStrOps`。

## 7. 非目标

- 不改任何方法名（`len` / `find` / `starts_with` / `to_lower` / `copy_to` … 保持原样），
  `s.to_lower(out, cap)` 这类调用点不受影响。
- 不动实现、不动 shim 的 C 函数（`std_str_*` 全部保留）。
- 不 bump 版本、不打 tag、不发布（落点留给 `0.4.0` 的独立改动）。
- 不处理 `README.md:515-519` 的保留名豁免（`len` / `max` / `min`），与本 issue 无关。

## 8. 处理结果（2026-11-19 复核）

- **trait 名已改**：`std/str/String.m.ct:52` 现为 `pub trait CStrOps`，映射行是
  `map trait CStrOps for type char *`（`:79`）。0.4.0（P0）落地，方法名与调用形态未变。
- **单元文件名未改**：仍是 `std/str/String.m.ct` / `String.shim.h`，`import std::str::String`
  保持不变。§4 设想的 `CStrOps.m.ct` 没有采用：P0 把范围收窄为 trait 改名，避免同时改
  import 路径与全部夹具；`tests/run.sh` 的 layout 闸门只校验 `.h.ct`，也不会强制它。
- **影响面核对**：`grep -rn "pub trait String\|map trait String " std/` 为 0 命中；本仓
  调用点全部通过 `import std::str::String` 使用新 trait 名。
- **遗留（未做，非阻塞）**：单元文件与 import 路径的 `String` 字样仍在。若负责人要求连
  unit 一起改成 `CStrOps.m.ct`，需要同步 10 个 fixture 与全部 README 的 import，单独开
  issue 处理；本 issue 不再跟踪。
