# `std` 的范围与呈现形态：完整方案（评审稿·两层模型）

| 项 | 值 |
| --- | --- |
| 性质 | 主张稿：只改文档，不写实现、不动版本、不开相卡 |
| 状态 | **待负责人过目**；点头前不落任何代码 |
| 前提 | 底座是零代码映射（标准 C 有什么就直连什么）；在此之上，凡挂到 `char *` 或基础类型上能明显省书写、读着更自然的，补方法层，这一层可以铺宽；`Drop` 句柄只留给真正持有资源的情况 |
| 风格准绳 | CT 自己的范式：字符串起头、工厂函数进、方法挂既有类型、链路一路走到底 |
| 范围约束 | std 的事只在 std 仓处理，不向 ct-lang 提任何安排 |
| 底稿 | [libc 覆盖盘点](../libc-inventory.md)、[issue 0001](../../issues/0001-str-string-trait-rename.md) |

## 1. 边界与形态模型

### 1.1 两层模型

| 层 | 形态 | 判据 | 代码量 |
| --- | --- | --- | --- |
| **① 底座** | 零代码映射：声明 + shim 一条转发 | 标准 C 有对应函数，语义能一一对应 | shim 1 行（M0）；出参/长度保护等几行（M1） |
| **② 便利层** | 方法挂到 `char *` / 基础类型（`int`/`long`/`double`/`char`）上 | **书写负担明显下降、读起来更自然**（见 §1.2） | 复用 ① 的映射，必要时 shim 里几行转换 |
| **③ OO** | 新类型 + `own` + `Drop` | **真的持有资源**（`DIR *`、fd）；零代码映射无法表达「谁关闭」 | 一个 struct + `Drop` + 薄方法 |

一句话：**能力入口靠 ①，顺手靠 ②，只有资源才配 ③**。

### 1.2 便利层的准入：便利性本身就是理由

不需要再拿生命周期论证。满足任一条即可补：

- 链式读法明显更短：`"abc".starts_with("x")` 对 `str_starts_with(s, "x")`；
- 基础类型的值能直接接方法：`n.to_string()`；
- 工厂从字符串进：`"f".open("r")`、`"dir".open_dir()`、`"host:80".connect()`；
- 结果还能继续走：`n.to_string().len()`。

不满足的（写了不比函数式短、或语义会被误读）不补，避免名字通胀。

### 1.3 在范围内

C 标准库 + POSIX.1 中 CT 侧有稳定用法的能力：字符串/内存视图、stdio、目录、`stat`、时间、
环境变量、进程身份、错误码文本、socket。自有设施：`Arena`、`Test`、`Utf8Ops`。

### 1.4 不归 std

| 类别 | 理由 |
| --- | --- |
| 编译器内置层（`Vec` / `Map` / `Str`） | 0.3.0 已删 `std::vec` / `std::map`；重名撞内置类型 |
| 泛型容器、分配器抽象、线程/并发、异步/事件循环 | 语言与运行时能力；**且这类安排不往 ct-lang 推** |
| 第三方包（JSON / HTTP / 正则 / 日志 / CLI） | 独立 ct-pkg |
| 进程控制与信号（`fork` / `exec*` / `system` / `popen`） | CT 无进程/并发模型 |
| locale、宽字符、多编码 | 编码走 `std::utf8`；locale 是平台全局状态 |
| Windows / 非 POSIX | 现有实现依赖 `dirent.h` / `unistd.h` |
| 编译器诊断、构建工具、包管理器 | 属于 ct / ct-pkg |

注：纯内存操作（`memcpy` / `memmove` / `memset` / `memcmp` / `memchr` / `strncmp`）不再排除——
已由 `char *` 方法层封装（§2.1），方法层编译期展开，无运行时开销。

### 1.5 ③ 的准入（只此一条）

**真的持有资源**：一次调用创建、多次调用共享、必须显式关闭。命中后仍先判能否寄生基本类型；
判不掉才允许新类型 + `Drop`，其余留在 ① 或 ②。已定 `Dir`、`Socket`；`Fd`、`Timer` 按此条判定。

### 1.6 风格准绳与偏离登记

CT 范式：**字符串起头 → 工厂函数进 → 方法挂既有类型 → 链路走到底**。凡偏离，必须在 §3.6 登记
取舍，不能默默引入「更标准但更别扭」的写法。

### 1.7 交付口径：全库覆盖，整块交付

std 是对标准库的一次整体转化，不是分批交付：在本方案划定的标准库范围内（§1.3/§1.4）该有的能力
一个都不缺，最终以全库覆盖为完成判据（§7）。单个模块仍不得做半截——同一模块的公共能力如果只封一半，
使用者无法判断何时该用封装、何时该退回 libc，封装反而增加决策成本（`net` 一次做全即按此条执行，
见 §2.7 与 §8.2 Q6）。

## 2. 分模块纳入清单

列格式：`形态①/②/③`，括号里是代码级 `M0`（单行转发）/ `M1`（几行适配）。

### 2.1 `std::str`（`char *`）

| API | 形态 | 状态 | 说明 |
| --- | --- | --- | --- |
| `len` `find` `find_char` `starts_with` `ends_with` `copy` `slice` `concat` `to_lower` `to_upper` `compare` | ② 方法层（M1） | 已有 | 字符串起头的核心链路 |
| `to_int` `to_double` | ② 方法层（M0） | 已有 | `-> atoi/atof` |
| `rfind_char` / `compare_ignore_case` | ② 方法层（M1） | 新增 | 补齐反向查找与忽略大小写比较 |
| `parse_long` / `parse_llong` / `parse_ulong` / `parse_real` | ② 方法层（M1） | 新增 | 需要 `strtol` 的 `endptr` + `errno` 才能报错；底座仍是映射 |
| `copy_from` / `fill` / `compare_with` / `find_byte` / `compare_prefix` | ② 方法层（M1） | 新增 | 内存操作（宿主 `char *`，`void *` 挂不了方法）：`copy_from` 统一映射 `memmove`（允许重叠，不另设 `memcpy` 名字）；`fill` → `memset`；`compare_with` → `memcmp`（返回三态，不布尔化）；`find_byte` → `memchr`（返回指针或 `NULL`）；`compare_prefix` → `strncmp`（`\0` 停） |

`memcpy` / `memmove` 合并为一个 `copy_from`：`memmove` 允许源/目标重叠，是 `memcpy` 的安全超集；
统一用它可消除“该选哪个”的成本，方法层编译期展开、无运行时开销。既有 `copy_to`
（`src.copy_to(dst, n)`）是同一能力、方向相反；按“方向只保留一个”，0.4.0 起统一为
`dst.copy_from(src, n)`，`copy_to` 退场（破坏性改名，随 issue 0001 同批通告）。

另：`String` → `CStrOps`（issue 0001，`0.4.0`），除 `copy_to` → `copy_from` 外方法名不变。

### 2.2 `std::num`（4 个宿主）

| API | 形态 | 状态 |
| --- | --- | --- |
| `abs` `clamp` `max` `min` `to_base` | ② 方法层（M0/M1） | 已有 |
| `to_str` `to_string` | ② 方法层（M1，`snprintf`） | 已有 |
| `is_digit` `is_alpha` `is_space` `is_upper` `is_lower` `to_upper` `to_lower` | ② 方法层（M0） | 已有 |
| `is_alnum` `is_punct` `is_print` `is_graph` `is_cntrl` `is_blank` `is_xdigit` | ② 方法层（M0） | 新增（`char` 宿主，各 1 行） |

### 2.3 `std::math`（`float` / `double`）

| API | 形态 | 状态 |
| --- | --- | --- |
| `abs` `sqrt` `floor` `ceil` `round`（`Float`/`Real`） | ② 方法层（M0） | 已有 |
| `is_nan` `is_inf`（`Real`） | ② 方法层（M0/M1） | 已有 |
| `pow` `exp` `exp2` `expm1` `log` `log2` `log10` `log1p` | ② 方法层（M0） | 新增：`2.0.pow(10.0)` 优于 `Math.pow(2.0, 10.0)` |
| `sin` `cos` `tan` `asin` `acos` `atan` `atan2` `sinh` `cosh` `tanh` `hypot` `cbrt` | ② 方法层（M0） | 新增 |
| `fmod` `remainder` `fdim` `fma` `copysign` | ② 方法层（M0） | 新增 |
| `truncate` `min` `max` `is_nan` `is_inf`（`Float` 补齐） | ② 方法层（M0） | 新增 |
| `Math.sqrt` / `Math.fabs`（namespace） | ① 底座（M0） | 已有，保留；新能力不再往 namespace 加（见 §3.6 D1） |

### 2.4 `std::io`

| API | 形态 | 状态 |
| --- | --- | --- |
| `"f".open(mode)`（`Path` → `IoFile *`） | ② 工厂方法（M1） | 已有 |
| `read` `write` `close`（`File`） | ② 方法层（M0/M1） | 已有 |
| `seek(offset, origin)` `tell()` `rewind_start()` | ② 方法层（M0） | 新增 |
| `at_eof()` `has_error()` `clear_error()` | ② 方法层（M0） | 新增 |
| `read_char()` `write_char(c)` `unread_char(c)` | ② 方法层（M0） | 新增 |
| `read_line(out, cap)` `write_line(s)` `flush()` | ② 方法层（M0/M1） | 新增 |
| `puts` `putchar` `getchar` `fflush`（`Stdout`） | ① 底座（M0） | 已有 |
| `printf` | ① 底座（M1，唯一变参） | 已有 |
| `Fd` 句柄 | 待判（§1.1） | **已纳入交付面**；先判 `int` 寄生是否可行，判不掉才建新类型；与 `IoFile` 的边界要写明 |

### 2.5 `std::fs`

| API | 形态 | 状态 |
| --- | --- | --- |
| `exists` `remove` `rename` `mkdir` | ② 方法层（M0/M1） | 已有 |
| `read_file` `write_file` `list` | ② 方法层（M1/C） | 已有（读满、缓冲拼接） |
| `get_last_error()`（`FsEnv`） | ① 底座（M1 状态快照） | 已有 |
| `read_link(out, cap)` `resize(len)` `set_mode(mode)` `resolve_path(out, cap)` | ② 方法层（M0/M1） | 新增 |
| `is_readable()` `is_writable()` `change_dir()` | ② 方法层（M0/M1） | 新增 |
| `file_size()` `file_mode()` `file_mtime()` | ② 方法层（M1，每次一次 `stat`） | 新增（取代 `FileStat`） |
| `get_dir(out, cap)`（namespace 普通函数） | ① 底座（M1） | 新增（无接收者，见 §3.6 D3） |
| `"d".open_dir()` | ② 工厂方法（M1） | 新增（字符串起头 → 句柄） |
| `next(out, cap)` `rewind()` `close()` `tell()` `seek(pos)` | **③ OO2：`Dir`** | 新增。**理由：真持有 `DIR *`**；零代码映射只能交裸指针，关闭无人兜底 |
| `FileStat` 值类型 | — | **降级为标量查询**（§2.8） |

### 2.6 `std::utf8` / `std::arena` / `std::test`

| 模块 | API | 形态 |
| --- | --- | --- |
| `std::utf8` | `decode` `count_codepoints` `encode` | ② 方法层（C，手写编解码） |
| `std::arena` | `Arena` + `ArenaOps` | ③（自有资源类型，非 libc） |
| `std::test` | `Test` + `TestOps` | ③（自有类型，非 libc） |

### 2.7 五个新模块的重判（按新判据）

| 模块 / API | 形态 | 依据 |
| --- | --- | --- |
| `time`：`now_sec()` `get_cpu_ticks()` `diff_seconds(a, b)` | ① 底座（M0） | `time(0)` / `clock()` / `difftime` 直连 |
| `time`：`now_ns()` / `sleep_ms(ms)` | ① 底座（M1） | `clock_gettime` / `nanosleep` 需要出参与换算 |
| `time`：`std::time::Time.year(t)` / `month(t)` / `day(t)` / `hour(t)` / `minute(t)` / `second(t)` / `format(t, fmt, out, cap)` | **① 底座（M1）** | 时间不挂 `long`（负责人已定 Q8）：宿主语义不相合（`size.year()` 会返回 2023）；只做 `Time` namespace 静态方法，`struct tm` 在 shim 内部用 `localtime_r` 填栈上变量，不暴露类型 |
| `time`：`std::time::Time.from_parts(y, m, d, h, mi, s)` | ① 工厂（M1） | 返回 `long` 时间值；不引入 `TimeParts` 值类型，后续用 `Time.format(t, ...)` |
| `time`：`Timer` + `Drop` | 纳入交付面；形态按 §1.1 判定 | 真资源；先判能否寄生基本类型，判不掉才建新类型 |
| `env`：`name.get_env(out, cap)`；`std::env::Env.set(name, value)` / `unset(name)` | ② 方法层 + ① 底座（M0） | `getenv` 返回借用指针，方法层负责拷进调用方缓冲（`get` 带 `out` 更安全）；set/unset 无接收者价值，走 namespace |
| `process`：`std::process::Process.get_pid()` `get_parent_pid()` `get_uid()` `get_euid()` `get_gid()` | ① 底座（M0） | 无接收者、无链式收益，五条单行 |
| `errno`：`std::errno::Errno.get_last()` | ① 底座（M1） | `errno` 是宏，一条 `return errno;` |
| `errno`：`code.message()` / `std::errno::Errno.message(code)` | ② 方法层（M0/M1） | `123.message()` 链式可用；底层 `strerror`，返回借用指针 |
| `net`：`"1.2.3.4".parse_ip(out)` | ② 方法层（M1） | `inet_pton` 出参适配 |
| `net`：`std::net`：`Socket` + `"host:80".connect()` + `bind_to(port)` / `listen_backlog(n)` / `accept()` / `send_bytes` / `recv_bytes` / `shutdown_both` / `close`；UDP 收发；`resolve_host`；`setsockopt` | **③ OO2：`Socket`**，方法本体 M0/M1 | **一次做全**（负责人已定裁定，见 §8.2 Q6）：UDP、`resolve_host`、`setsockopt` 与 TCP 同轮交付。**理由：真持有 fd**，`accept` 还会产生新 fd；零代码映射会把关闭责任丢给调用方。`connect` 是字符串起头的工厂（DNS 在 shim 内解析并释放） |
| `net`：`Address` | **不建模（负责人已定）** | 主机用 `char *`、端口用 `int`，方法寄生在这两个宿主上；`recv_from` 无法用返回值同时给出数据与来源，改用出参回报来源地址，形如 `sock.recv_from(buf, len, ipbuf, &port)`。依据：静态映射优先、能力寄生在基本类型上，不引入值类型。 |

### 2.8 与上一版的降级对照

| 原方案 | 现方案 | 变化 |
| --- | --- | --- |
| `FileStat` 值类型 + 字段方法 | `file_size()` / `file_mode()` / `file_mtime()` 方法层 | **OO1 → ②**（只补能力，不建值类型；代价是多次 `stat`） |
| `TimeParts` 值类型 + `make_epoch` / `to_local` / `to_utc` | `Time` namespace 静态方法（`Time.year(t)`…`Time.format(t, …)`）+ `Time.from_parts` 工厂；不挂 `long` | **OO1 → ① 且不建模**（Q8 已定；`tm` 藏在 shim 内） |
| `Timer` + `Drop` | 纳入交付面；形态按 §1.1 判定 | 先判能否寄生基本类型，判不掉才建新类型 |
| `Fd` + `FdOps`（13 个方法） | 纳入交付面；形态按 §1.1 判定 | 先判 `int` 寄生是否可行，判不掉才建新类型；与 `IoFile` 的边界要写明 |
| `Address` 值类型 | 主机 `char *`、端口 `int`（方法寄生）+ `resolve_host` + `parse_ip` | **OO1 → 不建模**（地址不引入类型；`recv_from` 以出参回报来源） |
| `math` 新增走 namespace `Math` | 新增走 `Float`/`Real` 方法层 | **① → ②**（链式更顺手） |
| 新 Drop 类型 4 个 | `Dir`/`Socket` 已定；`Fd`/`Timer` 按 §1.1 判定 | 只给真持有资源且无法寄生基本类型的类型 |
| 新值类型 3 个 | **0 个** | 全部不建值类型（`FileStat`/`TimeParts` 以标量/namespace 补能力，`Address` 不建模） |

### 2.9 明确不封装（能力仍可按 ① 直调 libc）

以下能力不新增 std 封装，但能力面不缺：调用方按 ① 直调 libc。内存操作不在本清单：已由 `char *`
方法层封装（§2.1）。

`qsort` `bsearch` `scandir`（回调）；
`scanf` 族、`sprintf`、`fprintf`（变参/无界）；`fork` `exec*` `system` `popen` `signal` `alarm`；
locale 与宽字符；`ldexp` `frexp` `modf`（出参）；`sendmsg` `recvmsg`（`iovec`）。

### 2.10 需要为某类型引入 struct 作「对等类型」的情况（单列，待负责人评估）

本节不与上面的纳入清单混在一起：**这些条目不是「要不要包方法」，而是「要不要为某个类型另造一个
struct 作为它的对等类型」**，一旦引入就多出装箱/拆箱、拷贝语义、所有权归属和与 libc 的互操作
解释成本。以下只列事实与代价，**不自行拍板**，全部标注「待评估」。

先纠正一处事实，避免清单建立在错误前提上：**`char` 现在不是靠 struct 承载方法的**。CT 允许标量
作宿主，`Char` 是一个直接映射到 `char` 的 trait（`std/num/Num.m.ct:94` 声明、`:104`
`map trait Char for type char`），没有任何 struct、没有装箱。因此 `char` 不构成本节条目；
若评审仍希望把 `Char` 改成 struct，那才落入本节，见 A1。

| 编号 | 原类型 | 为什么原类型承载不了方法 | struct 解决了什么 | 代价 | 状态 |
| --- | --- | --- | --- | --- | --- |
| A1 | `char`（现状：标量宿主可直接挂方法） | 现状**不需要** struct；`Char` 是 trait 不是类型 | 若改为 struct：可携带状态（如编码/大小写表），但现有方法都是纯函数，无状态可带 | 装箱：每个字符一次 `malloc` → 热路径不可接受；拷贝：值语义与 `char` 不再等价；互操作：所有 C 侧 `char` 参数都要拆箱；命名/文档：`Char` 从 trait 变类型，语义双写 | 待评估（默认不做） |
| A2 | `char *` → 内置 `Str`（已存在，非本包） | `char *` 能挂方法（`CStrOps` 就是），但**不拥有**；长度要每次 `strlen`，且无法表达「内嵌 `\0` + 长度」 | 拥有堆缓冲 + 显式长度 + `Drop`；非 NUL 结尾也能安全传递 | 装箱：每次构造一次 `malloc`；拷贝：带 `Drop` 的值不能浅拷贝，复制必须深拷贝或显式 `clone`；所有权：与借用 `char *` 混用时边界要写清；互操作：所有 libc/shim 入口仍要 `.data` 取裸指针，`const char *` 与 `Str` 两套 API 并存；命名/文档：同一能力两种写法（`"s".len()` vs `s.len()`），必须规定「何时用哪个」 | 已存在；本包只消费，不新增（policy 待评估） |
| A3 | `DIR *` → 本方案 `Dir` | C 侧 `DIR` 是不透明类型，CT 看不到定义；裸指针无法挂方法（需要可命名的 CT 类型），且必须 `closedir` | 可命名、可 `Drop`、`close` 幂等；`next()` 等方法有接收者 | 装箱：每个目录句柄一个 `malloc`（约一个指针 + 状态）；拷贝：句柄不可拷贝，需禁止或只允许 `own` 转移；所有权：与 `open_dir()` 的工厂语义绑定；互操作：`readdir` 的 `struct dirent` 仍在 shim 内，返回的是拷出的名字，调用方拿不到原始 `dirent`；命名/文档：`Dir` 与 `FsOps.list` 两种列举方式并存，需说明取舍 | 待评估（本方案建议做，但不拍板） |
| A4 | `int`（fd）→ 本方案 `Socket` | `int` 本身是合法宿主（`Int` 已存在），技术上能挂 `send`/`close`，但会把 socket 方法挂到**所有整数**上 | 可命名、可 `Drop`、`accept()` 产出新句柄，所有权清晰 | 装箱：每个 socket 一个包装 struct；拷贝：fd 不可拷贝，需禁止；所有权：`accept` 产生新 `own`，与 `open_socket` 一致；互操作：需要 fd 时得开 `raw_fd()`（是否暴露待定），否则无法和 `select`/`poll` 等未包裹能力配合；命名/文档：`Socket` 与 `Fd` 的边界要写明 | 待评估（本方案建议做，但不拍板） |
| A5 | 数组/列表（未来，如「目录项数组」「split 结果」） | CT 的 `char *` 承载不了「多个字符串」；调用方缓冲（`list(out, cap)`）在数量不定时不好用 | 可命名、可遍历、可 `Drop`；长度与容量显式 | 装箱：每个列表一个 struct + 元素缓冲；拷贝：元素是 `char *` 还是 `Str` 决定深浅拷贝；所有权：元素释放责任在列表；互操作：与现有 `list`/`read_file` 的缓冲式 API 并存；命名/文档：`StrList` 之类的名字要避免与内置 `Vec` 重复 | 待评估（本方案 v1 不引入；用缓冲式 API 规避） |

不属本节的两类（放在 §2.8 处理，不要混进来）：

- `FileStat` / `TimeParts`：它们是「一次调用多个字段」的值类型，不是「原类型挂不上方法」；
  本方案已分别降级为标量查询、`Time` namespace 静态方法。`Address` 已按负责人裁定不建模，主机 / 端口分别
  寄生在 `char *` / `int` 上（见 §2.7、§8.2 Q6）。
- `Arena` / `Test`：本包自有类型，不对应任何 C 类型，不涉及「对等包装」。

评估时统一问五个问题：① 装箱频率能否接受；② 值语义还是引用语义，拷贝怎么定义；③ 谁负责释放；
④ 与 libc/既有 API 的每一次互操作要写几次转换；⑤ 同一能力是否出现两种写法，文档怎么分工。

## 3. 呈现形态

### 3.1 ① 底座：声明 + shim 直连

CT 的映射是编译期展开：`m(a) -> libc(a)` 在调用点变成 C 调用，没有接口值、没有 vtable、没有运行时
对象。语义能对上时它就是最优解：零开销、失败语义与 libc 一致、新增成本一个声明 + 一行 shim。

**挂法一：namespace 静态方法**（无接收者）。

```ct
import std::math::Math;
import std::io::Io;

int main(void)
{
    std::io::printf("sqrt=%.1f\n", std::math::Math.sqrt(9.0));
    std::io::printf("fabs=%.1f\n", std::math::Math.fabs(-2.5));
    return 0;
}
```

```
sqrt=3.0
fabs=2.5
```

这是标准 M0：shim 里只有 `sqrt(x) -> sqrt(x)`。

**挂法二：类型宿主方法**（接收者是值）——见 §3.2。

### 3.2 ② 便利层：方法挂到既有类型

这是本版扩宽的一层。判断标准只有一条：**写起来是否明显更短、读起来是否更像 CT**。

```ct
import std::str::String;
import std::io::Io;

int main(void)
{
    char buf[8];

    std::io::printf("len=%d\n", "stdlib".len());
    std::io::printf("starts_with=%d\n", "stdlib".starts_with("std"));
    std::io::printf("copy=%d\n", "stdlib".copy(buf, sizeof buf));
    std::io::printf("buf=[%s]\n", buf);
    return 0;
}
```

```
len=6
starts_with=1
copy=0
buf=[stdlib]
```

这一组 `std::str` 方法在 shim 里带小循环（返回索引、带 `cap`），代码级是 **M1**；形态仍是 ②。

### 3.3 链路：一层套一层

CT 范式要求「链路走到底」：基础类型 → 文本 → 文本方法，中间不需要临时变量。

```ct
import std::num::Num;
import std::str::String;
import std::io::Io;

int main(void)
{
    long n = 65;
    char buf[8];

    std::io::printf("text=[%s]\n", n.to_string());
    std::io::printf("len=%d\n", n.to_string().len());
    std::io::printf("starts_with_digit=%d\n", n.to_string().starts_with("6"));
    std::io::printf("to_str=%s\n", n.to_str(buf, sizeof buf));
    return 0;
}
```

```
text=[65]
len=2
starts_with_digit=1
to_str=65
```

这正是负责人举的例子的形态：`n.to_string()`、`"abc".starts_with("x")` 都属这一层。

### 3.4 连声明都不做：直接 libc 原名

① 底座直连：即使已有方法层，也仍可直接调 libc 原名；零成本、零歧义。

```ct
#include <string.h>

import std::io::Io;

int main(void)
{
    char a[4] = "abc";
    char b[4] = "abd";

    std::io::printf("strcmp=%d\n", strcmp(a, b));
    std::io::printf("strlen=%d\n", (int)strlen(a));
    return 0;
}
```

```
strcmp=-1
strlen=3
```

### 3.5 ③ 例外：`Dir` / `Socket` 等资源类型为什么要新类型

零代码映射在资源上会漏掉**所有权**：`opendir` 的 `DIR *`、`socket` 的 fd 交给调用方后，关闭责任
在调用方。③ 只补这一层；方法本体仍是 M0/M1 转发。

规则（实测确认）：必须是**结构体指针 + `own`**，`Drop` 才会在出口执行；栈上值不会。
`own` 的初始化式只认 `malloc` / `calloc` / 返回 `struct X *` 的工厂，`(X *)malloc(...)` 强转**不认**。

```ct
#include <stdlib.h>

import std::io::Io;

struct Buf {
    char *data;
    int len;
} use Drop;

impl Drop for Buf {
    void drop()
    {
        std::io::printf("drop ran\n");
        free(self->data);
        self->data = 0;
        self->len = -1;
    }
}

int main(void)
{
    own Buf *b = malloc(sizeof(Buf));

    b->data = (char *)malloc(8);
    b->len = 8;
    std::io::printf("owned=%d len=%d\n", b->data != 0, b->len);
    /* no free() here: leaving the scope runs Buf.drop, then frees b */
    return 0;
}
```

```
owned=1 len=8
drop ran
```

### 3.6 偏离 CT 范式的登记（必须显式取舍）

| 编号 | 偏离 | 为什么保留 | 取舍/后续 |
| --- | --- | --- | --- |
| D1 | `std::math::Math.sqrt(x)` 是函数式，不是 `x.sqrt()` | 历史接口；数学写法普遍 | 新 math 能力一律走 `Float`/`Real` 方法层；namespace 不再扩张，长期可考虑让 `Math` 退役 |
| D2 | `printf` 变参走 namespace，不是字符串起头 | CT 没有变参接收者转发 | 唯一例外；不推广 |
| D3 | `get_dir(out, cap)` 用出参，不是 `get_dir()` 返回值 | 无接收者，且返回静态缓冲区不安全 | 若将来允许静态槽（如 `to_string`），可改成链式 |
| D4 | `parse_long(out)` / `format_time(out, cap)` 出参 + 状态码 | 失败必须可报；链式返回值无法同时表达 | 与 `copy`/`read_line` 同类，包内已有先例 |
| D5 | `std::env::Env.set` / `std::process::Process.get_pid` 走 namespace | 无接收者 | 不强行造接收者 |

## 4. 模块划分

事实澄清：`Path` / `File` 在 `std/io/File.m.ct`（`:11`、`:19`），`std/str` 只有 `String.m.ct`
一个方法层。`std/io` 是一个 unit 两个宿主。

### 4.1 目录结构

```
std/
  str/    CStrOps.m.ct + CStrOps.shim.h      （0.4.0 改名）
  io/     Io.h.ct  Stdout.m.ct  File.m.ct    （Path + File；`Fd` 按 §1.1 判定）
  fs/     Fs.m.ct  Dir.h.ct                  （`Dir`；`Fd` 另按 §1.1 判定）
  utf8/   Utf8Ops.m.ct + shim
  num/    Num.m.ct + shim
  math/   Math.m.ct  Real.m.ct + shim
  arena/  Arena.h.ct
  test/   Test.h.ct
  env/     Env.m.ct                          （环境变量）
  process/ Process.m.ct                      （进程身份）
  errno/   Errno.m.ct                        （错误码）
  time/   Time.m.ct                          （`Time` namespace 静态方法 + 工厂；不挂 `long`）
  net/    Net.m.ct  Socket.h.ct              （不建 Address）
```

`env/`、`process/`、`errno/` 各自成模块，不合并为 `std::sys`：目录划分与 libc／POSIX 原有头文件
一一对应，C 程序员不需要再学一套分类法。此条为负责人已定裁定（见 §8.2 Q4）；OO 封装只改变调用
形态，不引入新的分类维度。

### 4.2 import 形态

```ct
import std::str::CStrOps;     // 0.4.0 起
import std::io::File;         // 同时带出 Path 与 File
import std::fs::Dir;
import std::env::Env;
import std::process::Process;
import std::errno::Errno;
import std::time::Time;
import std::net::Socket;
```

规则：unit 名 = 该文件的类型/主 trait 名；namespace 宿主用 `.m.ct`；自有类型用 `.h.ct`。

### 4.3 `std/io/File.m.ct` 是否拆

建议 `0.4.0` 只改 trait 名、不拆 unit：`open` → `read`/`write`/`close` 是连续 API 面，拆开对消费者
是纯成本。是否拆见 §8 Q1。

## 5. 命名与契约

### 5.1 命名

- 守 §5.1：`snake_case`、动词开头；`get_` / `set_` / `has_` / `is_` / `to_` / `from_` 合规。
- libc 直映射沿用 `Math.sqrt` / `Char.is_alpha` 先例（`README.md:505-506` 豁免）。
- **便利层也受同宿主唯一名约束**：补方法前必须查表。

### 5.2 宿主名字余量

| 宿主 | 现状 | 本版新增 | 合计（最坏） |
| --- | --- | --- | --- |
| `char *` | 25（`README.md:145-153`） | `rfind_char` `compare_ignore_case` `parse_long` `parse_llong` `parse_ulong` `parse_real` `open_dir` `read_link` `resize` `set_mode` `resolve_path` `is_readable` `is_writable` `change_dir` `file_size` `file_mode` `file_mtime` `parse_ip` `connect` `fill` `compare_with` `find_byte` `compare_prefix` `copy_from`（24 个新增名；`copy_from` 替代 `copy_to`，净增 4） | 48 |
| `long` | 7 | 0（时间不挂 `long`，Q8 已定） | 7 |
| `float`/`double` | 5 / 9 | math 新族（约 20） | 25 / 29 |
| `char` | 7 | 7 个 `is_*` | 14 |

冲突处理：`open` 已被 `Path` 占用 → 目录用 `open_dir`；`read`/`write`/`close` 不再占 `char *`；
`copy_from` 取代既有 `copy_to`（`src.copy_to(dst, n)` → `dst.copy_from(src, n)`），内存版只保留一个
方向；`copy` 仍是带容量 / `\0` 的字符串拷贝；
`long` 不再新增时间方法（Q8 已定），因此没有 `format` 与既有 `to_str`/`to_string` 的冲突。

### 5.3 失败表达

| 场景 | 表达 |
| --- | --- |
| 布尔查询 | `0` / `1` |
| 未命中 | `-1` |
| 状态型操作 | `0` 成功 / `-1` 失败 |
| 字节数 | `n`；`0` = EOF；`-1` = 错误 |
| 构造资源 | `NULL`（`0`） |
| 错误详情 | 模块 `xxxEnv.get_last_error()`；新增 `std::errno::Errno.get_last()` |
| 缓冲区不足 | `-1`，不截断、不返回部分结果（`num.to_str` 例外） |

### 5.4 所有权

- **借用（默认）**：方法层不拥有输入、只写调用方缓冲；`env` / `errno.message` 返回**借用**指针，
  生命周期到下一次同类调用（README 必须写）。
- **持有**：`Dir` / `Socket` 已定；`Fd` / `Timer` 若按 §1.1 判为资源类型，同样按此规则处理。
  `pub struct X { ... } use Drop;` + 显式 `impl Drop`
  （字段胶水不释放指针字段，`ct-lang/docs/language-spec.md:2549`）；`own X *p = ...`；`close()` 幂等。
- **值类型**：不新增（`FileStat` / `TimeParts` 不建值类型，`Address` 不建模）。
- 内部工厂返回类型必须写成 `struct X *`（`own` 看字面量）。

## 6. 文档呈现

### 6.1 模块 README 模板（6 节）

```markdown
# `std::<mod>` — 一句话定位

## 1. 概览与边界
## 2. API 表（| 方法 | 签名 | 语义 | 层 ①/②/③ | 失败 |）
## 3. 样例（可运行，来自 tests/<mod>/）
## 4. 与相邻模块的关系（宿主、同名冲突）
## 5. 版本与变更记录
## 6. 未支持与已知限制
```

API 表新增 **层** 列：① 映射 / ② 便利 / ③ OO。读者不用翻 shim 就知道成本形态。样例必须来自
`tests/`，保证不腐。

### 6.2 顶层索引（三张表）

1. **按任务**：读文件、遍历目录、当前时间、网络……没有的显式写「未支持」。
2. **按宿主**：`char *` / `IoFile *` / `Dir *` / `Socket *` / `long` / `double` / namespace。
3. **libc → std 映射**：以 [libc 覆盖盘点](../libc-inventory.md) 为源，四列
   「已在 std（层）/ 建议纳入（层）/ 直调 / 不纳入（理由）」。

维护规则：新增 API 一次改三处（§4 表、模块 README、libc 映射表），缺一处视为未完成。

## 7. 施工顺序（全库覆盖为完成判据）

原则：先铺 ①②，再补 ③；③ 只给真正持有资源且无法寄生基本类型的类型（已定 `Dir`、`Socket`；`Fd`、`Timer` 先判）；每个模块整块交付（§1.7），不做半截。

P0–P7 是施工顺序，不是分批发布节奏。表中判据只作阶段验收，最终以 §2 能力面（含 §2.9 的直调能力）全库覆盖为完成，不以任一相的交付为完成；全库完成后再统一发布 `0.4.0`。

| 序 | 内容 | 层 | 依赖 | 阶段验收 |
| --- | --- | --- | --- | --- |
| **P0** | `String` → `CStrOps`（issue 0001，`0.4.0`） | 改名 | 无 | 旧名零命中；`tests/run.sh` 全绿；文档同步 |
| **P1** | `char` 7 个 `is_*`；`char *` 内存方法（`copy_from`/`fill`/`compare_with`/`find_byte`/`compare_prefix`）；`math` 新族（`Float`/`Real` 方法层） | ①② | P0 | 每条 M0/M1 转发；无宿主重名；重叠拷贝夹具；三态/指针返回验证 |
| **P2** | 新增 `std::env` / `std::process` / `std::errno`（各一个模块） | ①② | P1 | 无新类型；借用指针语义写进 README |
| **P3** | `stdio` 句柄映射补全（`seek`/`tell`/`rewind`/`eof`/`error`/`read_char`/`write_char`/`read_line`/`flush`）；`Fd` 形态判定（按 §1.1） | ②/③（`Fd` 待判） | P0 | EOF/错误位/幂等 close 有 fixture；`Fd` 判定结论落档 |
| **P4** | `std::fs` 补充（`read_link`/`resize`/`set_mode`/`resolve_path`/`is_*`/`change_dir`/`file_*`） | ② | P2 | 不引入 `FileStat` 值类型；重复 `stat` 代价写进 README |
| **P5** | `std::time`：`now_sec`/`now_ns`/`diff_seconds`/`sleep_ms` + `Time` namespace 静态方法（`year`…`format`、`from_parts`）；`Timer` 形态判定（按 §1.1） | ①/③（`Timer` 待判） | P2 | 不挂 `long`；不做 `TimeParts` 值类型；`tm` 只在 shim 内；时区行为写明；`Timer` 判定结论落档 |
| **P6** | `Dir` + `Drop`（模板）；`Fd`/`Timer` 若判为资源类型，按同一模板收口 | ③ | P3（模板） | `own` 样例；`close` 幂等；大目录流式列举；`Fd`/`Timer`（若适用） |
| **P7** | `std::net`：`Socket`（TCP + UDP）+ `"host:80".connect()` + `resolve_host` + `setsockopt` | ①②③ | P6（模板） | TCP/UDP loopback 回环；`Drop` 关闭；超时与 `setsockopt` 行为写明 |

「暂不排期」状态不再存在：`Fd`、`Timer`、`FileStat`、`TimeParts` 均纳入交付面。`FileStat`、`TimeParts` 只补能力、不建值类型；`Fd`、`Timer` 先按 §1.1 判能否寄生基本类型，判不掉才动新类型。

## 8. 风险与未决

### 8.1 已知风险

| 风险 | 说明 | 缓解 |
| --- | --- | --- |
| 便利层铺宽 → 名字通胀 | `char *` 25 → 48；`long` 保持 7（Q8 已定不挂时间） | 每个名字必须给出「比函数式省多少」；同宿主查表 |
| 宿主污染 | 领域方法挂基础类型会让所有同类型值获得该方法（如 `size.year()`） | Q8 已定：时间不挂 `long`，走 `Time` namespace；其他宿主按 §5.2 查表 |
| 时区/线程安全 | `localtime_r` 无时区参数；`strerror` 静态缓冲 | 文档写明「本地时区」与 `strerror` 非线程安全 |
| `own` 初始化式很窄 | 只认 `malloc`/`calloc`/`struct X *` 工厂；强转与别名不认（实测） | 工厂返回类型写 `struct X *`；每个新类型加 `own` 样例 |
| 栈上值不触发 `Drop` | 实测值不释放，只有 `own` 指针会 | 句柄必须指针 + `own` |
| `IoFile` 与新区则不一致 | `close` 只 `fclose`，包装体永不释放（`std/io/README.md:211-215`） | Q2；新类型一律 `Drop` 释放 |
| 多次 `stat` | `file_size`/`file_mode`/`file_mtime` 各一次 | 文档写明；`FileStat` 不建值类型，保持标量查询 |
| 借用指针生命周期 | `getenv` / `strerror` 可能被覆盖 | 方法层提供带 `out` 的复制版；文档写死 |
| 表达力限制 | 变参、回调、`iovec`、`struct stat` 布局 | 直调或藏进 shim（M1），不建模；能力不缺失 |
| 阻塞模型 | `Socket` 阻塞、DNS 无超时控制 | P7 写明阻塞语义与超时行为 |
| 版本节奏 | 全库完成后统一发布 `0.4.0`，不按相发 minor | 以全库覆盖为完成判据（§7） |

### 8.2 未决

- **Q1**：`std/io/File.m.ct` 是否拆成 `Path` / `File`？（建议不拆）
- **Q2**：`IoFile` 包装体不释放是否修正？（破坏性）
- **Q3**：`char *` 48 个名字是否可接受，还是要给便利层设上限、超出走新类型？
- **Q4（已定）**：`env`/`process`/`errno` **不合并**为 `std::sys`，三者各自成模块，目录与 libc／POSIX 原有头文件一一对应。依据：降低 C 程序员的心智成本——做的是 OO 封装，不引入新的分类法。
- **Q5（不再适用）**：P0–P7 改为施工顺序，不按相发布；合并相邻相发布的讨论随之取消，版本节奏见 §7 / §8.1。
- **Q6（已定）**：`net` 一次做全，UDP、`resolve_host`、`setsockopt` 全部纳入，不再留在暂不排期。依据：不做半截模块——只封一半会让使用者不知道到底该用封装还是退回 libc。`Address` 不建类型（已定）：主机用 `char *`、端口用 `int`，方法寄生在这两个宿主上；`recv_from(buf, len, ipbuf, &port)` 以出参回报来源。依据：静态映射优先、能力寄生在基本类型上，不引入值类型。
- **Q7（已定）**：`file_*` 保持标量查询、接受重复 `stat`；`FileStat` 不建值类型。
- **Q8（已定）**：时间方法不挂 `long`——宿主语义不相合（`size.year()` 会返回 2023）；时间只做 ① 底座 + `Time` namespace 静态方法，「迁就 `long`」正式否掉。
- **Q9**：`Math` namespace 是否逐步退役、全部并到 `Float`/`Real`？
- **Q10**（§2.10 A2）：`Str` 与 `char *` 两种写法并存，使用政策怎么定（何时必须用 `Str`）？
- **Q11**（§2.10 A3/A4）：`Dir` / `Socket` 接受 struct 对等类型，还是尝试用别名/裸类型宿主？
- **Q12**（§2.10 A5）：数组/列表类结果是否引入列表 struct，还是继续用调用方缓冲？

### 8.3 本文可被证伪的假设

1. 「便利层铺宽是净收益」——若评审认为 48 个 `char *` 名字的维护成本过高，需按 Q3 收缩。
2. 「时间方法可以挂 `long`」——Q8 已定否掉；时间走 `Time` namespace，不再作为假设。
3. 「`Fd` / `Timer` 可以先不做」——全库口径已把两者纳入交付面，本条不再作为假设；形态由 §1.1 判定。
4. 「`Address` 不建类型，多地址回退由调用方在 `resolve_host` 结果上自行处理」——若统一地址表示成为硬需求，需重开该裁定。

## 9. 交付与推进

1. 本文定稿 → 交负责人过目。
2. 负责人点头后才进入实现；未点头前只改文档，不动代码、不起版本号、不打 tag。
3. 实现阶段按 §7 施工顺序独立提交；表中判据只作阶段验收，最终以全库覆盖为完成；跨仓事项一律不向 ct-lang 提。
