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
requires = ct >= 0.19.1
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
    buf.copy(word, 16);
    word.toUpper();
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
| `String` | `len` `find` `findChar` `startsWith` `endsWith` `copy` `slice` `concat` `toLower` `toUpper` `compare` `copyFrom` `copyTo` `toInt` `toDouble` |
| `FsOps` | `exists` `readFile` `writeFile` `list` |
| `Utf8Ops` | `decode` `codepointCount` `encode` |
| `Path`（`std::io`） | `open` |

合计 15 + 4 + 3 + 1 = **23**。`const char *` 也匹配同一宿主：类型键丢掉顶层 `const`，
所以对字符串字面量调用原地方法（`toLower` / `toUpper`）不会被编译器拦住，写只读内存是未定义行为。

### 4.3 其它宿主

| 宿主 | trait / 入口 | 方法 |
| --- | --- | --- |
| namespace `std::io` | `Stdout` | `puts` `putchar` `getchar` `fflush` |
| namespace `std::io` | `Io` | `printf(fmt, ...)`（变参，`va_list` + `vprintf`） |
| `IoFile *` | `File` | `read` `write` `close`（`IoFile` 是 `typedef struct ct_io_file IoFile;`） |
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
| C 不透明句柄 | `path.open("w")` 返回 `IoFile *`，再 `f.read(...)` / `f.close()` | 句柄不是堆对象，关闭后置空 |
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
| 原地大小写 | `buf.toLower()` / `buf.toUpper()`，返回 0 / -1 | `std::str` |
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

## 8. 限制与未支持

- **索引只收录最新 tag**：没有历史版本，老编译器取不到匹配的旧版 std。
- **`requires` 只告警**：装得上不代表文档示例能编译。
- **字面量接收者需要 0.20.0**：v0.19.1 上所有示例都写成变量接收者。
- **`char *` 方法名全局共享**：新增 trait 必须避开 23 个已有名字。
- **`const` 不参与类型键**：原地方法能对字符串字面量调用，后果自负。
- **`num.toString` / `math.toString` 非线程安全**：四个静态槽轮转。
- **`num.toStr` 静默截断**：容量不够得到短文本，不报错。
- **没有泛型**：容器只有 `VecInt` / `VecStr` / `MapStrInt` / `MapIntPtr` 四个单态类型。

## 9. 版本与变更记录

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
