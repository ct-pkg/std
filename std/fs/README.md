# `std::fs` — 路径方法、目录流与元数据

`std::fs` 有三块：`char *` 上的路径方法（`path.exists()` / `path.size()` / `path.resolve_path(...)`）、
目录流 `Dir`、以及元数据值 `Stat`（`std::fs::stat_path(path, &st)`）。

## 1. 定位与边界

- 路径方法挂在 `char *` 上，宿主已交代「这是路径」，方法名不再带 `file_` 前缀。
- `Dir` 是 ③ 资源对象：持有 `DIR *` 与一份路径副本；`defer { delete d; }` 释放。
- `Stat` 是纯值类型（无 `Drop`、无 vptr）；`stat_path` / `lstat_path` 是自由函数，因为
  `.m.ct` 映射单元不能命名 `.h.ct` 里的类型，`path.stat(&st)` 这种接收者形态无法编译。
- **`struct stat` 不原样暴露**：CT 看不到 C 结构体字段，所以 `Stat` 是字段镜像
  （`size` / `mode` / `kind` / 三个时间 / uid / gid / device / inode / links / block_size / blocks）。
  `kind` 是文件类型；谓词 `is_regular` / `is_directory` / `is_symlink` / `is_other` 直接读它。
- `size()` / `mode()` / `modification_time()` 是便捷方法，各自一次 `stat`，**不替代**
  `stat_path`（后者一次拿全部字段）。
- `is_readable()` / `is_writable()` 用 `access(2)`，按**真实身份**判定，不代表生效身份下
  实际可读写（setuid 程序里可能不同）。
- `resolve_path` 是**词法**绝对路径：相对路径拼上 `getcwd`，折叠 `//`、`.`、`..`；
  不解析符号链接，路径不要求存在。这是为了避开 strict `-std=c99` 下 `realpath` 的声明问题。
- `scan_directory` 是流式回调（每个条目回调一次），不分配数组、没有释放责任；
  回调返回非 0 提前停止。`.` / `..` 跳过。
- `list` 保留原样：把条目用 `\n` 拼进调用方缓冲；大目录请用 `Dir` 或 `scan_directory`。

## 2. 依赖与安装

依赖 `<sys/stat.h>` / `<dirent.h>` / `<unistd.h>` / `<stdio.h>`；`requires ct >= 0.67.0`。

| 文件 | 内容 |
| --- | --- |
| `Fs.m.ct` | `FsOps`（`char *` 路径方法）+ `FsEnv`（namespace：错误快照、`access` 常量、`scan_directory`） |
| `Dir.h.ct` | `pub struct Dir { void *dp; char *stored_path; } use Drop`、自由工厂 `open_dir` / `current_dir`、`impl Dir` |
| `DirFactory.m.ct` | `char *` 宿主上的 `"dir".open_dir()` |
| `Stat.h.ct` | `pub struct Stat`（字段镜像）+ `stat_path` / `lstat_path` + 类型谓词 |
| `Fs.shim.h` / `Dir.shim.h` / `Stat.shim.h` | 各自的 `static inline` C 实现 |

引入：`import std::fs::Fs;` 带入路径方法；`import std::fs::Dir;` 带入目录流；
`import std::fs::Stat;` 带入元数据。

## 3. 快速开始

```ct
import std::fs::Fs;
import std::fs::Dir;
import std::fs::Stat;
import std::str::CharStr;

int main(void)
{
    Stat st;
    char name[64];
    char *dir = "/tmp";

    "/tmp/std-fs-demo.txt".write("hello");
    "/tmp/std-fs-demo.txt".size();                 /* 5 */
    std::fs::stat_path("/tmp/std-fs-demo.txt", &st);
    st.is_regular();                               /* 1 */

    Dir *d = dir.open_dir();
    defer { delete d; }
    while (d->next(name, 64) == 1) {
        /* one name per iteration */
    }
    d->close();
    return 0;
}
```

## 4. API 全表

### 4.1 `char *` 上的路径方法（`FsOps`）

| 方法 | 签名 | 语义 | 失败 |
| --- | --- | --- | --- |
| `exists` | `int path.exists()` | `stat` 可见即 1 | 0（`EACCES` 也是 0） |
| `read` | `int path.read(char *out, int cap)` | 读整个文件，补 NUL | `-1`；目录/放不下也 `-1` |
| `write` | `int path.write(const char *data)` | 覆盖写 | `-1` |
| `list` | `int path.list(char *out, int cap)` | 条目用 `\n` 拼接 | `-1`（不返回部分结果） |
| `mkdir` | `int path.mkdir(int mode)` | 建目录 | `-1` |
| `remove` | `int path.remove()` | 删文件/空目录 | `-1` |
| `rename` | `int path.rename(const char *to)` | 改名 | `-1` |
| `access` | `int path.access(int mode)` | `access(2)`，mode 用 `FsEnv.access_*()` | `-1` |
| `read_link` | `int path.read_link(char *out, int cap)` | `readlink(2)` | `-1`（放不下） |
| `resize` | `int path.resize(long length)` | `truncate(2)` | `-1` |
| `set_mode` | `int path.set_mode(int mode)` | `chmod(2)`，如 `0600` | `-1` |
| `size` | `int path.size()` | `stat` 的字节数 | `-1` |
| `mode` | `int path.mode()` | 权限位（`st_mode & 07777`） | `-1` |
| `modification_time` | `int path.modification_time()` | `st_mtime` | `-1` |
| `is_readable` | `int path.is_readable()` | `access(R_OK)` | 0/1 |
| `is_writable` | `int path.is_writable()` | `access(W_OK)` | 0/1 |
| `resolve_path` | `int path.resolve_path(char *out, int cap)` | 词法绝对路径 | `-1` |

### 4.2 namespace `std::fs::FsEnv`

| 方法 | 签名 | 语义 |
| --- | --- | --- |
| `get_last_error()` | `int` | 最近一次 `std::fs` 调用的 `errno` 快照（per-TU） |
| `access_read()` / `access_write()` / `access_execute()` / `access_exists()` | `int` | `R_OK` / `W_OK` / `X_OK` / `F_OK` |
| `scan_directory(path, cb, ctx)` | `int scan_directory(const char *path, void *cb, void *ctx)` | 流式遍历；返回回调次数或 `-1` |

扫描回调是 `int cb(const char *name, void *context)`；非 0 返回值提前停止。

### 4.3 `Dir`（自由工厂 + `impl Dir`）

| 入口 | 签名 | 语义 | 失败 |
| --- | --- | --- | --- |
| `std::fs::open_dir` | `Dir *open_dir(const char *path)` | `opendir` | `0` |
| `std::fs::current_dir` | `Dir *current_dir()` | 打开当前目录（保存绝对路径） | `0` |
| `path.open_dir` | `Dir *path.open_dir()` | 字符串工厂，同一类型 | `0` |
| `next` | `int d->next(char *out, int cap)` | 下一个名字（跳过 `.`/`..`） | 1/0/-1 |
| `rewind` | `int d->rewind()` | `rewinddir` | `-1` |
| `close` | `int d->close()` | `closedir`，幂等 | `-1` |
| `tell` | `long d->tell()` | `telldir` | `-1` |
| `seek` | `int d->seek(long pos)` | `seekdir` | `-1` |
| `path` | `int d->path(char *out, int cap)` | 取保存的路径 | `-1` |
| `enter` | `int d->enter()` | `chdir` 到保存路径 | `-1` |

### 4.4 `Stat`（`std::fs::Stat`）

字段：`size` `mode` `kind` `modification_time` `access_time` `change_time` `uid` `gid`
`device` `inode` `links` `block_size` `blocks`。

| 函数 / 方法 | 签名 | 语义 |
| --- | --- | --- |
| `stat_path` | `int std::fs::stat_path(const char *path, Stat *st)` | `stat(2)`，跟随符号链接 |
| `lstat_path` | `int std::fs::lstat_path(const char *path, Stat *st)` | `lstat(2)` |
| `is_regular` / `is_directory` / `is_symlink` / `is_other` | `int st->is_...()` | 读 `kind` |

## 5. 与相邻模块的关系

- 错误：`FsEnv.get_last_error()` 是 `std::fs` 自己的 per-TU 快照；也可用
  `std::errno::Errno.get_last()` 直读。
- `is_readable` / `is_writable` 与 `std::io::FileDesc` 的 `open` 失败是两层：前者只探测，
  后者真打开。
- `Stat` 的时间字段是 `long` 秒，不是 `std::time` 的格式化时间；需要年月日时用
  `std::time::Time`（P5）。
- `Dir` 与 `list` 并存：小目录用 `list` 拿拼接字符串，大目录用 `Dir` 流式读。

## 6. 限制与未支持

- **`Stat` 是字段镜像，不是 `struct stat` 本体**：没有平台专有字段；需要原始结构体时直接调 C。
- **`size` / `modification_time` 返回 `int`**：超过 `INT_MAX` 的字节数 / 时间返回 `-1`（`ERANGE`），
  避免静默截断。
- **`resolve_path` 不解析符号链接、不要求路径存在**：与 `realpath(3)` 不同。
- **`is_*` 按真实身份**：`access(2)` 不看 setuid 之后的生效身份。
- **`.ct` 不能 `#include <sys/stat.h>`**：`Stat` 的字段由 shim 填，平台差异不暴露。
- **扫描回调是顶层函数**：CT 没有闭包；上下文只能经 `void *` 传。
- **`Dir` 句柄不要值拷贝**：释放写 `defer { delete d; }`；`close()` 后再 `delete` 安全。

## 7. 版本与变更记录

| 版本 | 状态 | 变更 |
| --- | --- | --- |
| `0.8.0` | 本轮变更 | 新增 `access` / `read_link` / `resize` / `set_mode` / `size` / `mode` / `modification_time` / `is_readable` / `is_writable` / `resolve_path`；新增 `Dir`（`own` 自由工厂 + 字符串工厂 + `defer delete`）；新增 `Stat` 字段镜像与 `stat_path` / `lstat_path`；新增 `scan_directory` 流式回调。 |
| `0.6.0` | 已发布 | `exists` / `read_file` / `write_file` / `list` / `mkdir` / `remove` / `rename` 与 `FsEnv.get_last_error`。 |

编译器下限 `ct >= 0.67.0`。
