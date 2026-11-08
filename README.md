# `std` — CT 标准库（ct-pkg 包）

`std` 是**源码库**，不是 `ct` 二进制的一部分：`import std::str::String` 只在显式 `-I` 下解析，
编译器没有默认搜索路径。仓库根就是包根，`std/` 子目录是命名空间根。

## 1. 定位与边界

- 十个模块：`str` / `io` / `fs` / `utf8` / `num` / `math` / `vec` / `map` / `arena` / `test`。
- 两种实现形态，机制不同，不要互相套用：
  - **C 宿主映射**（`.m.ct` + 同名 `.shim.h`）：`str` / `io` / `fs` / `utf8` / `num` / `math`。
    接收者是 C 已有类型（`char *`、`IoFile *`、`int`、`double` …），调用点直接展开成 C 调用。
  - **CT 自有类型**（`.h.ct`）：`vec` / `map` / `arena` / `test`。`pub trait` + `struct ... use Trait`
    + `impl` 写在同一文件里，分派走 vptr。
- 不提供：堆字符串类型、泛型容器、Unicode 规范化 / 字素簇、网络、时间、进程、正则。
- 不分配内存的模块（`str` / `fs` / `utf8` / `num` / `math` 的文本化）一律写调用方缓冲区，
  容量不足返回 `-1`；唯一使用静态环形槽的是 `num.toString` / `math.toString`，非线程安全。

## 2. 依赖与安装

`ct.pkg`：

```
name = std
version = 0.1.0
requires = ct >= 0.20.3
ldflags = -lm
```

`requires` 只告警、不拒装；`ldflags` 里的 `-lm` 给 `std::math` 用，在 libm 已并入 libc 的平台上
是空操作。

### 安装

```sh
ct get std@^0.1.0      # 解析并写入 ct.lock、vendor/std/
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
需要某个具体版本时，在 `ct.pkg` 里写 `dep = std@0.1.0` 并提交 `ct.lock`。

### 当前发布状态

`0.1.0` **尚未打 tag、尚未发布**。在 `v0.1.0` 打出来之前本包不可通过索引安装。
索引只收录每个仓库最高的 `v*` tag，不保留历史版本，所以老编译器无法从索引取到与它兼容的旧版 std；
需要旧版时只能用 `file://` 路径安装，或手工 checkout 对应 tag 的 `vendor/std`。

## 3. 快速开始

跨四个模块的完整程序：写文件、读回来、转大写、打印字节数。

```ct
import std::str::String;
import std::fs::Fs;
import std::num::Num;
import std::io::Io;

int main(void)
{
    char buf[64];
    char word[16];
    char *path = "/tmp/std-overview.txt";
    char *s = "ct-pkg";
    int n;

    path.writeFile(s);
    n = path.readFile(buf, 64);
    buf.toUpper(word, 16);
    std::io::printf("file=%s bytes=%s upper=%s\n", buf, n.toString(), word);
    return 0;
}
```

`ct build` 实际执行的 `cc` 行（本机实测，路径随检出位置变化）：

```
'cc' -std=c99 -Wall -Wextra -I '/Users/samxx/vscspace/ct-pkg/std/std/str' -I '/Users/samxx/vscspace/ct-pkg/std/std/fs' -I '/Users/samxx/vscspace/ct-pkg/std/std/num' '.ctbuild/main.c' '.ctbuild/Io.c' '-o' '/tmp/std-doc-verify/bin/overview'
```

输出：

```text
file=ct-pkg bytes=6 upper=CT-PKG
```

接收者必须写成变量（`path`、`buf`、`n`、`word`）。字面量接收者 `"abc".len()` 在 v0.19.1 报
`method receiver must be a variable, an element, a call or a field chain`，需要 **0.20.0**。

## 4. API 全表（按宿主分组）

### 4.1 模块索引

| 模块 | 入口 import | 宿主 | 方法数 | 文档 |
| --- | --- | --- | --- | --- |
| `std::str` | `std::str::String` | `char *` | 15 | [str/README.md](std/str/README.md) |
| `std::io` | `std::io::Io` / `Stdout` / `File` | namespace、`char *`、`IoFile *` | 1 + 4 + 1 + 3 | [io/README.md](std/io/README.md) |
| `std::fs` | `std::fs::Fs` | `char *` | 4 | [fs/README.md](std/fs/README.md) |
| `std::utf8` | `std::utf8::Utf8Ops` | `char *` | 3 | [utf8/README.md](std/utf8/README.md) |
| `std::num` | `std::num::Num` | `int` / `long` / `long long` / `char` | 7 × 4 | [num/README.md](std/num/README.md) |
| `std::math` | `std::math::Math` / `Real` | namespace、`float`、`double` | 2 + 5 + 9 | [math/README.md](std/math/README.md) |
| `std::vec` | `std::vec::Vec` | `VecInt *` / `VecStr *` | 6 × 2 | [vec/README.md](std/vec/README.md) |
| `std::map` | `std::map::Map` | `MapStrInt *` / `MapIntPtr *` | 6 × 2 | [map/README.md](std/map/README.md) |
| `std::arena` | `std::arena::Arena` | `Arena *` | 4 | [arena/README.md](std/arena/README.md) |
| `std::test` | `std::test::Test` | `Test *` | 5 | [test/README.md](std/test/README.md) |

### 4.2 `char *` 宿主（23 个方法，名字全局共享）

同一接收者类型上，**方法名不能重复**：任意两个可见 trait 声明同名方法，调用点就报
`ambiguous type-host map for method 'x' on type 'char *'`。按名字判定，不看参数个数。
给 `char *` 新增 trait 前先查这张表。

| 模块 | 方法 |
| --- | --- |
| `String` | `len` `find` `findChar` `startsWith` `endsWith` `copy` `slice` `concat` `toLower` `toUpper` `compare` `copyTo` `toInt` `toDouble` |
| `FsOps` | `exists` `readFile` `writeFile` `list` |
| `Utf8Ops` | `decode` `codepointCount` `encode` |
| `Path`（`std::io`） | `open` |

合计 14 + 4 + 3 + 1 = **22**。`const char *` 也匹配同一宿主：类型键丢掉顶层 `const`，
但 `String` 里已没有任何写接收者的方法，字面量接收者不会踩到只读内存。

### 4.3 其它宿主

| 宿主 | trait / 入口 | 方法 |
| --- | --- | --- |
| namespace `std::io` | `Stdout` | `puts` `putchar` `getchar` `fflush` |
| namespace `std::io` | `Io` | `printf(fmt, ...)`（变参，`va_list` + `vprintf`） |
| `IoFile *` | `File` | `read` `write` `close`（`pub type IoFile = struct ct_io_file;`，C tag 私有） |
| `int` | `Int` | `abs` `clamp` `max` `min` `toStr` `toString` `toBase` |
| `long` | `Long` | 同上 7 个 |
| `long long` | `LLong` | 同上 7 个 |
| `char` | `Char` | `isDigit` `isAlpha` `isSpace` `isUpper` `isLower` `toUpper` `toLower` |
| namespace `std::math` | `Math` | `sqrt` `fabs` |
| `float` | `Float` | `abs` `sqrt` `floor` `ceil` `round` |
| `double` | `Real` | 同上 5 个，另有 `isNan` `isInf` `toStr` `toString` |

`int` / `long` / `long long` 与 `char` 是四个独立宿主，同名方法跨宿主允许（`n.toString()` 与
`c.isDigit()` 互不冲突）；`char` 不会提升成 `int`。

### 4.4 失败返回

- 缓冲区不够：`-1`，**不截断**（`num.toStr` 是唯一的例外，它按 `snprintf` 语义静默截断；
  `num.toBase` 不截断，放不下返回 `NULL`）。
- 查询未命中：`-1`（`fs.exists` 是 `0` / `1`）。
- 打开失败：`NULL`（`path.open`）。
- 出错不 abort，不设 `errno`。

## 5. 教程正文（跨模块）

### 5.1 四种调用形态

| 形态 | 例子 | 说明 |
| --- | --- | --- |
| `char *` 上的值方法 | `s.len()`、`path.exists()`、`buf.encode(cp, cap)` | 最常用；`encode` 的接收者是输出缓冲区 |
| 命名空间函数 | `std::io::printf(...)`、`std::math::Math.sqrt(x)` | 必须带全命名空间 |
| C 不透明句柄 | `path.open("w")` 返回 `IoFile *`，再 `f.read(...)` / `f.close()` | 句柄包装私有 `FILE *`；`close()` 可重复调用，但不回收包装体 |
| CT 自有类型 | `vp->push(1)`、`mp->put("k", 1)`、`ap->alloc(8)`、`tp->report()` | 先声明值再取地址；裸 `malloc` 的 vptr 是垃圾 |

### 5.2 两种实现，两个代价

**映射式**（`.m.ct` + `.shim.h`）：shim 里全是 `static inline`，实现按消费方复制一份
（断点要下在消费者里），`std_*` 不再是外部符号，跨语言 FFI 需要自己再导出一层。
好处是零开销：`s.len()` 在调用点就是 `std_str_len(s)`，没有 wrapper、没有 vtable、布局不变。

**原生 OO**（`.h.ct`）：`use` 会给结构体尾部追加 vptr，实测 `VecInt` / `VecStr` 16 → 24、
`MapStrInt` 32 → 40、`MapIntPtr` 40 → 48；vptr 出现在导出头里。方法永远不能被内联，
这是机制属性。堆对象不能裸 `malloc` 后直接调方法（vptr 是垃圾，实测段错误），先声明值或赋复合字面量。

### 5.3 字节与码点

`String.len()` 数字节，`Utf8Ops.codepointCount()` 数码点。同一段 `"é"` 是 `len() == 2`、
`codepointCount() == 1`。两个名字不能互换，也不要再引入叫 `length` 的第三个。
需要按字符遍历时用 `decode` 拿到步长，累加字节偏移。

### 5.4 容器与测试

`vec` / `map` / `arena` / `test` 是原生 OO：先声明值、取地址，再调方法；每个持有资源的类型都有
显式 `free()` / `reset()` / `freeAll()` / `clear()`，没有析构。容器都是有界实现，溢出返回 `-1`。
`VecStr` 的字符串是借用指针，容器不释放元素；`MapIntPtr` 在 `init` 时接收 `free_fn`。
详见各自 README。

## 6. 常见配方

| 任务 | 写法 | 模块 |
| --- | --- | --- |
| 字符串长度 | `s.len()` | `std::str` |
| 子串 | `s.slice(start, end, buf, cap)`（半开区间） | `std::str` |
| 大小写转换 | `s.toLower(out, cap)` / `s.toUpper(out, cap)`，返回 0 / -1 | `std::str` |
| 字符串转数字 | `s.toInt()` / `s.toDouble()` | `std::str` |
| 数字转文本 | `n.toString()` 或 `n.toStr(buf, cap)` | `std::num` |
| 进制转换 | `n.toBase(16, buf, cap)` | `std::num` |
| 读整个文件 | `path.readFile(buf, cap)` | `std::fs` |
| 流式读写 | `path.open("r")` → `f.read(...)` → `f.close()` | `std::io` |
| 遍历码点 | `p.decode(&cp)` 返回步长 | `std::utf8` |
| 编码码点 | `buf.encode(cp, cap)`，自己补 NUL | `std::utf8` |
| 取整 | `x.floor()` / `x.ceil()` / `x.round()` | `std::math` |
| 可往返浮点文本 | `x.toString()` | `std::math` |

## 7. 排错

### 7.1 字面量接收者

```ct
import std::str::String;

int main(void)
{
    int n = "abc".len();
    return n;
}
```

```text
main.ct:5:1: error: method receiver must be a variable, an element, a call or a field chain
```

v0.19.1 只接受变量、元素、调用、字段链作接收者。先写 `char *s = "abc";` 再 `s.len()`。
调用结果可以直接链式：`s.len().toString()` 合法（`s.len()` 是调用）。字面量接收者需要 **0.20.0**。

### 7.2 忘了 import

```ct
int main(void)
{
    char *s = "abc";
    return s.len();
}
```

```text
main.ct:4:1: error: no method 'len' for type 'char *'
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
| `.m.ct` | 映射式单元：只接受 `#include` / `#define` 作前导，没有函数体，方法经 `map trait` 转发 | `str/String.m.ct`、`fs/Fs.m.ct`、`utf8/Utf8Ops.m.ct`、`io/Stdout.m.ct`、`io/File.m.ct`、`math/Math.m.ct`、`math/Real.m.ct`、`num/Num.m.ct` |
| `.h.ct` | 可写 `pub trait` / `pub struct` / `impl` 与函数体的单元；本包的原生 OO 都走它 | `vec/Vec.h.ct`、`map/Map.h.ct`、`arena/Arena.h.ct`、`test/Test.h.ct`；`io/Io.h.ct` 只放 `printf` 与标记用的 `Io` struct |
| `host`（宿主） | 方法挂载的接收者类型；同一宿主上方法名必须唯一 | `char *`、`IoFile *`、`int` / `long` / `long long` / `char`、`float` / `double`、`VecInt *` / `VecStr *`、`MapStrInt *` / `MapIntPtr *`、`Arena *`、`Test *`，以及 namespace `std::io` / `std::math` |
| `map trait` | `.m.ct` 专有声明，把 trait 方法逐条映射成 C 调用，调用点展开，没有 vtable | `String.m.ct` 的 `map trait String for type char *`、`File.m.ct` 的 `map trait File for type IoFile *`、`Stdout.m.ct` 的 `map trait Stdout for namespace std::io` |
| `pub trait` | 方法集声明；映射式里配 `map trait`，原生 OO 里配 `impl` | `String`、`FsOps`、`Utf8Ops`、`Path`、`File`、`Stdout`、`Int` / `Long` / `LLong` / `Char`、`Math` / `Float` / `Real`、`VecIntOps` / `VecStrOps`、`MapStrIntOps` / `MapIntPtrOps`、`ArenaOps`、`TestOps` |
| `impl` | 为已声明的 `pub trait` 写方法体的块 | `Vec.h.ct` 的 `impl VecIntOps for VecInt`、`Map.h.ct` 的 `impl MapStrIntOps for MapStrInt`、`Arena.h.ct` 的 `impl ArenaOps for Arena` |
| `use` | `struct ... use Trait` 把 vtable 指针追加到结构体尾部，并绑定该 trait | `struct Arena { ... } use ArenaOps`、`struct Test { ... } use TestOps` |
| `vptr` | 由 `use` 追加的 vtable 指针，方法调用经它间接分派 | 生成代码里的 `__vptr`，以及导出头里的 `__arenaops_vptr`；`sizeof(Arena) == 32`（24 字节字段 + 8 字节 vptr） |
| `shim` | 与 `.m.ct` 同名的 `.shim.h`，C 实现在里面且是 `static inline`，每个消费单元各复制一份 | `String.shim.h`、`Fs.shim.h`、`Utf8Ops.shim.h`、`Stdout.shim.h`、`File.shim.h`、`Num.shim.h`、`Real.shim.h` |
| `pub type` | CT 侧的类型别名，只改 CT 名字，不改任何 C 符号 | `File.m.ct` 的 `pub type IoFile = struct ct_io_file;`；C tag 仍是 `struct ct_io_file`，helper 仍是 `std_io_*` |
| `IoFile` | CT 侧唯一可见的不透明文件句柄类型，包装私有的 `struct ct_io_file`（内含 `FILE *`） | `Path.open` 返回 `IoFile *`；`File.read` / `File.write` / `File.close` 挂在 `IoFile *` 上 |
| `handle`（句柄）与裸指针 | 句柄是包装私有 C 状态的不透明指针，只能经方法使用；裸指针没有附加语义 | `IoFile *` 是句柄，`close()` 关闭内部 `FILE *`；`char *` / `void *` 是裸指针，`f.close()` 不是 `free(f)` |
| `char *` | 全仓共享的映射宿主，任何模块都能往上挂 trait，代价是方法名全局唯一 | `String`、`FsOps`、`Utf8Ops` 和 `Path` 都映射到它上面；新增方法前先查 §4.2 |
| `type key`（类型键） | 编译器给宿主去重的键，丢弃顶层 `const` | `const char *` 与 `char *` 是同一宿主；字面量能直接作接收者，因为 `String` 没有写接收者的方法 |
| `borrow`（借用） | 只保存指针，不复制，也不负责释放 | `VecStr` 的元素和 `MapStrInt` 的键都是借用；`free()` / `clear()` 不回收它们 |
| `ownership`（所有权） | 资源由谁负责释放 | 本包不提供析构；`Vec.free()` 只释放数组，`MapIntPtr` 的值交给 `free_fn`，`Arena.freeAll()` 释放全部块，`IoFile` 的包装体由 `open` 分配而 `close` 不回收 |
| `free_fn` | `MapIntPtr` 的逐值释放回调 | `MapIntPtrOps.init(want, free_fn)` 记住它；`clear()` / `free()` 对每个值调用一次 |
| `cap` | 容量；单位随宿主而变 | `String` / `FsOps` 与 `num` / `math` 的文本化方法是缓冲区字节数（含结尾 NUL 的位置）；`Utf8Ops.encode` 的 `cap` 只是可用字节数，不写 NUL；`Vec` 是元素个数；`Map` 是槽位数；`Arena` 是首块字节数 |
| `len()` | 当前长度；单位随宿主而变 | `String.len()` 数字节（不含 NUL），`Vec.len()` 数元素，`Map.len()` 数条目 |
| `n` | 结构体字段或方法参数里的计数值 | `Vec` / `Map` 的 `n` 字段是元素 / 条目数；`read` / `write` / `copyTo` / `alloc` 的 `n` 参数是字节数 |
| `arena` | bump 分配器及其生命周期 | `std::arena` 的 `Arena`：`init(block)` 拿第一块（`block < 16` 按 16），`alloc(n)` 返回 8 字节对齐且不初始化的内存，`reset()` 释放副块并保留首块，`freeAll()` 全部释放 |
| `slice` | 取子串；结果是写进调用方缓冲区的状态码，不是视图 | `String.slice(start, end, out, cap)` 按字节取半开区间 `[start, end)`，结果写入 `out` 并补 NUL；越界或放不下返回 `-1` |
| `codepoint`（码点） | UTF-8 里一个字符的编号；一个码点占 1..4 字节，码点数不等于字节数 | `Utf8Ops.codepointCount()` 数码点，`decode()` 返回本次消耗的 1..4 字节，`encode()` 写 1..4 字节且不补 NUL；解码拒绝代理区（U+D800..U+DFFF）和超过 U+10FFFF 的值 |
| `NUL` | 值为 0x00 的字节，C 字符串的终止符，不是 `NULL` 指针 | `len()` 不计 NUL，`cap` 必须给它留位置；`read` / `write` / `encode` 都不自动补 NUL |
| `iterator`（迭代器） | 遍历容器的游标；本包没有实现 | `Vec` / `Map` 都不提供迭代器；最接近的失效规则是 `Arena.alloc()` 的指针在 `reset()` / `freeAll()` 后作废、`MapStrInt` 的键被改写后条目再也查不到 |
| `literal receiver`（字面量接收者） | 直接拿字面量作方法接收者 | `"abc".len()`；v0.19.1 报 `method receiver must be a variable, an element, a call or a field chain`，需要 0.20.0 |
| `chaining`（链式调用） | 把一个方法调用的结果继续当接收者 | `s.len().toString()` 合法，因为 `s.len()` 是调用而不是字面量 |
| `Drop` | 语言内建的析构钩子 | 包单元里的 `Drop` 发射为 `static`，消费者不可见，所以本包只提供显式 `free()` / `reset()` / `freeAll()` / `clear()` |
| `static slot`（静态环形槽） | 函数内的静态缓冲区，跨调用复用 | `Num.shim.h` 与 `Real.shim.h` 的 `toString` 各用四个 32 字节静态槽轮转；第五个活结果会覆盖第一个，非线程安全 |

## 9. 限制与未支持

- **索引只收录最新 tag**：没有历史版本，老编译器取不到匹配的旧版 std。
- **`requires` 只告警**：装得上不代表文档示例能编译。
- **字面量接收者需要 0.20.0**：v0.19.1 上所有示例都写成变量接收者。
- **`char *` 方法名全局共享**：新增 trait 必须避开 22 个已有名字。
- **`const` 不参与类型键**：字面量能直接当 `String` 接收者，因为该 trait 没有写接收者的方法。
- **`num.toString` / `math.toString` 非线程安全**：四个静态槽轮转。
- **`num.toStr` 静默截断**：容量不够得到短文本，不报错。
- **没有泛型**：容器只有 `VecInt` / `VecStr` / `MapStrInt` / `MapIntPtr` 四个单态类型。

## 10. 版本与变更记录

| 版本 | 状态 | 变更 |
| --- | --- | --- |
| `0.1.0` | 未打 tag、未发布 | 当前版本。文档重写：包根 README 改为总览；十个模块 README 全部吸收各自教程并删除教程文件；新建 `math/README.md`。API 未变。 |

版本策略：

| 项 | 约定 |
| --- | --- |
| 版本号 | 独立 semver，从 `0.1.0` 起；与编译器版本号无关 |
| `0.x` 期间 | 破坏性改动走 minor（`0.1.0` → `0.2.0`），修复走 patch；不承诺向后兼容 |
| `1.0.0` 之后 | 破坏性改动走 major |
| tag | `v<version>`；索引只认 `v*` 且取最高一个 |
| 编译器下限 | `ct >= 0.19.1`，写在 `ct.pkg` 的 `requires` |

打 tag 之后索引里的摘要会覆盖包目录下除 `.git`、`vendor`、`.ctcache` 外的所有文件，
包括 `README.md`、`tests/`、`.github/`。所以打 tag 前必须让所有文件定稿；tag 之后任何文件改动
都需要新版本号和新 tag，否则 `ct verify` 会报摘要不一致。

## License

GPL-3.0-or-later，附 GCC 运行时例外（见 `LICENSE` 与 `LICENSE.EXCEPTION`）。
你写的 `.ct` 源码与编译器生成物不受 GPL 约束。
