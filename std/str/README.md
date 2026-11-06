# `std::str` — 字节字符串（`String`）

`char *` 上的一组方法：查询、拷贝、拼接、大小写转换。全部是**静态映射**，`s.len()` 在调用点直接
展开成 `std_str_len(s)`，没有 wrapper、没有 vtable、不改变任何 C 布局。

- 教程：[tutorial.md](tutorial.md)
- 完整可运行示例：[`test/std/str/guide.ct`](../../tests/str/guide.ct)（gate 每次编译并运行）
- 回归矩阵：[`test/std/str/main.ct`](../../tests/str/main.ct)

## import

```ct
import std::str::String;
```

编译时必须有显式 include 根（D17-2，没有默认搜索路径）：

```sh
ct build -I . main.ct -o main
```

## 形态

映射式，两个文件：

| 文件 | 内容 |
| --- | --- |
| `String.m.ct` | `pub trait String` + `map trait String for type char *` |
| `String.shim.h` | `std_str_*` 的 `static inline` 实现，由编译器自动 include |

宿主是 C 已存在的 `char *`，所以不建 struct、不建 `.h.ct`。`map trait` 只能写在 `.m.ct` 里，而
`.m.ct` 的 prologue 只允许 `#include` / `#define`（`#ct-raw` 会报错，见
[SH-58](https://github.com/samxxu/ct-lang/blob/main/issues/selfhost/sh58-mct-prologue-raw-rejected.md)），因此实现必须在 shim 头里，
这也是「一个模块两个文件」的由来。

## 类型与 trait

| 名字 | 种类 | 宿主 | 说明 |
| --- | --- | --- | --- |
| `String` | `pub trait`（映射） | `char *` | 唯一入口。没有 struct；字符串本身就是接收者 |

`const char *` 也能调用：类型键比较时会丢掉顶层 `const`。所以 `"hi".len()`、`char *s`、
`const char *s` 三种写法都匹配同一个 trait。

## 方法

接收者写在参数表的第一个位置（`s`），调用时写成 `s.method(...)`。

| 方法 | 参数 | 返回 | 语义 |
| --- | --- | --- | --- |
| `len()` | — | `int` | **字节数**，不含结尾 NUL；`s` 为 NULL 时返回 0 |
| `find(needle)` | `char *needle` | `int` | `needle` 第一次出现的**字节偏移**；找不到或入参 NULL 返回 -1 |
| `findChar(c)` | `int c` | `int` | 字符 `c` 第一次出现的字节偏移；找不到返回 -1 |
| `startsWith(prefix)` | `char *prefix` | `int` | 前缀匹配返回 1，否则 0（入参 NULL 返回 0） |
| `endsWith(suffix)` | `char *suffix` | `int` | 后缀匹配返回 1，否则 0；`suffix` 比 `s` 长返回 0 |
| `copy(out, cap)` | `char *out, int cap` | `int` | 整体拷入 `out`，含结尾 NUL；空间不足返回 -1 |
| `slice(start, end, out, cap)` | `int start, int end, char *out, int cap` | `int` | 拷贝半开区间 `[start, end)`；越界或空间不足返回 -1 |
| `concat(b, out, cap)` | `char *b, char *out, int cap` | `int` | 把 `b` 接到 `s` 后面写进 `out`；空间不足返回 -1 |
| `toLower()` | — | `int` | **原地**转小写，返回 0；`s` 为 NULL 返回 -1 |
| `toUpper()` | — | `int` | **原地**转大写，返回 0；`s` 为 NULL 返回 -1 |
| `compare(b)` | `char *b` | `int` | 字典序，形如 `strcmp`：小于 / 等于 / 大于分别是负数 / 0 / 正数；NULL 当空串 |

`cap` 一律包含结尾 NUL 的位置：`copy("abc", buf, 4)` 成功，`copy("abc", buf, 3)` 返回 -1。

## 内存与所有权

- **本模块不分配任何内存。** 需要产出一个新字符串时，`out` 和 `cap` 都由调用方提供。
- 空间不足是返回值 `-1`，**不是**截断。调用方必须检查返回值，否则拿到的是上次残留内容。
- `toLower` / `toUpper` **原地改写**接收者，不返回新字符串；只对调用方自己的可写缓冲区使用。
- 输入字符串不被修改（除上面两个原地方法），也不被保存——调用返回后 `std` 不再持有任何指针。
- `std_str_*` 是 `static inline`：每个消费方各有一份实现，不是外部符号。跨语言 FFI 需要自己再导出。

## 坑

1. **`len()` 数字节，不数字符。** `"é".len() == 2`（UTF-8 两字节）。要数码点用
   [`std::utf8::Utf8Ops.codepointCount()`](../utf8/README.md)。
2. **`toLower()` / `toUpper()` 是原地修改，且类型键丢掉了 `const`。** 所以
   `"abc".toLower()` **能编译通过**，然后在只读页上写入——编译器不会拦。正确做法是先 `copy` 到
   自己的缓冲区再改（见 guide）。
3. **`copy` / `slice` / `concat` 的输出缓冲由调用方给**，容量不够返回 -1；不要假设它们会分配。
4. **`char *` 上的方法名是全局共享的。** 同一个接收者上两个可见 trait 声明同名方法，调用点就报
   `ambiguous type-host map for method 'x' on type 'char *'`（不做参数重载，也不会静默挑一个）。
   给 `char *` 新增 trait 前先 grep：`len` / `find` / `findChar` / `startsWith` / `endsWith` /
   `copy` / `slice` / `concat` / `toLower` / `toUpper` / `compare` 已被本模块占用；
   `exists` / `readFile` / `writeFile` / `list` 被 `std::fs` 占用；`decode` / `codepointCount` /
   `encode` 被 `std::utf8` 占用。
5. **编译器目前不校验映射方法的实参个数**：`"abc".len(1)` 会编译通过并静默丢弃那个 `1`。
   这是编译器缺陷，不是本模块的约定，台账见
   [SH-56](https://github.com/samxxu/ct-lang/blob/main/issues/selfhost/sh56-map-method-arity-unchecked.md)。
