# `std::fs` — 文件系统动词（`FsOps`）

`char *` 上的一组路径方法：判断存在、读整个文件、写整个文件、列目录。全部是**静态映射**，
`path.exists()` 在调用点直接展开成 `std_fs_exists(path)`，没有 wrapper、没有 vtable、不改任何布局。

- 教程：[tutorial.md](tutorial.md)
- 完整可运行示例：[`test/std/fs/guide.ct`](../../tests/fs/guide.ct)（gate 每次编译并运行）
- 回归矩阵：[`test/std/fs/main.ct`](../../tests/fs/main.ct)

## import

```ct
import std::fs::Fs;
```

编译时必须有显式 include 根（D17-2，没有默认搜索路径）：

```sh
ct build -I . main.ct -o main
```

## 形态

映射式，两个文件：

| 文件 | 内容 |
| --- | --- |
| `Fs.m.ct` | `pub trait FsOps` + `map trait FsOps for type char *` |
| `Fs.shim.h` | `std_fs_*` 的 `static inline` 实现，由编译器自动 include |

宿主是 C 已存在的 `char *`，所以不建 struct、不建 `.h.ct`。`.m.ct` 的 prologue 只允许
`#include` / `#define`，函数体写不进去（`#ct-raw` 会报错，见
[SH-58](https://github.com/samxxu/ct-lang/blob/main/issues/selfhost/sh58-mct-prologue-raw-rejected.md)），所以实现放在 shim 头里。
这就是「一个映射模块两个文件」的由来。

## 类型与 trait

| 名字 | 种类 | 宿主 | 说明 |
| --- | --- | --- | --- |
| `FsOps` | `pub trait`（映射） | `char *` | 唯一入口。路径就是接收者，没有 `Fs` 实例 |

`const char *` 也能调用：类型键比较时丢掉顶层 `const`。`"a.txt".exists()`、
`char *p`、`const char *p` 三种写法都匹配同一个 trait。

## 方法

接收者写在参数表的第一个位置（`path`），调用时写成 `path.method(...)`。

| 方法 | 参数 | 返回 | 语义 |
| --- | --- | --- | --- |
| `exists()` | — | `int` | `stat` 能看到就返回 1，否则 0；`NULL` 或 `""` 返回 0 |
| `readFile(out, cap)` | `char *out, int cap` | `int` | 整个文件读进 `out`，返回**字节数**；打不开或放不下返回 -1 |
| `writeFile(data)` | `const char *data` | `int` | 截断/创建后写入 `data`，成功返回 0，失败返回 -1 |
| `list(out, cap)` | `char *out, int cap` | `int` | 目录项用 `'\n'` 连接写进 `out`，返回条目数；打不开或放不下返回 -1 |

`cap` 包含结尾 NUL 的位置：一个 11 字节的文件需要 `cap >= 12`，`readFile(buf, 11)` 返回 -1。
`list` 不写结尾换行，跳过 `.` 和 `..`。

## 内存与所有权

- **本模块不分配任何内存。** `readFile` / `list` 写进调用方给的 `out`，容量也是调用方给的。
- 容量不够是返回值 `-1`，**不是**截断；`readFile` 失败时 `out` 里可能已有部分字节。
- `list` 逐个追加：中途放不下时返回 -1，但 `out` 里**可能已经写进了前面几个名字**。
  失败后不要使用 `out` 的内容，返回值是唯一可信的信号。
- `writeFile` 只在调用期间读 `data`，不保存指针。
- 路径字符串同样不被保存；任何方法返回后 `std` 不再持有它。
- `std_fs_*` 是 `static inline`：每个消费方各有一份实现，不是外部符号。跨语言 FFI 需要自己再导出。

## 布局与代价

- 没有新类型、没有 vptr：`sizeof` 与裸 `char *` 完全一样。
- shim 约 130 行，**按消费方复制**：每个 import `std::fs::Fs` 的编译单元里各有一份
  `std_fs_*`。调试时断点下在消费方，不在 `std/fs/` 里的某个「库」中——那里没有单独的翻译单元。
- 用 `static inline` 而不是普通 `static`：后者在某个消费方未调用时会触发 `-Wunused-function`，
  前者未使用会被直接丢掉。

## 坑

1. **接收者是 `char *`，不是路径类型。** 任何字符串都能调这些方法：`"hello".exists()` 编译通过、
   运行期返回 0；`"hello".readFile(buf, 64)` 编译通过、返回 -1。编译器不会分辨「像不像路径」。
2. **`cap` 包含 NUL。** `readFile` 对 11 字节文件要求 `cap >= 12`；差一个字节就是 -1，不是读到 10 字节。
3. **`list` 返回 -1 时 `out` 可能已被部分写入。** 它不是事务性的；不要用「列表里有什么」来判断失败，
   只看返回值。
4. **`list` 的容量检查是按每个名字做的。** 缓冲区放得下第一个名字、放不下第二个，仍会返回 -1——
   此时 `out` 里已经有第一个名字。
5. **空串和 NULL 是「不存在的路径」。** `"".exists() == 0`，`"".readFile(...) == -1`，
   `"".list(...) == -1`；不要用空串表示「当前目录」，用 `"."`。
6. **`readFile` / `list` 数的是字节**，与 [`std::str::String.len()`](../str/README.md) 同一单位。
   `"é"` 写进文件再读出是 2，不是 1；要按码点处理用 [`std::utf8`](../utf8/README.md)。
7. **编译器目前不校验映射方法的实参个数**：多给的实参被静默丢弃，例如
   `path.readFile(buf, 64, 9)` 编译通过，那个 `9` 不起任何作用。少给实参则会报
   `a map call has fewer arguments than its binding`。这是编译器缺陷，不是本模块的约定，台账见
   [SH-56](https://github.com/samxxu/ct-lang/blob/main/issues/selfhost/sh56-map-method-arity-unchecked.md)。
