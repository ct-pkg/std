# 教程：用 `std::fs` 读写文件

目标：从空文件开始，写到能判断路径、整读、整写、列目录，并且清楚每个输出缓冲区由谁提供、
失败时返回什么。

模块参考（方法全集、语义、所有权）：[README.md](README.md)。

> 标了「可运行」的代码块都来自 [`test/std/fs/guide.ct`](../../tests/fs/guide.ct)，第 2 节的
> 第一个程序整段就是 [`test/std/fs/first.ct`](../../tests/fs/first.ct)。拆段时只重复了必要
> 的变量声明，逻辑与顺序和文件一致。gate（`test/std/run.sh`）每次都编译并运行这两个文件，
> 并传入两个参数：一个恰好有两个条目的目录，和目录外的一个文件路径。
> 带 `error:` 输出或标「错误示意」的块则是故意不通过的。

## 1. 准备

`std` 是源码库，编译器没有默认 include 根（D17-2），所以要显式给 `-I`：

```sh
cd /path/to/ct        # 仓库根，里面有 std/
ct build -I . hello.ct -o hello
```

`import` 路径就是「include 根 + namespace」：

```ct
import std::fs::Fs;
```

没有 `FS`、也没有 `file` 版本——trait 名是 `FsOps`，import 的单元名是 `Fs`。

## 2. 第一个程序：存在、写、读

```ct
import std::fs::Fs;
import std::test::Test;

int main(int argc, char **argv)
{
    Test t;
    Test *tp = &t;
    char buf[64];

    tp->init();
    tp->checkEqInt(argc, 3, "the runner passes a directory and a file");
    if (argc < 3) return tp->report();

    tp->checkTrue(argv[1].exists(), "the directory exists");
    tp->checkEqInt(argv[2].writeFile("hi\n"), 0, "write the file");
    tp->checkEqInt(argv[2].readFile(buf, 64), 3, "read returns the byte count");
    tp->checkEqStr(buf, "hi\n", "read value");

    return tp->report();
}
```

三个方法都是 `路径.动词(...)`，没有函数形式。自己跑时给同样的两个参数：

```sh
ct build -I . test/std/fs/first.ct -o /tmp/first
/tmp/first /tmp /tmp/ct-fs-first.txt
# 5 checks, 0 failures
```

## 3. 路径就是接收者

```ct
    tp->checkTrue(dir.exists(), "the directory exists");
    tp->checkTrue(!file.exists(), "the file does not exist yet");
    tp->checkEqInt("/no/such/path".exists(), 0, "a missing path is 0");
    tp->checkEqInt("".exists(), 0, "an empty string is not a path");
```

`exists()` 只看 `stat` 的返回值：1 或 0。空串和 `NULL` 直接按 0 处理，所以空串不是「当前目录」，
要表示当前目录写 `"."`。

关键点是**接收者就是 `char *`**：字面量、`argv[1]`、变量、函数返回值都能调，编译器不检查它
「像不像路径」。`"hello".exists()` 是合法程序，运行期返回 0。这不是 bug，是映射式模块的固有形态
（见 [README 的坑 1](README.md#坑)）。

## 4. 读文件：缓冲区与容量都由你给

```ct
    n = file.readFile(buf, 64);
    tp->checkEqInt(n, 11, "read returns the byte count");
    tp->checkEqStr(buf, "hello\nworld", "read value");
    tp->checkEqInt(file.readFile(buf, 11), -1,
                   "read needs room for the terminating NUL");
    tp->checkEqInt("/no/such/file".readFile(buf, 64), -1, "read a missing file");
```

三条规则：

1. `readFile` **不分配**，写进你给的 `buf`；
2. 返回值是**字节数**，不是字符数（第 6 节有 `"é"` 的例子）；
3. `cap` 包含结尾 NUL：11 字节的内容要 `cap >= 12`，给 11 就是 -1。

放不下时返回 -1，不会只读一半；打不开文件同样是 -1。两种失败不给区分码，需要区分就先用
`exists()` 判断。

## 5. 写文件：整文件替换，不追加

```ct
    tp->checkEqInt(file.writeFile("hello\nworld"), 0, "write the file");
    tp->checkTrue(file.exists(), "the file exists now");
```

```ct
    tp->checkEqInt(file.writeFile("oo\n"), 0, "rewrite the file");
    n = file.readFile(buf, 64);
    tp->checkEqInt(n, 3, "read the rewritten file");
    tp->checkEqStr(buf, "oo\n", "rewritten value");
```

`writeFile` 先截断再写：第二次写入之后文件里只剩 `"oo\n"`，旧内容不会留下。没有 `append` 方法，
需要追加就自己读出、拼接、再整写——拼接用
[`std::str` 的 `concat`](../str/README.md)，缓冲区仍然由你提供。

## 6. 列目录

```ct
    n = dir.list(listing, 128);
    tp->checkEqInt(n, 2, "list two entries");
    tp->checkTrue(listing.find("a.txt") >= 0, "the listing has a.txt");
    tp->checkTrue(listing.find("b.txt") >= 0, "the listing has b.txt");
    tp->checkTrue(listing.findChar('\n') >= 0, "entries are joined with a newline");
    tp->checkEqInt(dir.list(listing, 4), -1,
                   "list refuses a buffer that cannot hold the first name");
    tp->checkEqInt("/no/such/dir".list(listing, 128), -1, "list a missing directory");
```

`list` 把条目用 `'\n'` 连接写进 `out`，没有结尾换行，跳过 `.` 和 `..`，返回条目数。目录项顺序
由 `readdir` 决定，**不保证排序**，所以判断内容时用 `find`，不要按位置切。

容量检查是按每个名字做的，而且是「边写边查」：

- 第一个名字就放不下：直接 -1；
- 第一个放得下、第二个放不下：**第一个已经写进 `out` 了**，然后返回 -1。

所以 -1 之后 `out` 里可能是残缺内容。返回值是唯一可信的信号：

```ct
/* 错误示意：失败后继续读 out */
if (dir.list(listing, 8) < 0) {
    /* listing 里可能残留着半个目录列表 */
}
```

## 7. 字节，不是字符

```ct
    tp->checkEqInt(file.writeFile("é"), 0, "write a multi-byte character");
    n = file.readFile(buf, 64);
    tp->checkEqInt(n, 2, "read counts bytes, not characters");
```

`"é"` 在 UTF-8 里是两个字节：写进去两个字节，`readFile` 就返回 2。单位与
[`String.len()`](../str/README.md) 一致。要按码点切分文本，用 [`std::utf8`](../utf8/README.md)。

## 8. 常见错误

### 8.1 方法名不存在

```ct
import std::fs::Fs;

int main(void)
{
    "/tmp".nope();
    return 0;
}
```

```
main.ct:5:1: error: no method 'nope' for type 'char *'
```

方法按名字绑定到宿主类型 `char *`，不存在的名字直接报在调用点。

### 8.2 忘了 import

```ct
int main(void)
{
    return "/tmp".exists();
}
```

```
main.ct:3:1: error: no method 'exists' for type 'char *'
```

报错信息不会提示「你可能忘了 import」——`char *` 上本来就没有这个方法，只有 import 之后才存在。

### 8.3 少给参数

```ct
import std::fs::Fs;

int main(void)
{
    char buf[8];
    return "/no/such/file".readFile(buf);
}
```

```
main.ct:6:1: error: a map call has fewer arguments than its binding
```

少给的实参会在编译期报错，这是映射调用唯一被检查的方向。

### 8.4 多给参数（编译器待修）

```ct
import std::fs::Fs;

int main(void)
{
    char buf[8];
    return "/no/such/file".readFile(buf, 8, 9);
}
```

**编译通过**，生成代码里只有前两个实参，那个 `9` 被丢弃。把参数顺序写错（例如多写一个长度）
不会报错，行为与源码不符。
台账见 [SH-56](https://github.com/samxxu/ct-lang/blob/main/issues/selfhost/sh56-map-method-arity-unchecked.md)。

### 8.5 缓冲区差一个字节

```ct
    tp->checkEqInt(file.readFile(buf, 11), -1,
                   "read needs room for the terminating NUL");
```

11 字节的内容需要 `cap >= 12`。这个错误没有编译期提示，只能靠检查返回值。

## 9. 下一步

- 解析读出来的文本：[`std::str`](../str/README.md)
- 按码点处理：[`std::utf8`](../utf8/README.md)
- 把目录项存成列表：[`std::vec`](../vec/README.md) 的 `VecStr`
