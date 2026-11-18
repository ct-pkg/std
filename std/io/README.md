# `std::io` — stdio 流与文件描述符

`File` 拥有一个 `FILE *`，`FileDesc` 拥有一个 fd；两者都是 CT struct + `use Drop`。
`"path".open_file(mode)` / `"path".open_as_file_desc(flags)` 是表达式工厂，
`std::io::open_file(path, mode)` / `std::io::open_as_file_desc(path, flags)` 是 `own` 工厂。

## 1. 定位与边界

- **两个资源对象**：`File`（`FILE *`）与 `FileDesc`（fd）。两者独立，不互相包装；
  `raw_file_desc()` 把 fd 交给未包装的 POSIX 接口。
- **两个工厂，一个类型**：
  - 字符串工厂 `path.open_file(mode)` 返回 `File *`，是 CT 的默认写法；
  - 自由函数 `std::io::open_file(path, mode)` 返回同一个 `File *`，可写 `own File *f = ...`。
  释放协议一律是拿到句柄后写 `defer { delete f; }`：覆盖正常落出 / `return` / `break` /
  `continue`，`delete` 先跑 `Drop`（关闭）再释放结构体。
  **`own` 只是可选糖**：ct 0.65.1 在方法返回值被局部捕获时会静默跳过 `own` 的释放
  （`int n = f->read(...)` 就触发），所以 README 不把它当默认；详见
  [资源对象裁定](../../docs/design/decision-resource-ownership.md)。
  `FileDesc` 同理：`path.open_as_file_desc(flags)` 与 `std::io::open_as_file_desc(path, flags)`。
- **方法用 `->`**：`File` / `FileDesc` 是 CT struct 指针，方法调用写成 `f->read(...)`，
  不是旧 `IoFile *` 时代的 `f.read(...)`。
- **`close()` 幂等**：关闭后把内部句柄置空/`-1`，再次 `close`、`Drop`、或方法调用都安全。
  NULL 句柄同样被方法接受：`close` 返回 0，其它方法返回 `-1`。
- **`stdout` / `stderr` / `stdin` 不包**：它们不是本包拥有的句柄，输出走
  `std::io::Stdout` 的 namespace 方法（`puts` / `write` / `write_line` / `putchar` /
  `fflush`），输入用 `File` 打开文件或直接调 C。
- **变参不包**：`printf` / `fprintf` / `scanf` / `sprintf` 族在 0.4.0 已删除，替代是
  `to_str` / `concat` 拼接后 `write`，解析用 `std::str` 的 `parse_*`。
- **不分配文本**：`read_line` 写调用方缓冲；行太长返回 `-1`，不返回半行。

## 2. 依赖与安装

依赖 C 的 `<stdio.h>` / `<fcntl.h>` / `<unistd.h>` / `<sys/socket.h>`；
`requires ct >= 0.64.0`。

| 文件 | 内容 |
| --- | --- |
| `File.h.ct` | `pub struct File { void *fp; } use Drop`、自由工厂 `open_file` / `adopt_file`、`impl File` 的全部流方法 |
| `Path.m.ct` | `char *` 宿主上的 `open_file(mode)`（返回同一个 `File *`） |
| `FileDesc.h.ct` | `pub struct FileDesc { int fd; } use Drop`、自由工厂 `open_as_file_desc` / `adopt_file_desc`、flag 常量、`impl FileDesc` |
| `FileDescFactory.m.ct` | `char *` 宿主上的 `open_as_file_desc(flags)` |
| `Stdout.m.ct` | `std::io::Stdout` 的 namespace 方法（`puts` / `putchar` / `getchar` / `fflush` / `write` / `write_line`） |
| `*.shim.h` | 各自的 `static inline` C 实现 |

引入：`import std::io::File;` 带入 `File`、`Path` 与 `Stdout` 的文件方法；
`import std::io::FileDesc;` 带入 `FileDesc` 与它的字符串工厂。

## 3. 快速开始

```ct
import std::io::File;

int main(void)
{
    char buf[32];
    File *f = "/tmp/std-io-demo.txt".open_file("w+");

    f->write("hello\n", 6);
    f->rewind_start();
    f->read_line(buf, 32);     /* "hello" */
    defer { delete f; }        /* Drop closes, delete frees; runs on every exit */
    f->write("more", 4);
    return 0;
}
```

带 `close()` 的提前释放（`delete` 仍必须执行）：

```ct
File *f = "/tmp/std-io-demo.txt".open_file("w");
defer { delete f; }
f->write_line("owned");
f->close();                    /* idempotent; the deferred delete still frees */
```

## 4. API 全表

### 4.1 工厂

| 入口 | 签名 | 语义 | 层 | 失败 |
| --- | --- | --- | --- | --- |
| `std::io::open_file` | `File *open_file(const char *path, const char *mode)` | `fopen`，`own` 可用（自由工厂） | ② | `0` |
| `path.open_file` | `File *path.open_file(const char *mode)` | 同上，字符串工厂；`defer { delete }` | ② | `0` |
| `std::io::adopt_file` | `File *adopt_file(void *fp)` | 接管已打开的 `FILE *` | ② | `0` |
| `std::io::open_as_file_desc` | `FileDesc *open_as_file_desc(const char *path, int flags)` | `open(2)`，`own` 可用（自由工厂） | ② | `0` |
| `path.open_as_file_desc` | `FileDesc *path.open_as_file_desc(int flags)` | 同上，字符串工厂；`defer { delete }` | ② | `0` |
| `std::io::adopt_file_desc` | `FileDesc *adopt_file_desc(int fd)` | 接管已有 fd | ② | `0` |
| `seek_start` / `seek_current` / `seek_end` | `int` | `File` 的 `fseek` origin | ① | — |
| `desc_seek_start` / `desc_seek_current` / `desc_seek_end` | `int` | `FileDesc` 的 `lseek` origin | ① | — |

`FileDesc` flag 常量：`open_read` / `open_write` / `open_read_write` / `open_append` /
`open_create` / `open_update`（`O_RDWR|O_CREAT|O_TRUNC`）/ `open_exclusive`。

### 4.2 `File`（`impl File`）

| 方法 | 签名 | 语义 | 失败 |
| --- | --- | --- | --- |
| `read` | `int f->read(char *buf, int n)` | `fread` | `-1`；EOF `0` |
| `write` | `int f->write(const char *buf, int n)` | `fwrite` | `-1` |
| `close` | `int f->close()` | `fclose`，幂等 | `-1` |
| `seek` | `int f->seek(long offset, int origin)` | `fseek` | `-1` |
| `tell` | `long f->tell()` | `ftell` | `-1` |
| `rewind_start` | `int f->rewind_start()` | `fseek(0, SEEK_SET)` | `-1` |
| `at_end_of_file` | `int f->at_end_of_file()` | `feof` | 0/1 |
| `has_error` | `int f->has_error()` | `ferror` | 0/1 |
| `clear_error` | `void f->clear_error()` | `clearerr` | — |
| `read_char` | `int f->read_char()` | `fgetc` | `-1`（EOF 与错误不可分） |
| `write_char` | `int f->write_char(int c)` | `fputc` | `-1` |
| `unread_char` | `int f->unread_char(int c)` | `ungetc` | `-1` |
| `read_line` | `int f->read_line(char *out, int cap)` | 读一行，去掉 `\n` | `-1`（含放不下） |
| `write_line` | `int f->write_line(const char *text)` | 写文本 + `\n` | `-1` |
| `flush` | `int f->flush()` | `fflush` | `-1` |

### 4.3 `FileDesc`（`impl FileDesc`）

| 方法 | 签名 | 语义 | 失败 |
| --- | --- | --- | --- |
| `read` | `int d->read(char *buf, int cap)` | `read(2)` | `-1`；EOF `0` |
| `write` | `int d->write(const char *data, int n)` | `write(2)`，可能短写 | `-1` |
| `seek` | `long d->seek(long offset, int origin)` | `lseek(2)`，返回新偏移 | `-1` |
| `tell` | `long d->tell()` | `lseek(fd, 0, SEEK_CUR)` | `-1` |
| `close` | `int d->close()` | `close(2)`，幂等 | `-1` |
| `raw_file_desc` | `int d->raw_file_desc()` | 裸 fd，供 `poll` / `select` 等 | 关闭后 `-1` |
| `send` | `FileDesc *d->send(const char *data, int n)` | `send(2)`，返回接收者 | 链式；错误留给 `errno` |
| `receive` | `FileDesc *d->receive(char *buf, int cap, int *count)` | `recv(2)`，字节数出参 | `count = -1` |

## 5. 与相邻模块的关系

- 错误码：失败后用 `std::errno::Errno.get_last()` 读；本模块不缓存错误。
- 信号 / 管道：`std::process` 的 `pipe_open` 返回 `void *`（`FILE *`），不是 `File`，
  只能 `pipe_close`。
- 网络：`std::net::Socket` 是 `FileDesc` 的同义别名（P6/P7），`send` / `receive` 直接复用。
- 文本：`read_line` 只拷贝，不做 UTF-8 校验；需要码点时接 `std::utf8`。

## 6. 限制与未支持

- **`own` 只认自由函数，且在 0.65.1 上有缺陷**：`own File *f = path.open_file(mode)` 编译不过；
  即使 `own` 编译通过，方法返回值被局部捕获时释放会被丢掉。默认用 `defer { delete }`。
- **不要值拷贝句柄**：`File g = *f;` 会产生两个所有者；只传指针。
- **`read_line` 行太长会消费到超限处**：返回 `-1` 时该行剩余字节仍在流里，下一次读会接着读。
- **EOF 与错误不可分**：`read_char` 返回 `-1` 时用 `at_end_of_file()` / `has_error()` 区分。
- **`FileDesc` 的 `send` / `receive` 对普通文件失败**：`ENOTSOCK`，错误从 `errno` 读。
- **不提供 `fileno` / `fdopen`**：`File` 不暴露底层 fd；需要 fd 时用 `FileDesc` 打开，
  或 `adopt_file` 接管 C 侧已有的流。
- **不提供变参格式化**：`printf` / `fprintf` / `scanf` 永久不做。

## 7. 版本与变更记录

| 版本 | 状态 | 变更 |
| --- | --- | --- |
| `0.7.0` | 本轮变更 | `IoFile` 不透明句柄退役：`File` 改为 CT struct + `use Drop` + 自由工厂；新增 `seek` / `tell` / `rewind_start` / `at_end_of_file` / `has_error` / `clear_error` / `read_char` / `write_char` / `unread_char` / `read_line` / `write_line` / `flush`；新增 `FileDesc`、`adopt_file` / `adopt_file_desc` 与 flag 常量；`Stdout` 增加 `write` / `write_line`。 |
| `0.4.0` | 已发布 | 变参 `printf` 删除。 |

编译器下限 `ct >= 0.64.0`。
