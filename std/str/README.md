# `std::str` — `char *` 的字节字符串方法（`String`）

`char *` 上的查询、拷贝、拼接、大小写与数字解析。全部是**静态类型宿主映射**：
`s.len()` 在调用点直接展开成 `std_str_len(s)`，没有 wrapper、没有 vtable、不改变 C 布局。

## 1. 定位与边界

- 处理**以 NUL 结尾的字节串**：`len` / `find` / `slice` 的单位都是字节。
- 不处理 UTF-8 码点语义；需要按字符处理时用 [`std::utf8`](../utf8/README.md) 的 `codepointCount` / `decode`。
- **不分配内存**：所有产出写进调用方提供的 `out + cap`，容量不足返回 `-1`，不静默截断。
- 不是 `std::string`：没有堆对象、没有析构、没有隐式扩容。
- 不新增类型：接收者就是 C 的 `char *`，`const char *` 也匹配（类型键丢掉顶层 `const`）。

## 2. 依赖与安装

依赖 C 标准库的 `<string.h>` / `<stdlib.h>`；`ct.pkg` 的约束是 `requires ct >= 0.19.1`。

| 文件 | 内容 |
| --- | --- |
| `String.m.ct` | `pub trait String` + `map trait String for type char *`（15 个方法） |
| `String.shim.h` | `std_str_*` 的 `static inline` 实现，编译器自动 include |

引入方式：`import std::str::String;`。

包安装后（详见[包根 README](../../README.md#安装)）：

```sh
ct get std@^0.1.0
ct build src/main.ct -I vendor/std -o main --cflags "-Wall -Wextra"
```

在包根内开发时：

```sh
ct build -I . main.ct -o /tmp/std-str --cflags "-Wall -Wextra"
```

## 3. 快速开始

```ct
import std::str::String;
import std::io::Io;

int main(void)
{
    char buf[32];
    char *s = "Hello, ct";
    int n;

    std::io::printf("len=%d find=%d\n", s.len(), s.find("ct"));

    n = s.slice(0, 5, buf, 32);
    std::io::printf("slice rc=%d value=%s\n", n, buf);

    n = s.copy(buf, 32);
    std::io::printf("copy rc=%d value=%s\n", n, buf);
    return 0;
}
```

`ct build` 实际执行的 `cc` 行（本机实测，路径随检出位置变化）：

```
'cc' -std=c99 -Wall -Wextra -I '/Users/samxx/vscspace/ct-pkg/std/std/str' '.ctbuild/main.c' '.ctbuild/Io.c' '-o' '/tmp/std-str'
```

输出：

```text
len=9 find=7
slice rc=0 value=Hello
copy rc=0 value=Hello, ct
```

注意接收者写成变量 `s`。v0.19.1 及更早版本不接受字面量接收者，`"abc".len()` 会报
`method receiver must be a variable, an element, a call or a field chain`；
字面量写法需要 **0.20.0**。

## 4. API 全表

宿主：`char *`（`const char *` 同样匹配）。接收者是第一个参数，调用写成 `s.method(...)`。

| 方法 | 签名 | 返回 / 失败 |
| --- | --- | --- |
| `len()` | `int len()` | 字节数，不含 NUL；`NULL` 返回 0 |
| `find(needle)` | `int find(const char *needle)` | 首次出现的字节偏移；找不到或入参 `NULL` 返回 `-1` |
| `findChar(c)` | `int findChar(int c)` | 字符首次出现的字节偏移；找不到返回 `-1` |
| `startsWith(prefix)` | `int startsWith(const char *prefix)` | 是 1 / 否 0；入参 `NULL` 返回 0 |
| `endsWith(suffix)` | `int endsWith(const char *suffix)` | 是 1 / 否 0；后缀比原串长返回 0 |
| `copy(out, cap)` | `int copy(char *out, int cap)` | 成功 0；放不下（含 NUL）返回 `-1` |
| `slice(start, end, out, cap)` | `int slice(int start, int end, char *out, int cap)` | 半开区间 `[start, end)`；越界或放不下返回 `-1` |
| `concat(other, out, cap)` | `int concat(const char *other, char *out, int cap)` | 成功 0；放不下返回 `-1` |
| `toLower()` | `int toLower()` | **原地**改小写，成功 0；`NULL` 返回 `-1` |
| `toUpper()` | `int toUpper()` | **原地**改大写，成功 0；`NULL` 返回 `-1` |
| `compare(other)` | `int compare(const char *other)` | 字典序，负 / 0 / 正；`NULL` 当空串 |
| `copyFrom(src, n)` | `void copyFrom(const char *src, int n)` | `memcpy` 形状：恰好 `n` 字节，不补 NUL，不查容量 |
| `copyTo(dst, n)` | `void copyTo(char *dst, int n)` | `memcpy` 形状：接收者的恰好 `n` 字节写进 `dst` |
| `toInt()` | `int toInt()` | `atoi`；`NULL` 返回 0，溢出是 C 的未定义行为 |
| `toDouble()` | `double toDouble()` | `atof`；`NULL` 返回 0.0 |

`cap` 一律包含结尾 NUL 的位置：`"abc"` 需要 `cap >= 4`。

`len()` 返回 `int` 而不是 `size_t`，与 `find` / `slice` 的偏移同型。因为 `std::num` 已有 `Int`
（`int` 宿主），**调用结果可以直接链式**：

```ct
import std::str::String;
import std::num::Num;
import std::io::Io;

int main(void)
{
    char *s = "abc";
    std::io::printf("len=%s\n", s.len().toString());
    return 0;
}
```

输出 `len=3`。这里合法是因为 `s.len()` 是**调用**，属于允许的接收者形态；
字面量 `(5).toString()` 仍然需要 0.20.0。

## 5. 教程

### 5.1 一次完整的读写

```ct
import std::str::String;
import std::io::Io;

int main(void)
{
    char buf[16];
    char word[8];
    char raw[8];
    char *s = "Hello, ct";
    char *other = "hello";
    char *seed = "Hello";
    char *hw = "hello";
    char *num = "42";
    char *real = "2.5";
    int n;

    n = s.copy(buf, 9);
    std::io::printf("copy cap=9 rc=%d\n", n);

    n = s.slice(0, 99, buf, 16);
    std::io::printf("slice past end rc=%d\n", n);

    n = s.compare(other);
    std::io::printf("compare rc=%d\n", n < 0 ? -1 : (n > 0 ? 1 : 0));

    seed.copy(word, 8);
    word.toLower();
    std::io::printf("lower=%s\n", word);
    word.toUpper();
    std::io::printf("upper=%s\n", word);

    raw.copyFrom(hw, 5);
    raw[5] = 0;
    std::io::printf("copyFrom=%s\n", raw);
    hw.copyTo(raw, 5);
    raw[5] = 0;
    std::io::printf("copyTo=%s\n", raw);

    std::io::printf("toInt=%d toDouble=%g\n", num.toInt(), real.toDouble());
    return 0;
}
```

输出：

```text
copy cap=9 rc=-1
slice past end rc=-1
compare rc=-1
lower=hello
upper=HELLO
copyFrom=hello
copyTo=hello
toInt=42 toDouble=2.5
```

### 5.2 查询：偏移是字节，布尔是 0/1

- `find` / `findChar` 返回字节偏移，`0` 是合法结果，所以要用 `>= 0` 判断命中，不能当布尔用。
- `startsWith` / `endsWith` 返回 `0` / `1`，不是索引。
- 比较大小写敏感：`"Hello"` 与 `"hello"` 的 `compare` 不为 0。

### 5.3 产出新字符串：缓冲区由调用方给

`copy` / `slice` / `concat` 都不分配内存：

1. `cap` 包含结尾 NUL，`"Hello, ct"` 是 9 字节，`cap` 至少 10；
2. 放不下返回 `-1`，不会截断，也不会扩容；
3. 返回 `-1` 时 `buf` 里是残留内容，必须靠返回值判断成败。

`slice` 是半开区间：`slice(0, 5, ...)` 得到 `"Hello"`。

### 5.4 `copyFrom` / `copyTo`：`memcpy` 的 OO 写法

它们与 `copy` 的规则不同：

- `n` 是**字节数本身**，不包含也不追加 NUL；
- 不检查目标容量，也不返回状态（`void`）；
- `n <= 0` 或任一指针为 `NULL` 时什么也不做。

想得到 C 字符串，自己留位置并写 `raw[n] = 0;`。

### 5.5 原地大小写：只改写自己的缓冲区

`toLower` / `toUpper` **不返回新字符串**，返回状态码，改的是接收者指向的内存。
先 `char *seed = "Hello"; seed.copy(word, 8);` 拷到自己的数组，再 `word.toLower()`；
此时 `word` 是 `"hello"`。

转换只覆盖 ASCII 的 `A-Z` / `a-z`，不做 locale、也不处理非 ASCII 字节。

类型键会丢掉 `const`，所以在支持字面量接收者的 0.20.0 上，对字符串字面量调用 `toLower()`
**能编译通过**，然后写只读页，运行期未定义；0.19.1 会先报 receiver 错误。
编译器不会替你拦内存可写性。

### 5.6 字节不是字符

`"é"` 在 UTF-8 里是两个字节：

`char *s = "é";` 时 `s.len() == 2`；`import std::utf8::Utf8Ops;` 之后
`s.codepointCount() == 1`。

`find` / `findChar` / `slice` 的索引同样是字节偏移，按字节切可能切断一个字符。
需要按码点计数或遍历时用 [`std::utf8`](../utf8/README.md)。

### 5.7 解析数字

`toInt()` / `toDouble()` 是 `atoi` / `atof` 的映射，不做错误报告：

`char *num = "42";` 的 `num.toInt()` 是 `42`；`char *real = "2.5";` 的 `real.toDouble()` 是
`2.5`。两者都不做错误报告。

输入不是数字时得到 0 或部分解析结果，需要严格校验就自己先扫描。

## 6. 常见配方

### 解析 `key=value` 行

```ct
import std::str::String;
import std::io::Io;

int main(void)
{
    char key[16];
    char val[16];
    char *line = "count=42";
    int pos;
    int n;

    pos = line.findChar('=');
    line.slice(0, pos, key, 16);
    line.slice(pos + 1, line.len(), val, 16);
    n = val.toInt();
    key.toUpper();
    std::io::printf("key=%s value=%d\n", key, n);
    return 0;
}
```

输出：

```text
key=COUNT value=42
```

### 其它常用组合

| 任务 | 写法 |
| --- | --- |
| 安全复制 | `if (s.copy(buf, sizeof buf) < 0) { /* 缓冲区太小 */ }` |
| 取前缀 / 后缀 | `s.slice(0, 5, out, cap)` / `s.slice(s.len() - 3, s.len(), out, cap)` |
| 拼接 | `a.concat(b, out, cap)` |
| 原地转大写 | 先 `copy` 到自己的数组，再 `word.toUpper()` |
| 找子串位置 | `int pos = s.find("needle"); if (pos >= 0) { ... }` |
| 得到长度的十进制文本 | `s.len().toString()`（`int` 宿主，0.19.1 可用） |

## 7. 排错

### 7.1 方法名不存在

```ct
import std::str::String;

int main(void)
{
    char *s = "abc";
    int n = s.nope();
    return n;
}
```

```text
main.ct:6:1: error: no method 'nope' for type 'char *'
```

方法按名字绑定到宿主 `char *`，不存在的名字直接报在调用点。

### 7.2 忘了 import

```ct
int main(void)
{
    char *s = "abc";
    int n = s.len();
    return n;
}
```

```text
main.ct:4:1: error: no method 'len' for type 'char *'
```

报错不会提示“你可能忘了 import”；`char *` 上本来就没有这个方法，只有 import 之后才存在。

### 7.3 字面量接收者（0.19.1）

```ct
import std::str::String;

int main(void)
{
    "abc".toLower();
    return 0;
}
```

```text
main.ct:5:1: error: method receiver must be a variable, an element, a call or a field chain
```

先赋给变量：`char *s = "abc"; s.toLower();`（注意这仍然会改只读内存，正确做法是先 `copy`）。
字面量接收者需要 0.20.0。

### 7.4 少给参数

```ct
import std::str::String;

int main(void)
{
    char buf[8];
    char *s = "abc";
    return s.copy(buf);
}
```

```text
main.ct:7:1: error: a map call has fewer arguments than its binding
```

多给参数则是编译器缺陷：编译通过，多余实参被静默丢弃（见第 8 节）。

## 8. 限制与未支持

- **不分配、无动态字符串**：没有 `append` / `format` / `replace`；产出永远走 `out + cap`。
- **字节单位**：`len` / `find` / `slice` 都是字节；没有码点索引。
- **原地方法不防只读内存**：`toLower` / `toUpper` 对字面量调用在 0.19.1 上先报 receiver 错误，
  在 0.20.0 上编译通过但写只读页；正确做法是先 `copy` 到自己的缓冲区。
- **大小写只覆盖 ASCII**：`toLower` / `toUpper` 按 `A-Z` / `a-z` 逐字节转换，不做 locale。
- **`char *` 上的方法名全局共享**：同一接收者上两个可见 trait 声明同名方法，调用点报
  `ambiguous type-host map for method 'x' on type 'char *'`。本模块占用
  `len` / `find` / `findChar` / `startsWith` / `endsWith` / `copy` / `copyFrom` / `copyTo` /
  `slice` / `concat` / `toLower` / `toUpper` / `compare` / `toInt` / `toDouble`；
  新增 `char *` trait 前先 grep。
- **字面量接收者需要 0.20.0**；0.19.1 只接受变量、元素、调用和字段链。
- **多给实参不报错**（SH-56）：`s.copy(buf, 8, 9)` 编译通过，`9` 被丢弃；少给才报
  `a map call has fewer arguments than its binding`。
- **`toInt` / `toDouble` 不做错误报告**：与 `atoi` / `atof` 一致，非法输入不抛错。

## 9. 版本与变更记录

| 版本 | 状态 | 变更 |
| --- | --- | --- |
| `0.1.0` | 未打 tag、未发布 | 当前版本。文档重写：教程并入本 README，删除 `tutorial.md`；API 未变。 |

编译器下限 `ct >= 0.19.1`。0.19.1 上库本体与本文档中的程序均可编译运行；
字面量接收者写法需要 0.20.0。
