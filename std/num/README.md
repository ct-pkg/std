# `std::num` — 整数与字符宿主（`Int` / `Long` / `LLong` / `Char`）

给 C 的数值与字符类型挂方法：`n.to_str(buf, cap)` / `n.to_base(16, buf, cap)` / `n.clamp(lo, hi)` /
`c.is_digit()` / `c.uppercase()`。四个宿主各映射一次，同名方法跨宿主允许，同一宿主内唯一。

## 1. 定位与边界

- 覆盖四种宿主：`int`（`Int`）、`long`（`Long`）、`long long`（`LLong`）、`char`（`Char`）。
- 数值宿主提供：`abs` / `clamp` / `max` / `min` / `to_str` / `to_base`。
- `char` 宿主提供 ctype 表面：`is_digit` / `is_alpha` / `is_space` / `is_upper` / `is_lower` /
  `is_alnum` / `is_punct` / `is_print` / `is_graph` / `is_cntrl` / `is_blank` / `is_xdigit` /
  `uppercase` / `lowercase`；`char` 不会提升成 `int`，所以两套方法互不干扰。
- **不分配**：`to_str` / `to_base` 写调用方缓冲区，容量不足的规则见第 4 节。
- 不做格式化宽度、精度、千分位；需要拼接时先用 `to_str` 得到文本，再用 `std::str` 的 `concat`。

## 2. 依赖与安装

依赖 C 的 `<stdlib.h>` / `<ctype.h>` / `<stdio.h>`；`ct.pkg` 的约束是 `requires ct >= 0.67.0`。

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
ct get ct/std@0.20.0
ct build src/main.ct -o main --cflags "-Wall -Wextra"
```

## 3. 快速开始

```ct
import std::num::Num;
import std::str::CharStr;
import std::io::Stdout;

int main(void)
{
    char buf[32];
    char num[16];
    char line[32];
    char up[2];
    int n = 65;
    int lo = 5;
    char digit = '7';
    char letter = 'a';

    n.to_str(buf, 32);
    "int=".concat(buf, line, 32);
    std::io::Stdout.puts(line);

    "base16=".concat(n.to_base(16, buf, 32), line, 32);
    std::io::Stdout.puts(line);

    "clamp=".concat(lo.clamp(0, 3).to_str(num, 16), line, 32);
    std::io::Stdout.puts(line);

    "digit=".concat(digit.is_digit().to_str(num, 16), line, 32);
    std::io::Stdout.puts(line);

    "alpha=".concat(letter.is_alpha().to_str(num, 16), line, 32);
    std::io::Stdout.puts(line);

    up[0] = (char)letter.uppercase();
    up[1] = 0;
    "upper=".concat(up, line, 32);
    std::io::Stdout.puts(line);
    return 0;
}
```

`ct build` 实际执行的 `cc` 行（本机实测，路径随检出位置变化）：

```
'cc' -std=c99 -Wall -Wextra -I '/Users/samxx/vscspace/ct-pkg/std/std/num' -I '/Users/samxx/vscspace/ct-pkg/std/std/str' -I '/Users/samxx/vscspace/ct-pkg/std/std/io' '.ctbuild/main.c' '-o' '/tmp/std-num'
```

输出：

```text
int=65
base16=41
clamp=3
digit=1
alpha=1
upper=A
```

`int` 宿主是后加的：在此之前只有 `Long` / `LLong` / `Char`，`int` 值没有自己的文本化落点。
字面量接收者本身从 **0.20.0** 起可用（本包下限 0.67.0）；v0.19.1 先写 `int n = 5;`，再调
`n.to_str(buf, cap)`。

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
| `to_base(base, out, cap)` | `char *to_base(int base, char *out, int cap)` | 同左 | 同左 | 2..36 进制、小写字母；放不下返回 `NULL` 且不写内容；非法 base 返回空串 |

`to_base` 与 `to_str` 的失败语义不同：

- `to_str`：按容量静默截断，永远返回 `out`；
- `to_base`：**绝不截断**，符号与所有数字放不下时返回 `NULL`；`base < 2 || base > 36` 时写入空串并返回 `out`。

### `char`（`Char`）

| 方法 | 签名 | 返回 |
| --- | --- | --- |
| `is_digit()` | `int is_digit()` | 是数字返回非 0，否则 0 |
| `is_alpha()` | `int is_alpha()` | 是字母返回非 0 |
| `is_space()` | `int is_space()` | 是空白返回非 0 |
| `is_upper()` | `int is_upper()` | 是大写返回非 0 |
| `is_lower()` | `int is_lower()` | 是小写返回非 0 |
| `is_alnum()` | `int is_alnum()` | 字母或数字返回非 0 |
| `is_punct()` | `int is_punct()` | 标点返回非 0 |
| `is_print()` | `int is_print()` | 可打印（含空格）返回非 0 |
| `is_graph()` | `int is_graph()` | 可打印且非空格返回非 0 |
| `is_cntrl()` | `int is_cntrl()` | 控制字符返回非 0 |
| `is_blank()` | `int is_blank()` | 空格或制表符返回非 0 |
| `is_xdigit()` | `int is_xdigit()` | 十六进制数字返回非 0 |
| `uppercase()` | `int uppercase()` | 转换后的字符（`int`）；非字母原样返回 |
| `lowercase()` | `int lowercase()` | 转换后的字符（`int`）；非字母原样返回 |

**契约**：所有接收 `char` 的判定/转换方法都按**无符号字节**解释——实现内部先转成 `unsigned char`
再交给 C 的 ctype。`char` 在多数平台有符号，UTF-8 多字节字符的单个字节（如 `0xC3` 读作 `-61`）
直接传给 ctype 是未定义行为；转换之后行为与平台无关。

返回值只保证「非 0 为真」，不保证等于 1——C 库可能返回任意非零值，需要布尔值时写 `!= 0`。

## 5. 教程

### 5.1 四个宿主，四套同名方法

```ct
import std::num::Num;
import std::str::CharStr;
import std::io::Stdout;

int main(void)
{
    char buf[32];
    char num[16];
    char line[64];
    long lv = 65;
    long long ll = -88;
    int n = -77;
    int a = 5;
    int b = 9;
    int is_null;
    char small[2];

    n.to_str(buf, 32);
    "int=".concat(buf, line, 64);
    std::io::Stdout.puts(line);
    "int base16=".concat(n.to_base(16, buf, 32), line, 64);
    std::io::Stdout.puts(line);
    "clamp=".concat(a.clamp(0, 3).to_str(num, 16), line, 64);
    std::io::Stdout.puts(line);
    "max=".concat(a.max(b).to_str(num, 16), line, 64);
    std::io::Stdout.puts(line);

    "long=".concat(lv.to_str(buf, 32), line, 64);
    std::io::Stdout.puts(line);
    "longlong base2=".concat(ll.to_base(2, buf, 32), line, 64);
    std::io::Stdout.puts(line);

    is_null = n.to_base(16, small, 2) == 0;
    "base small null=".concat(is_null.to_str(num, 16), line, 64);
    std::io::Stdout.puts(line);
    return 0;
}
```

输出：

```text
int=-77
int base16=-4d
clamp=3
max=9
long=65
longlong base2=-1011000
base small null=1
```

要点：

1. `n` 是 `int`，`lv` 是 `long`，`ll` 是 `long long`；三者各自匹配自己的 trait，同名不冲突。
2. `-77` 的十六进制是 `-4d`（小写）。
3. `-88` 的二进制是 `-1011000`。
4. `to_base(16, small, 2)` 放不下 `-4d\0`，返回 `NULL`。
5. `Char` 只挂在 `char` 上；把 `char` 传给 `int` 宿主不会自动发生。

### 5.2 `to_str` 的返回值与 `NULL` 处理

`to_str` 不分配，永远把文本写进调用方缓冲区：成功与截断都返回 `out` 本身，方便直接传给
`concat` 或 `Stdout.puts`。`out == NULL` 或 `cap <= 0` 时原样返回 `out`（可能是 `NULL`），
不写任何字节；需要区分「放不下」时要靠调用方自己检查缓冲区，而不是看返回值。

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
import std::str::CharStr;
import std::io::Stdout;

int main(void)
{
    char b16[32];
    char b2[32];
    char bneg[32];
    char line[32];
    int a = 255;
    int b = 10;
    int c = -1;

    "hex=".concat(a.to_base(16, b16, 32), line, 32);
    std::io::Stdout.puts(line);
    "bin=".concat(b.to_base(2, b2, 32), line, 32);
    std::io::Stdout.puts(line);
    "neg=".concat(c.to_base(2, bneg, 32), line, 32);
    std::io::Stdout.puts(line);
    return 0;
}
```

输出：

```text
hex=ff
bin=1010
neg=-1
```

每个 `to_base` 用独立缓冲区：如果一个调用还没打印就被下一个覆盖，三个结果会互相踩。

### 其它常用组合

| 任务 | 写法 |
| --- | --- |
| 十进制文本 | `n.to_str(buf, sizeof buf)` |
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
    char buf[8];
    char *s = (5).to_str(buf, 8);
    return s != 0;
}
```

```text
main.ct:6:1: error: method receiver must be a variable, an element, a call or a field chain
```

先写 `int n = 5; n.to_str(buf, 8);`。字面量接收者需要 0.20.0。

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
    char buf[8];
    return n.to_str(buf, 8) == 0;
}
```

```text
main.ct:5:1: error: no method 'to_str' for type 'int'
```

`int` 宿主来自 `std::num::Int`，必须 `import std::num::Num;`。

## 8. 限制与未支持

- **没有 `unsigned` 宿主**：`unsigned int` / `unsigned long` / `size_t` 都不匹配这四个 trait。
- **没有浮点宿主**：`float` / `double` 的数值方法在 [`std::math`](../math/README.md)。
- **`to_str` 静默截断**：容量不够时得到短文本，不报错；需要完整结果用 `to_base` 或更大缓冲区。
- **`to_base` 只支持 2..36**：非法 base 返回空串。
- **`abs` 不防溢出**：`INT_MIN` / `LONG_MIN` 是 C 的未定义行为。
- **`char` 不提升**：不要期待 `char` 变量匹配 `Int`。
- **字面量接收者需要 0.20.0**。
- **多给实参不报错**：与其它映射模块相同的编译器行为（SH-56）。

## 9. 版本与变更记录

| 版本 | 状态 | 变更 |
| --- | --- | --- |
| `0.5.0` | 已打 `v0.5.0` | `Char` 新增 7 个谓词：`is_alnum` / `is_punct` / `is_print` / `is_graph` / `is_cntrl` / `is_blank` / `is_xdigit`；契约写明按无符号字节解释。 |
| `0.1.0` | 已打 `v0.1.0` | 新增 `Int`（`int`）宿主，补上最常用的整数类型；此前只有 `Long` / `LLong` / `Char`。文档重写：教程并入本 README，删除 `docs/tutorial.md`。 |

编译器下限 `ct >= 0.67.0`。
