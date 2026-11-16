# `std::num` — 整数与字符宿主（`Int` / `Long` / `LLong` / `Char`）

给 C 的数值与字符类型挂方法：`n.to_string()` / `n.to_base(16, buf, cap)` / `n.clamp(lo, hi)` /
`c.is_digit()` / `c.uppercase()`。四个宿主各映射一次，同名方法跨宿主允许，同一宿主内唯一。

## 1. 定位与边界

- 覆盖四种宿主：`int`（`Int`）、`long`（`Long`）、`long long`（`LLong`）、`char`（`Char`）。
- 数值宿主提供：`abs` / `clamp` / `max` / `min` / `to_str` / `to_string` / `to_base`。
- `char` 宿主提供 ctype 表面：`is_digit` / `is_alpha` / `is_space` / `is_upper` / `is_lower` /
  `uppercase` / `lowercase`；`char` 不会提升成 `int`，所以两套方法互不干扰。
- **不分配**：`to_str` / `to_base` 写调用方缓冲区；`to_string` 用静态环形槽（非线程安全）。
- 不做格式化宽度、精度、千分位；需要时用 C 的 `printf`。

## 2. 依赖与安装

依赖 C 的 `<stdlib.h>` / `<ctype.h>` / `<stdio.h>`；`ct.pkg` 的约束是 `requires ct >= 0.30.0`。

| 文件 | 内容 |
| --- | --- |
| `Num.m.ct` | `Int` / `Long` / `LLong` / `Char` 四个 `pub trait` + 四个 `map trait ... for type ...` |
| `Num.shim.h` | `std_num_*` 的 `static inline` 实现；`Int` / `Long` / `LLong` 共用 `long long` 辅助函数 |

| 宿主 | trait | map 行 |
| --- | --- | --- |
| `int` | `Int` | `map trait Int for type int` |
| `long` | `Long` | `map trait Long for type long`（`long int` 折叠到同一宿主） |
| `long long` | `LLong` | `map trait LLong for type long long` |
| `char` | `Char` | `map trait Char for type char` |

引入方式：`import std::num::Num;`。

包安装后（详见[包根 README](../../README.md#安装)）：

```sh
ct get std@^0.2.0
ct build src/main.ct -I vendor/std -o main --cflags "-Wall -Wextra"
```

## 3. 快速开始

```ct
import std::num::Num;
import std::io::Io;

int main(void)
{
    char buf[32];
    int n = 65;
    int lo = 5;
    char digit = '7';
    char letter = 'a';

    std::io::printf("%s %s %d\n", n.to_string(), n.to_base(16, buf, 32), lo.clamp(0, 3));
    std::io::printf("digit=%d alpha=%d upper=%c\n",
                    digit.is_digit(), letter.is_alpha(), (char)letter.uppercase());
    return 0;
}
```

`ct build` 实际执行的 `cc` 行（本机实测，路径随检出位置变化）：

```
'cc' -std=c99 -Wall -Wextra -I '/Users/samxx/vscspace/ct-pkg/std/std/num' '.ctbuild/main.c' '.ctbuild/Io.c' '-o' '/tmp/std-num'
```

输出：

```text
65 41 3
digit=1 alpha=1 upper=A
```

`int` 宿主是后加的：在此之前只有 `Long` / `LLong` / `Char`，`(5).to_string()` 这类写法没有落点。
字面量接收者本身从 **0.20.0** 起可用（本包下限 0.30.0）；v0.19.1 先写 `int n = 5;`，再用 `n.to_string()`。

## 4. API 全表

### `int` / `long` / `long long`（`Int` / `Long` / `LLong`）

`T` 表示宿主类型，接收者是第一个参数。

| 方法 | `int` 宿主 | `long` 宿主 | `long long` 宿主 | 返回 / 失败 |
| --- | --- | --- | --- | --- |
| `abs()` | `int abs()` | `long abs()` | `long long abs()` | 绝对值；`INT_MIN` 等溢出是 C 的未定义行为 |
| `clamp(lo, hi)` | `int clamp(int lo, int hi)` | `long clamp(long, long)` | `long long clamp(long long, long long)` | 夹到 `[lo, hi]`；要求 `lo <= hi` |
| `max(other)` | `int max(int other)` | `long max(long other)` | `long long max(long long other)` | 较大者 |
| `min(other)` | `int min(int other)` | `long min(long other)` | `long long min(long long other)` | 较小者 |
| `to_str(out, cap)` | `char *to_str(char *out, int cap)` | 同左 | 同左 | 写入十进制文本；**静默截断**到 `cap - 1`；`out == NULL` 或 `cap <= 0` 原样返回 `out` |
| `to_string()` | `char *to_string()` | 同左 | 同左 | 十进制文本；四个静态槽轮转，**非线程安全** |
| `to_base(base, out, cap)` | `char *to_base(int base, char *out, int cap)` | 同左 | 同左 | 2..36 进制、小写字母；放不下返回 `NULL` 且不写内容；非法 base 返回空串 |

`to_base` 与 `to_str` 的失败语义不同：

- `to_str`：`snprintf` 截断，永远返回 `out`；
- `to_base`：**绝不截断**，符号与所有数字放不下时返回 `NULL`；`base < 2 || base > 36` 时写入空串并返回 `out`。

### `char`（`Char`）

| 方法 | 签名 | 返回 |
| --- | --- | --- |
| `is_digit()` | `int is_digit()` | 是数字返回非 0，否则 0 |
| `is_alpha()` | `int is_alpha()` | 是字母返回非 0 |
| `is_space()` | `int is_space()` | 是空白返回非 0 |
| `is_upper()` | `int is_upper()` | 是大写返回非 0 |
| `is_lower()` | `int is_lower()` | 是小写返回非 0 |
| `uppercase()` | `int uppercase()` | 转换后的字符（`int`）；非字母原样返回 |
| `lowercase()` | `int lowercase()` | 转换后的字符（`int`）；非字母原样返回 |

负数会先转成 `unsigned char` 再交给 C 的 ctype，这是 C 标准要求的安全写法。

## 5. 教程

### 5.1 四个宿主，四套同名方法

```ct
import std::num::Num;
import std::io::Io;

int main(void)
{
    char buf[32];
    char small[2];
    long lv = 65;
    long long ll = -88;
    int n = -77;
    int a = 5;
    int b = 9;
    char digit = '7';
    char letter = 'a';
    char *r1;
    char *r2;

    std::io::printf("int %s %s %d %d\n",
                    n.to_string(), n.to_base(16, buf, 32), a.clamp(0, 3), a.max(b));
    std::io::printf("long %s longlong %s\n", lv.to_string(), ll.to_base(2, buf, 32));

    r1 = a.to_string();
    r2 = b.to_string();
    std::io::printf("ring %s %s\n", r1, r2);
    std::io::printf("base small null=%d\n", n.to_base(16, small, 2) == 0);
    std::io::printf("char digit=%d alpha=%d upper=%c\n",
                    digit.is_digit(), letter.is_alpha(), (char)letter.uppercase());
    return 0;
}
```

输出：

```text
int -77 -4d 3 9
long 65 longlong -1011000
ring 5 9
base small null=1
char digit=1 alpha=1 upper=A
```

要点：

1. `n` 是 `int`，`lv` 是 `long`，`ll` 是 `long long`；三者各自匹配自己的 trait，同名不冲突。
2. `-77` 的十六进制是 `-4d`（小写）。
3. `-88` 的二进制是 `-1011000`。
4. `to_base(16, small, 2)` 放不下 `-4d\0`，返回 `NULL`。
5. `Char` 只挂在 `char` 上；把 `char` 传给 `int` 宿主不会自动发生。

### 5.2 `to_string` 的四个槽

`to_string()` 不分配，返回指向静态 `char[4][32]` 的指针，四次调用轮转：

- `char *r1 = a.to_string();` 拿槽 0，`char *r2 = b.to_string();` 拿槽 1；
- `r1` 仍然有效；第五个活结果会覆盖槽 0；
- 同一表达式里两次调用拿两个不同槽，所以 `printf("%s %s", a.to_string(), b.to_string())` 安全；
- 超过四个同时存活的结果，最早的那个被覆盖；
- **非线程安全**：多线程各自调用会踩同一组槽；
- 需要明确生命周期或线程安全时用 `to_str(buf, cap)`。

### 5.3 `to_str` 截断，`to_base` 不截断

`char buf[4]; int v = 12345;` 下调用 `v.to_str(buf, 4)`，`buf` 里是 `"123"`，静默截断。
同样空间下 `v.to_base(10, small, 2)`（`char small[2];`）返回 `NULL`，一个字节都不写。

这是刻意的差异：十进制展示可以截断，进制转换的结果不应该悄悄变短。

### 5.4 `char` 与 `int` 是不同宿主

`char` 不会提升成 `int` 去匹配 `Int`：

`char c = '7';` 时 `c.is_digit()` 走 `Char`；`c.clamp(0, 9)` 会报
`no method 'clamp' for type 'char'`。

需要数值运算时显式写 `int n = c - '0';`，再对 `n` 调 `Int` 的方法。

### 5.5 溢出与边界

- `abs(INT_MIN)` / `abs(LONG_MIN)` 在 C 里是未定义行为，本模块不额外检查；
- `clamp(lo, hi)` 要求 `lo <= hi`，反序不会报错，但结果没有意义；
- `to_base` 的 base 超出 2..36 返回空串，不报错；
- 所有 `toXxx` 都不设置 errno。

## 6. 常见配方

### 十六进制 / 二进制 / 负数

```ct
import std::num::Num;
import std::io::Io;

int main(void)
{
    char b16[32];
    char b2[32];
    char bneg[32];
    int a = 255;
    int b = 10;
    int c = -1;

    std::io::printf("%s %s %s\n",
                    a.to_base(16, b16, 32),
                    b.to_base(2, b2, 32),
                    c.to_base(2, bneg, 32));
    return 0;
}
```

输出：

```text
ff 1010 -1
```

每个 `to_base` 用独立缓冲区：如果三个调用共用同一块 `buf`，三个返回值都指向它，打印结果会互相覆盖。

### 其它常用组合

| 任务 | 写法 |
| --- | --- |
| 十进制文本 | `n.to_string()` 或 `n.to_str(buf, sizeof buf)` |
| 十六进制 | `n.to_base(16, buf, sizeof buf)` |
| 夹取范围 | `n.clamp(0, 100)` |
| 两数较大 / 较小 | `a.max(b)` / `a.min(b)` |
| 判断数字字符 | `c.is_digit()` |
| 字符转大写 | `(char)c.uppercase()` |
| 字符转数值 | `c - '0'`（先确认 `c.is_digit()`） |

## 7. 排错

### 7.1 字面量接收者（0.19.1）

```ct
import std::num::Num;

int main(void)
{
    char *s = (5).to_string();
    return s != 0;
}
```

```text
main.ct:5:1: error: method receiver must be a variable, an element, a call or a field chain
```

先写 `int n = 5; char *s = n.to_string();`。字面量接收者需要 0.20.0。

### 7.2 在 `char` 上调用整数方法

```ct
import std::num::Num;

int main(void)
{
    char c = '7';
    return c.clamp(0, 3);
}
```

```text
main.ct:6:1: error: no method 'clamp' for type 'char'
```

`clamp` 属于 `Int` / `Long` / `LLong`，宿主是整数；`char` 只有 `Char` 的 ctype 方法。

### 7.3 忘了 import

```ct
int main(void)
{
    int n = 5;
    return n.to_string() != 0;
}
```

```text
main.ct:4:1: error: no method 'to_string' for type 'int'
```

`int` 宿主来自 `std::num::Int`，必须 `import std::num::Num;`。

## 8. 限制与未支持

- **没有 `unsigned` 宿主**：`unsigned int` / `unsigned long` / `size_t` 都不匹配这四个 trait。
- **没有浮点宿主**：`float` / `double` 的数值方法在 [`std::math`](../math/README.md)。
- **`to_string` 非线程安全**：四个静态槽，第五个活结果覆盖第一个。
- **`to_str` 静默截断**：容量不够时得到短文本，不报错；需要完整结果用 `to_base` 或更大缓冲区。
- **`to_base` 只支持 2..36**：非法 base 返回空串。
- **`abs` 不防溢出**：`INT_MIN` / `LONG_MIN` 是 C 的未定义行为。
- **`char` 不提升**：不要期待 `char` 变量匹配 `Int`。
- **字面量接收者需要 0.20.0**。
- **多给实参不报错**：与其它映射模块相同的编译器行为（SH-56）。

## 9. 版本与变更记录

| 版本 | 状态 | 变更 |
| --- | --- | --- |
| `0.1.0` | 未打 tag、未发布 | 当前版本。新增 `Int`（`int`）宿主，补上最常用的整数类型；此前只有 `Long` / `LLong` / `Char`。文档重写：教程并入本 README，删除 `docs/tutorial.md`。 |

编译器下限 `ct >= 0.30.0`。
