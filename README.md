# `std` — CT 标准库（ct-pkg 包）

`std` 是**源码库**，不是 `ct` 二进制的一部分：`import std::str::String` 只在显式 `-I` 下解析，
编译器没有默认搜索路径。仓库根就是包根，`std/` 子目录是命名空间根。

## 1. 定位与边界

- 十六个模块：`str` / `io` / `fs` / `utf8` / `num` / `math` / `search` / `arena` / `test` /
  `env` / `process` / `errno` / `locale` / `wchar` / `time` / `net`。
- 两种实现形态，机制不同，不要互相套用：
  - **C 宿主映射**（`.m.ct` + 同名 `.shim.h`）：`str` / `io` / `fs` / `utf8` / `num` / `math` /
    `search` / `env` / `process` / `errno` / `wchar` / `time` 的方法层。接收者是 C 已有类型
    （`char *`、`int`、`double` …），调用点直接展开成 C 调用。
  - **CT 自有类型**（`.h.ct`）：`arena` / `test` 的值类型，以及资源句柄 `io::File` /
    `io::FileDesc`、`fs::Dir`、`locale::LocaleHandle`、`time::Timer`；`net::Socket` 是
    `FileDesc` 的别名。`Arena` / `Test` 走 trait + vptr，资源句柄用固有 `impl` + `use Drop`
    （无 vptr）。
- 容器不在本包：`Vec<T>` / `Map<K,V>` 自 ct 0.30.0 起是编译器的内置层（`lib/std/Vec.h.ct`，
  自动加载，不需要 import、不需要 `-I`）。`0.3.0` 删掉了与内置名冲突的 `std::vec` / `std::map`。
- 不提供：堆字符串类型、Unicode 规范化 / 字素簇、正则。
- 不分配内存的模块（`str` / `fs` / `utf8` / `num` / `math` 的文本化）一律写调用方缓冲区，
  容量不足返回 `-1`。

## 2. 依赖与安装

`ct.pkg`：

```
name = std
publisher = std
ns-root = .
version = 0.11.0
licence = MIT
requires = ct >= 0.64.0
ldflags = -lm
```

`requires` 只告警、不拒装；`ldflags` 里的 `-lm` 给 `std::math` 用，在 libm 已并入 libc 的平台上
是空操作。

`publisher` 与 `ns-root` 属于替换 `-I vendor/<包名>` 的新包机制（需求 SP-002 / SP-003）：
发布方是 `std`，命名空间根就是包根自身（`import std::<mod>::<Unit>` 里的 `std/` 段来自目录树，
不由这个键提供）。**实测 ct v0.53.0**：这两个键会被静默忽略——`read_manifest` 只认
`name` / `version` / `dep` / `requires` / `ldflags`，未知键直接跳过，`ct build` 与 `tests/run.sh`
行为不变、无告警。也就是说现在写上它们是「为将来的工具准备好的声明」，不影响当前构建。

### 安装

```sh
ct get std@^0.2.0      # 解析并写入 ct.lock、vendor/std/
ct verify              # 核对 vendor/ 与 lock 的摘要
ct install             # 打印建议的构建命令
```

`ct install` 会打印 `-I vendor/std ...`，把这一项加进 `ct build` 即可：

```sh
ct build src/main.ct -I vendor/std -o main --cflags "-Wall -Wextra"
```

`vendor/std/std/str/String.m.ct` 这样的双层目录不是笔误：包工具按包打印 `-I vendor/<包名>`，
而命名空间根是 `std/`，两层合起来正好还原 `import std::str::String`。

### 在包根内开发

本仓库根就是包根，直接指 `-I .`：

```sh
ct build -I . src/main.ct -o /tmp/main --cflags "-Wall -Wextra"
```

### 离线 / 钉版本

把 `vendor/` 连同 `ct.lock` 一起提交，CI 里用 `ct install --check`（或 `ct verify`）即可离线构建。
需要某个具体版本时，在 `ct.pkg` 里写 `dep = std@0.2.0` 并提交 `ct.lock`。

### 当前发布状态

`0.11.0` 是当前版本，待打 `v0.11.0`。`0.11.0`（P7）补全 `std::net`：TCP/UDP 工厂、`bind_to` /
`listen_backlog` / `accept`、`send_to` / `receive_from`、`send_message` / `receive_message`、
`resolve_host` / `parse_ip`、超时与 `set_option`；`Socket` 仍是 `FileDesc` 别名。同时补齐 P1 漏项
`std::str` 的 `parse_long` / `parse_long_long` / `parse_unsigned_long` / `parse_real`。至此
`docs/roadmap.md` 的 P0–P7 全部完成。`0.10.0`（P6）收口四个资源对象的 `Drop` 模板（统一经幂等
`close()`），新增 `Timer.close()`，并把 `std::net::Socket` 声明为 `std::io::FileDesc` 的同义
别名。`0.9.0`（P5）新增 `std::time`：`Time` namespace 的时钟、日历、
格式化与 `from_parts`，以及 `Timer`（周期/一次性、`SIGEV_SIGNAL` 或 `setitimer` 回退、
`defer { delete }` 释放）。`0.8.0`（P4）补全 `std::fs`：`access` / `read_link` / `resize` /
`set_mode` / `size` / `mode` / `modification_time` / `is_readable` / `is_writable` / `resolve_path`、
`scan_directory` 流式回调、`Dir` 目录流与 `Stat` 元数据镜像；`read_file` / `write_file` 改名
`read` / `write`。`0.7.0`（P3）把 `std::io` 的资源句柄改成 CT struct：`File`（`FILE *`）
与 `FileDesc`（fd）都带 `use Drop` 与幂等 `close()`；新增 `seek` / `tell` / `rewind_start` /
`at_end_of_file` / `has_error` / `clear_error` / `read_char` / `write_char` / `unread_char` /
`read_line` / `write_line` / `flush`、`adopt_file` / `adopt_file_desc` 与 FileDesc flag 常量；
`own` 只能由自由工厂初始化（方法调用不行，ct 0.65.1 限制），方法形式用 `delete`。
`0.6.0`（P2）新增五个模块：`std::env`（环境变量复制式读写）、
`std::process`（身份、`fork` / `execute*` / `system` / `wait*`、信号、`pipe_open`）、
`std::errno`（`errno` 与 `strerror`）、`std::locale`（`LocaleHandle` + `apply` / `conventions`）、
`std::wchar`（多字节↔宽字符）。`0.5.0`（P1）补全 `char` 的 7 个 ctype 谓词、`char *` 的
`copy_from` / `move_from` / `fill` / `compare_with` / `find_byte` / `compare_prefix` /
`split_into`，以及 `Float` / `Real` 的完整方法族；数学改为只提供方法形态，
`is_inf` 改名 `is_infinite`，新增 `std::search`（`qsort` / `bsearch`）。
`0.4.0`（P0）是破坏性改名与删除批：`String` trait 改为 `CStrOps`，`len` 改为 `length`，
`open` 改为 `open_file`，大小写转换改为 `uppercase` / `lowercase`，删除 `to_string` 与
`std::io` 的变参 `printf`。`0.3.2` 只统一了包身份与许可，`publisher` 改为 `std`，API 无变化。
`v0.1.0` 仍指向旧接口（大小写转换方法原地改写、
`copyFrom` 仍在、`requires` 为 0.20.0），不要拿它配本 README。
索引只收录每个仓库最高的 `v*` tag，不保留历史版本，所以老编译器无法从索引取到与它兼容的旧版 std；
需要旧版时只能用 `file://` 路径安装，或手工 checkout 对应 tag 的 `vendor/std`。

## 3. 快速开始

跨四个模块的完整程序：写文件、读回来、转大写、打印字节数。

```ct
import std::str::String;
import std::fs::Fs;
import std::num::Num;
import std::io::Stdout;

int main(void)
{
    char buf[64];
    char word[16];
    char count[16];
    char line[128];
    char tmp[128];
    char *path = "/tmp/std-overview.txt";
    char *s = "ct-pkg";
    int n;

    path.write(s);
    n = path.read(buf, 64);
    buf.uppercase(word, 16);
    n.to_str(count, 16);

    "file=".concat(buf, line, 128);
    line.concat(" bytes=", tmp, 128);
    tmp.concat(count, line, 128);
    line.concat(" upper=", tmp, 128);
    tmp.concat(word, line, 128);
    std::io::Stdout.puts(line);
    return 0;
}
```

`ct build` 实际执行的 `cc` 行（本机实测，路径随检出位置变化）：

```
'cc' -std=c99 -Wall -Wextra -I '/Users/samxx/vscspace/ct-pkg/std/std/str' -I '/Users/samxx/vscspace/ct-pkg/std/std/fs' -I '/Users/samxx/vscspace/ct-pkg/std/std/num' -I '/Users/samxx/vscspace/ct-pkg/std/std/io' '.ctbuild/main.c' '-o' '/tmp/std-doc-verify/bin/overview'
```

输出：

```text
file=ct-pkg bytes=6 upper=CT-PKG
```

接收者必须写成变量（`path`、`buf`、`n`、`word`）。本包下限是 **ct 0.64.0**（资源对象的 `.h.ct` shim 从 0.64.0 起；`Drop` / `own` 工厂的 0.48 / 0.49 已覆盖）；字面量接收者
`"abc".length()` 从 0.20.0 起可用，低于 0.64.0 的编译器不满足 `ct.pkg` 的 `requires`。

## 4. API 全表（按宿主分组）

### 4.1 模块索引

| 模块 | 入口 import | 宿主 | 方法数 | 文档 |
| --- | --- | --- | --- | --- |
| `std::str` | `std::str::String` | `char *` | 26 | [str/README.md](std/str/README.md) |
| `std::io` | `std::io::File` / `FileDesc` / `Stdout` | namespace、`char *`、`File *`、`FileDesc *` | 6 + 1 + 15 + 8 | [io/README.md](std/io/README.md) |
| `std::fs` | `std::fs::Fs` / `Dir` / `Stat` | `char *`、`Dir *`、`Stat *` | 17 + 7 + 4 | [fs/README.md](std/fs/README.md) |
| `std::utf8` | `std::utf8::Utf8Ops` | `char *` | 3 | [utf8/README.md](std/utf8/README.md) |
| `std::num` | `std::num::Num` | `int` / `long` / `long long` / `char` | 6 × 3 + 14 | [num/README.md](std/num/README.md) |
| `std::math` | `std::math::Real`（`Float` / `Real`） | `float` / `double` | 34 + 38 | [math/README.md](std/math/README.md) |
| `std::search` | `std::search::Search` | `char *` + 元素大小 | 2 | — |
| `std::arena` | `std::arena::Arena` | `Arena *` | 4 | [arena/README.md](std/arena/README.md) |
| `std::test` | `std::test::Test` | `Test *` | 5 | [test/README.md](std/test/README.md) |
| `std::time` | `std::time::Time` / `Timer` | namespace、`int`、`Timer *` | 14 + 2 + 4 | [time/README.md](std/time/README.md) |
| `std::net` | `std::net::Socket` | `FileDesc *`（别名）、`char *` | 17 + 2 + 1 | [net/README.md](std/net/README.md) |

`Vec<T>` / `Map<K,V>` 不在表里，因为它们是编译器内置层，不属于本包（见 §1、§5.4）。

### 4.2 `char *` 宿主（51 个方法，名字全局共享）

同一接收者类型上，**方法名不能重复**：任意两个可见 trait 声明同名方法，调用点就报
`ambiguous type-host map for method 'x' on type 'char *'`。按名字判定，不看参数个数。
给 `char *` 新增 trait 前先查这张表。

| 模块 | 方法 |
| --- | --- |
| `CStrOps` | `length` `find` `find_char` `reverse_find_char` `starts_with` `ends_with` `copy` `slice` `concat` `lowercase` `uppercase` `compare` `compare_ignore_case` `copy_from` `move_from` `fill` `compare_with` `find_byte` `compare_prefix` `split_into` `to_int` `to_double` `parse_long` `parse_long_long` `parse_unsigned_long` `parse_real` |
| `FsOps` | `exists` `read` `write` `list` `mkdir` `remove` `rename` `access` `read_link` `resize` `set_mode` `size` `mode` `modification_time` `is_readable` `is_writable` `resolve_path` |
| `PathDir`（`std::fs`） | `open_dir` |
| `ConnectText`（`std::net`） | `connect` |
| `IpText`（`std::net`） | `parse_ip` |
| `Utf8Ops` | `decode` `count_codepoints` `encode` |
| `Path`（`std::io`） | `open_file` |
| `PathFileDesc`（`std::io`） | `open_as_file_desc` |

合计 26 + 17 + 3 + 1 + 1 + 1 + 1 + 1 = **51**（`split_into` 是便利层，不是 libc 映射）。`errno` 访问器不在 `char *` 上：它是每翻译单元一份的全局值，
挂在 namespace 宿主（`std::fs::FsEnv.get_last_error()`，见 §4.3）。`const char *` 也匹配同一宿主：
类型键丢掉顶层 `const`，但 `CStrOps` 里已没有任何写接收者的方法，字面量接收者不会踩到只读内存。

### 4.3 其它宿主

| 宿主 | trait / 入口 | 方法 |
| --- | --- | --- |
| namespace `std::time` | `Time` | `now_seconds` `now_nanoseconds` `diff_seconds` `sleep_milliseconds` `get_cpu_ticks` `year` `month` `day` `hour` `minute` `second` `day_of_week` `format` `from_parts` |
| `int` | `IntTimer` | `make_timer` `make_timer_once`（`std::time`） |
| namespace `std::io` | `Stdout` | `puts` `putchar` `getchar` `fflush` `write` `write_line` |
| namespace `std::fs` | `FsEnv` | `get_last_error`（最近一次 `std::fs` 调用的 `errno` 快照，成功为 0） |
| `File *` | `impl File`（CT struct + `Drop`） | `read` `write` `close` `seek` `tell` `rewind_start` `at_end_of_file` `has_error` `clear_error` `read_char` `write_char` `unread_char` `read_line` `write_line` `flush` |
| `FileDesc *` | `impl FileDesc`（CT struct + `Drop`） | `read` `write` `seek` `tell` `close` `raw_file_desc` `send` `receive` |
| `Dir *` | `impl Dir`（CT struct + `Drop`） | `next` `rewind` `close` `tell` `seek` `path` `enter` |
| `Stat *` | `impl Stat`（纯值） | `is_regular` `is_directory` `is_symlink` `is_other` |
| `Timer *` | `impl Timer`（CT struct + `Drop`） | `start` `stop` `remaining` `close` |
| `Socket *` | `pub type Socket = FileDesc;` | 与 `FileDesc *` 同一套方法，另加 `bind_to` `bind_to_address` `listen_backlog` `accept` `local_address` `local_port` `shutdown_*` `set_option` `set_reuse_address` `set_*_timeout` `send_to` `receive_from` `send_message` `receive_message` |
| namespace `std::net` | `Net` | `resolve_host` |
| `int` | `Int` | `abs` `clamp` `max` `min` `to_str` `to_base` |
| `long` | `Long` | 同上 6 个 |
| `long long` | `LLong` | 同上 6 个 |
| `char` | `Char` | `is_digit` `is_alpha` `is_space` `is_upper` `is_lower` `is_alnum` `is_punct` `is_print` `is_graph` `is_cntrl` `is_blank` `is_xdigit` `uppercase` `lowercase` |
| namespace `std::search` | `Search` | `sort` `binary_search`（接收 `char *` + 元素大小；比较器是 C 的 `int cmp(const void *, const void *)`） |
| `float` | `Float` | 34 个（`abs` … `is_nan` `is_infinite`，全表见 [math/README.md](std/math/README.md#4-api-全表)） |
| `double` | `Real` | 与 `Float` 相同的 34 个，另有 `scale_by_power_of_two` `split_exponent` `split_fraction` `to_str`（共 38 个） |

`int` / `long` / `long long` 与 `char` 是四个独立宿主，同名方法跨宿主允许（`n.to_str(buf, cap)` 与
`c.is_digit()` 互不冲突）；`char` 不会提升成 `int`。

### 4.4 失败返回

- 缓冲区不够：`-1`，**不截断**（`num.to_str` 是唯一的例外，它按带容量的格式化语义静默截断；
  `num.to_base` 不截断，放不下返回 `NULL`）。
- 查询未命中：`-1`（`fs.exists` 是 `0` / `1`）。
- 路径是目录：`fs.read` 返回 `-1`，不会把目录当空文件给出 0 字节的「成功」。
- 打开失败：`NULL`（`path.open_file`）。
- 出错不 abort，不设 `errno`。

## 5. 教程正文（跨模块）

### 5.1 四种调用形态

| 形态 | 例子 | 说明 |
| --- | --- | --- |
| `char *` 上的值方法 | `s.length()`、`path.exists()`、`buf.encode(cp, cap)` | 最常用；`encode` 的接收者是输出缓冲区 |
| 命名空间函数 | `std::io::Stdout.puts(s)`、`std::search::Search.sort(base, n, size, cmp_int)` | 必须带全命名空间 |
| 资源句柄（CT struct + `Drop`） | `File *f = path.open_file("w")`，再 `f->write(...)` / `f->close()`；或 `own File *f = std::io::open_file(...)` | 指针 + `use Drop`；`close()` 幂等，`Drop` 释放，方法用 `->` |
| CT 自有类型 | `ap->alloc(8)`、`tp->report()` | 先声明值再取地址；裸 `malloc` 的 vptr 是垃圾 |

### 5.2 两种实现，两个代价

**映射式**（`.m.ct` + `.shim.h`）：shim 里全是 `static inline`，实现按消费方复制一份
（断点要下在消费者里），`std_*` 不再是外部符号，跨语言 FFI 需要自己再导出一层。
好处是零开销：`s.length()` 在调用点就是 `std_str_len(s)`，没有 wrapper、没有 vtable、布局不变。

**原生 OO**（`.h.ct`）：`use Trait` 会给结构体尾部追加 vptr（`use Drop` 是例外，只置位、不加
vptr），本机实测 `sizeof(Arena) == 32`、`sizeof(Test) == 16`、`sizeof(File) == 8`；vptr 出现在导出头里。方法永远不能被内联，
这是机制属性。堆对象不能裸 `malloc` 后直接调方法（vptr 是垃圾，实测段错误），先声明值或赋复合字面量。

### 5.3 字节与码点

`CStrOps.length()` 数字节，`Utf8Ops.count_codepoints()` 数码点。同一段 `"é"` 是 `length() == 2`、
`count_codepoints() == 1`。两个名字不能互换。
需要按字符遍历时用 `decode` 拿到步长，累加字节偏移。

### 5.4 容器与测试

容器是**编译器内置**的，不在本包、不需要 import：`Vec<T>`、`Map<K,V>`、`MapEntry<K,V>` 由
`lib/std/Vec.h.ct` 提供（ct 0.30.0 起；编译模式预解析，`ct build` 自动加载）。用法：

```ct
import std::num::Num;
import std::io::Stdout;

int main(void)
{
    Vec<int> v;
    Map<char *, int> m;
    int *vslot;
    int *slot;
    int value;
    char buf[16];

    v.h = 0;                       /* 局部结构体不自动清零 */
    v.push(4);
    v.set(0, 5);
    vslot = v.at(0);
    value = *vslot;
    value.to_str(buf, 16);
    std::io::Stdout.puts(buf);
    v.release();

    m.h = 0;
    m.put("one", 1);
    slot = m.get("one");
    if (slot) {
        value = *slot;
        value.to_str(buf, 16);
        std::io::Stdout.puts(buf);
    }
    m.release();
    return 0;
}
```

注意两点：内置层要求编译器能找到 `lib/std/Vec.h.ct`（`CT_STD_DIR`、当前目录下的 `lib/std/`，
或输入文件旁的 `../lib/std/`），找不到时报的是 `use of an undeclared variable` 而不是
「内置层缺失」（ct-lang SH-76）；局部声明的 `Vec`/`Map` 不自动清零，先写 `v.h = 0`。

`arena` / `test` 仍是本包的原生 OO：先声明值、取地址，再调方法，`Arena` 用 `reset()` /
`free_all()`，`Test` 无资源。资源句柄 `File` / `FileDesc` / `Dir` / `Timer` 是 CT struct +
`use Drop` + 幂等 `close()`，拿到句柄后写 `defer { delete h; }`；`own` 是可选糖（ct 0.65.1
在方法返回值被捕获时会丢掉释放，见[裁定](docs/design/decision-resource-ownership.md)）。
有界实现溢出返回 `-1`。详见各自 README。

## 6. 常见配方

| 任务 | 写法 | 模块 |
| --- | --- | --- |
| 字符串长度 | `s.length()` | `std::str` |
| 子串 | `s.slice(start, end, buf, cap)`（半开区间） | `std::str` |
| 大小写转换 | `s.lowercase(out, cap)` / `s.uppercase(out, cap)`，返回 0 / -1 | `std::str` |
| 字符串转数字 | `s.to_int()` / `s.to_double()` | `std::str` |
| 数字转文本 | `n.to_str(buf, cap)` | `std::num` |
| 进制转换 | `n.to_base(16, buf, cap)` | `std::num` |
| 读整个文件 | `path.read(buf, cap)` | `std::fs` |
| 流式读写 | `path.open_file("r")` → `f.read(...)` → `f.close()` | `std::io` |
| 遍历码点 | `p.decode(&cp)` 返回步长 | `std::utf8` |
| 编码码点 | `buf.encode(cp, cap)`，自己补 NUL | `std::utf8` |
| 取整 | `x.floor()` / `x.ceil()` / `x.round()` | `std::math` |
| 可往返浮点文本 | `x.to_str(buf, cap)` | `std::math` |

## 7. 排错

### 7.1 字面量接收者

```ct
import std::str::String;

int main(void)
{
    int n = "abc".length();
    return n;
}
```

```text
main.ct:5:1: error: method receiver must be a variable, an element, a call or a field chain
```

这个错误来自低于 0.20.0 的编译器（v0.19.1 只接受变量、元素、调用、字段链作接收者）；本包下限
是 **ct 0.64.0**。先写 `char *s = "abc";` 再 `s.length()`。调用结果可以直接继续当接收者：
`s.length().to_str(buf, cap)` 合法（`s.length()` 是调用）。

### 7.2 忘了 import

```ct
int main(void)
{
    char *s = "abc";
    return s.length();
}
```

```text
main.ct:4:1: error: no method 'length' for type 'char *'
```

方法随 trait 一起可见；没有 import，`char *` 上就什么都没有。补 `import std::str::String;`。

### 7.3 同名方法冲突

两个可见 trait 在同一宿主上声明同名方法时，调用点报
`ambiguous type-host map for method 'x' on type 'char *'`，并列出两个 `.m.ct`。
只 import 不调用不报错。按 4.2 节的表查名字，改掉新方法的名字。

### 7.4 链接失败

`std::math` 调用 `sqrt` / `fabs` / `isnan` / `isinf`。libm 单独提供的平台上需要 `-lm`：
`ct build ... --ldflags "-lm"`，或用 `ct install` 打印的命令（`ct.pkg` 的 `ldflags` 已带）。

## 8. 术语表（Glossary）

按本包的实际用法收录，不追求通用编程词典。术语列保留源码里的原名；「本包中的所指」只写
本包能对上的类型、方法或文件。

| 术语 | 含义 | 本包中的所指 |
| --- | --- | --- |
| `std` | 包名，也是 `import` 的命名空间前缀 | 仓库根是包根，`std/` 子目录是命名空间根；`import std::str::String` 对应 `std/str/String.m.ct` |
| `.m.ct` | 映射式单元：只接受 `#include` / `#define` 作前导，没有函数体，方法经 `map trait` 转发 | `str/String.m.ct`、`fs/Fs.m.ct`、`utf8/Utf8Ops.m.ct`、`io/Stdout.m.ct`、`io/Path.m.ct`、`io/FileDescFactory.m.ct`、`math/Real.m.ct`、`num/Num.m.ct`、`env/Env.m.ct` |
| `.h.ct` | 可写 `pub struct` / `impl` / 自由函数与函数体的单元；资源句柄用它 | `arena/Arena.h.ct`、`test/Test.h.ct`、`io/File.h.ct`、`io/FileDesc.h.ct`、`locale/LocaleHandle.h.ct` |
| `host`（宿主） | 方法挂载的接收者类型；同一宿主上方法名必须唯一 | `char *`、`int` / `long` / `long long` / `char`、`float` / `double`、`File *`、`FileDesc *`、`Arena *`、`Test *`，以及 namespace `std::io` / `std::search` / `std::process` |
| `map trait` | `.m.ct` 专有声明，把 trait 方法逐条映射成 C 调用，调用点展开，没有 vtable | `String.m.ct` 的 `map trait CStrOps for type char *`、`Path.m.ct` 的 `map trait Path for type char *`、`Stdout.m.ct` 的 `map trait Stdout for namespace std::io` |
| `pub trait` | 方法集声明；映射式里配 `map trait`，原生 OO 里配 `impl` 或固有 `impl X` | `CStrOps`、`FsOps`、`Utf8Ops`、`Path`、`PathFileDesc`、`Stdout`、`Int` / `Long` / `LLong` / `Char`、`Float` / `Real`、`Search`、`ArenaOps`、`TestOps` |
| `impl` | 为已声明的 `pub trait` 写方法体的块；`impl X { ... }` 是固有方法（无 vptr） | `Arena.h.ct` 的 `impl ArenaOps for Arena`、`Test.h.ct` 的 `impl TestOps for Test`、`File.h.ct` 的 `impl File` |
| `use` | `struct ... use Trait` 把 vtable 指针追加到结构体尾部；`use Drop` 只置位、不加 vptr | `struct Arena { ... } use ArenaOps`、`struct File { ... } use Drop` |
| `vptr` | 由 `use` 追加的 vtable 指针，方法调用经它间接分派 | 生成代码里的 `__vptr`，以及导出头里的 `__arenaops_vptr`；`sizeof(Arena) == 32`（24 字节字段 + 8 字节 vptr） |
| `shim` | 与 `.m.ct` / `.h.ct` 同名的 `.shim.h`，C 实现在里面且是 `static inline`，每个消费单元各复制一份 | `String.shim.h`、`Fs.shim.h`、`File.shim.h`、`FileDesc.shim.h`、`LocaleHandle.shim.h`、`Process.shim.h` |
| `pub type` | CT 侧的类型别名，只改 CT 名字，不改任何 C 符号 | `FileDescFactory.m.ct` 的 `pub type FileDescHandle = struct std_io_FileDesc;`：映射单元用它命名 `.h.ct` 生成的 C tag |
| `File` / `FileDesc` | 包内两个资源句柄：`File` 包 `FILE *`，`FileDesc` 包 fd；都是 CT struct + `use Drop` | `File *` 用 `->read` / `->write` / `->close`；`FileDesc *` 用 `->read` / `->send` / `->raw_file_desc` |
| `handle`（句柄）与裸指针 | 句柄是包装私有 C 状态的 CT 指针，只能经方法使用；裸指针没有附加语义 | `File *` / `FileDesc *` 是句柄，`->close()` 关闭内部资源；`char *` / `void *` 是裸指针，`f->close()` 不是 `free(f)` |
| `char *` | 全仓共享的映射宿主，任何模块都能往上挂 trait，代价是方法名全局唯一 | `CStrOps`、`FsOps`、`Utf8Ops` 和 `Path` 都映射到它上面；新增方法前先查 §4.2 |
| `type key`（类型键） | 编译器给宿主去重的键，丢弃顶层 `const` | `const char *` 与 `char *` 是同一宿主；字面量能直接作接收者，因为 `CStrOps` 没有写接收者的方法 |
| `borrow`（借用） | 只保存指针，不复制，也不负责释放 | 本包的 `str` / `fs` 只写调用方缓冲区、不保存指针；内建 `Vec` / `Map`（不属于本包）的元素与键是借用，`release` 不回收它们 |
| `ownership`（所有权） | 资源由谁负责释放 | 资源句柄拿到后写 `defer { delete h; }`（覆盖落出 / return / break / continue）；`own` 是可选糖，0.65.1 在方法返回值被捕获时会丢掉释放（裁定见 [decision-resource-ownership.md](docs/design/decision-resource-ownership.md)）。`Arena.free_all()` 与内建容器的 `release` 仍由调用方负责 |
| `cap` | 容量；单位随宿主而变 | `CStrOps` / `FsOps` 与 `num` / `math` 的文本化方法是缓冲区字节数（含结尾 NUL 的位置）；`Utf8Ops.encode` 的 `cap` 只是可用字节数，不写 NUL；`Arena` 的 `init` 参数是首块字节数 |
| `length()` | 当前长度；单位随宿主而变 | `CStrOps.length()` 数字节（不含 NUL）；内建 `Vec` / `Map` 的长度查询数元素 / 条目 |
| `n` | 结构体字段或方法参数里的计数值 | `read` / `write` / `copy_to` / `alloc` 的 `n` 参数是字节数 |
| `arena` | bump 分配器及其生命周期 | `std::arena` 的 `Arena`：`init(block)` 拿第一块（`block < 16` 按 16），`alloc(n)` 返回 8 字节对齐且不初始化的内存，`reset()` 释放副块并保留首块，`free_all()` 全部释放 |
| `slice` | 取子串；结果是写进调用方缓冲区的状态码，不是视图 | `CStrOps.slice(start, end, out, cap)` 按字节取半开区间 `[start, end)`，结果写入 `out` 并补 NUL；越界或放不下返回 `-1` |
| `codepoint`（码点） | UTF-8 里一个字符的编号；一个码点占 1..4 字节，码点数不等于字节数 | `Utf8Ops.count_codepoints()` 数码点，`decode()` 返回本次消耗的 1..4 字节，`encode()` 写 1..4 字节且不补 NUL；解码拒绝代理区（U+D800..U+DFFF）和超过 U+10FFFF 的值 |
| `NUL` | 值为 0x00 的字节，C 字符串的终止符，不是 `NULL` 指针 | `length()` 不计 NUL，`cap` 必须给它留位置；`read` / `write` / `encode` 都不自动补 NUL |
| `iterator`（迭代器） | 遍历容器的游标；本包没有实现 | 本包不提供容器；内建 `Vec` / `Map` 用 `foreach (v)` 的 `item` / `i` 绑定遍历，没有迭代器对象。最接近的失效规则是 `Arena.alloc()` 的指针在 `reset()` / `free_all()` 后作废 |
| `literal receiver`（字面量接收者） | 直接拿字面量作方法接收者 | `"abc".length()`；0.20.0 起可用，本包下限 0.64.0。v0.19.1 报 `method receiver must be a variable, an element, a call or a field chain` |
| `chaining`（链式调用） | 把一个方法调用的结果继续当接收者 | `s.length().to_str(buf, cap)` 合法，因为 `s.length()` 是调用而不是字面量 |
| `Drop` | 语言内建的析构钩子 | 包单元里的 `Drop` 发射为 `static`，消费者不可见，所以本包只提供显式 `free()` / `reset()` / `free_all()` / `clear()` |

## 9. 限制与未支持

- **索引只收录最新 tag**：没有历史版本，老编译器取不到匹配的旧版 std。
- **`requires` 只告警**：装得上不代表文档示例能编译。
- **编译器下限 0.64.0**：`.h.ct` 单元自己的 `<base>.shim.h`（自动进公开头与 `-I`）从 0.64.0
  起（ct-lang PH-65），资源对象全部依赖它；跨单元 `Drop` 需要 0.48.0、`own` 工厂初始化需要
  0.49.0，都在 0.64.0 之下。本包只在 0.65.1 上完整验证；0.49.0 实测在 `.h.ct` shim 上失败，
  所以下限不能写 0.49。历史下限：`pub type IoFile` 别名 0.20.3、字面量接收者 0.20.0、内置容器
  0.30.0。低于 0.64.0 的编译器不满足 `requires`（只告警，不代表能编过）。
- **`char *` 方法名全局共享**：新增 trait 必须避开 51 个已有名字（§4.2）。
- **`const` 不参与类型键**：字面量能直接当 `CStrOps` 接收者，因为该 trait 没有写接收者的方法。
- **`num.to_str` 静默截断**：容量不够得到短文本，不报错。
- **容器不在这里**：本包的自有类型是 `Arena` / `Test` 与资源句柄 `File` / `FileDesc` / `Dir` / `LocaleHandle` / `Timer`（`Socket` 为别名）；泛型容器是编译器的内置层，
  需要编译器能找到 `lib/std/Vec.h.ct`（见 §5.4），找不到时报 `use of an undeclared variable`。

## 10. 版本与变更记录

| 版本 | 状态 | 变更 |
| --- | --- | --- |
| `0.11.0` | 当前版本，待打 `v0.11.0` | **P7 net**：TCP/UDP 工厂与 `bind_to` / `listen_backlog` / `accept` / `local_address` / `local_port`；`send_to` / `receive_from` / `send_message` / `receive_message`；`shutdown_*`、`set_option` / `set_reuse_address` / `set_*_timeout`；`resolve_host` / `parse_ip`。`Socket` 保持 `FileDesc` 别名。同时补齐 `std::str` 的 `parse_long` / `parse_long_long` / `parse_unsigned_long` / `parse_real`、`reverse_find_char` 与 `compare_ignore_case`（P1 漏项）。 |
| `0.10.0` | 待打 `v0.10.0` | **P6 资源模板收口**：`File` / `FileDesc` / `Dir` / `Timer` 的 `Drop` 统一调用幂等 `close()`；`Timer` 新增 `close`；`std::net::Socket` 声明为 `FileDesc` 别名（无新类型）。 |
| `0.9.0` | 待打 `v0.9.0` | **P5 time**：`Time` namespace（`now_*` / `diff_seconds` / `sleep_milliseconds` / `get_cpu_ticks` / 日历字段 / `format` / `from_parts`）与 `Timer`（`make_timer` / `make_timer_once` + `start` / `stop` / `remaining`；无回调，到期走信号）。 |
| `0.8.0` | 待打 `v0.8.0` | **P4 fs 补全**：路径方法补 10 个；`read_file`/`write_file` → `read`/`write`；新增 `Dir`（`own` 工厂 + 字符串工厂）与 `Stat` 字段镜像；`scan_directory` 流式回调；`resolve_path` 为词法绝对路径。 |
| `0.7.0` | 待打 `v0.7.0` | **P3 stdio 资源化**：`IoFile` 退役，`File` / `FileDesc` 改为 CT struct + `use Drop`；补全 seek/tell/EOF/错误位/字符与行 IO/flush/adopt；方法调用改 `->`；`own` 只认自由工厂。 |
| `0.6.0` | 待打 `v0.6.0` | **P2 五个新模块**：`std::env` / `std::process` / `std::errno` / `std::locale` / `std::wchar`；`LocaleHandle` 用 `own` + `Drop`，`apply` 明确为进程全局副作用。 |
| `0.5.0` | 待打 `v0.5.0` | **P1 补齐**：`Char` 新增 7 个谓词；`CStrOps` 新增 `copy_from` / `move_from` / `fill` / `compare_with` / `find_byte` / `compare_prefix` / `split_into`（`copy_to` 移除）；`Float` / `Real` 扩到完整 libm 方法族，七个生僻名改全称，`is_inf` → `is_infinite`，数学只保留方法形态；新增 `std::search`。 |
| `0.4.0` | 已打 `v0.4.0` | **破坏性改名与删除**（P0）：`String` → `CStrOps`；`len` → `length`；`open` → `open_file`；`to_upper` / `to_lower` → `uppercase` / `lowercase`；删除 `to_string`（只剩 `to_str(out, cap)`）与 `std::io` 的变参 `printf`（`Io.h.ct` 移除，输出改用 `std::io::Stdout`）。 |
| `0.3.2` | 已打 `v0.3.2` | 包身份与许可统一：`publisher` 改为 `std`，许可改为 MIT（`LICENSE`、README 与各单元头部表述一并更新）；`ns-root` 与 API 无变化。 |
| `0.3.1` | 已打 `v0.3.1` | 文档修正：`0.3.0` 打 tag 后仍写着「未打 tag」，本版订正该状态并同步 `ct.pkg` 版本号；API 无变化（包摘要因文档与版本号变动而更新）。 |
| `0.3.0` | 已打 `v0.3.0` | **三处破坏性变更 + 四个新接口**（见 §10.1）：删除 `std::vec` / `std::map`（`Vec` / `Map` 已是编译器内置类型，重名会让消费者撞 `'Vec' is a built-in type and cannot be redeclared`）；全量改名到 `snake_case`（`char *` 宿主 25 个方法及其他宿主的同名方法）；`fs.read_file` 对目录返回 `-1`；新增 `mkdir` / `remove` / `rename` 与 `FsEnv.get_last_error`；`requires` 抬到 `ct >= 0.30.0`。 |
| `0.2.0` | 已打 `v0.2.0` | **破坏性变更**（见 §10.2）：大小写转换方法改为写 `out + cap`，移除 `copyFrom`，编译器下限提到 0.20.3。 |
| `0.1.0` | 已打 `v0.1.0`（旧接口） | 文档重写：包根 README 改为总览；十个模块 README 全部吸收各自教程并删除教程文件；新建 `math/README.md`。API 未变。 |

### 10.1 `0.3.0` 变更（删除 + 改名 + 一处行为修正 + 四个新接口）

**1) 删除 `std::vec` / `std::map`（BREAKING）**

`Vec<T>` / `Map<K,V>` / `MapEntry<K,V>` 自 ct **0.30.0** 起是编译器的内置层（`lib/std/Vec.h.ct`，
编译模式预解析、`ct build` 自动加载，不需要 import 也不需要 `-I`），这三个拼写是保留名。
本包里的 `std/vec/Vec.h.ct`、`std/map/Map.h.ct` 是同一批容器的旧实现，任何消费者只要同时
加载内置层就会在解析期硬错：

```text
./std/vec/Vec.h.ct:47:1: error: 'Vec' is a built-in type and cannot be redeclared
```

因此 `0.3.0` 删除：

| 删除项 | 内容 |
| --- | --- |
| `std/vec/` | `Vec.h.ct`（`VecInt` / `VecStr`）、README |
| `std/map/` | `Map.h.ct`（`MapStrInt` / `MapIntPtr`）、README |
| `tests/vec/`、`tests/map/` | 各自 `main.ct` / `first.ct` / `guide.ct` / `coverage.ct` 夹具 |
| `tests/all/main.ct` | 去掉两处 import 与容器断言（改为注释指向内置层） |
| `tests/run.sh` | 布局检查的模块列表去掉 `vec` / `map` |

消费者迁移：

| `0.2.0`（已删） | 内置层等价写法 |
| --- | --- |
| `import std::vec::Vec;` + `VecInt v; vp->init();` | 删掉 import；`Vec<int> v; v.h = 0;`（局部声明不自动清零） |
| `vp->push(x)` / `vp->at(i, &x)` / `vp->free()` | `v.push(x)` / `v.at(i)`（返回 `int *`）/ `v.release()`；长度查询用内置层的对应方法 |
| `import std::map::Map;` + `MapStrInt m; mp->init(4);` | 删掉 import；`Map<char *, int> m; m.h = 0;` |
| `mp->put(k, v)` / `mp->get(k, &x)` / `mp->free()` | `m.put(k, v)` / `m.get(k)`（返回 `int *`）/ `m.release()`；长度查询用内置层的对应方法 |
| `MapIntPtr` 的 `free_fn` | 内置层没有逐值回调，`release` 只释放自身缓冲区，值由调用方负责 |

`requires` 同步从 `ct >= 0.20.3` 抬到 **`ct >= 0.30.0`**：内置容器从 0.30.0 才有
（ct-lang Phase 29，「泛型 struct + 内置 `Vec` / `Map`」，随 v0.30.0 合并发布；
`std` 在 0.30.0 之前不存在 `lib/std/`，也没有保留名校验）。

**2) `fs.read_file` 对目录返回 `-1`**

`std::fs` 的 `read_file` 对**目录**以前是静默错值：POSIX 允许以只读方式打开目录，随后第一次读取
就得到 EOF，于是它返回 `0` 并给出空字符串，调用方看不出「这不是文件」。`0.3.0` 起在打开后对
文件描述符做一次 `fstat`，命中目录即返回 `-1`，与「打不开」「放不下」同一约定。

明确**没有**变的部分：

- 普通文件：仍然返回字节数并补 NUL；容量不足（含 NUL）仍返回 `-1`。
- 空文件：仍然返回 `0`，`out[0]` 为 NUL。
- 含 `'\0'` 的文件：仍然按字节读满、返回完整字节数，NUL 之后的字节照样写入缓冲区；截断发生在
  字符串层（`write_file` 按 `strlen` 写、`%s` 打印停在下个 NUL），本轮不动。
- 没有新增错误返回值：目录与其他失败一样只是 `-1`，原因可用 `FsEnv.get_last_error()` 查询。

**3) 全量改名到 `snake_case`（BREAKING）**

CT 语言规范 §5.1「源码命名约定」把函数 / 方法统一到 `snake_case`（见 §11），本包相关方法随之改名，
不保留旧名、不提供别名——旧名与新名同宿主会触发
`ambiguous type-host map for method '<name>' on type 'char *'`，属性上也无法并存。

| 模块 | 旧名 → 新名 |
| --- | --- |
| `std::fs` | `readFile` → `read_file`、`writeFile` → `write_file` |
| `std::str` | `findChar` → `find_char`、`startsWith` → `starts_with`、`endsWith` → `ends_with`、`toLower` → `lowercase`、`toUpper` → `uppercase`、`copyTo` → `copy_to`、`toInt` → `to_int`、`toDouble` → `to_double` |
| `std::utf8` | `codepointCount` → `count_codepoints` |
| `std::num`（`int` / `long` / `long long`） | `toStr` → `to_str`、`toBase` → `to_base`（十进制文本方法进 `to_str`，不再单独存在） |
| `std::num`（`char`） | `isDigit` → `is_digit`、`isAlpha` → `is_alpha`、`isSpace` → `is_space`、`isUpper` → `is_upper`、`isLower` → `is_lower`、`toUpper` → `uppercase`、`toLower` → `lowercase` |
| `std::math`（`float` / `double`） | `isNan` → `is_nan`、`isInf` → `is_infinite`（`0.5.0` 定名）、`toStr` → `to_str`（可往返文本只保留写缓冲区的形态） |
| `std::arena` | `freeAll` → `free_all` |
| `std::test` | `checkTrue` → `check_true`、`checkEqInt` → `check_eq_int`、`checkEqStr` → `check_eq_str` |

`std::io` 的方法名本来就合规：`open_file` / `read` / `write` / `close`，以及 `puts` / `putchar` /
`getchar` / `fflush`（命中 §5.1 的 libc 直映射例外）。

**4) 新增 `mkdir` / `remove` / `rename` / `FsEnv.get_last_error`**

```ct
int mkdir(int mode);              /* 0 / -1；已存在也返回 -1（EEXIST），不静默成功 */
int remove();                     /* remove(3)：文件或空目录；非空目录 ENOTEMPTY，不递归 */
int rename(const char *to);       /* rename(2)：跨文件系统 EXDEV；POSIX 覆盖目标 */

int get_last_error();             /* std::fs::FsEnv.get_last_error()，见 §10.1 的错误约定 */
```

`mode` 必须显式传（`p.mkdir(0755)`）：映射不支持重载，同宿主不能有第二个 `mkdir`。

错误约定：每次 `std::fs` 调用都刷新 `get_last_error()` —— 成功置 `0`，系统调用失败记当次
`errno`，本模块自身的拒绝记 `EINVAL`（参数非法）/ `ERANGE`（缓冲区放不下）/ `EISDIR`（读目录）。
缓存是 shim 里每翻译单元一份的 `static int`，**非线程安全、不跨翻译单元共享**。

### 10.2 `0.2.0` 破坏性变更（BREAKING）

> 本节记录 `0.2.0` 当时的接口。当时的命名是小驼峰，`0.3.0` 起统一改为 `snake_case`
> （见 §10.1 第 3 条）；下文按现行拼写书写，旧名只在迁移对照里以中文描述。

升级到 `0.2.0` 前逐条核对：

1. **大小写转换方法换了签名**。旧写法是原地改写接收者、返回状态码；新写法与 `copy` 同构，
   接收者只读，结果写调用方缓冲区：
   `s.lowercase(out, cap)` / `s.uppercase(out, cap)`（`cap` 含结尾 NUL，放不下返回 `-1`）。
   好处是字面量接收者不再有写只读页的风险。
2. **`char *` 上的 `copyFrom` 已删除**。旧写法 `dst.copyFrom(src, n)` 改成 `src.copy_to(dst, n)`，
   语义完全等价（恰好 `n` 字节、不补 NUL、不查容量）。
3. **编译器下限从 0.19.1 / 0.20.0 提到 0.20.3**。`std::io` 的 `pub type IoFile = struct ct_io_file;`
   需要 0.20.3；字面量接收者需要 0.20.0。`ct.pkg` 的 `requires` 当时写的是 `ct >= 0.20.3`；
   0.3.0 起抬到 `ct >= 0.30.0`（见 §10.1）。
4. **`char *` 上的方法数从 15 降到 14**（少一个 `copyFrom`）。同宿主方法名唯一，新增 trait 前
   仍按 §4.2 查名。

迁移对照：

| `0.1.0` | `0.2.0` |
| --- | --- |
| 原地改写接收者的大小写转换 | `s.lowercase(out, cap)` / `s.uppercase(out, cap)`（`out` 是调用方缓冲区） |
| `dst.copyFrom(src, n)` | `src.copy_to(dst, n)` |

版本策略：

| 项 | 约定 |
| --- | --- |
| 版本号 | 独立 semver，从 `0.1.0` 起；与编译器版本号无关 |
| `0.x` 期间 | 破坏性改动走 minor（`0.1.0` → `0.2.0`），修复走 patch；不承诺向后兼容 |
| `1.0.0` 之后 | 破坏性改动走 major |
| tag | `v<version>`；索引只认 `v*` 且取最高一个 |
| 编译器下限 | `ct >= 0.64.0`，写在 `ct.pkg` 的 `requires` |

打 tag 之后索引里的摘要会覆盖包目录下除 `.git`、`vendor`、`.ctcache` 外的所有文件，
包括 `README.md`、`tests/`、`.github/`。所以打 tag 前必须让所有文件定稿；tag 之后任何文件改动
都需要新版本号和新 tag，否则 `ct verify` 会报摘要不一致。

## 11. 命名约定

函数 / 方法一律 `snake_case` 且以动作动词开头，类型 `PascalCase`，任何标识符位置都不用
小驼峰；直接映射 libc 的同名函数（`puts` / `putchar` / `getchar` / `fflush` / `sqrt` / `fabs` 等）
保留原名。

完整规则与评审判据见 **CT 语言规范 §5.1「源码命名约定」**（`ct-lang/docs/language-spec.md`），
其适用范围覆盖 CT 语言自身暴露的名字、`lib/std` 内置层、本包以及所有 ct-pkg 包。本包不再另立
一套规则：本节只记录执行口径，规范修订时以 §5.1 为准。

本包当前口径（`0.3.0` 起）：`char *` 宿主 25 个方法、其余宿主全部符合 §5.1 判据 1
（`^[a-z][a-z0-9_]*$`，首词为动词或 `get_` / `set_` / `has_` / `is_` / `to_` / `from_` 前缀）。

保留名豁免（**不是合规范例**，按规范 §5.1 的保留名条款处理，不随本条改名）：

- `std::str` 的 `length()` —— 字符串长度查询的惯用名，首词不是动词，按保留名处理；
- `std::num` 的 `max()` / `min()` —— `0.2.0` 起的存量名，数学惯用词，首词不是动词，
  但已列入保留清单（CT 侧同步补进 §5.1）。

除以上保留名与 libc 直映射外，本包没有任何首词非动词的方法。

## License

MIT（见 `LICENSE`）。

来源与授权确认：`std/` 源码源自 `ct-lang`（GPLv3）；经负责人确认，本包整体改为 MIT 发布。
