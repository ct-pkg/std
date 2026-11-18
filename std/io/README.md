# `std::io` — 标准输出与文件（`Stdout` / `File`）

`stdio.h` 的一层薄映射：命名空间上的 `puts` / `putchar` / `getchar` / `fflush`，以及
`"path".open_file(mode)` 拿到的 `IoFile *` 与 `read` / `write` / `close`。
全是静态映射，调用点直接展开成 C 调用。

## 1. 定位与边界

- 只包装 C 标准库的 `stdio`：不做缓冲管理、不做路径拼接、不做编码转换。
- `File` 不是堆对象：`open_file` 返回不透明句柄 `IoFile *`，`close` 关闭它。
- `read` / `write` 按字节工作，**不补 NUL**。
- 没有变参格式化函数：需要拼接的输出先用 `std::num` 的 `to_str` 得到文本，
  再用 `std::str` 的 `concat` 拼好，最后交给 `Stdout.puts`。
- 不提供目录操作、删除、重命名；这些用 [`std::fs`](../fs/README.md) 或直接 C API。

## 2. 依赖与安装

依赖 C 标准库的 `<stdio.h>`；`ct.pkg` 的约束是 `requires ct >= 0.30.0`。

| 文件 | 内容 |
| --- | --- |
| `Stdout.m.ct` + `Stdout.shim.h` | `pub trait Stdout` + `map trait Stdout for namespace std::io` |
| `File.m.ct` + `File.shim.h` | `pub type IoFile`、`pub trait Path`（宿主 `char *`）+ `pub trait File`（宿主 `IoFile *`） |
| `File.shim.h` | C tag `ct_io_file` 与 `std_io_*` 实现（`ct_io_file` 只在本文件内可见） |

引入方式（按需）：

- `import std::io::Stdout;` → `puts` / `putchar` / `getchar` / `fflush`；
- `import std::io::File;` → `open_file` / `read` / `write` / `close`。

包安装后（详见[包根 README](../../README.md#安装)）：

```sh
ct get std@^0.2.0
ct build src/main.ct -I vendor/std -o main --cflags "-Wall -Wextra"
```

## 3. 快速开始

```ct
import std::io::Stdout;
import std::io::File;
import std::num::Num;
import std::str::String;

int main(void)
{
    IoFile *f;
    char buf[16];
    char num[16];
    char line[32];
    char *path = "/tmp/std-io-demo.txt";
    int n;

    std::io::Stdout.puts("hello, ct");
    std::io::Stdout.puts("second line");

    f = path.open_file("w");
    if (f == 0) return 1;
    f.write("hello", 5);
    f.close();

    f = path.open_file("r");
    if (f == 0) return 1;
    n = f.read(buf, 15);
    buf[n] = 0;
    f.close();

    n.to_str(num, 16);
    "read bytes=".concat(num, line, 32);
    std::io::Stdout.puts(line);
    std::io::Stdout.puts(buf);
    return 0;
}
```

`ct build` 实际执行的 `cc` 行（本机实测，路径随检出位置变化）：

```
'cc' -std=c99 -Wall -Wextra -I '/Users/samxx/vscspace/ct-pkg/std/std/io' -I '/Users/samxx/vscspace/ct-pkg/std/std/num' -I '/Users/samxx/vscspace/ct-pkg/std/std/str' '.ctbuild/main.c' '-o' '/tmp/std-io'
```

输出：

```text
hello, ct
second line
read bytes=5
hello
```

CT 侧的句柄类型是 `IoFile *`：`File.m.ct` 用 `pub type IoFile = struct ct_io_file;`
给 C 侧的不透明句柄起了一个 CT 私名。C tag 仍是 `struct ct_io_file`，helper 符号
仍是 `std_io_*`，一个字没改。这个别名需要 **0.20.3**；字面量接收者
（`"/tmp/x".open_file("r")`）从 **0.20.0** 起可用（本包下限 0.30.0），v0.19.1 请先赋给 `char *` 变量。

## 4. API 全表

### 命名空间宿主 `std::io`（trait `Stdout`）

调用形态是 `std::io::Stdout.method(...)`。

| 方法 | 签名 | 返回 / 失败 |
| --- | --- | --- |
| `puts(s)` | `int puts(const char *s)` | 非负成功；失败返回 `EOF` |
| `putchar(c)` | `int putchar(int c)` | 写出的字节；失败返回 `EOF` |
| `getchar()` | `int getchar()` | `0..255`；EOF 返回 `-1` |
| `fflush()` | `int fflush()` | 成功 0；失败 `EOF` |

`fflush()` 固定刷新 `stdout`，没有参数。

### 宿主 `char *`（trait `Path`）

| 方法 | 签名 | 返回 / 失败 |
| --- | --- | --- |
| `open_file(mode)` | `IoFile *open_file(const char *mode)` | 成功返回句柄；失败返回 `NULL`（`0`） |

`mode` 就是 `fopen` 的 mode：`"r"` / `"w"` / `"a"` / `"rb"` / `"wb"` …

### 宿主 `IoFile *`（trait `File`）

| 方法 | 签名 | 返回 / 失败 |
| --- | --- | --- |
| `read(buf, n)` | `int read(char *buf, int n)` | 实际读到的字节数；EOF 返回 0；句柄/缓冲区为空或 `n < 0` 返回 `-1` |
| `write(buf, n)` | `int write(const char *buf, int n)` | 实际写出的字节数；失败或句柄已关闭返回 `-1` |
| `close()` | `int close()` | 成功或已关闭返回 0；`fclose` 失败返回 `-1` |

`close()` 是幂等的：`NULL` 句柄和已关闭句柄都返回 0。

## 5. 教程

### 5.1 两个 import 各管什么

- `std::io::Stdout` → `puts` / `putchar` / `getchar` / `fflush`；
- `std::io::File` → `char *` 上的 `open_file` 与 `IoFile *` 上的 `read` / `write` / `close`。

只 import 不调用不会产生任何代码。

### 5.2 文件生命周期与失败路径

```ct
import std::io::Stdout;
import std::io::File;
import std::num::Num;
import std::str::String;

int main(void)
{
    IoFile *f;
    IoFile *missing;
    char buf[16];
    char num[16];
    char line[48];
    char *path = "/tmp/std-io-tutorial.txt";
    char *bad = "/no/such/file";
    int n;
    int rc;

    missing = bad.open_file("r");
    rc = missing == 0;
    "missing null=".concat(rc.to_str(num, 16), line, 48);
    std::io::Stdout.puts(line);
    "close(NULL)=".concat(missing.close().to_str(num, 16), line, 48);
    std::io::Stdout.puts(line);

    f = path.open_file("w");
    "write=".concat(f.write("hello", 5).to_str(num, 16), line, 48);
    std::io::Stdout.puts(line);
    "close1=".concat(f.close().to_str(num, 16), line, 48);
    std::io::Stdout.puts(line);
    "close2=".concat(f.close().to_str(num, 16), line, 48);
    std::io::Stdout.puts(line);
    "write-after-close=".concat(f.write("x", 1).to_str(num, 16), line, 48);
    std::io::Stdout.puts(line);

    f = path.open_file("r");
    n = f.read(buf, 15);
    buf[n] = 0;
    "read=".concat(n.to_str(num, 16), line, 48);
    std::io::Stdout.puts(line);
    std::io::Stdout.puts(buf);
    "eof=".concat(f.read(buf, 1).to_str(num, 16), line, 48);
    std::io::Stdout.puts(line);
    f.close();
    return 0;
}
```

输出：

```text
missing null=1
close(NULL)=0
write=5
close1=0
close2=0
write-after-close=-1
read=5
hello
eof=0
```

要点：

1. `open_file` 失败返回 `NULL`，不是错误码；必须 `if (f == 0)`。
2. `read` 返回字节数，**不补 NUL**；`buf[n] = 0;` 由调用方写。
3. 读到文件末尾返回 0，不是 `-1`。
4. `close` 幂等；关闭后再 `write` 返回 `-1`，不会写进文件。
5. `write` / `read` / `close` 的返回值都是 `int`，可以直接 `to_str` 打印；`puts` 只接受现成字符串。

### 5.3 需要格式化输出时

本模块没有变参函数，格式化分两步：

1. 数值先用 `std::num` 的 `to_str(buf, cap)`（或 `std::math` 的 `to_str`）变成文本；
2. 用 `std::str` 的 `concat` 把前缀、文本、后缀拼进一个缓冲区，再 `std::io::Stdout.puts(...)`。

`.ct` 文件不允许 `#include`，所以不能借 C 的格式化函数；拼接必须走上面两条既有 API。

### 5.4 句柄的所有权

`open_file` 分配一个包装体（一个 `FILE *` + 关闭状态），`close` 只 `fclose` 并把内部指针置空，
**不释放包装体**。这是为了 `close` 幂等：第二次 `close` 读到的仍是合法内存。

代价是每次 `open_file` 约 16 字节不回收。长期运行且反复开关文件的程序应改成“一个句柄用到底”，
而不是把 `open_file` / `close` 放进热循环。

## 6. 常见配方

### 复制一个文件

```ct
import std::io::File;
import std::io::Stdout;
import std::num::Num;
import std::str::String;

int main(void)
{
    IoFile *in;
    IoFile *out;
    char buf[64];
    char num[16];
    char line[32];
    char *src = "/tmp/std-io-src.txt";
    char *dst = "/tmp/std-io-dst.txt";
    int n;

    in = src.open_file("w");
    in.write("copy me", 7);
    in.close();

    in = src.open_file("r");
    out = dst.open_file("w");
    n = in.read(buf, 64);
    out.write(buf, n);
    in.close();
    out.close();

    in = dst.open_file("r");
    n = in.read(buf, 64);
    buf[n] = 0;
    in.close();

    "copied=".concat(n.to_str(num, 16), line, 32);
    std::io::Stdout.puts(line);
    std::io::Stdout.puts(buf);
    return 0;
}
```

输出：

```text
copied=7
copy me
```

真实文件通常大于一个缓冲区，把上面的单次 `read` / `write` 换成循环：
每次 `n = in.read(buf, 64)` 后执行 `out.write(buf, n)`，直到 `read` 返回 0（EOF）或负数（错误）。
`read` 返回 0 是正常结束，不要当失败。

### 其它常用组合

| 任务 | 写法 |
| --- | --- |
| 输出一行 | `std::io::Stdout.puts("done")` |
| 输出单个字符 | `std::io::Stdout.putchar('x')` |
| 逐字节读取 | `int c = std::io::Stdout.getchar();` |
| 追加写 | `f = path.open_file("a");` |
| 二进制读 | `f = path.open_file("rb");` |
| 读满一个缓冲区 | `n = f.read(buf, sizeof buf);` |

## 7. 排错

### 7.1 少给参数

```ct
import std::io::File;

int main(void)
{
    IoFile *f;
    char buf[8];
    char *p = "/tmp/x";
    f = p.open_file("r");
    return f.read(buf);
}
```

```text
main.ct:9:1: error: a map call has fewer arguments than its binding
```

`read` 需要 `buf` 和 `n` 两个实参。

### 7.2 忘了 import

```ct
int main(void)
{
    char *p = "/tmp/x";
    IoFile *f = p.open_file("r");
    return f == 0;
}
```

```text
main.ct:4:1: error: no method 'open_file' for type 'char *'
```

`open_file` 属于 `std::io::File`，必须 `import std::io::File;` 之后才存在于 `char *` 上。

### 7.3 字面量接收者（0.19.1）

```ct
import std::io::File;

int main(void)
{
    IoFile *f = "/tmp/x".open_file("r");
    return f == 0;
}
```

```text
main.ct:5:1: error: method receiver must be a variable, an element, a call or a field chain
```

先写 `char *p = "/tmp/x"; f = p.open_file("r");`。字面量接收者需要 0.20.0。

## 8. 限制与未支持

- **没有目录 / 删除 / 重命名**：本模块只做 `stdio`。
- **`open_file` 的包装体不释放**：`close` 只关文件，约 16 字节/次不回收。
- **`read` 不补 NUL**：必须自己写 `buf[n] = 0;`，且 `n` 可能为 `-1`。
- **`write` / `read` 不检查缓冲区容量**：`n` 给大了就是越界。
- **没有格式化函数**：拼接文本要先用 `to_str` 再 `concat`；写错参数个数由编译器按映射检查。
- **`char *` 方法名全局共享**：本模块占用 `open_file`；`length` / `find` 等属于
  [`std::str`](../str/README.md)，`exists` / `read_file` / `write_file` / `list` 属于
  [`std::fs`](../fs/README.md)，`decode` / `count_codepoints` / `encode` 属于
  [`std::utf8`](../utf8/README.md)。
- **`IoFile *` 是唯一句柄类型**：不要用 `void *` 当句柄宿主——`void *` 无法限定调用
  范围，任何 `void *` 变量都会凭空获得 `read` / `write` / `close`，别的包一旦也挂
  `void *` 宿主就会歧义。
- **字面量接收者需要 0.20.0**。
- **多给实参不报错**：与 `std::str` 相同的编译器行为（SH-56）。

## 9. 版本与变更记录

| 版本 | 状态 | 变更 |
| --- | --- | --- |
| `0.1.0` | 未打 tag、未发布 | 当前版本。文档重写，教程并入本 README，删除 `docs/tutorial.md`。句柄在 CT 侧为 `IoFile *`（`pub type IoFile = struct ct_io_file;`；C tag 仍 `struct ct_io_file`）。 |

编译器下限 `ct >= 0.30.0`。
