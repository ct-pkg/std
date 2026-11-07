# `std::fs` — `char *` 上的文件系统方法（`FsOps`）

路径就是接收者：`path.exists()` / `path.readFile(out, cap)` / `path.writeFile(data)` / `path.list(out, cap)`。
实现是 `static inline`，调用点直接展开成 `std_fs_*`，没有 wrapper、没有 vtable。

## 1. 定位与边界

- 只做四件事：判断存在、整文件读、整文件写、列目录。
- **不分配内存**：`readFile` / `list` 写进调用方提供的 `out + cap`，放不下返回 `-1`。
- `writeFile` 是**整文件替换**，不是追加；内容按 `strlen` 写，不能含 NUL 字节。
- 依赖 POSIX 的 `<dirent.h>` / `<sys/stat.h>`；`list` 在 Windows 上不可用。
- 不提供 `mkdir` / `remove` / `rename` / 递归遍历 / 文件属性；需要时直接用 C API。

## 2. 依赖与安装

依赖 POSIX 头文件 `<stdio.h>` / `<dirent.h>` / `<sys/stat.h>`；`ct.pkg` 的约束是
`requires ct >= 0.19.1`。

| 文件 | 内容 |
| --- | --- |
| `Fs.m.ct` | `pub trait FsOps` + `map trait FsOps for type char *`（4 个方法） |
| `Fs.shim.h` | `std_fs_*` 的 `static inline` 实现，编译器自动 include |

没有 `Fs.h.ct`：本模块不引入新类型，路径本身就是 `char *`。

引入方式：`import std::fs::Fs;`。

包安装后（详见[包根 README](../../README.md#安装)）：

```sh
ct get std@^0.1.0
ct build src/main.ct -I vendor/std -o main --cflags "-Wall -Wextra"
```

## 3. 快速开始

```ct
import std::fs::Fs;
import std::io::Io;

int main(void)
{
    char buf[64];
    char *path = "/tmp/std-fs-demo.txt";
    int n;

    path.writeFile("hello");
    n = path.readFile(buf, 64);
    std::io::printf("exists=%d bytes=%d data=%s\n", path.exists(), n, buf);
    return 0;
}
```

`ct build` 实际执行的 `cc` 行（本机实测，路径随检出位置变化）：

```
'cc' -std=c99 -Wall -Wextra -I '/Users/samxx/vscspace/ct-pkg/std/std/fs' '.ctbuild/main.c' '.ctbuild/Io.c' '-o' '/tmp/std-fs'
```

输出：

```text
exists=1 bytes=5 data=hello
```

字面量接收者（`"/tmp/x".exists()`）需要 **0.20.0**；v0.19.1 先赋给 `char *` 变量。

## 4. API 全表

宿主：`char *`（`const char *` 同样匹配）。接收者是第一个参数。

| 方法 | 签名 | 返回 / 失败 |
| --- | --- | --- |
| `exists()` | `int exists()` | `stat` 能看到返回 1，否则 0；`NULL` 或空串返回 0 |
| `readFile(out, cap)` | `int readFile(char *out, int cap)` | 读到的字节数，并补 NUL；打不开或放不下（含 NUL）返回 `-1` |
| `writeFile(data)` | `int writeFile(const char *data)` | 成功 0；打不开、写失败或 `data` 为 `NULL` 返回 `-1` |
| `list(out, cap)` | `int list(char *out, int cap)` | 条目数；目录打不开或列表放不下返回 `-1`（不返回部分结果） |

`readFile` 的 `cap` 包含结尾 NUL：5 字节的文件至少需要 `cap >= 6`。

`list` 用 `'\n'` 连接条目，**结尾没有换行**，跳过 `.` 和 `..`；顺序是 `readdir` 的顺序，
不排序。返回值是条目数，不是字节数。

## 5. 教程

### 5.1 读写一个文件

```ct
import std::fs::Fs;
import std::io::Io;

int main(void)
{
    char buf[64];
    char *path = "/tmp/std-fs-demo.txt";

    path.writeFile("hello");
    int n = path.readFile(buf, 64);
    std::io::printf("bytes=%d data=%s\n", n, buf);
    return 0;
}
```

输出：

```text
bytes=5 data=hello
```

`writeFile` 只写 `strlen(data)` 个字节，所以 `"hello"` 的 5 个字节全部落盘，没有额外内容。
写完后 `readFile` 自动补 `buf[n] = 0`。

### 5.2 失败路径与容量

```ct
#include <sys/stat.h>

import std::fs::Fs;
import std::str::String;
import std::io::Io;

int main(void)
{
    char buf[32];
    char listing[128];
    char *dir = "/tmp/std-fs-demo-dir";
    char *pa = "/tmp/std-fs-demo-dir/a.txt";
    char *pb = "/tmp/std-fs-demo-dir/b.txt";
    char *missing = "/no/such/file";
    int n;
    int has_a;
    int has_b;

    mkdir(dir, 0755);
    pa.writeFile("A");
    pb.writeFile("B");

    n = dir.list(listing, 128);
    has_a = listing.find("a.txt") >= 0;
    has_b = listing.find("b.txt") >= 0;
    std::io::printf("list count=%d has_a=%d has_b=%d\n", n, has_a, has_b);

    n = pa.readFile(buf, 32);
    std::io::printf("read bytes=%d data=%s\n", n, buf);
    std::io::printf("tiny rc=%d missing rc=%d\n",
                    pa.readFile(buf, 1), missing.readFile(buf, 32));

    pa.writeFile("oo");
    n = pa.readFile(buf, 32);
    std::io::printf("rewrite bytes=%d data=%s\n", n, buf);
    return 0;
}
```

输出：

```text
list count=2 has_a=1 has_b=1
read bytes=1 data=A
tiny rc=-1 missing rc=-1
rewrite bytes=2 data=oo
```

要点：

1. `readFile` 的 `cap` 必须容得下文件内容 **加 NUL**；`cap = 1` 连 1 字节的文件都读不了。
2. 文件不存在、目录不存在、权限不足都统一返回 `-1`，没有 errno 出口。
3. `list` 放不下时返回 `-1`，不返回部分列表；要么完整，要么失败。
4. 再写一次是**替换**，不是追加：`"oo"` 覆盖了原来的 `"A"`。

这里的 `mkdir` 来自 C 的 `<sys/stat.h>`，不是本模块的 API；列目录需要目录先存在。

### 5.3 `list` 的输出格式

```text
a.txt\nb.txt
```

- 条目之间一个 `'\n'`，最后一个条目后面没有换行；
- `.` 和 `..` 被跳过；
- 不排序，不要假设字母序；
- 不区分文件、目录、符号链接，名字原样输出。

按行拆分时用 [`std::str`](../str/README.md) 的 `findChar('\n')` 或 `find`。

## 6. 常见配方

### 追加内容（读—拼—写）

```ct
import std::fs::Fs;
import std::str::String;
import std::io::Io;

int main(void)
{
    char buf[64];
    char next[64];
    char *path = "/tmp/std-fs-append.txt";
    int n;

    path.writeFile("one");
    n = path.readFile(buf, 64);
    buf.concat("two", next, 64);
    path.writeFile(next);
    n = path.readFile(buf, 64);
    std::io::printf("bytes=%d data=%s\n", n, buf);
    return 0;
}
```

输出：

```text
bytes=6 data=onetwo
```

本模块没有 append 模式；追加必须整读、拼接、整写，因此文件不能超过 `cap`。

### 其它常用组合

| 任务 | 写法 |
| --- | --- |
| 判断路径是否存在 | `if (path.exists()) { ... }` |
| 整文件读入 | `n = path.readFile(buf, sizeof buf); if (n < 0) { /* 太大或打不开 */ }` |
| 整文件替换 | `path.writeFile(data)` |
| 列目录 | `n = dir.list(buf, sizeof buf);` |
| 遍历列目录结果 | 用 `std::str` 的 `findChar('\n')` 逐行切 |

## 7. 排错

### 7.1 少给参数

```ct
import std::fs::Fs;

int main(void)
{
    char buf[8];
    char *p = "/tmp/x";
    return p.readFile(buf);
}
```

```text
main.ct:7:1: error: a map call has fewer arguments than its binding
```

`readFile` 需要 `out` 和 `cap` 两个实参。

### 7.2 忘了 import

```ct
int main(void)
{
    char *p = "/tmp/x";
    return p.exists();
}
```

```text
main.ct:4:1: error: no method 'exists' for type 'char *'
```

`exists` 属于 `std::fs::Fs`，import 之后才存在。

### 7.3 字面量接收者（0.19.1）

```ct
import std::fs::Fs;

int main(void)
{
    return "/tmp/x".exists();
}
```

```text
main.ct:5:1: error: method receiver must be a variable, an element, a call or a field chain
```

先写 `char *p = "/tmp/x"; p.exists();`。字面量接收者需要 0.20.0。

## 8. 限制与未支持

- **POSIX only**：`list` 用 `<dirent.h>`，Windows 不适用。
- **不分配**：`readFile` / `list` 全靠调用方缓冲区；大文件请自己分块读，或换用
  [`std::io`](../io/README.md) 的 `File.read`。
- **`writeFile` 按 `strlen` 写**：内容里有 `'\0'` 时后面的字节不会落盘；二进制数据用
  `std::io` 的 `File.write(buf, n)`。
- **`list` 不排序、不递归、不给类型**：只输出名字，顺序来自 `readdir`。
- **没有 errno / 错误码**：所有失败都是 `-1`（`exists` 是 0），无法区分原因。
- **`char *` 方法名全局共享**：本模块占用 `exists` / `readFile` / `writeFile` / `list`；
  `len` / `find` 等属于 [`std::str`](../str/README.md)，`open` 属于
  [`std::io`](../io/README.md)，`decode` / `codepointCount` / `encode` 属于
  [`std::utf8`](../utf8/README.md)。
- **字面量接收者需要 0.20.0**。
- **多给实参不报错**：与其它映射模块相同的编译器行为（SH-56）。

## 9. 版本与变更记录

| 版本 | 状态 | 变更 |
| --- | --- | --- |
| `0.1.0` | 未打 tag、未发布 | 当前版本。文档重写：教程并入本 README，删除 `tutorial.md`；API 未变。 |

编译器下限 `ct >= 0.19.1`。
