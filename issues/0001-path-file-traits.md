# `std::io` 的 `Path` / `File` 到底是什么

| 项 | 值 |
| --- | --- |
| 性质 | 只读梳理：**不改代码、不改文档、不动版本**（历史记录，结论已被 0.7.0 取代） |
| 状态 | **已关闭（0.7.0 接口重构后本文结论失效；保留历史）** |
| 关联 | [issue 0001](0001-str-string-trait-rename.md) §6 的「同类问题」 |
| 结论 | 两个都是方法层 trait，不是类型；`Path` 挂在 `char *`，`File` 挂在 `IoFile *` |
| 日期 | 2026-11-15 |
| 说明 | 记录的是 0.3.x 的接口形态；0.4.0 起 `open` → `open_file`、`printf` 已删除，本文只作历史对照，示例不再保证可直接构建 |

## 1. 声明形态与宿主

| trait | 声明 | 形态 | `map trait` | 宿主 | 方法数 |
| --- | --- | --- | --- | --- | --- |
| `Path` | `std/io/File.m.ct:11` `pub trait Path {` | `pub trait`，无字段 | `std/io/File.m.ct:15` `map trait Path for type char *` | 裸 `char *` | 1 |
| `File` | `std/io/File.m.ct:19` `pub trait File {` | `pub trait`，无字段 | `std/io/File.m.ct:25` `map trait File for type IoFile *` | `IoFile *` | 3 |

`IoFile` 不是 `File` 的同义词，而是另一处的类型别名：

- `std/io/File.m.ct:9`：`pub type IoFile = struct ct_io_file;`，`ct_io_file` 是 shim 私有的 C tag
  （`std/io/File.shim.h:14-18`，内含 `FILE *fp`）。
- `Path` / `File` 都是映射式 trait（配 `map trait`，无 vtable、无接口值），不是 struct，
  也不是 `use Drop` 的所有权类型。`import std::io::File;` 一次带出这两组方法
  （`std/io/README.md:23,30`）。

## 2. 方法集（逐个）

### `Path`（宿主 `char *`）

| 方法 | 做什么 | 分配内存 | 失败怎么表达 |
| --- | --- | --- | --- |
| `open(mode)` → `IoFile *` | 以 `mode` 打开路径，返回不透明句柄 | **是**：`malloc` 包装体（`File.shim.h:23`）+ `fopen`（`:25`） | `NULL`（`0`）：参数为空、`malloc` 失败、`fopen` 失败都走这一条（`:22,24,28`），失败时已 `free` 包装体（`:26-29`） |

### `File`（宿主 `IoFile *`）

| 方法 | 做什么 | 分配内存 | 失败怎么表达 |
| --- | --- | --- | --- |
| `read(buf, n)` | 从句柄读至多 `n` 字节到调用方缓冲区 | 否 | 返回字节数；`0` = EOF；`-1` = 句柄/缓冲区无效或 `n < 0`（`File.shim.h:33-36`）。`fread` 的错误也以短计数返回，与 EOF 不可区分 |
| `write(buf, n)` | 把 `n` 字节写进句柄 | 否 | 返回写入字节数；`-1` = 句柄/参数无效（`:38-41`） |
| `close()` | `fclose` 底层 `FILE *` 并把内部指针置空 | 否（**也不释放包装体**） | `0` = 成功或已关闭（幂等）；`-1` = `fclose` 失败（`:46-53`） |

所有权口径（`std/io/README.md:211-215` 原文）：

> `open` 分配一个包装体（一个 `FILE *` + 关闭状态），`close` 只 `fclose` 并把内部指针置空，
> **不释放包装体**。这是为了 `close` 幂等：第二次 `close` 读到的仍是合法内存。

代价是每次 `open` 约 16 字节不回收（同上 `:214`）。

## 3. 为什么说它们「看着像类型、其实是方法层」

**3.1 直接当类型写会编译失败。** 实测：

```sh
$ ct build -I <repo> badtype.ct -o /tmp/badtype.bin
badtype.ct:5:5: error: a static trait has no interface value
```

源码就是 `File *f;`（`import std::io::File;` 之后）。也就是说 `File` 在类型位置不可用；
真正的句柄类型叫 `IoFile *`（`File.m.ct:9`）。名字与实际类型错位，正是 `String` 那类问题的轻量版。

**3.2 会误以为 `File` 是 RAII 句柄。** 看到 `path.open("r")` 返回 `IoFile *`、随后 `f.read()`，
很自然会以为「`File` 拥有句柄，出作用域自动关」。事实：

- `IoFile` 没有 `Drop`（`File.m.ct:9-29` 无 `use Drop`）；
- `close` 只是 `fclose`，包装体永不 `free`（`File.shim.h:43-53`）；
- 句柄拿到 `NULL` 时方法仍可调用：`read` 返回 `-1`、`close` 返回 `0`
  （`File.shim.h:34,48`，测试 `tests/io/main.ct` 与 `tests/io/coverage.ct` 就是这么断言的）。

具体踩法：循环里 `f = path.open("r"); ...;` 不写 `close`，编译器不会提示，fd 直接泄漏；
或以为 `close` 会释放包装体而反复开关文件，按文档口径每次约 16 字节不回收。

**3.3 会误以为 `Path` 有路径状态。** 这个名字在别处通常表示「拥有字符串、能 `join` / 规范化 /
判断存在性的路径对象」。这里：

- `Path` 只有 1 个方法 `open`（`File.m.ct:11-13`），没有字段、没有缓存、没有规范化；
- 接收者是裸 `char *`，`"a/b.txt".open("r")` 直接可用；`Path` 不能声明成变量（同 3.1 的错误）。

具体踩法：写 `path.exists()` 或 `path.join(...)` 时找不到方法；或以为同一字符串上的两次
`open` 共享某个 `Path` 实例的状态。

**3.4 `import` 名与 trait 名也不是一回事。** `import std::io::File;` 同时引入 `Path.open` 与
`File.read/write/close`（`std/io/README.md:30,314`），读 `import` 更容易把 `File` 当成「文件类」。

## 4. 与 `CStrOps`（原 `String`）的关系

宿主坐标：

| trait | 宿主 | 位置 |
| --- | --- | --- |
| `String`（拟改 `CStrOps`） | `char *` | `std/str/String.m.ct:66` |
| `FsOps` | `char *` | `std/fs/Fs.m.ct:45` |
| `Utf8Ops` | `char *` | `std/utf8/Utf8Ops.m.ct:41` |
| `Path` | `char *` | `std/io/File.m.ct:15` |
| `File` | `IoFile *` | `std/io/File.m.ct:25` |

- **同宿主 = 并列，不是包含。** `Path` 与 `CStrOps` 都映射 `char *`，两者没有继承、没有包装、
  方法也不会互相转发；它们只是把方法名汇入同一个「`char *` 方法名空间」。
- **同宿主名字全局唯一。** 原文（`README.md:141-142`）：

  > 同一接收者类型上，**方法名不能重复**：任意两个可见 trait 声明同名方法，调用点就报
  > `ambiguous type-host map for method 'x' on type 'char *'`。按名字判定，不看参数个数。

  现存 25 个名字见 `README.md:145-153`（`String` 14 + `FsOps` 7 + `Utf8Ops` 3 + `Path` 1）。
  `Path` 的 `open` 与另外 24 个不重名；反过来，`CStrOps` 若将来新增 `open`，会直接撞上 `Path`。
- **`File` 与 `CStrOps` 不同宿主，互不影响。** `IoFile *` 是另一类型；`read` / `write` / `close`
  在 `IoFile *` 上，不会与 `char *` 的 25 个名字冲突（`README.md:172-173` 有同类说明）。
- **接力关系：** `Path.open` 产出的正是 `File` 的宿主。所以顺序是「`Path` 造句柄 → `File` 用句柄」，
  不是「`File` 包含 `Path`」。
- **方法集交集：** `Path {open}` ∩ `String 14 名` = ∅；∩ `FsOps {exists, read_file, write_file, list,
  mkdir, remove, rename}` = ∅（`read_file`/`write_file` 与 `open` 语义相邻，但名字不同）；
  ∩ `Utf8Ops {decode, count_codepoints, encode}` = ∅。
- **`const char *`：** 类型键丢掉顶层 `const`（`README.md:153-154`），字面量接收者与 `char *` 同宿主；
  `open` 不写接收者，字面量安全。

## 5. 现状怎么用（真编译、真运行）

样例（CT 源文件）：

```ct
import std::io::Io;
import std::io::File;

int main(int argc, char **argv)
{
    char *path;
    IoFile *f;
    char buf[8];
    int n;

    path = (argc > 1) ? argv[1] : "std-pathfile-demo.txt";

    /* `path` 是裸 char *；open 来自 trait Path。 */
    f = path.open("w");
    std::io::printf("path.open(w) -> handle=%d\n", f != 0);
    std::io::printf("f.write(4)   -> %d\n", f.write("demo", 4));
    std::io::printf("f.close      -> %d\n", f.close());
    std::io::printf("f.close again-> %d\n", f.close());

    /* 同一个 char * 再开读；File 的方法作用于句柄。 */
    f = path.open("r");
    n = f.read(buf, 4);
    buf[n] = 0;
    std::io::printf("path.open(r) -> handle=%d\n", f != 0);
    std::io::printf("f.read(4)    -> %d buf=[%s]\n", n, buf);
    std::io::printf("f.close      -> %d\n", f.close());

    /* 不存在的路径：Path.open 返回 NULL，File 的方法仍可调用。 */
    f = "/nonexistent-dir/ct-demo".open("r");
    std::io::printf("open(missing)-> handle=%d\n", f != 0);
    std::io::printf("missing.read -> %d\n", f.read(buf, 1));
    std::io::printf("missing.close-> %d\n", f.close());

    return 0;
}
```

构建并运行（`ct` 在 PATH 上；在仓根执行，`-I .` 指向本仓）：

```sh
$ ct build -I . \
      --cflags "-Wall -Wextra" demo.ct -o demo.bin
$ ./demo.bin /tmp/ct-pathfile-demo/out.txt
```

实际输出：

```
path.open(w) -> handle=1
f.write(4)   -> 4
f.close      -> 0
f.close again-> 0
path.open(r) -> handle=1
f.read(4)    -> 4 buf=[demo]
f.close      -> 0
open(missing)-> handle=0
missing.read -> -1
missing.close-> 0
```

这段同时印证 §2 的三点：`open` 失败给 `NULL`；`close` 幂等；`read` 在空句柄上给 `-1`。

## 6. 若要改名（只给建议，未改）

| 现值 | 建议 | 代码改动 | 文档改动 | `import` 影响 |
| --- | --- | --- | --- | --- |
| `Path` | `PathOps` | 2 处：`File.m.ct:11`、`:15` | 5 处：`std/io/README.md:23,112`；`README.md:150,326,334` | **不变**（unit 仍叫 `File.m.ct`，`import std::io::File` 照旧）；调用点零改动 |
| `File` | `FileOps` | 3 处：`File.m.ct:19`、`:25`、文件头注释 `:4` | 11 处：`std/io/README.md:1,10,23,30,120,314`；`README.md:129,163,325,326`；`std/README.md:9` | **不变**（同上） |

补充：

- `tests/io/coverage.ct:26,29,31,35,50,63` 还有 6 条断言消息写着 `"Path.open: ..."`，那是字符串标签，
  不改也能编过；改名时顺手同步即可。
- 只改 trait 名时，`import std::io::File;` 仍叫 `File`，import 层面的「像类型」问题不解决；
  若连 unit 一起改成 `FileOps.m.ct`，`import` 会变成 `std::io::FileOps`，测试与所有 README 的
  import 都要动（10 处以上），代价明显上升。**建议先只改 trait 名，unit 名留给负责人另行决定。**
- 两项都**不**并入 issue 0001 一起做；是否改名、何时改，等负责人确认后单独开 issue
  （issue 0001 §6 已有同样的范围声明）。

## 7. 处理结果（2026-11-19 复核）

本文的结论「两个都是方法层 trait，`File` 挂 `IoFile *`」在 **0.7.0（P3）** 之后不再成立：

- `std/io/File.m.ct` 已删除；`IoFile` 别名退役。
- `File` / `FileDesc` 改为 CT struct + `use Drop` + 幂等 `close()`（commit `54660df`），
  方法用 `->`；`open` 在 0.4.0 已改名 `open_file`，`printf` 已删除。
- 因此本文 §2 的方法表、§6 的改名建议（`Path` → `PathOps`、`File` → `FileOps`）**不再执行**：
  `File` 不再是 trait；`Path` 保留现名（0.7.0 的字符串工厂单元是 `Path.m.ct`，返回
  `File *`，见 `docs/design/decision-resource-ownership.md`）。
- §6 里「`IoFile` 包装体不回收」的问题随 `IoFile` 退役消失；释放协议改为
  `defer { delete h; }`（`own` 是可选的自由工厂路径）。

结论：本 issue 的**问题本身已被资源对象重构解决**，不是通过改名解决的；保留本文作为
0.3.x 接口的历史证据，不再跟踪后续动作。
