# `std::fs` — `char *` 上的文件系统方法（`FsOps`）

路径就是接收者：`path.exists()` / `path.read_file(out, cap)` / `path.write_file(data)` / `path.list(out, cap)`。
实现是 `static inline`，调用点直接展开成 `std_fs_*`，没有 wrapper、没有 vtable。

## 1. 定位与边界

- 只做七件事：判断存在、整文件读、整文件写、列目录、建目录、删文件或空目录、改名。
- **不分配内存**：`read_file` / `list` 写进调用方提供的 `out + cap`，放不下返回 `-1`。
- `write_file` 是**整文件替换**，不是追加；内容按 `strlen` 写，不能含 NUL 字节。
- 依赖 POSIX 的 `<dirent.h>` / `<sys/stat.h>`；`list` 在 Windows 上不可用。
- 不提供递归删除 / 递归遍历 / 文件属性 / `chmod` / 符号链接操作；需要时直接用 C API。

## 2. 依赖与安装

依赖 POSIX 头文件 `<stdio.h>` / `<dirent.h>` / `<sys/stat.h>`；`ct.pkg` 的约束是
`requires ct >= 0.30.0`。

| 文件 | 内容 |
| --- | --- |
| `Fs.m.ct` | `pub trait FsOps` + `map trait FsOps for type char *`（7 个方法）、`pub trait FsEnv` + `map trait FsEnv for namespace std::fs`（`get_last_error`） |
| `Fs.shim.h` | `std_fs_*` 的 `static inline` 实现，编译器自动 include |

没有 `Fs.h.ct`：本模块不引入新类型，路径本身就是 `char *`。

引入方式：`import std::fs::Fs;`。

包安装后（详见[包根 README](../../README.md#安装)）：

```sh
ct get std@^0.2.0
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

    path.write_file("hello");
    n = path.read_file(buf, 64);
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

字面量接收者（`"/tmp/x".exists()`）从 **0.20.0** 起可用（本包下限 0.30.0）；v0.19.1 先赋给 `char *` 变量。

## 4. API 全表

宿主：`char *`（`const char *` 同样匹配）。接收者是第一个参数。

| 方法 | 签名 | 返回 / 失败 |
| --- | --- | --- |
| `exists()` | `int exists()` | `stat` 能看到返回 1，否则 0；`NULL` 或空串返回 0 |
| `read_file(out, cap)` | `int read_file(char *out, int cap)` | 读到的字节数，并补 NUL；路径是目录、打不开或放不下（含 NUL）返回 `-1` |
| `write_file(data)` | `int write_file(const char *data)` | 成功 0；打不开、写失败或 `data` 为 `NULL` 返回 `-1` |
| `list(out, cap)` | `int list(char *out, int cap)` | 条目数；目录打不开或列表放不下返回 `-1`（不返回部分结果） |
| `mkdir(mode)` | `int mkdir(int mode)` | 成功 0；已存在（`EEXIST`）、父目录不存在、`mode` 非法返回 `-1` |
| `remove()` | `int remove()` | 成功 0；删文件或**空**目录；非空目录（`ENOTEMPTY`）、不存在返回 `-1`，不递归 |
| `rename(to)` | `int rename(const char *to)` | 成功 0；跨文件系统（`EXDEV`）、`to` 非法返回 `-1` |

`read_file` 的 `cap` 包含结尾 NUL：5 字节的文件至少需要 `cap >= 6`。

`list` 用 `'\n'` 连接条目，**结尾没有换行**，跳过 `.` 和 `..`；顺序是 `readdir` 的顺序，
不排序。返回值是条目数，不是字节数。

`mkdir` 的 `mode` **必须显式给**（典型 `p.mkdir(0755)`）：映射按方法名分派、不支持重载，
同一宿主上不能有第二个 `mkdir`，所以没有「默认 0755」的形态。

### 4.1 `errno` 访问器

```ct
int std::fs::FsEnv.get_last_error();
```

`errno` 是每翻译单元一份的全局值，不是某条路径的属性，所以它挂在 namespace 宿主
`std::fs::FsEnv` 上，而不是 `path.get_last_error()`。

| 情形 | `get_last_error()` |
| --- | --- |
| 上一次调用成功 | `0` |
| 系统调用失败 | 当次 `errno`（`ENOENT`、`EEXIST`、`ENOTEMPTY`、`EXDEV`、`EACCES` …） |
| 本模块自身拒绝 | `EINVAL`（`NULL` / 空串参数）、`ERANGE`（缓冲区放不下）、`EISDIR`（读目录） |

限制：缓存是 `Fs.shim.h` 里的 `static int`，**每个消费单元一份**——非线程安全、不跨翻译单元
共享，也不跨 `import`。多线程或多单元程序请自己在调用点保存返回值，不要依赖它做错误分支。
与 `std::num` 的静态槽同性质。

## 5. 教程

### 5.1 读写一个文件

```ct
import std::fs::Fs;
import std::io::Io;

int main(void)
{
    char buf[64];
    char *path = "/tmp/std-fs-demo.txt";

    path.write_file("hello");
    int n = path.read_file(buf, 64);
    std::io::printf("bytes=%d data=%s\n", n, buf);
    return 0;
}
```

输出：

```text
bytes=5 data=hello
```

`write_file` 只写 `strlen(data)` 个字节，所以 `"hello"` 的 5 个字节全部落盘，没有额外内容。
写完后 `read_file` 自动补 `buf[n] = 0`。

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
    pa.write_file("A");
    pb.write_file("B");

    n = dir.list(listing, 128);
    has_a = listing.find("a.txt") >= 0;
    has_b = listing.find("b.txt") >= 0;
    std::io::printf("list count=%d has_a=%d has_b=%d\n", n, has_a, has_b);

    n = pa.read_file(buf, 32);
    std::io::printf("read bytes=%d data=%s\n", n, buf);
    std::io::printf("tiny rc=%d missing rc=%d\n",
                    pa.read_file(buf, 1), missing.read_file(buf, 32));

    pa.write_file("oo");
    n = pa.read_file(buf, 32);
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

1. `read_file` 的 `cap` 必须容得下文件内容 **加 NUL**；`cap = 1` 连 1 字节的文件都读不了。
2. 文件不存在、**路径是目录**、权限不足都统一返回 `-1`，没有 errno 出口。目录以前会「成功」
   返回 0 字节（POSIX 能只读打开目录、随后立即 EOF），`0.3.0` 起按失败处理。
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

按行拆分时用 [`std::str`](../str/README.md) 的 `find_char('\n')` 或 `find`。

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

    path.write_file("one");
    n = path.read_file(buf, 64);
    buf.concat("two", next, 64);
    path.write_file(next);
    n = path.read_file(buf, 64);
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
| 整文件读入 | `n = path.read_file(buf, sizeof buf); if (n < 0) { /* 太大或打不开 */ }` |
| 整文件替换 | `path.write_file(data)` |
| 列目录 | `n = dir.list(buf, sizeof buf);` |
| 遍历列目录结果 | 用 `std::str` 的 `find_char('\n')` 逐行切 |

## 7. 排错

### 7.1 少给参数

```ct
import std::fs::Fs;

int main(void)
{
    char buf[8];
    char *p = "/tmp/x";
    return p.read_file(buf);
}
```

```text
main.ct:7:1: error: a map call has fewer arguments than its binding
```

`read_file` 需要 `out` 和 `cap` 两个实参。

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
- **不分配**：`read_file` / `list` 全靠调用方缓冲区；大文件请自己分块读，或换用
  [`std::io`](../io/README.md) 的 `File.read`。
- **`write_file` 按 `strlen` 写**：内容里有 `'\0'` 时后面的字节不会落盘；二进制数据用
  `std::io` 的 `File.write(buf, n)`。
- **`list` 不排序、不递归、不给类型**：只输出名字，顺序来自 `readdir`。
- **没有 errno / 错误码**：所有失败都是 `-1`（`exists` 是 0），无法区分原因。
- **`char *` 方法名全局共享**：本模块占用 `exists` / `read_file` / `write_file` / `list`；
  `len` / `find` 等属于 [`std::str`](../str/README.md)，`open` 属于
  [`std::io`](../io/README.md)，`decode` / `count_codepoints` / `encode` 属于
  [`std::utf8`](../utf8/README.md)。
- **字面量接收者需要 0.20.0**。
- **多给实参不报错**：与其它映射模块相同的编译器行为（SH-56）。

## 9. 版本与变更记录

| 版本 | 状态 | 变更 |
| --- | --- | --- |
| `0.3.0` | 未打 tag | ① 新增 `mkdir` / `remove` / `rename` 与 `FsEnv.get_last_error`（`errno` 快照，每翻译单元一份）；② `read_file` 对目录返回 `-1`（此前静默返回 `0` 并给出空字符串），普通文件、空文件、含 `'\0'` 文件的行为不变；③ 全量改名到 `snake_case`：`readFile` → `read_file`、`writeFile` → `write_file`，无别名、无兼容层。 |
| `0.1.0` | 未打 tag、未发布 | 文档重写：教程并入本 README，删除 `tutorial.md`；API 未变。 |

编译器下限 `ct >= 0.30.0`。
