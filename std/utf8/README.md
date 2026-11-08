# `std::utf8` — UTF-8 编解码（`Utf8Ops`）

在 `char *` 上按 UTF-8 解码、数码点、编码：`s.decode(&cp)` / `s.codepointCount()` /
`buf.encode(cp, cap)`。严格校验，不分配内存。

## 1. 定位与边界

- 只做 UTF-8 与码点（`int`）之间的转换，以及码点计数。
- **严格解码**：拒绝坏续字节、overlong、代理区（U+D800..U+DFFF）和超过 U+10FFFF 的值。
- **不分配**：`encode` 写进调用方缓冲区，`cap` 不够返回 `-1`。
- 不做 Unicode 规范化、字素簇、大小写折叠、分类；只认码点。
- `encode` 的接收者是**输出缓冲区**（`buf.encode(cp, cap)`），不是输入文本。

## 2. 依赖与安装

只依赖 C 的 `<string.h>` 级操作，无外部库；`ct.pkg` 的约束是 `requires ct >= 0.20.3`。

| 文件 | 内容 |
| --- | --- |
| `Utf8Ops.m.ct` | `pub trait Utf8Ops` + `map trait Utf8Ops for type char *`（3 个方法） |
| `Utf8Ops.shim.h` | `std_utf8_*` 的 `static inline` 实现，编译器自动 include |

引入方式：`import std::utf8::Utf8Ops;`。

包安装后（详见[包根 README](../../README.md#安装)）：

```sh
ct get std@^0.2.0
ct build src/main.ct -I vendor/std -o main --cflags "-Wall -Wextra"
```

## 3. 快速开始

```ct
import std::utf8::Utf8Ops;
import std::str::String;
import std::io::Io;

int main(void)
{
    char buf[8];
    char *s = "é";
    int cp;
    int n;

    n = s.decode(&cp);
    std::io::printf("bytes=%d cp=%d len=%d count=%d\n",
                    n, cp, s.len(), s.codepointCount());

    n = buf.encode(cp, 8);
    buf[n] = 0;
    std::io::printf("encoded=%d text=%s\n", n, buf);
    return 0;
}
```

`ct build` 实际执行的 `cc` 行（本机实测，路径随检出位置变化）：

```
'cc' -std=c99 -Wall -Wextra -I '/Users/samxx/vscspace/ct-pkg/std/std/utf8' -I '/Users/samxx/vscspace/ct-pkg/std/std/str' '.ctbuild/main.c' '.ctbuild/Io.c' '-o' '/tmp/std-utf8'
```

输出：

```text
bytes=2 cp=233 len=2 count=1
encoded=2 text=é
```

同一个 `"é"`：`len()`（字节）是 2，`codepointCount()`（码点）是 1。
字面量接收者（`"é".decode(&cp)`）从 **0.20.0** 起可用（本包下限 0.20.3）；v0.19.1 先赋给 `char *`。

## 4. API 全表

宿主：`char *`（`const char *` 同样匹配）。接收者是第一个参数。

| 方法 | 签名 | 返回 / 失败 |
| --- | --- | --- |
| `decode(cp)` | `int decode(int *cp)` | 消耗的字节数（1..4）；不是合法 UTF-8 返回 `-1`；`cp` 可为 `NULL`（只求步长） |
| `codepointCount()` | `int codepointCount()` | 码点数；`NULL` 返回 0；不是合法 UTF-8 返回 `-1` |
| `encode(cp, cap)` | `int encode(int cp, int cap)` | 写出的字节数（1..4）；码点非法或 `cap` 不够返回 `-1` |

`decode` 的输入是接收者本身；`cp` 是输出参数，成功时写入第一个码点。
`encode` 的接收者是**输出缓冲区**，例如 `char buf[8]; buf.encode(8364, 8)`。
`encode` **不写结尾 NUL**，只返回写出的字节数。

## 5. 教程

### 5.1 字节 vs 码点

| 文本 | `len()`（`std::str`） | `codepointCount()`（本模块） |
| --- | --- | --- |
| `"A"` | 1 | 1 |
| `"é"` | 2 | 1 |
| `"€"` | 3 | 1 |
| `"😀"`（U+1F600） | 4 | 1 |

两个名字不能互换：`String.len` 数字节，`Utf8Ops.codepointCount` 数码点。

### 5.2 解码一个码点

`decode` 返回“这次吃掉几个字节”，成功时把码点写进 `int` 输出参数：

```ct
import std::utf8::Utf8Ops;
import std::io::Io;

int main(void)
{
    char *s = "Aé€";
    char *p;
    int cp;
    int i = 0;
    int step;

    while (s[i]) {
        p = s + i;
        step = p.decode(&cp);
        if (step < 0) break;
        std::io::printf("cp=%d bytes=%d\n", cp, step);
        i += step;
    }
    return 0;
}
```

输出：

```text
cp=65 bytes=1
cp=233 bytes=2
cp=8364 bytes=3
```

注意接收者是 `p = s + i`，不是 `s`；`decode` 永远只看开头那个码点。
`&cp` 也可以传 `0`（`NULL`），只求步长、不需要码点。

### 5.3 严格校验：哪些输入会被拒绝

```ct
import std::utf8::Utf8Ops;
import std::str::String;
import std::io::Io;

int main(void)
{
    char buf[8];
    char *s = "héllo";
    char *bad1 = "\xC0\x80";
    char *bad2 = "\x80";
    char *p;
    int cp;
    int i = 0;
    int n = 0;
    int step;

    while (s[i]) {
        p = s + i;
        step = p.decode(&cp);
        if (step < 0) break;
        i += step;
        n++;
    }
    std::io::printf("walk cp=%d bytes=%d\n", n, i);

    std::io::printf("overlong=%d lone=%d count_bad=%d\n",
                    bad1.decode(&cp), bad2.decode(&cp), bad2.codepointCount());

    step = buf.encode(8364, 8);
    buf[step] = 0;
    std::io::printf("euro bytes=%d text=%s\n", step, buf);
    std::io::printf("surrogate=%d too_big=%d\n",
                    buf.encode(0xD800, 8), buf.encode(0x110000, 8));
    return 0;
}
```

输出：

```text
walk cp=5 bytes=6
overlong=-1 lone=-1 count_bad=-1
euro bytes=3 text=€
surrogate=-1 too_big=-1
```

被拒绝的四类：

1. **坏续字节**：`"\x80"` 不是任何序列的开头，返回 `-1`。
2. **overlong**：`"\xC0\x80"` 用两个字节表示 U+0000，违反最短编码，返回 `-1`。
3. **代理区**：`0xD800..0xDFFF` 不是合法码点，`encode` 返回 `-1`。
4. **超出范围**：大于 `0x10FFFF`，`encode` 返回 `-1`。

`codepointCount` 遇到非法序列立即返回 `-1`，不会给出“前面有几个合法码点”。

### 5.4 编码

`char buf[8]; int n = buf.encode(8364, 8);` 之后 `n` 是 3，`buf` 的前 3 个字节是 `€` 的
UTF-8；再写 `buf[n] = 0;` 才得到 C 字符串。

- 返回值是写出的字节数（`€` 是 3）；
- 不写 NUL，自己补 `buf[n] = 0;`；
- `cap < 需要的字节数` 返回 `-1`，不写部分结果；
- `cp < 0`、代理区、`> 0x10FFFF` 返回 `-1`。

### 5.5 与 `std::str` 的分工

- 需要字节偏移、字节长度、字节切分：`std::str`。
- 需要码点、按字符遍历、码点计数：本模块。
- 按码点切分没有内置方法：先 `decode` 走一遍，记录字节偏移，再用 `slice` 切。

## 6. 常见配方

### 遍历并打印每个码点

```ct
import std::utf8::Utf8Ops;
import std::io::Io;

int main(void)
{
    char *s = "Aé€";
    char *p;
    int cp;
    int i = 0;
    int step;

    while (s[i]) {
        p = s + i;
        step = p.decode(&cp);
        if (step < 0) break;
        std::io::printf("cp=%d bytes=%d\n", cp, step);
        i += step;
    }
    return 0;
}
```

输出：

```text
cp=65 bytes=1
cp=233 bytes=2
cp=8364 bytes=3
```

（这与 5.2 的程序相同，配方表里单独列出便于复制。）

### 其它常用组合

| 任务 | 写法 |
| --- | --- |
| 数码点 | `int n = s.codepointCount();` |
| 判断是否为合法 UTF-8 | `if (s.codepointCount() >= 0) { ... }` |
| 读第一个码点 | `int cp; int step = s.decode(&cp);` |
| 只求步长 | `int step = s.decode(0);` |
| 编码一个码点 | `int n = buf.encode(cp, sizeof buf);`，再 `buf[n] = 0;` |
| 字节长度 | `int bytes = s.len();`（`std::str`） |

## 7. 排错

### 7.1 少给参数

```ct
import std::utf8::Utf8Ops;

int main(void)
{
    char *s = "abc";
    int n = s.decode();
    return n;
}
```

```text
main.ct:7:1: error: a map call has fewer arguments than its binding
```

`decode` 需要一个 `int *` 输出参数；只求步长也要显式写 `s.decode(0)`。

### 7.2 忘了 import

```ct
int main(void)
{
    char *s = "abc";
    return s.codepointCount();
}
```

```text
main.ct:4:1: error: no method 'codepointCount' for type 'char *'
```

`codepointCount` 属于 `std::utf8::Utf8Ops`，import 之后才存在。

### 7.3 字面量接收者（0.19.1）

```ct
import std::utf8::Utf8Ops;

int main(void)
{
    return "é".codepointCount();
}
```

```text
main.ct:5:1: error: method receiver must be a variable, an element, a call or a field chain
```

先写 `char *s = "é"; s.codepointCount();`。字面量接收者需要 0.20.0。

### 7.4 把 `encode` 的接收者写反

```ct
import std::utf8::Utf8Ops;

int main(void)
{
    int cp = 8364;
    int n = cp.encode(cp, 8);
    return n;
}
```

```text
main.ct:6:1: error: no method 'encode' for type 'int'
```

`encode` 的宿主是 `char *`（输出缓冲区），不是 `int`。正确写法是
`char buf[8]; int n = buf.encode(cp, 8);`。

## 8. 限制与未支持

- **严格 UTF-8**：overlong、代理区、超范围一律 `-1`；不提供“宽松解码”开关。
- **不分配**：`encode` 只写调用方缓冲区；没有返回新字符串的 API。
- **`encode` 不补 NUL**：必须自己写 `buf[n] = 0;`。
- **不做规范化 / 字素簇 / 分类**：`"é"` 的两种 Unicode 表示都合法，但本模块不合并。
- **`codepointCount` 遇错即停**：非法输入返回 `-1`，不是“合法前缀的码点数”。
- **`char *` 方法名全局共享**：本模块占用 `decode` / `codepointCount` / `encode`；
  `len` / `find` 等属于 [`std::str`](../str/README.md)，`open` 属于
  [`std::io`](../io/README.md)，`exists` / `readFile` / `writeFile` / `list` 属于
  [`std::fs`](../fs/README.md)。
- **字面量接收者从 0.20.0 起可用**（本包下限 0.20.3）；0.19.1 只接受变量、元素、调用和字段链。
- **多给实参不报错**：与其它映射模块相同的编译器行为（SH-56）。

## 9. 版本与变更记录

| 版本 | 状态 | 变更 |
| --- | --- | --- |
| `0.1.0` | 未打 tag、未发布 | 当前版本。文档重写：教程并入本 README，删除 `tutorial.md`；API 未变。 |

编译器下限 `ct >= 0.20.3`。
