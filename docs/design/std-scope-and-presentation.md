# `std` 的范围与呈现形态：完整方案（评审稿）

| 项 | 值 |
| --- | --- |
| 性质 | 主张稿：只出方案，不写实现、不动版本、不开相卡、不改既有代码 |
| 底稿 | [libc 覆盖盘点](../libc-inventory.md)、[issue 0001](../../issues/0001-str-string-trait-rename.md) |
| 现状基线 | `0.3.1`；八个模块：`str` `io` `fs` `utf8` `num` `math` `arena` `test` |
| 评审方式 | 本文为「主张方」；另有一席反证，负责人裁 |

## 1. 边界

### 1.1 在范围内

1. **C 标准库 + POSIX.1 里 CT 侧有稳定用法的部分**：字符串/内存视图、stdio 句柄、目录、`stat`、
   时间、环境变量、进程身份、fd、socket。
2. **同一宿主的一层统一拼写**：`char *`、`char`、`int` / `long` / `long long`、`float` / `double`、
   namespace、`IoFile *`。方法名全局唯一、调用点零成本展开。
3. **资源类型的生命周期**：句柄 + `Drop`，让「忘记关」不再是调用方的责任。
4. **本包自有设施**：`Arena`（池化）、`Test`（断言）、`Utf8Ops`（字节 ↔ 码点）。

### 1.2 不归 std

| 类别 | 理由 |
| --- | --- |
| 编译器内置层（`Vec` / `Map` / `Str`） | 0.3.0 已删除 `std::vec` / `std::map`，重名会撞 `'Vec' is a built-in type and cannot be redeclared`；内置层归 ct-lang |
| 泛型容器、分配器抽象、线程/并发、异步/事件循环 | 语言与运行时能力，std 不做第二套 |
| 第三方包（JSON / HTTP / 正则 / 日志 / CLI） | 独立 ct-pkg，按需引入 |
| 纯 libc 直映射（第三档：`memcpy` / `memmove` / `memset` / `memcmp` / `memchr` / `strncmp`） | 包装只加一层、不增安全；热路径必须零成本 |
| 进程控制与信号（`fork` / `exec*` / `system` / `popen` / `alarm` / `signal`） | CT 无进程/并发模型，语义无法兜底 |
| locale、宽字符、多编码转换 | 编码统一走 `std::utf8`；locale 是平台全局状态 |
| Windows / 非 POSIX 平台 | 现有实现已依赖 `dirent.h` / `unistd.h`；跨平台是另一个项目 |
| 编译器诊断、构建工具、包管理器 | 属于 ct / ct-pkg |

### 1.3 判据（用于新提案的准入）

一条 libc 能力进 std，必须至少满足一条，且不违反「不归 std」：

1. 有资源或生命周期 → 第一档，包成句柄 + `Drop`。
2. 纯函数但命名分散、失败口径不一 → 第二档，包成 namespace/类型宿主方法。
3. 第三档（热路径、纯内存）→ **不包**，直接 libc 原名。
4. 需要回调、任意变参、或结构体出参而当前语言表达不了 → **暂不包**，列入未决而不是硬做。

## 2. 分模块纳入清单

「现状」与「新增」分开列；每个新增都落到具体函数名、宿主、是否新类型。第三档条目只列不包。

### 2.1 `std::str`（1 个方法层，`char *`）

| API | 宿主 | 新类型 | 状态 |
| --- | --- | --- | --- |
| `len` `find` `find_char` `starts_with` `ends_with` `copy` `slice` `concat` `to_lower` `to_upper` `compare` `copy_to` `to_int` `to_double` | `char *` | 否 | 已有 |
| `rfind_char` | `char *` | 否 | 新增 |
| `compare_ignore_case` | `char *` | 否 | 新增 |
| `parse_long` / `parse_llong` / `parse_ulong` / `parse_real` | `char *` | 否 | 新增（含错误报告，替代 `atoi` 系） |

另：trait `String` → `CStrOps`（issue 0001，落 `0.4.0`）；方法名不变。

### 2.2 `std::num`（4 个宿主）

| API | 宿主 | 新类型 | 状态 |
| --- | --- | --- | --- |
| `abs` `clamp` `max` `min` `to_str` `to_string` `to_base` | `int` / `long` / `long long` | 否 | 已有（三套同形） |
| `is_digit` `is_alpha` `is_space` `is_upper` `is_lower` `to_upper` `to_lower` | `char` | 否 | 已有 |
| `is_alnum` `is_punct` `is_print` `is_graph` `is_cntrl` `is_blank` `is_xdigit` | `char` | 否 | 新增 |

### 2.3 `std::math`

| API | 宿主 | 新类型 | 状态 |
| --- | --- | --- | --- |
| `sqrt` `fabs` | namespace `std::math`（`Math`） | 否 | 已有 |
| `abs` `sqrt` `floor` `ceil` `round` | `float` / `double` | 否 | 已有 |
| `is_nan` `is_inf` | `double`（`Real`） | 否 | 已有 |
| `pow` `exp` `exp2` `expm1` `log` `log2` `log10` `log1p` | namespace `std::math` | 否 | 新增 |
| `sin` `cos` `tan` `asin` `acos` `atan` `atan2` `sinh` `cosh` `tanh` `hypot` `cbrt` | namespace `std::math` | 否 | 新增 |
| `fmod` `remainder` `fdim` `fma` `copysign` | namespace `std::math` | 否 | 新增 |
| `truncate` `min` `max` `is_nan` `is_inf` | `float` / `double` | 否 | 新增（`Float` 补 `is_*`；`truncate` 对应 `trunc`） |

### 2.4 `std::io`（stdio 面）

| API | 宿主 | 新类型 | 状态 |
| --- | --- | --- | --- |
| `puts` `putchar` `getchar` `fflush` | namespace `std::io`（`Stdout`） | 否 | 已有 |
| `printf` | namespace `std::io`（普通函数） | 否 | 已有（唯一支持的变参入口） |
| `open`（返回 `IoFile *`） | `char *`（`Path`） | 否 | 已有 |
| `read` `write` `close` | `IoFile *`（`File`） | 否 | 已有 |
| `seek(offset, origin)` `tell()` `rewind_start()` | `IoFile *` | 否 | 新增 |
| `at_eof()` `has_error()` `clear_error()` | `IoFile *` | 否 | 新增 |
| `read_char()` `unread_char(c)` `write_char(c)` | `IoFile *` | 否 | 新增 |
| `read_line(out, cap)` `write_line(s)` | `IoFile *` | 否 | 新增 |
| `stat(out)` | `IoFile *` | 依赖 `FileStat` | 新增（见 2.5） |
| `open_fd(flags, mode)` | `char *` | 否（返回 `Fd *`） | 新增 |
| `read` `write` `close` `seek` `tell` `read_at` `write_at` `sync` `truncate` `is_tty` `set_mode` `lock` `stat` | `Fd *` | **是：`Fd`** | 新增 |

### 2.5 `std::fs`

| API | 宿主 | 新类型 | 状态 |
| --- | --- | --- | --- |
| `exists` `read_file` `write_file` `list` `mkdir` `remove` `rename` | `char *`（`FsOps`） | 否 | 已有 |
| `get_last_error()` | namespace `std::fs`（`FsEnv`） | 否 | 已有 |
| `open_dir()` | `char *` | 否（返回 `Dir *`） | 新增 |
| `next(out, cap)` `rewind()` `close()` `tell()` `seek(pos)` | `Dir *` | **是：`Dir`** | 新增 |
| `read_stat(out)` `read_link_stat(out)` `read_link(out, cap)` `resize(len)` `set_mode(mode)` `resolve_path(out, cap)` `is_readable()` `is_writable()` `change_dir()` | `char *` | 否 | 新增 |
| `get_dir(out, cap)` | namespace `std::fs`（普通函数） | 否 | 新增 |
| `is_dir()` `is_reg()` `is_link()` `size()` `mode()` `mtime()` | `FileStat` | **是：`FileStat`** | 新增 |

### 2.6 `std::utf8` / `std::arena` / `std::test`

| 模块 | API | 状态 |
| --- | --- | --- |
| `std::utf8` | `decode` `count_codepoints` `encode`（`char *`） | 保持 |
| `std::arena` | `Arena` + `ArenaOps`：`init` `alloc` `reset` `free_all` | 保持 |
| `std::test` | `Test` + `TestOps`：`init` `check_true` `check_eq_int` `check_eq_str` `report` | 保持 |

### 2.7 新模块

| 模块 | API | 宿主 | 新类型 |
| --- | --- | --- | --- |
| `std::time` | `now_sec()` `now_ns()` `get_cpu_ticks()` `diff_seconds(a, b)` `sleep_ms(ms)` `make_epoch(parts)` `to_local(epoch, parts)` `to_utc(epoch, parts)` `format_time(parts, fmt, out, cap)` | namespace `std::time`（`Time`） | `TimeParts`（值类型） |
| `std::time` | `open_timer()` `start(ms)` `stop()` `close()` | `Timer *` | **`Timer`（+`Drop`）** |
| `std::env` | `get(name, out, cap)` `set(name, value)` `unset(name)` | namespace `std::env`（`Env`） | 否 |
| `std::process` | `get_pid()` `get_parent_pid()` `get_uid()` `get_euid()` `get_gid()` | namespace `std::process`（`Process`） | 否 |
| `std::errno` | `get_last()` `message(code, out, cap)` | namespace `std::errno`（`Errno`） | 否 |
| `std::net` | `open_socket(domain, type, proto)` | namespace `std::net`（`Net`） | 否（返回 `Socket *`） |
| `std::net` | `bind_to(host, port)` `listen_backlog(n)` `accept_one()` `connect_to(host, port)` `send_bytes(buf, n)` `recv_bytes(buf, n)` `shutdown_both()` `close()` `set_option(opt, val)` | `Socket *` | **`Socket`（+`Drop`）** |
| `std::net` | `resolve_host(name, out)`；`parse_ip(out)`（`char *`）；`format_ip(family, out, cap)`（namespace） | 混合 | **`Address`（值类型）** |

### 2.8 明确不纳入本包（第三档与暂不包）

`memcpy` `memmove` `memset` `memcmp` `memchr` `strncmp`（第三档直映射）；
`qsort` `bsearch` `scandir`（回调）；`scanf` 族、`sprintf`、`fprintf`、`snprintf`（变参/无界）；
`fork` `exec*` `system` `popen` `signal` `alarm`；`locale` 与宽字符；`ldexp` `frexp` `modf`（出参）；
`sendmsg` `recvmsg`（`iovec`）。

## 3. 呈现形态（重点）

四种形态，各自一票：适合什么、代价是什么、什么时候不该用。样例全部真编译真运行（`ct` 0.53，
`-Wall -Wextra` 无告警），输出照贴。

### 3.1 类型宿主方法：`char *` 上挂

适合：**接收者天然是一个值**、方法只读它或不改变所有权（`len`、`starts_with`、`parse_long`）。
代价：① 方法名在该宿主上**全局唯一**，`char *` 已占 25 个名字；② 没有命名空间隔离，读者看不出
方法来自哪个模块；③ 只能「加方法」，不能加状态。

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

不该用：接收者本身是个资源（`FILE *`、目录、socket）——那会把「谁负责关闭」藏进方法层；
以及需要返回新对象/新状态的场景（没有地方挂状态）。

### 3.2 namespace 静态方法

适合：**没有接收者，或接收者是标量、调用点想显式看到模块名**（`std::math::Math.sqrt`、
`std::time::Time.now_sec`、`std::io::printf`）。代价：每次调用多写一段前缀；不能像方法层那样
`x.foo()` 那样链式；同一 namespace 内方法名也要唯一。

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

不该用：能力天然属于某个值（`"s".len()` 比 `std::str::Str.len("s")` 可读）；以及需要状态的
操作（namespace 上没有字段）。

### 3.3 新类型 + `Drop`

适合：**有资源、有生命周期**：目录遍历、fd、socket、定时器、`FileStat` 这类复合值。
规则（实测确认）：

- 必须是**结构体指针 + `own`**，`Drop` 才会在作用域出口自动执行；栈上值不会。
- `own` 的初始化式只认 `malloc(...)` / `calloc(...)` / 返回 `struct X *` 的工厂调用；
  `(X *)malloc(...)` 强转**不认**（实测报 `'own' requires a malloc, calloc or factory call`）。

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

代价：① 每个句柄都要一个 `struct` + trait + `impl`，比方法层贵；② `own` 的三种初始化式限制
会传导到工厂函数签名（返回类型必须字面是 `struct X *`，别名可能不认）；③ `close()` 仍需显式提供
（幂等），`Drop` 只是兜底。

不该用：无资源的纯值（`FileStat` 是值类型，不需要 `Drop`）；以及会被大量短命创建的小对象
（`own` 每次 `malloc`/`free`，热路径不合适）。

### 3.4 不包装，直接 libc 原名

适合：热路径、纯内存操作、语义已经稳定且没有错误分支可统一（`memcpy` / `memcmp` / `strcmp` /
`strncmp` / `memset`）。代价：调用点保留 C 拼写，失败语义靠 libc；好处是零成本、零歧义。

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

不该用：有错误码/资源/缓冲区的操作——那些必须走前三形态，否则每个调用点自己处理 `errno`
和长度。

### 3.5 形态选择表

| 场景 | 形态 | 例子 |
| --- | --- | --- |
| 接收者是一个值，无状态 | 类型宿主方法 | `"s".len()`、`'c'.is_alnum()` |
| 无接收者 / 标量入参 / 要显式模块名 | namespace 静态方法 | `std::math::Math.sqrt(x)`、`std::io::printf` |
| 有资源、需要关闭 | 新类型 + `own` + `Drop` | `Dir`、`Fd`、`Socket`、`Timer` |
| 纯内存 / 热路径 | 不包装 | `memcmp`、`strncmp` |
| 复合只读结果 | 值类型（无 `Drop`） | `FileStat`、`TimeParts`、`Address` |

## 4. 模块划分

先纠正一处事实：`Path` / `File` 不在 `std/str`，而在 `std/io/File.m.ct`（`:11` 与 `:19`）；
`std/str` 只有 `String.m.ct` 一个方法层。所以「str 里塞了三套」不成立，真正的问题是
**`std/io` 的一个 unit 承载了两个 trait、两个宿主**（`Path`@`char *`、`File`@`IoFile *`）。

### 4.1 目录结构（新增模块用粗体）

```
std/
  str/    CStrOps.m.ct + CStrOps.shim.h      （0.4.0 由 String.* 改名）
  io/     Io.h.ct  Stdout.m.ct  File.m.ct    （Path + File 两个 trait）
          Fd.h.ct                            （新句柄）
  fs/     Fs.m.ct  Dir.h.ct  FileStat.h.ct
  utf8/   Utf8Ops.m.ct + shim
  num/    Num.m.ct + shim
  math/   Math.m.ct  Real.m.ct + shim
  arena/  Arena.h.ct
  test/   Test.h.ct
  time/   Time.m.ct  TimeParts.h.ct  Timer.h.ct
  env/    Env.m.ct
  process/ Process.m.ct
  errno/  Errno.m.ct
  net/    Net.m.ct  Socket.h.ct  Address.h.ct
```

### 4.2 import 形态

```ct
import std::str::CStrOps;      // 0.4.0 起
import std::io::File;          // 仍同时带出 Path 与 File 两组方法
import std::io::Fd;
import std::fs::Dir;
import std::fs::FileStat;
import std::time::Time;
import std::time::Timer;
import std::net::Socket;
```

规则：**unit 名 = 该文件暴露的类型/主 trait 名**（现有 `Utf8Ops.m.ct`、`Arena.h.ct` 即此），
一个 unit 一个宿主；namespace 宿主用 `.m.ct`，自有类型用 `.h.ct`。

### 4.3 是否拆 `std/io/File.m.ct`

- 拆的理由：一个 unit 两个宿主，README 与变更记录会混；`Path` 改名（`PathOps`）与 `File`
  改名（`FileOps`）是两件事。
- 不拆的理由：`open` → `read`/`write`/`close` 是一个连续 API 面，拆开后 `import` 从 1 条变 2 条，
  对消费者是纯成本；`tests/run.sh` 的 layout 闸门只查 `*.h.ct`，拆不拆都不影响门禁。
- **建议**：`0.4.0` 只改 trait 名、不拆 unit；是否拆文件列入 §8 未决 Q1，由负责人裁。

## 5. 命名与契约

### 5.1 命名

- 守 §5.1：`snake_case`、动词开头；`get_` / `set_` / `has_` / `is_` / `to_` / `from_` 前缀合规。
- 数学/字符直映射沿用 `Math.sqrt` / `Char.is_alpha` 先例（`README.md:505-506` 豁免）。
- 一个新名字上线前必须过三关：① 不在同宿主既有名字里（§5.2）；② 不以 libc 同名占位（第三档除外）；
  ③ 与模块 README、`README.md` §4 表、libc 映射表同时更新。

### 5.2 `char *` 余量与冲突

现状 25 个（`README.md:145-153`）。本方案若全落，`char *` 上再加 18 个：
`rfind_char`、`compare_ignore_case`、`parse_long`、`parse_llong`、`parse_ulong`、`parse_real`、
`open_fd`、`open_dir`、`read_stat`、`read_link_stat`、`read_link`、`resize`、`set_mode`、
`resolve_path`、`is_readable`、`is_writable`、`change_dir`、`parse_ip` → 合计 43。

冲突处理：

- **`open` 已被 `Path` 占用**，fd/目录入口改用 `open_fd` / `open_dir`；
- `read` / `write` / `close` 在 `char *` 上空闲，但**不再占**（与 `IoFile *` 同名会误导），
  fd 层的这三个名字只挂在 `Fd *` 上；
- 新增前查 `README.md` §4.2 表；建议把「同宿主重名」做成静态闸门（见 §7 判据），但那是实现工作。
- 结论：`char *` 应视为**接近冻结**——新增能力优先走新类型或 namespace（§8 Q3）。

### 5.3 失败表达（统一口径）

| 场景 | 表达 | 例 |
| --- | --- | --- |
| 布尔查询 | `0` / `1` | `exists`、`is_digit`、`at_eof` |
| 未命中查询 | `-1` | `find`、`find_char`、`rfind_char` |
| 状态型操作 | `0` 成功 / `-1` 失败 | `mkdir`、`close`、`seek`、`bind_to` |
| 字节数 | `n`；`0` = EOF；`-1` = 错误 | `read`、`write`、`format_time` |
| 构造资源 | `NULL`（`0`） | `open`、`open_dir`、`open_fd`、`open_socket` |
| 错误详情 | 模块快照 `xxxEnv.get_last_error()`；新增 `std::errno::Errno.get_last()` | `std::fs::FsEnv` |
| 缓冲区不足 | `-1`，**不截断、不返回部分结果** | `list`、`copy`；`num.to_str` 是唯一例外 |

禁止：abort/exit；失败时写全局 `errno` 让调用方自己读；返回「部分成功」。

### 5.4 所有权与 `Drop`

- `char *` 方法层**从不拥有**：输入不改、输出只写调用方缓冲区 + 容量，不分配。
- 自有类型必须 `pub struct X { ... } use Drop;` + 显式 `impl Drop for X`（字段胶水不释放指针字段，
  `ct-lang/docs/language-spec.md:2549`）。
- 句柄统一：`own X *p = open_x(...)`；`close()` 幂等；`Drop` 兜底；`close` 后再调其它方法返回 `-1`。
- 值类型（`FileStat` / `TimeParts` / `Address`）不带 `Drop`，随调用方栈/缓冲存放。
- 内部工厂的返回类型必须写成 `struct X *`（`own` 的判定看字面量，别名可能不认）。

## 6. 文档呈现

### 6.1 模块 README 模板（固定 6 节）

```markdown
# `std::<mod>` — 一句话定位

## 1. 概览与边界          （做什么、不做什么、宿主一览表）
## 2. API 表              （| 方法 | 签名 | 一句语义 | 失败 | 分配？ |）
## 3. 样例                （可运行；对应 tests/<mod>/guide.ct）
## 4. 与相邻模块的关系    （同名冲突、谁产出谁消费）
## 5. 版本与变更记录      （版本 | 状态 | 变更；一行一条）
## 6. 未支持与已知限制
```

API 表每行只写一件事：签名 + 一句语义 + 失败表达；不写长段落。样例必须来自 `tests/`，保证不腐。

### 6.2 顶层索引（三张表，回答「有没有」）

1. **按任务**（新增）：`读文件` → `std::io::File`；`遍历目录` → `std::fs::Dir`；`当前时间` →
   `std::time::Time.now_sec`；`网络` → `std::net`。没有的能力显式写「未支持」。
2. **按宿主**（现有 §4 表）：`char *` / `IoFile *` / `Fd *` / namespace / 标量宿主。
3. **libc → std 映射**（以 [libc 覆盖盘点](../libc-inventory.md) 为源）：每个头文件一节，
   三列「已在 std / 建议包 / 不建议包」。读者查 `strftime` 一行就知道 std 有没有、叫什么。

维护规则：新增 API 一次改三处（§4 表、模块 README、libc 映射表），缺一处视为未完成。

## 7. 落地节奏

每相独立发布；新增接口按既有实践走 minor（`0.1.0` 新增 `std::io` 是先例）。与 issue 0001 的
`0.4.0` 冲突时由负责人排序。

| 相 | 内容 | 依赖 | 完成判据 |
| --- | --- | --- | --- |
| P0 | `String` → `CStrOps`（issue 0001，`0.4.0`） | 无 | 旧名零命中；`tests/run.sh` 全绿；README/文档同步；打 tag |
| P1 | `char` 补 7 个 `is_*`；`str` 补 `rfind_char` / `compare_ignore_case` / `parse_*` | P0（命名口径） | 每个新 API 有 fixture；无 `char *` 重名 |
| P2 | `math` 补全；新增 `env` / `process` / `errno` 三个 namespace 模块 | P0 | 同上；`std::math` 内无重名 |
| P3 | `stdio` 句柄补全（`IoFile *`：`seek`/`tell`/`eof`/`read_line` 等） | P0 | 样例覆盖 EOF、seek、错误位；无分配泄漏 |
| P4 | `Dir` + `Drop`（首个新类型，定下句柄模板） | P3 | `own` 生命周期样例；`close` 幂等；fixture 清理目录 |
| P5 | `Fd` + `FileStat` + `stat`/`chmod`/`read_link` | P4（模板） | fd 关闭与 `Drop` 双路径测试；`FileStat` 字段断言 |
| P6 | `time`：`TimeParts` + `now_*` + 格式化 + `sleep_ms`；`Timer` 可选 | P4 | 格式化输出可断言（固定 epoch）；`Drop` 停表 |
| P7 | `net`：`Socket` + `Address`，先 TCP 阻塞最小闭环，再 UDP/DNS | P4、P5 | 本机 loopback 回环用例；`Drop` 关闭；超时行为写明 |

相与相之间只允许「后相依赖前相的命名/模板」，不允许后相回头改前相 API 名；否则重开一相。

## 8. 风险与未决

### 8.1 已知风险

| 风险 | 说明 | 缓解 |
| --- | --- | --- |
| `own` 的初始化式很窄 | 只认 `malloc`/`calloc`/`struct X *` 工厂；强转与别名不认（实测） | 工厂返回类型写 `struct X *`；文档写死；每相加 `own` 样例 |
| 栈上值不触发 `Drop` | 实测值类型不释放；只有 `own` 指针会 | 句柄必须指针 + `own`；评审检查项 |
| `IoFile` 与新区则不一致 | `close` 只 `fclose`、包装体永不释放（`std/io/README.md:211-215`） | 列入 Q2；新类型一律 `Drop` 释放 |
| `char *` 拥挤 | 25 → 最坏 43 个名字；重名只在调用点报歧义 | 冻结新增；优先新类型/namespace |
| 语言表达力缺口 | 变参、回调、结构体出参都进不来（`printf` 是唯一例外） | 永久排除类列入 §2.8；不硬做 |
| 阻塞模型 | `Socket` 首期只有阻塞调用，无事件循环 | P7 文档写明；非阻塞留后续 |
| 平台 | POSIX-only；Windows 无计划 | 边界写死；跨平台另立项 |
| errno 双轨 | 各模块 per-TU 快照 + 新 `std::errno` 可能并存 | Q4 定口径后再落 P2 |
| 版本节奏 | 每相一个 minor，版本号推高快 | Q5：允许合并相邻小相 |

### 8.2 未决（需负责人裁）

- **Q1**：`std/io/File.m.ct` 是否拆成 `Path` / `File` 两个 unit？（本文建议不拆）
- **Q2**：`IoFile` 包装体不释放是否修正？（修正即破坏性，须新 minor）
- **Q3**：`char *` 是否正式冻结，新能力一律走新类型/namespace？
- **Q4**：错误详情用各模块 `xxxEnv`，还是统一 `std::errno::Errno`，还是两者并存？
- **Q5**：是否允许把 P1/P2、P4/P5 合并发布以减少 minor 次数？
- **Q6**：`net` 是否首期就纳入？（依赖面最大，收益最不确定）
- **Q7**：非 POSIX 平台是否永久排除？

### 8.3 本文可被证伪的假设（供反证席）

1. 「`char *` 方法层是净收益」——若反证指出共享方法名空间的维护成本高于收益，则 P1 需缩减。
2. 「每相一个 minor 可承受」——若发布成本高，则按 Q5 合并。
3. 「`Drop` 能兜住资源」——已实测只对 `own` 指针成立，若反证要求栈值也自动释放，需语言层改动，
   本方案无权承诺。
4. 「先阻塞 socket 后补」——若反证认为半成品网络 API 比没有更糟，P7 应整体后移。
