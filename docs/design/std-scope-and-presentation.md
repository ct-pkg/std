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
- 基础类型的值能直接接方法：`n.to_str(buf, sizeof buf)`；
- 工厂从字符串进：`"f".open_file("r")`、`"dir".open_dir()`、`"host:80".connect()`；
- 结果还能继续走：`"abc".length()`。

不满足的（写了不比函数式短、或语义会被误读）不补，避免名字通胀。

### 1.3 在范围内

C 标准库 + POSIX.1 的接口：只要有对应函数就纳入（① 映射优先）；唯一例外是 CT 无变参声明机制
（`scanf` 族、`sprintf`、`fprintf` 等不做；0.3.0 遗留的 `printf` 一并删除，按破坏性变更记录），按 §2.9 的
成对替代处理。CT 模型差异只写成使用注意事项，不构成排除。
字符串/内存、stdio、目录、`stat`、时间、环境变量、进程身份/控制/信号、错误码、socket、
locale/宽字符、线程/异步等 POSIX 能力都在交付面。自有设施：`Arena`、`Test`、`Utf8Ops`。

### 1.4 不归 std（仅三类）

| 类别 | 理由 |
| --- | --- |
| 编译器内置层（`Vec` / `Map` / `Str`）的类型定义 | 0.3.0 已删 `std::vec` / `std::map`；重名撞内置类型。std **不重复定义**这些类型，也不在 `Str` 上新增方法层，只作消费方（`Str` 归语言侧，见 §2.10 A2） |
| 第三方包（JSON / HTTP / 日志 / CLI 框架） | 独立 ct-pkg；POSIX 正则 / 命令行解析不在此列，按 libc 纳入 |
| 编译器诊断、构建工具、包管理器 | 属于 ct / ct-pkg |

判据是：**libc／POSIX 有没有对应函数，有就映射**。CT 模型差异（`fork` 后的运行时状态、
`signal` 回调、`Locale.apply` 全局状态、平台差异、回调/`iovec` 等）只写成使用注意事项，不构成排除。
变参不是模型差异，而是 CT 无变参声明机制，按 §2.9 的成对替代处理（0.3.0 遗留的 `printf` 按破坏性变更删除）。纯内存操作已由 `char *` 方法层封装（§2.1）。

### 1.5 ③ 的准入（只此一条）

**真的持有资源**：一次调用创建、多次调用共享、必须显式关闭。命中后仍先判能否寄生基本类型；
判不掉才允许新类型 + `Drop`，其余留在 ① 或 ②。`Dir`、`File`、`FileDesc`、`Timer` 均为 ③；`Socket` 是 `FileDesc` 的同义别名（同一类型、方法一套），别名能力依赖语言侧。

### 1.6 风格准绳与偏离登记

CT 范式：**字符串起头 → 工厂函数进 → 方法挂既有类型 → 链路走到底**。凡偏离，必须在 §3.6 登记
取舍，不能默默引入「更标准但更别扭」的写法。

### 1.7 交付口径：全库覆盖，整块交付

std 是对标准库的一次整体转化，不是分批交付：在本方案划定的标准库范围内（§1.3/§1.4）该有的能力
一个都不缺，最终以全库覆盖为完成判据（§7）。单个模块仍不得做半截——同一模块的公共能力如果只封一半，
使用者无法判断何时该用封装、何时该退回 libc，封装反而增加决策成本（`net` 一次做全即按此条执行，
见 §2.7 与 §8.2 Q6）。

### 1.8 指针不对外，包内消化（负责人准绳）

CT 里的裸指针（来自 C 互操作）是使用者的负担，接口应尽量走 CT 自己的「工厂方法 + 链式调用 + `own`」
机制：由工厂产出对象，调用方拿 `own` 句柄按方法链继续用，释放责任留在包内，不要求调用方手工
`free`/`close`。工厂方法、`own`、链式调用都是 CT 自身的机制，不按其他语言的 OO 类比来解释。

两条硬要求：

1. **工厂返回的对象必须能落到 `own` 上自动释放**；不能自动释放的中间值不该由工厂产出。
2. **链式调用的每一步都要返回可续接的对象或值**，不允许中途出现「调用方必须自己释放」的形态；
   出现这种形态即设计不合格。

适用范围：`Dir`、`File`、`FileDesc`（`Socket` 为其同义别名）、`Timer` 这类真持有资源的对象直接适用；
本条同时作为实现约束：任何只能靠裸指针或裸整数句柄交出的能力，都必须包成可 `own` 的对象。

## 2. 分模块纳入清单

列格式：`形态①/②/③`，括号里是代码级 `M0`（单行转发）/ `M1`（几行适配）。

### 2.1 `std::str`（`char *`）

| API | 形态 | 状态 | 说明 |
| --- | --- | --- | --- |
| `length` `find` `find_char` `starts_with` `ends_with` `copy` `slice` `lowercase` `uppercase` `compare` | ② 方法层（M1） | 已有 | 字符串起头的核心链路；`length` 整条替换旧 `len`，不留别名 |
| `concat` 等产出型 API | ② 方法层（M1） | 已有 | 本仓只描述宿主为 `char *` 时的行为；若声明处目标为语言侧内置 `Str`，其行为以语言侧规范为准，本仓不复述、不定义 |
| `to_int` `to_double` | ② 方法层（M0） | 已有 | `-> atoi/atof` |
| `reverse_find_char` / `compare_ignore_case` | ② 方法层（M1） | 新增 | 补齐反向查找与忽略大小写比较 |
| `parse_long` / `parse_long_long` / `parse_unsigned_long` / `parse_real` | ② 方法层（M1） | 新增 | 需要 `strtol` 的 `endptr` + `errno` 才能报错；底座仍是映射 |
| `copy_from` / `move_from` / `fill` / `compare_with` / `find_byte` / `compare_prefix` | ② 方法层（M1） | 新增 | 内存操作（宿主 `char *`，`void *` 挂不了方法）：`copy_from` → `memcpy`（要求源/目标不重叠，快路径）；`move_from` → `memmove`（允许重叠，内部做方向判断，略慢）；`fill` → `memset`；`compare_with` → `memcmp`（返回三态，不布尔化）；`find_byte` → `memchr`（返回指针或 `NULL`）；`compare_prefix` → `strncmp`（`\0` 停） |
| `split_into(separator, out, cap, slot_size)` | ② 方法层（C） | 新增 | 拷贝语义：按 `separator` 切分，每段拷进调用方缓冲 `out`。`cap` 是段数上限，`slot_size` 是每段字节上限；返回实际段数；缓冲不足时返回所需段数且不写入任何段；**原串一个字节都不改**。函数不分配内存，也不提供 `free_split`。不是 libc 映射（libc 只有 `strtok`/`strtok_r`，且会改写源串），底层可用 `strtok_r` 或自行扫描 |

`memcpy` / `memmove` 不合并，拆成 `copy_from` / `move_from` 两个名字：前者要求不重叠、是快路径，
后者允许重叠（含原地左移/右移）、内部多一次方向判断。合并成一个名字会让所有普通拷贝都付
`memmove` 的检查成本；「是否可能重叠」应由调用点声明，而不是牺牲快路径换取少一个名字。
既有 `copy_to`（`src.copy_to(dst, n)`）是同一能力、方向相反；按“方向只保留一个”，0.4.0 起统一为
`dst.copy_from(src, n)` 或 `dst.move_from(src, n)`，`copy_to` 退场（破坏性改名，随 issue 0001 同批通告）。

`split_into` 属 ② 便利层，不是 libc 映射：`strtok`/`strtok_r` 会就地写入 `\0` 改写源串，语义不同，
不能直接对外暴露；`split_into` 只读源串、每段另行拷进调用方缓冲。取缓冲式而非返回值分配，与 §1.5
「只有真持有资源才建类型」同口径：若由 `split_into` 自己分配，切分结果就带上了所有权与释放责任，
必须再配一个 `free_split`；缓冲式把这两件事留在调用方，函数本身无状态、无分配。

另：`String` → `CStrOps`（issue 0001，`0.4.0`），除 `copy_to` → `copy_from`/`move_from` 外方法名不变。

`Str` 是语言侧内置的拥有型字符串：本仓不定义、不提供、也不在 `Str` 上挂方法，只作为消费方依赖它；
`char *` 是借用视图，两者的分工由语言侧决定，不在本仓定义。

**`char *` 与语言侧内置 `Str` 共存所依赖的 CT 语言机制（std 侧的依赖与假设）：**

以下三条不是 std 定义的规则，也不是本仓提供的能力，而是 CT 语言提供的机制；**若 CT 提供这些机制，
则 std 的接口按此设计**。涉及 `Str` 的细节以语言侧规范为准，本仓不复述、不定义：

1. **结果类型由声明处的目标类型决定**：产出型操作的返回类型随声明处走，转换由语言侧完成；
   本仓只声明宿主为 `char *` 时的行为。
2. **不产出新值的地方退到最容易的表示**：相等比较用 `char *` 视图比内容，不产生拷贝、不产生隐式分配。
3. **不同目标类型的释放责任不同**：目标为 `char *` 时，结果是由被调方分配、调用方释放的裸内存；
   `Str` 目标的释放语义以语言侧规范为准。

`char *` 与 `Str` 的分工由语言侧决定，本仓不再制定细则。

### 2.2 `std::num`（`int` / `long` / `char`）

| API | 形态 | 状态 |
| --- | --- | --- |
| `abs` `clamp` `max` `min` `to_base` | ② 方法层（M0/M1） | 已有；只登记整数宿主（`int`/`long`），浮点宿主归 §2.3 |
| `to_str` | ② 方法层（M1，`snprintf`） | 已有；`to_string` 已删除 |
| `is_digit` `is_alpha` `is_space` `is_upper` `is_lower` `uppercase` `lowercase` | ② 方法层（M0） | 已有；ctype 名只做 snake_case 化，不展开英文 |
| `is_alnum` `is_punctuation` `is_printable` `is_graphical` `is_control` `is_blank` `is_hex_digit` | ② 方法层（M0） | 新增（`char` 宿主，各 1 行） |

### 2.3 `std::math`（`float` / `double`）

| API | 形态 | 状态 |
| --- | --- | --- |
| `abs` `sqrt` `floor` `ceil` `round`（`Float`/`Real`） | ② 方法层（M0） | 已有；`abs` 只登记浮点宿主 |
| `is_nan` `is_infinite`（`Real`） | ② 方法层（M0/M1） | 已有 |
| `pow` `exp` `expm1` `log` `log2` `log10` `log1p` | ② 方法层（M0） | 新增：方法形态让 `2.0.pow(10.0)` 直接可用 |
| `sin` `cos` `tan` `asin` `acos` `atan` `atan2` `sinh` `cosh` `tanh` `hypot` `cbrt` | ② 方法层（M0） | 新增 |
| `fmod` `remainder` `fdim` `fma` `copysign` | ② 方法层（M0） | 新增 |
| `truncate` `min` `max` `is_nan` `is_infinite`（`Float` 补齐） | ② 方法层（M0） | 新增；`min`/`max` 只登记浮点宿主 |

| `exponential_minus_one` `logarithm_one_plus` `positive_difference` `fused_multiply_add` `cube_root` `floating_modulo` `copy_sign` | ② 方法层（M0） | 新增（由生僻短名改为全称） |

数学函数一律只提供方法形态：删除 `Math.sqrt`/`Math.fabs` 这类 namespace 入口，不再保留第二套写法。
命名只改看不出意思的 7 个：`expm1`→`exponential_minus_one`、`log1p`→`logarithm_one_plus`、
`fdim`→`positive_difference`、`fma`→`fused_multiply_add`、`cbrt`→`cube_root`、`fmod`→`floating_modulo`、
`copysign`→`copy_sign`；其余（`abs`/`sqrt`/`pow`/`log`/`sin`/`cos`/`max`/`min`/`floor`/`round`/
`remainder`/`truncate`/`is_nan`/`is_infinite`/`exp`/`exp2`/`log2`/`log10`/`tan`/`asin`/`acos`/`atan`/
`atan2`/`sinh`/`cosh`/`tanh`/`hypot`/`ceil`）保留原名，只做 snake_case 化。清单见
[数学全名草案](std-math-full-names-draft.md)，已按此定稿并入正文。

### 2.4 `std::io`

| API | 形态 | 状态 |
| --- | --- | --- |
| `"f".open_file(mode)`（`Path` → `own File`） | ② 工厂方法（M1） | 已有（原名 `open`） |
| `read` `write` `close` `seek` `tell` `rewind_start` | ③（`File`，方法寄生 `FILE *`） | 已有/新增 |
| `at_end_of_file()` `has_error()` `clear_error()` | ③（`File`） | 新增 |
| `read_char()` `write_char(c)` `unread_char(c)` | ③（`File`） | 新增 |
| `read_line(out, cap)` `write_line(s)` `flush()` | ③（`File`） | 新增 |
| `"a.bin".open_as_file_desc(open_read)`（`Path` → `own FileDesc`） | ② 工厂 + ③ | 新增 |
| `read` `write` `seek` `close` `send` `receive`（`FileDesc`） | ③ | 新增；`send`/`receive` 返回接收者本身，可链式续接 |
| `printf` | — | **已删除**（0.3.0 遗留变参接口，按破坏性变更记录） |

`Stdout`/`Stdin` 不建：`io` 只保留 `File`；`File` 的方法寄生在 `FILE *` 上，`stdout`/`stderr`/`stdin`
直接当宿主使用，不封装。`File` 与 `Dir` 是两个独立对象，属性方法各不同，形态都归 ③。

`File` 释放：`own` 句柄离开作用域由包内关闭；另给显式 `close()` 供提前关闭，关后句柄失效、重复调用
是空操作；`stdout` 这类裸 `FILE *` 不归包管。

### 2.5 `std::fs`

| API | 形态 | 状态 |
| --- | --- | --- |
| `exists` | ② 方法层（M0） | 已有；底层用 `stat` |
| `remove` `rename` `make_directory` | ② 方法层（M0/M1） | 已有 |
| `read` `write` `list` | ② 方法层（M1/C） | 已有（读满、缓冲拼接）；宿主已是路径，不再带 `file_` 前缀 |
| `access(mode)` | ② 方法层（M0） | 保留；`access` 直连 |
| `stat(&st)` | ② 方法层（M0） | 保留；`struct stat` **原样暴露**，不建包装类型 |
| `get_last_error()`（`FsEnv`） | ① 底座（M1 状态快照） | 已有 |
| `read_link(out, cap)` `resize(len)` `set_mode(mode)` `resolve_path(out, cap)` | ② 方法层（M0/M1） | 新增 |
| `is_readable()` `is_writable()` | ② 方法层（M0/M1） | 新增；按**真实身份**判定，不代表生效身份下实际可读写 |
| `size()` `mode()` `modification_time()` | ② 方法层（M1） | 新增；基于 `stat` 的便捷方法，不替代 `stat(&st)`；宿主已交代路径，不再带 `file_` |
| `"d".open_dir()` | ② 工厂方法（M1） | 新增（字符串起头 → `own Dir`） |
| `Dir.current()` → `own Dir` | ③ 静态工厂 | 新增；对象式取当前目录 |
| `dir.path(out, cap)` | ③（`Dir`） | 新增；取路径 |
| `dir.enter()` | ③（`Dir`） | 新增；切换当前目录 |
| `next(out, cap)` `rewind()` `close()` `tell()` `seek(pos)` | ③（`Dir`） | 新增。**理由：真持有 `DIR *`**；零代码映射只能交裸指针，关闭无人兜底 |
| `FileStat` 值类型 | — | **不建包装类型**：`stat(&st)` 原样暴露 `struct stat`，`size`/`mode`/`modification_time` 为便捷方法 |

### 2.6 `std::utf8` / `std::arena` / `std::test`

| 模块 | API | 形态 |
| --- | --- | --- |
| `std::utf8` | `decode` `count_codepoints` `encode` | ② 方法层（C，手写编解码） |
| `std::arena` | `Arena` + `ArenaOps` | ③（自有资源类型，非 libc） |
| `std::test` | `Test` + `TestOps` | ③（自有类型，非 libc） |

### 2.7 五个新模块的重判（按新判据）

| 模块 / API | 形态 | 依据 |
| --- | --- | --- |
| `time`：`now_seconds()` `get_cpu_ticks()` `diff_seconds(a, b)` | ① 底座（M0） | `time(0)` / `clock()` / `difftime` 直连 |
| `time`：`now_nanoseconds()` / `sleep_milliseconds(ms)` | ① 底座（M1） | `clock_gettime` / `nanosleep` 需要出参与换算 |
| `time`：`std::time::Time.year(t)` / `month(t)` / `day(t)` / `hour(t)` / `minute(t)` / `second(t)` / `format(t, fmt, out, cap)` | **① 底座（M1）** | 时间不挂 `long`（负责人已定 Q8）：宿主语义不相合（`size.year()` 会返回 2023）；只做 `Time` namespace 静态方法，`struct tm` 在 shim 内部用 `localtime_r` 填栈上变量，不暴露类型 |
| `time`：`std::time::Time.from_parts(y, m, d, h, mi, s)` | ① 工厂（M1） | 返回 `long` 时间值；不引入 `TimeParts` 值类型，后续用 `Time.format(t, ...)` |
| `time`：`(100).make_timer(SIGRTMIN)` / `(250).make_timer_once(SIGRTMIN)`；`start`/`stop`/`remaining` | **③ 资源对象** | 周期/一次性定时器；到期通知仍走 `signal` 直连，本包只包管理面，不含回调 |
| `env`：`name.get_env(out, cap)`；`std::env::Env.set(name, value)` / `unset(name)` | ② 方法层 + ① 底座（M0） | `getenv` 返回借用指针，方法层负责拷进调用方缓冲（`get` 带 `out` 更安全）；set/unset 无接收者价值，走 namespace |
| `process`：`std::process::Process.get_pid()` `get_parent_pid()` `get_uid()` `get_effective_uid()` `get_gid()` | ① 底座（M0） | 无接收者、无链式收益，五条单行；控制/信号见 §2.9 |
| `errno`：`std::errno::Errno.get_last()` | ① 底座（M1） | `errno` 是宏，一条 `return errno;` |
| `errno`：`code.error_message()` / `std::errno::Errno.error_message(code)` | ② 方法层（M0/M1） | `123.error_message()` 链式可用；底层 `strerror`，返回借用指针 |
| `net`：`"1.2.3.4".parse_ip(out)` | ② 方法层（M1） | `inet_pton` 出参适配 |
| `net`：`std::net`：`Socket` + `"host:80".connect()` + `bind_to(port)` / `listen_backlog(n)` / `accept()` / `send` / `receive` / `shutdown_both` / `close`；UDP 收发；`resolve_host`；`set_option` | **③：`Socket`（`FileDesc` 的同义别名）**，方法本体 M0/M1 | **一次做全**（负责人已定裁定，见 §8.2 Q6）：UDP、`resolve_host`、`set_option` 与 TCP 同轮交付。`Socket` 与 `FileDesc` 是同一类型、方法一套、可互相赋值；别名能力依赖语言侧。**理由：真持有 fd**，`accept` 还会产生新 fd；零代码映射会把关闭责任丢给调用方。`connect` 是字符串起头的工厂（DNS 在 shim 内解析并释放） |
| `net`：`Address` | **不建模（负责人已定）** | 主机用 `char *`、端口用 `int`，方法寄生在这两个宿主上；`receive_from` 无法用返回值同时给出数据与来源，改用出参回报来源地址，形如 `sock.receive_from(buf, len, ipbuf, &port)`。依据：静态映射优先、能力寄生在基本类型上，不引入值类型。 |

`FileDesc` 的收发为链式：`receive(buf, cap, &n)` 与 `send(data, n)` 都返回接收者本身，`receive` 的字节数
由出参带回；旧名 `recv` 不再保留：

```ct
int n = 0;
conn.receive(buf, 1024, &n)
    .send("ok", 2);
```

### 2.8 与上一版的降级对照

| 原方案 | 现方案 | 变化 |
| --- | --- | --- |
| `FileStat` 值类型 + 字段方法 | `stat(&st)` 原样暴露 `struct stat` + `size()` / `mode()` / `modification_time()` 便捷方法 | **OO1 → 不建包装**（`stat` 直连；便捷方法不替代 `stat`） |
| `TimeParts` 值类型 + `make_epoch` / `to_local` / `to_utc` | `Time` namespace 静态方法（`Time.year(t)`…`Time.format(t, …)`）+ `Time.from_parts` 工厂；不挂 `long` | **OO1 → ① 且不建模**（Q8 已定；`tm` 藏在 shim 内） |
| `Timer` + `Drop` | `(100).make_timer(SIGRTMIN)` / `(250).make_timer_once(SIGRTMIN)` + `start`/`stop`/`remaining` | **③ 资源对象（已定）**；通知走 `signal`，本包不含回调 |
| `Fd` + `FdOps`（13 个方法） | `FileDesc`：`"a.bin".open_as_file_desc(open_read)` → `own FileDesc`；`read`/`write`/`seek`/`close` | **③ 资源对象（已定）**；`Socket` 为同义别名，关闭由包内做 |
| `Address` 值类型 | 主机 `char *`、端口 `int`（方法寄生）+ `resolve_host` + `parse_ip` | **OO1 → 不建模**（地址不引入类型；`receive_from` 以出参回报来源） |
| `math` 新增走 namespace `Math` | 删除 namespace，新增只走 `Float`/`Real` 方法层 | **① → ②**（已定；全名为主、短名为别名） |
| 新 Drop 类型 4 个 | `File` / `Dir` / `FileDesc`（含 `Socket` 别名）/ `Timer` | 全部按 ③ 资源对象落地；`stdout` 等裸 `FILE *` 不归包管 |
| 新值类型 3 个 | **0 个** | 全部不建值类型（`FileStat` 原样暴露 `struct stat`、`TimeParts` 用 namespace、`Address` 不建模） |

### 2.9 原排除项：除变参外全部纳入（① 底座直连 + 注意事项）

判据改为「libc／POSIX 有没有对应函数」：以下原排除项除语言限制的变参函数外全部纳入，形态一律
① 直连，不新增类型。内存操作已由 `char *` 方法层封装（§2.1）；其余 POSIX 接口（线程、异步 I/O、
内存映射、多路复用等）除变参函数外同样按此判据纳入，归属按头文件就近，不新增类型。

| 能力 | 形态 | 归属 | 注意事项 |
| --- | --- | --- | --- |
| `fork` / `execute*` / `system` / `pipe_open` / `wait*` | ① 底座 | `std::process` | `fork` 后子进程中的运行时状态不可假定安全；`execute*` 替换进程镜像；`system`/`pipe_open` 经 shell |
| `signal` / `alarm` | ① 底座 | `std::process` | 回调式 `signal` 受函数指针限制，README 写清可支持的用法与不可用场景；不因此排除 |
| `"zh_CN.UTF-8".make_locale()` → `own Locale`；`apply(category)` / `conventions()` | ② 工厂 + ③ | `std::locale` | `apply` 改的是**进程全局区域设置**，线程不安全；`conventions` 返回借用指针；`make_locale` 只保存名字，不调用 `setlocale` |

**`LC_NUMERIC` 与浮点往返**：`setlocale` / `Locale.apply` 改的是进程全局区域设置，其中 `LC_NUMERIC`
决定小数点字符。受影响的只有浮点相关路径：`parse_real`（`strtod` 族）的解析、`to_str` 对浮点值的
格式化（`snprintf`）；整数解析与格式化（`strtol` 族、`to_base`）不受影响。可执行建议二选一：
启动时固定 `"C"`；或显式跟随环境，并在浮点往返处始终按同一 `Locale` 设置处理。
| `<wchar.h>` 族（按 §5.1 展开，见 §6.2 索引） | ① 底座 | `std::wchar` | 宽字符宽度/编码随平台与 locale 变化；边界写清 |
| `sort` / `binary_search` | ① 底座 | `std::search` | 回调式比较；CT 侧函数指针支持范围写清 |
| `scan_directory` | ① 底座 | `std::fs` | 回调过滤/排序；返回数组的释放责任写清 |
| `scale_by_power_of_two` / `split_exponent` / `split_fraction` | ① 底座 | `std::math` | 出参形态；不建值类型 |
| `send_message` / `receive_message` | ① 底座 | `std::net` | `iovec` 由调用方构造；边界与所有权写清 |

全部 ① 直连项不新增类型；方法层只在 §1.2 的便利性判据成立时另加，不再以「CT 模型不支持」排除。

**不封装：语言限制例外（与模型差异不同）**：CT 无变参声明机制，`scanf` 族、`sprintf`、`fprintf` 不做
封装，也无法靠 shim 绕开。0.3.0 遗留的 `printf` 一并删除（破坏性变更）。成对替代：

- 格式化拼接 → `to_str()` / `concat()`
- 解析 → `parse_int` / `parse_long` / `parse_double` 等 `parse_*`
- `fprintf` → 先格式化，再 `write`

### 2.10 需要为某类型引入 struct 作「对等类型」的情况（单列；A1–A5 均已定）

本节不与上面的纳入清单混在一起：**这些条目不是「要不要包方法」，而是「要不要为某个类型另造一个
struct 作为它的对等类型」**，一旦引入就多出装箱/拆箱、拷贝语义、所有权归属和与 libc 的互操作
解释成本。以下只列事实与代价；**A1–A5 均已由负责人裁定，本表只作裁定记录**。

先纠正一处事实，避免清单建立在错误前提上：**`char` 现在不是靠 struct 承载方法的**。CT 允许标量
作宿主，`Char` 是一个直接映射到 `char` 的 trait（`std/num/Num.m.ct:94` 声明、`:104`
`map trait Char for type char`），没有任何 struct、没有装箱。因此 `char` 不构成本节条目；
负责人已裁定维持现状（A1 划掉，不建 struct），本表 A1 行只作裁定记录。

| 编号 | 原类型 | 为什么原类型承载不了方法 | struct 解决了什么 | 代价 | 状态 |
| --- | --- | --- | --- | --- | --- |
| A1 | `char`（现状：标量宿主可直接挂方法） | 现状**不需要** struct；`Char` 是 trait 不是类型 | 若改为 struct：可携带状态（如编码/大小写表），但现有方法都是纯函数，无状态可带 | 装箱：每个字符一次 `malloc` → 热路径不可接受；拷贝：值语义与 `char` 不再等价；互操作：所有 C 侧 `char` 参数都要拆箱；命名/文档：`Char` 从 trait 变类型，语义双写 | **已定：不建 struct**。`Char` 保持 trait 直接映射 `char`；trait 编译期展开、无装箱；现有方法纯函数，无状态可带 |
| A2 | `char *` 与语言侧内置 `Str`（非本包） | `char *` 能挂方法（`CStrOps` 就是），但**不拥有**；长度要每次 `strlen`，且无法表达「内嵌 `\0` + 长度」。`Str` 由语言侧提供拥有语义，两者不是替代关系 | 拥有堆缓冲 + 显式长度 + `Drop`；非 NUL 结尾也能安全传递 | 装箱：每次构造一次 `malloc`；拷贝：带 `Drop` 的值不能浅拷贝，复制必须深拷贝或显式 `clone`；所有权：与借用 `char *` 混用时边界要写清；互操作：所有 libc/shim 入口仍要 `.data` 取裸指针；命名/文档：同一能力可能两种写法（`"s".length()` vs `s.length()`） | **已定：`Str` 归语言侧内置类型，本仓完全删除——不定义、不提供、不提供方法层，只作消费方**。`char *` 是借用视图、`Str` 是拥有型，二者关系与分工由语言侧决定，不在本仓定义 |
| A3 | `DIR *` → 本方案 `Dir` | C 侧 `DIR` 是不透明类型，CT 看不到定义；裸指针无法挂方法（需要可命名的 CT 类型），且必须 `closedir` | 可命名、可 `Drop`、`close` 幂等；`next()` 等方法有接收者 | 装箱：每个目录句柄一个 `malloc`（约一个指针 + 状态）；拷贝：句柄不可拷贝，需禁止或只允许 `own` 转移；所有权：与 `open_dir()` 的工厂语义绑定；互操作：`readdir` 的 `struct dirent` 仍在 shim 内，返回的是拷出的名字，调用方拿不到原始 `dirent`；命名/文档：`Dir` 与 `FsOps.list` 两种列举方式并存，需说明取舍 | **已定：建 `Dir`（③ 资源对象）**；`File` 与 `Dir` 是两个独立对象，属性方法各不同 |
| A4 | `int`（fd）→ 本方案 `FileDesc`（`Socket` 同义别名） | `int` 本身是合法宿主（`Int` 已存在），技术上能挂 `send`/`close`，但会把 socket 方法挂到**所有整数**上 | 可命名、可 `Drop`、`accept()` 产出新句柄，所有权清晰；`Socket` 与 `FileDesc` 是同一类型、方法一套、可互相赋值 | 装箱：每个句柄一个包装 struct；拷贝：fd 不可拷贝，需禁止；所有权：`accept` 产生新 `own`，与 `open_as_file_desc` 一致；互操作：需要 fd 时得开 `raw_file_desc()`（按 P6 实现），否则无法和 `select`/`poll` 等未包裹能力配合；命名/文档：`Socket` 是别名，不是第二套类型 | **已定：建 `FileDesc`（③ 资源对象），`Socket` 为同义别名**；别名能力依赖语言侧 |
| A5 | 数组 / 动态列表 / `Map`（语言侧内置类型） | 不属本包：这些容器由语言侧提供，std 不再造对等类型 | — | — | **已定：不引入**。数组、动态列表、`Map` 均归语言侧，std 不涉及；删除 `make_list` 一类表述 |

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

**方法形态（唯一形态）**。

```ct
import std::math::Real;
import std::io::File;

int main(void)
{
    char text[32];

    std::io::write_line(9.0.sqrt().to_str(text, sizeof text));
    std::io::write_line((-2.5).abs().to_str(text, sizeof text));
    return 0;
}
```

```
3.0
2.5
```

数学函数只提供方法形态，`Math.sqrt`/`Math.fabs` 这类 namespace 入口已删除；`sqrt` 保留原名，
只有 7 个生僻短名改全称（§2.3）。shim 仍是 M0 一行转发到 libc。

**挂法二：类型宿主方法**（接收者是值）——见 §3.2。

### 3.2 ② 便利层：方法挂到既有类型

这是本版扩宽的一层。判断标准只有一条：**写起来是否明显更短、读起来是否更像 CT**。

```ct
import std::str::String;
import std::io::File;

int main(void)
{
    char buf[8];
    char text[32];

    std::io::write_line("stdlib".length().to_str(text, sizeof text));
    std::io::write_line("stdlib".starts_with("std").to_str(text, sizeof text));
    "stdlib".copy(buf, sizeof buf);
    std::io::write_line(buf);
    return 0;
}
```

```
6
1
stdlib
```

这一组 `std::str` 方法在 shim 里带小循环（返回索引、带 `cap`），代码级是 **M1**；形态仍是 ②。

### 3.3 链路：一层套一层

CT 范式要求「链路走到底」：基础类型 → 文本 → 文本方法，中间不需要临时变量。

```ct
import std::num::Num;
import std::str::String;
import std::io::File;

int main(void)
{
    long n = 65;
    char buf[8];
    char text[8];

    std::io::write_line(n.to_str(buf, sizeof buf));
    std::io::write_line(n.to_str(buf, sizeof buf).length().to_str(text, sizeof text));
    std::io::write_line(n.to_str(buf, sizeof buf).starts_with("6").to_str(text, sizeof text));
    std::io::write_line(n.to_str(buf, sizeof buf));
    return 0;
}
```

```
65
2
1
65
```

这正是负责人举的例子的形态：`n.to_str(buf, sizeof buf)`、`"abc".starts_with("x")` 都属这一层。

### 3.4 ① 底座：一个能力一个展开名

① 底座只暴露展开名；shim 内部仍映射到对应 libc 函数，对应关系见 §6.2 查证索引。示例见 §3.1。

### 3.5 ③ 例外：`Dir` / `Socket` 等资源类型为什么要新类型

零代码映射在资源上会漏掉**所有权**：`opendir` 的 `DIR *`、`socket` 的 fd 交给调用方后，关闭责任
在调用方。③ 只补这一层；方法本体仍是 M0/M1 转发。

规则（实测确认）：必须是**结构体指针 + `own`**，`Drop` 才会在出口执行；栈上值不会。
`own` 的初始化式只认 `malloc` / `calloc` / 返回 `struct X *` 的工厂，`(X *)malloc(...)` 强转**不认**。

```ct
#include <stdlib.h>

import std::io::File;

struct Buf {
    char *data;
    int len;
} use Drop;

impl Drop for Buf {
    void drop()
    {
        std::io::write_line("drop ran");
        free(self->data);
        self->data = 0;
        self->len = -1;
    }
}

int main(void)
{
    own Buf *b = malloc(sizeof(Buf));

    char text[16];
    b->data = (char *)malloc(8);
    b->len = 8;
    std::io::write_line("owned");
    std::io::write_line(b->len.to_str(text, sizeof text));
    /* no free() here: leaving the scope runs Buf.drop, then frees b */
    return 0;
}
```

```
owned
8
drop ran
```

### 3.6 偏离 CT 范式的登记（必须显式取舍）

| 编号 | 偏离 | 为什么保留 | 取舍/后续 |
| --- | --- | --- | --- |
| D1 | ~~`std::math::Math.sqrt(x)` namespace~~ **已删除** | 负责人裁定：数学函数只提供方法形态，namespace 入口不再保留 | 无偏离；数学只改 7 个生僻名，其余保留原名（§2.3） |
| D2 | 变参格式化/解析不封装（`scanf`/`sprintf`/`fprintf`） | CT 无变参声明机制，shim 也绕不开；`printf` 按破坏性变更删除 | 不做；成对替代见 §2.9（`to_str`/`concat`、`parse_*`、格式化+`write`） |
| D3 | ~~`get_dir(out, cap)`~~ **已删除** | 负责人裁定改为对象式：`Dir.current()` / `dir.path(out, cap)` / `dir.enter()` | 无偏离 |
| D4 | `parse_long(out)` / `format_time(out, cap)` 出参 + 状态码 | 失败必须可报；链式返回值无法同时表达 | 与 `copy`/`read_line` 同类，包内已有先例 |
| D5 | `std::env::Env.set` / `std::process::Process.get_pid` 走 namespace | 无接收者 | 不强行造接收者 |

## 4. 模块划分

事实澄清：`Path` / `File` 在 `std/io/File.m.ct`（`:11`、`:19`），`std/str` 只有 `String.m.ct`
一个方法层。`std/io` 是一个 unit 两个宿主。

### 4.1 目录结构

```
std/
  str/    CStrOps.m.ct + CStrOps.shim.h      （0.4.0 改名）
  io/     File.m.ct  FileDesc.h.ct           （Path + File + FileDesc；stdout/stderr/stdin 直接作 `FILE *` 宿主，不封装）
  fs/     Fs.m.ct  Dir.h.ct                  （`Dir`；`FileStat` 不建包装）
  utf8/   Utf8Ops.m.ct + shim
  num/    Num.m.ct + shim
  math/   Real.m.ct + shim                   （只提供方法形态，不建 `Math` namespace）
  search/ Search.m.ct                        （sort/binary_search 等 ① 直连）
  arena/  Arena.h.ct
  test/   Test.h.ct
  env/     Env.m.ct                          （环境变量）
  process/ Process.m.ct                      （进程身份 + 控制/信号）
  errno/   Errno.m.ct                        （错误码）
  locale/  Locale.m.ct                       （`"zh_CN.UTF-8".make_locale()` → `own Locale`；`apply`/`conventions`）
  wchar/   Wchar.m.ct                        （<wchar.h> 族 ① 直连）
  time/   Time.m.ct  Timer.h.ct              （`Time` namespace 静态方法 + `Timer` ③；不挂 `long`）
  net/    Net.m.ct                           （`Socket` 为 `FileDesc` 同义别名；不建 Address）
```

`env/`、`process/`、`errno/` 各自成模块，不合并为 `std::sys`：这三个模块与 libc／POSIX 原有头文件
一一对应，C 程序员不需要再学一套分类法。此条为负责人已定裁定（见 §8.2 Q4）；OO 封装只改变调用
形态，不引入新的分类维度。`locale/`、`wchar/` 分别对应 `<locale.h>`、`<wchar.h>`；`search/` 收纳
`sort`/`binary_search` 等通用算法。以上均按 ① 直连，不新增类型。

### 4.2 import 形态

```ct
import std::str::CStrOps;     // 0.4.0 起
import std::io::File;         // 同时带出 Path 与 File
import std::io::FileDesc;     // FileDesc；Socket 为其同义别名
import std::fs::Dir;
import std::env::Env;
import std::process::Process;
import std::errno::Errno;
import std::time::Time;
import std::time::Timer;
import std::net::Socket;      // 同义别名，与 FileDesc 互相赋值
```

规则：unit 名 = 该文件的类型/主 trait 名；namespace 宿主用 `.m.ct`；自有类型用 `.h.ct`。

### 4.3 `std/io/File.m.ct` 是否拆

已定不拆（Q1）：`0.4.0` 只改 trait 名、不拆 unit；`open` → `read`/`write`/`close` 是连续 API 面，
拆开对消费者是纯成本。一张映射表放一处。

## 5. 命名与契约

### 5.1 命名

- 守命名规范：`snake_case`、动词开头；`get_` / `set_` / `has_` / `is_` / `to_` / `from_` 合规。
- **方法层一律用展开式**：能读出「动作 + 对象」的 `snake_case`，不再豁免 libc 缩写。
  例：`frexp` → `split_exponent`、`ldexp` → `scale_by_power_of_two`、`modf` → `split_fraction`、
  `qsort` → `sort`、`bsearch` → `binary_search`、`strtol` → `parse_long`、`atoi` → `to_int`、
  `strerror` → `error_message`。
- **例外：数学只改 7 个生僻名，ctype 只做 snake_case 化。** `expm1`/`log1p`/`fdim`/`fma`/`cbrt`/`fmod`/`copysign`
  改全称（§2.3）；其余数学名保留原名。ctype 的 `is_*` 全部保留；`to_upper`/`to_lower` 去掉 `to`，定名
  `uppercase`/`lowercase`。`concat` 保留短名；`to_string` 删除，只留 `to_str`。
- **例外二：已成为通用词的缩写保留**：`http`、`utf8`、`pid` 等；`isalnum` → `is_alnum` 这类拆分照旧。
- 方案只立展开名这一套；§6.2 的映射表是**查证索引**，用来从展开名反查对应 libc 函数，不构成第二套写法。
- **便利层也受同宿主唯一名约束**：补方法前必须查表。
- **方法名不重复宿主类型或命名空间已交代的信息**：路径宿主上的 `size` / `mode` / `modification_time`，
  不再写 `file_size` / `file_mode` / `file_mtime`；`read_file` / `write_file` 同理砍掉 `file_`，定名
  `read` / `write`；`Socket` 上的 `set_socket_option` 简化为 `set_option`。
- **方法名不设长度上限**：按「语义写清、类型名已交代过的不再重复」执行（如 `File.length` 不写 `file_length`）。

### 5.2 宿主名字余量

| 宿主 | 现状 | 本版新增 | 合计（最坏） |
| --- | --- | --- | --- |
| `char *` | 25（`README.md:145-153`） | `reverse_find_char` `compare_ignore_case` `parse_long` `parse_long_long` `parse_unsigned_long` `parse_real` `open_dir` `make_locale` `read_link` `resize` `set_mode` `resolve_path` `is_readable` `is_writable` `access` `stat` `open_as_file_desc` `size` `mode` `modification_time` `parse_ip` `connect` `fill` `compare_with` `find_byte` `compare_prefix` `copy_from` `move_from` `split_into`（29 个新增名；`copy_from`/`move_from` 替代既有 `copy_to`，`length` 一换一替代 `len`，`open` 一换一改 `open_file`，删除 `get_dir`/`change_dir`，净增 28，即 25 − 1 + 29；名字均按 §5.1） | 53 |
| `long` | 7 | 0（时间不挂 `long`，Q8 已定） | 7 |
| `float`/`double` | 5 / 9 | math 新族（约 20；`abs`/`min`/`max` 只登记浮点宿主，不重复计数） | 25 / 29 |
| `char` | 7 | 7 个 `is_*`（`is_alnum` / `is_punctuation` / `is_printable` / `is_graphical` / `is_control` / `is_blank` / `is_hex_digit`） | 14 |

`char *` 合计 53：本版新增 `access`/`stat`/`open_as_file_desc`/`make_locale`，删除 `get_dir`/`change_dir`，把 `len` 换成
`length`（一换一）、`open` 换成 `open_file`（一换一），并把 `memcpy`/`memmove` 拆成 `copy_from`/`move_from`、
新增 `split_into`。换名本身不改变名字数量，`char` 仍为 14；展开名与既有同宿主名无冲突。`float`/`double` 合计以全名清单
确认后复核（短名是别名，不重复计数）。

冲突处理：`open` 已被 `Path` 占用 → 目录用 `open_dir`；`read`/`write`/`close` 不再占 `char *`；
`copy_from`/`move_from` 取代既有 `copy_to`（`src.copy_to(dst, n)` → `dst.copy_from(src, n)` 或
`dst.move_from(src, n)`），内存版只保留一个方向、但区分是否允许重叠；`copy` 仍是带容量 / `\0` 的字符串拷贝；
`long` 不再新增时间方法（Q8 已定），因此没有 `format` 与既有 `to_str` 的冲突。

### 5.3 失败表达

| 场景 | 表达 |
| --- | --- |
| 布尔查询 | `0` / `1` |
| 未命中 | `-1` |
| 状态型操作 | `0` 成功 / `-1` 失败 |
| 字节数 | `n`；`0` = EOF；`-1` = 错误 |
| 构造资源 | `NULL`（`0`） |
| 错误详情 | 模块 `xxxEnv.get_last_error()`；新增 `std::errno::Errno.get_last()` |
| 缓冲区不足 | `-1`，不截断、不返回部分结果（`to_str` 例外） |

### 5.4 所有权

- **借用（默认）**：方法层不拥有输入、只写调用方缓冲；`env` / `errno.error_message` 返回**借用**指针，
  生命周期到下一次同类调用（README 必须写）。
- **产出型结果的归属随目标类型**（依赖 CT 语言机制，见 §2.1）：若目标声明为 `char *`，结果是由被调方
  分配、调用方释放的裸内存；若目标声明为语言侧内置 `Str`，则是拥有型、由 `Drop` 回收。同一句调用，
  内存归属随目标类型变化，README 必须把两种目标的释放责任分别写清。
- **比较不产出值**：相等比较用 `char *` 视图比内容，不拷贝、不隐式分配。
- **持有**：`File` / `Dir` / `FileDesc`（`Socket` 为同义别名）/ `Timer` 均为 ③ 资源对象。
  `own` 句柄离开作用域由包内关闭；另给显式 `close()` 供提前关闭，关后句柄失效、重复调用是空操作；
  `stdout` 这类裸 `FILE *` 不归包管。`pub struct X { ... } use Drop;` + 显式 `impl Drop`
  （字段胶水不释放指针字段）；`own X *p = ...`。
- **值类型**：不新增（`FileStat` 不建包装、`stat(&st)` 原样暴露 `struct stat`；`TimeParts` 用
  `Time` namespace；`Address` 不建模）。
- **拷贝语义的输入**：`split_into` 只读源串，不写入、不移动原串；每段写入调用方缓冲，函数自身不分配内存。
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
## 6. 使用注意事项与已知限制
```

API 表新增 **层** 列：① 映射 / ② 便利 / ③ OO。读者不用翻 shim 就知道成本形态。样例必须来自
`tests/`，保证不腐。

### 6.2 顶层索引（任务、宿主、查证）

1. **按任务**：读文件、遍历目录、当前时间、网络……没有的写「不归 std（仅 §1.4 三类）」或注意事项。
2. **按宿主**：`char *` / `File *` / `FileDesc *` / `Dir *` / `long` / `double` / namespace。
3. **查证索引**：从 std 展开名反查对应 libc 函数；只用于查证，不表示第二套写法。以
   [libc 覆盖盘点](../libc-inventory.md) 为源。

| std 展开名 | libc 函数 |
| --- | --- |
| `length` | `strlen` |
| `to_str` | `snprintf` |
| `to_int` | `atoi` |
| `parse_long` | `strtol` |
| `parse_long_long` | `strtoll` |
| `parse_unsigned_long` | `strtoul` |
| `is_infinite` | `isinf` |
| `is_printable` / `is_graphical` / `is_control` / `is_punctuation` / `is_hex_digit` | `isprint` / `isgraph` / `iscntrl` / `ispunct` / `isxdigit` |
| `exists` | `stat` |
| `read` / `write` / `list` | `fopen` + `fread` / `fopen` + `fwrite` / `opendir` + `readdir` |
| `size` / `mode` / `modification_time` | `stat` |
| `access` | `access` |
| `stat` | `stat`（原样暴露 `struct stat`） |
| `open_file` | `fopen` |
| `open_as_file_desc` | `open` |
| `path` / `enter` | `getcwd` / `chdir` |
| `make_timer` / `make_timer_once` | `timer_create`（周期/一次性由 `timer_settime` 参数区分） |
| `at_end_of_file` | `feof` |
| `make_directory` | `mkdir` |
| `now_seconds` / `now_nanoseconds` / `sleep_milliseconds` | `time` / `clock_gettime` / `nanosleep` |
| `get_effective_uid` | `geteuid` |
| `error_message` | `strerror` |
| `receive` / `send` / `set_option` / `receive_from` | `recv` / `send` / `setsockopt` / `recvfrom` |
| `sort` / `binary_search` / `scan_directory` | `qsort` / `bsearch` / `scandir` |
| `scale_by_power_of_two` / `split_exponent` / `split_fraction` | `ldexp` / `frexp` / `modf` |
| `send_message` / `receive_message` | `sendmsg` / `recvmsg` |
| `make_locale` | 无对应，自实现（只保存 locale 名字，不调用 `setlocale`） |
| `apply` | `setlocale`（改进程全局状态，线程不安全） |
| `conventions` | `localeconv`（返回借用指针） |
| `pipe_open` / `execute*` | `popen` / `exec*` |
| `wide_string_length` / `multibyte_to_wide_string` / `wide_to_multibyte_string` | `wcslen` / `mbstowcs` / `wcstombs` |
| `reverse_find_char` | `strrchr` |
| `copy_from` / `move_from` | `memcpy` / `memmove` |
| `fill` / `compare_with` / `find_byte` / `compare_prefix` | `memset` / `memcmp` / `memchr` / `strncmp` |

`split_into` 不列入本索引：libc 只有会改写源串的 `strtok`/`strtok_r`，没有等价函数；它是 ② 便利层的
缓冲式拷贝切分（签名 `split_into(separator, out, cap, slot_size)`，见 §2.1）。

维护规则：新增 API 一次改三处（§4 表、模块 README、查证索引），缺一处视为未完成。

## 7. 施工顺序（全库覆盖为完成判据）

原则：先铺 ①②，再补 ③；③ 只给真正持有资源且无法寄生基本类型的类型（`File`/`Dir`/`FileDesc`/`Timer` 均已定为 ③，`Socket` 为 `FileDesc` 别名）；每个模块整块交付（§1.7），不做半截。

P0–P7 是施工顺序，不是分批发布节奏。表中判据只作阶段验收，最终以 §2 能力面（含 §2.9 的 ① 直连能力）全库覆盖为完成，不以任一相的交付为完成；全库完成后再统一发布 `0.4.0`。

| 序 | 内容 | 层 | 依赖 | 阶段验收 |
| --- | --- | --- | --- | --- |
| **P0** | `String` → `CStrOps`（issue 0001，`0.4.0`）；`len` → `length`；`open` → `open_file`；`to_string` 删除（只留 `to_str`）；`to_upper`/`to_lower` → `uppercase`/`lowercase`；删除 `printf`（破坏性） | 改名/删除 | 无 | 旧名零命中；`tests/run.sh` 全绿；文档同步 |
| **P1** | `char` 7 个 `is_*`；`char *` 内存方法（`copy_from`/`move_from`/`fill`/`compare_with`/`find_byte`/`compare_prefix`）与 `split_into`；`math` 新族（`Float`/`Real` 方法层，全名为主、短名为别名）；`sort`/`binary_search`、`scale_by_power_of_two`/`split_exponent`/`split_fraction` ① 直连 | ①② | P0 | 每条 M0/M1 转发；无宿主重名；重叠/不重叠拷贝夹具；三态/指针返回验证；回调/出参用法写明 |
| **P2** | 新增 `std::env` / `std::process`（含控制/信号）/ `std::errno` / `std::locale` / `std::wchar`（各一个模块） | ②③ | P1 | 无新类型（`Locale` 除外）；借用指针语义、`fork` 后状态、`Locale.apply` 改进程全局状态且线程不安全，写进 README |
| **P3** | `stdio`：`File`（`FILE *` 宿主）+ `FileDesc`（`open_as_file_desc`）；方法补全（`seek`/`tell`/`rewind`/`at_end_of_file`/`has_error`/`read_char`/`write_char`/`read_line`/`flush`）；变参格式化/解析按 §2.9 成对替代，不封装（`printf` 已在 P0 删除） | ③ | P0 | `own` 关闭与幂等 `close` fixture；`stdout` 裸宿主不归包管；`to_str`/`concat`、`parse_*`、格式化+`write` 样例覆盖 |
| **P4** | `std::fs` 补充（`access`/`stat`/`read_link`/`resize`/`set_mode`/`resolve_path`/`is_*`/`size`/`mode`/`modification_time`）；`Dir.current()`/`dir.path`/`dir.enter`；`scan_directory` ① 直连 | ①②③ | P2 | `struct stat` 原样暴露；三个便捷方法不替代 `stat`；`is_readable`/`is_writable` 按真实身份判定的限制写进 README |
| **P5** | `std::time`：`now_seconds`/`now_nanoseconds`/`diff_seconds`/`sleep_milliseconds` + `Time` namespace 静态方法（`year`…`format`、`from_parts`）；`Timer`（`make_timer`/`make_timer_once` + `start`/`stop`/`remaining`） | ①③ | P2 | 不挂 `long`；不做 `TimeParts` 值类型；`tm` 只在 shim 内；时区行为写明；`Timer` 通知走 `signal`、不含回调 |
| **P6** | `File` / `Dir` / `FileDesc` / `Timer` 的 `Drop` 模板收口（`Socket` 为 `FileDesc` 别名） | ③ | P3（模板） | `own` 样例；`close` 幂等；大目录流式列举；别名与 `raw_file_desc()` 边界 |
| **P7** | `std::net`：`Socket`（TCP + UDP，`FileDesc` 同义别名）+ `"host:80".connect()` + `resolve_host` + `set_option`；`send`/`receive` 链式；`send_message`/`receive_message` ① 直连 | ②③ | P6（模板） | TCP/UDP loopback 回环；`Drop` 关闭；`receive` 字节数出参；超时与 `set_option` 行为写明；`iovec` 构造/所有权写明 |

「暂不排期」状态不再存在：`File`、`Dir`、`FileDesc`、`Timer` 均按 ③ 资源对象纳入交付面，形态已定；
`FileStat`、`TimeParts` 只补能力、不建值类型。

其余 POSIX 接口（`pthread_*`、`aio_*`、`mmap`、`poll`/`select` 等）随所属模块的施工序完成，不单列相位；归属按头文件就近，形态一律 ① 直连。

## 8. 风险与未决

### 8.1 已知风险

| 风险 | 说明 | 缓解 |
| --- | --- | --- |
| 便利层铺宽 → 名字通胀 | `char *` 25 → 53；`long` 保持 7（Q8 已定不挂时间） | 每个名字必须给出「比函数式省多少」；同宿主查表 |
| 宿主污染 | 领域方法挂基础类型会让所有同类型值获得该方法（如 `size.year()`） | Q8 已定：时间不挂 `long`，走 `Time` namespace；其他宿主按 §5.2 查表 |
| 时区/线程安全 | `localtime_r` 无时区参数；`strerror` 静态缓冲 | 文档写明「本地时区」与 `strerror` 非线程安全 |
| `own` 初始化式很窄 | 只认 `malloc`/`calloc`/`struct X *` 工厂；强转与别名不认（实测） | 工厂返回类型写 `struct X *`；每个新类型加 `own` 样例 |
| 栈上值不触发 `Drop` | 实测值不释放，只有 `own` 指针会 | 句柄必须指针 + `own` |
| `File` 释放 | 旧 `IoFile` 包装体不释放；现已定 `own` + `Drop` + 幂等 `close()` | 0.4.0 破坏性变更；`stdout` 等裸 `FILE *` 不归包管 |
| 多次 `stat` | `size`/`mode`/`modification_time` 各一次 | `stat(&st)` 原样暴露；三个便捷方法不替代 `stat`，重复代价写进 README |
| 借用指针生命周期 | `getenv` / `strerror` 可能被覆盖 | 方法层提供带 `out` 的复制版；文档写死 |
| 表达力注意 | 回调、`iovec`、`struct stat` 布局 | ① 直连并在 README 写使用边界；不排除 |
| 变参限制 | CT 无变参声明机制；`scanf`/`sprintf`/`fprintf` 等不做 | 不做封装；按 §2.9 成对替代（`to_str`/`concat`、`parse_*`、格式化+`write`）；`printf` 按破坏性变更删除 |
| `Socket` 别名依赖语言侧 | `Socket` 与 `FileDesc` 是同一类型、可互相赋值，依赖语言侧别名能力 | 方案只记这一依赖；未提供前不造第二套类型 |
| 数学全名清单 | 全名为主、短名为别名，全名清单尚未确认 | 清单确认后再并入正文；短名保持可用 |
| 阻塞模型 | `Socket` 阻塞、DNS 无超时控制 | P7 写明阻塞语义与超时行为 |
| 版本节奏 | 全库完成后统一发布 `0.4.0`，不按相发 minor | 以全库覆盖为完成判据（§7） |

### 8.2 未决

- **Q1（已定）**：`std/io/File.m.ct` 不拆编译单元，一张映射表放一处。
- **Q2（已定）**：`File` 按 ③ 资源对象：`own` 离开作用域由包内关闭；显式 `close()` 供提前关闭，关后失效、重复调用是空操作；`stdout` 等裸 `FILE *` 不归包管。
- **Q3（已定）**：不设方法名长度上限，也不设 `char *` 名字数量上限（当前 53）；保留准入判据与软阈值复审。
- **Q4（已定）**：`env`/`process`/`errno` **不合并**为 `std::sys`，三者各自成模块，目录与 libc／POSIX 原有头文件一一对应。依据：降低 C 程序员的心智成本——做的是 OO 封装，不引入新的分类法。
- **Q5（不再适用）**：P0–P7 改为施工顺序，不按相发布；合并相邻相发布的讨论随之取消，版本节奏见 §7 / §8.1。
- **Q6（已定）**：`net` 一次做全，UDP、`resolve_host`、`set_option` 全部纳入，不再留在暂不排期。依据：不做半截模块——只封一半会让使用者不知道到底该用封装还是退回 libc。`Address` 不建类型（已定）：主机用 `char *`、端口用 `int`，方法寄生在这两个宿主上；`receive_from(buf, len, ipbuf, &port)` 以出参回报来源。依据：静态映射优先、能力寄生在基本类型上，不引入值类型。
- **Q7（已定）**：`stat(&st)` 原样暴露 `struct stat`；`size`/`mode`/`modification_time` 为基于 `stat` 的便捷方法，不建 `FileStat` 包装。
- **Q8（已定）**：时间方法不挂 `long`——宿主语义不相合（`size.year()` 会返回 2023）；时间只做 ① 底座 + `Time` namespace 静态方法，「迁就 `long`」正式否掉。
- **Q9（已定）**：删除 `Math` namespace；数学函数只提供方法形态，全名为主、短名为别名（全名清单待确认）。
- **Q10（已定）**：`Str` 归语言侧内置类型，本仓完全删除——不定义、不提供、不提供方法层，只作消费方。此前「允许在 `Str` 上挂方法」的结论作废；`char *` 与 `Str` 的分工由语言侧决定，本仓不再制定。
- **Q11（已定）**：`Dir` / `File` / `FileDesc` 均接受 struct 对等类型（③）；`Socket` 为 `FileDesc` 同义别名，不用裸类型宿主。
- **Q12（已定）**：不引入列表 struct；数组、动态列表、`Map` 归语言侧，std 不涉及。
- 评审项均已关闭；`Socket` 别名能力为唯一外部依赖（待语言侧）。

### 8.3 本文可被证伪的假设

1. 「便利层铺宽是净收益」——若评审认为 53 个 `char *` 名字的维护成本过高，需按 Q3 收缩。
2. 「时间方法可以挂 `long`」——Q8 已定否掉；时间走 `Time` namespace，不再作为假设。
3. 「`FileDesc` / `Timer` 可以先不做」——已定按 ③ 资源对象落地，本条不再作为假设。
4. 「`Address` 不建类型，多地址回退由调用方在 `resolve_host` 结果上自行处理」——若统一地址表示成为硬需求，需重开该裁定。
5. 「数学全名清单可一次定稿」——清单尚待确认（§2.3、§5.1），确认前短名保持可用。

## 9. 交付与推进

1. 本文定稿 → 交负责人过目。
2. 负责人点头后才进入实现；未点头前只改文档，不动代码、不起版本号、不打 tag。
3. 实现阶段按 §7 施工顺序独立提交；表中判据只作阶段验收，最终以全库覆盖为完成；跨仓能力只记为依赖，不向 ct-lang 提排期安排，也不引用外部仓路径。
