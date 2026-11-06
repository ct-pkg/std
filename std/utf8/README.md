# `std::utf8` — UTF-8 编解码（`Utf8Ops`）

`char *` 上的码点操作：解一个码点、数码点、把一个码点编码进缓冲区。全部是**静态映射**，
`s.decode(&cp)` 在调用点直接展开成 `std_utf8_decode(s, &cp)`，没有 wrapper、没有 vtable。

- 教程：[tutorial.md](tutorial.md)
- 完整可运行示例：[`test/std/utf8/guide.ct`](../../tests/utf8/guide.ct)（gate 每次编译并运行）
- 回归矩阵：[`test/std/utf8/main.ct`](../../tests/utf8/main.ct)

## import

```ct
import std::utf8::Utf8Ops;
```

编译时必须有显式 include 根（D17-2，没有默认搜索路径）：

```sh
ct build -I . main.ct -o main
```

## 形态

映射式，两个文件：

| 文件 | 内容 |
| --- | --- |
| `Utf8Ops.m.ct` | `pub trait Utf8Ops` + `map trait Utf8Ops for type char *` |
| `Utf8Ops.shim.h` | `std_utf8_*` 的 `static inline` 实现，由编译器自动 include |

宿主是 C 已存在的 `char *`，所以不建 struct、不建 `.h.ct`。`.m.ct` 的 prologue 只允许
`#include` / `#define`（[SH-58](https://github.com/samxxu/ct-lang/blob/main/issues/selfhost/sh58-mct-prologue-raw-rejected.md)），
实现只能放在 shim 头里。

## 类型与 trait

| 名字 | 种类 | 宿主 | 说明 |
| --- | --- | --- | --- |
| `Utf8Ops` | `pub trait`（映射） | `char *` | 唯一入口。字节串是接收者；`encode` 的接收者是输出缓冲区 |

`const char *` 也能调用：类型键丢掉顶层 `const`。`decode` / `codepointCount` 只读接收者，
`encode` **写**接收者，所以 `buf.encode(cp, cap)` 里的 `buf` 必须可写。

## 方法

接收者写在参数表的第一个位置，调用时写成 `s.method(...)` / `buf.method(...)`。

| 方法 | 参数 | 返回 | 语义 |
| --- | --- | --- | --- |
| `decode(cp)` | `int *cp` | `int` | 解出第一个码点写进 `*cp`，返回消费的字节数（1–4）；无效序列或空串返回 -1 |
| `codepointCount()` | — | `int` | 码点个数；无效 UTF-8 返回 -1；`NULL` 返回 0 |
| `encode(cp, cap)` | `int cp, int cap` | `int` | 把码点编码进接收者缓冲区，返回写入的字节数（1–4）；放不下 / 代理区 / 越界返回 -1 |

`decode` 的 `cp` 可以是 `NULL`（只关心消费了几个字节）。`encode` 的 `cap` 是**字节数**，
不是码点数，也**不包含**结尾 NUL——它根本不写 NUL。

## 内存与所有权

- **本模块不分配任何内存。** `encode` 写进接收者指向的缓冲区，容量由调用方给。
- `encode` 成功时只写 `n` 个字节（`n` 是返回值），**不追加 NUL**。要把结果当 C 字符串用，
  必须自己 `buf[n] = 0;`。
- 失败（-1）时 `encode` 一个字节都不写：先检查容量和码点合法性，再落笔。
- `decode` / `codepointCount` 不修改输入，也不保存指针。
- `std_utf8_*` 是 `static inline`：每个消费方各有一份实现，不是外部符号。

## 布局与代价

- 没有新类型、没有 vptr：`sizeof` 与裸 `char *` 完全一样。
- shim 约 100 行，**按消费方复制**：每个 import 的编译单元各有一份。断点下在消费方。
- 与 [`std::str`](../str/README.md) 共用 `char *` 这个宿主，方法名不能重名。

## 坑

1. **单位不同：`String.len()` 数字节，`codepointCount()` 数码点。** 同一个 `"é"` 是
   `len() == 2`、`codepointCount() == 1`；`"héllo"` 是 6 和 5。两个名字不能互换。
2. **`decode` 只解第一个码点。** 它不遍历字符串，返回的是消费的字节数；遍历就自己
   用返回值前进（`i += step`），不要 `i++`——那会把多字节序列的续字节当成新码点的开头。
3. **`encode` 不写 NUL。** `buf.encode(233, 8)` 返回 2，`buf[0..1]` 是 `é` 的两个字节，
   `buf[2]` 保持原样（或未初始化）。紧接着 `buf.len()` / `codepointCount()` 会越界读。
   先 `buf[n] = 0;`。
4. **`cap` 是字节宽度，不是码点数。** `encode(233, 2)` 成功（两字节），`encode(233, 1)`
   返回 -1；不会写半个字符。
5. **严格解码：无效序列返回 -1，不替换成 U+FFFD。** 过长编码（`C0 80`）、代理区
   （`ED A0 80`）、孤立续字节（`80`）、截断序列（`C3`）全部 -1；`codepointCount()` 遇到
   同样的输入也返回 -1，而不是给出一个偏小的计数。
6. **`encode` 拒绝代理区与越界码点。** `encode(0xD800, ...)`、`encode(0x110000, ...)`
   都是 -1：UTF-8 不表示这些值。
7. **编译器目前不校验映射方法的实参个数**：多给的实参被静默丢弃，例如
   `s.decode(&cp, 1)` 编译通过，那个 `1` 被丢弃；少给实参报
   `a map call has fewer arguments than its binding`。台账见
   [SH-56](https://github.com/samxxu/ct-lang/blob/main/issues/selfhost/sh56-map-method-arity-unchecked.md)。
