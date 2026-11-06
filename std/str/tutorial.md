# 教程：用 `std::str` 处理字符串

目标：从空文件开始，写到能查询、能生成新字符串、能原地改大小写，并且知道每一步的返回值和所有权。

模块参考（方法全集、参数、语义）：[README.md](README.md)。

> 标了「可运行」的代码块都来自 [`test/std/str/guide.ct`](../../tests/str/guide.ct)，第 2 节的
> 第一个程序整段就是 [`test/std/str/first.ct`](../../tests/str/first.ct)。拆段时只重复了必要
> 的变量声明，逻辑与顺序和文件一致。gate（`test/std/run.sh`）每次都编译并运行这两个文件。
> 带 `error:` 输出或标「错误示意」的块则是故意不通过的。

## 1. 准备

`std` 是源码库，编译器没有默认 include 根（D17-2），所以要显式给 `-I`：

```sh
cd /path/to/ct        # 仓库根，里面有 std/
ct build -I . hello.ct -o hello
```

`import` 路径就是「include 根 + namespace」：

```ct
import std::str::String;
```

没有 `Str`、也没有 `str` 版本——trait 名是 `String`，因为它描述的是「字符串能做什么」。

## 2. 第一个程序：读

```ct
import std::str::String;
import std::test::Test;

int main(void)
{
    Test t;
    Test *tp = &t;
    char *s = "Hello, ct";

    tp->init();

    tp->checkEqInt(s.len(), 9, "len");
    tp->checkEqInt(s.find("ct"), 7, "find");
    tp->checkEqInt(s.find("zz"), -1, "find misses");
    tp->checkEqInt(s.findChar(','), 5, "findChar");
    tp->checkEqInt(s.findChar('z'), -1, "findChar misses");
    tp->checkEqInt(s.startsWith("Hello"), 1, "startsWith hit");
    tp->checkEqInt(s.startsWith("hello"), 0, "startsWith is case sensitive");
    tp->checkEqInt(s.endsWith("ct"), 1, "endsWith hit");
    tp->checkEqInt(s.endsWith("tc"), 0, "endsWith miss");
    tp->checkEqInt("abc".compare("abd"), -1, "compare orders like strcmp");

    return tp->report();
}
```

要点：

- 接收者可以是变量，也可以直接是字面量（`"abc".compare(...)`）——类型键会丢掉 `const`。
- `find` / `findChar` 返回的是**字节偏移**，找不到返回 `-1`。别用 `>= 0` 之类的「长度」判断，
  偏移 0 是合法结果。
- `startsWith` / `endsWith` 返回 0 / 1，不是索引。

跑起来：

```sh
ct build -I . test/std/str/guide.ct -o /tmp/guide && /tmp/guide
# 26 checks, 0 failures
```

## 3. 产出新字符串：缓冲区由你给

`std::str` 不分配内存。要「生成」一个字符串，就提供一个缓冲区：

```ct
    char buf[16];

    tp->checkEqInt(s.copy(buf, 16), 0, "copy");
    tp->checkEqStr(buf, "Hello, ct", "copy result");
    tp->checkEqInt(s.copy(buf, 9), -1, "copy needs room for the NUL");

    tp->checkEqInt(s.slice(0, 5, buf, 16), 0, "slice");
    tp->checkEqStr(buf, "Hello", "slice is half-open [start, end)");
    tp->checkEqInt(s.slice(0, 99, buf, 16), -1, "slice past the end");

    tp->checkEqInt("Hello, ".concat("ct", buf, 16), 0, "concat");
    tp->checkEqStr(buf, "Hello, ct", "concat result");
    tp->checkEqInt("Hello, ".concat("ct", buf, 9), -1, "concat needs room for the NUL");
```

三条规则：

1. `cap` 包含结尾 NUL：`"Hello, ct"` 是 9 个字节，`cap` 至少要 10；
2. 放不下返回 `-1`，**不会**截断，也不会替你扩容；
3. 返回 0 之前 `buf` 的内容是上一次的残留，必须用返回值判断成败。

`slice` 是半开区间 `[start, end)`，`slice(0, 5, ...)` 得到 `"Hello"`。

## 4. 原地修改：只对自己的缓冲区做

```ct
    char word[8];

    tp->checkEqInt("Hello".copy(word, 8), 0, "seed a mutable buffer");
    tp->checkEqInt(word.toLower(), 0, "toLower");
    tp->checkEqStr(word, "hello", "toLower rewrites the buffer");
    tp->checkEqInt(word.toUpper(), 0, "toUpper");
    tp->checkEqStr(word, "HELLO", "toUpper rewrites the buffer");
```

`toLower` / `toUpper` **不返回新字符串**，返回的是状态码，改的是接收者指向的那块内存。
所以必须先有可写的副本：字面量走 `copy`，函数返回值先落盘到数组，堆上的字符串则要确认可写。

## 5. 字节，不是字符

```ct
    tp->checkEqInt("é".len(), 2, "len counts bytes");
```

`"é"` 在 UTF-8 里是两个字节，`len()` 报 2。`find` / `findChar` / `slice` 的索引同样是字节偏移，
在多字节文本里按字节切可能切断一个字符。要按「字符（码点）」处理，交给
[`std::utf8`](../utf8/README.md)：`"é".codepointCount() == 1`。

## 6. 常见错误

### 6.1 方法名不存在

```ct
int n = "abc".nope();
```

```
main.ct:5:1: error: no method 'nope' for type 'char *'
```

方法按名字绑定到宿主类型 `char *`，不存在的名字直接报在调用点。

### 6.2 两个 trait 抢同一个名字

自己再写一个 `char *` 的 trait，里面也有 `len`：

```ct
namespace probe;

pub trait Extra {
    int len();
}

map trait Extra for type char * {
    len(s) -> std_str_len(s);
}
```

然后同时 import：

```ct
import std::str::String;
import probe::Extra;

int main(void)
{
    return "abc".len();
}
```

```
main.ct:6:1: error: ambiguous type-host map for method 'len' on type 'char *': '/path/to/std/str/String.m.ct' and '/path/to/probe/Extra.m.ct'
```

判定只看名字，不看参数个数；也不会静默挑一个。只 import 不调用则不报错。新增 `char *` trait
前先 grep 现用名（清单见 [README](README.md#坑)）。

### 6.3 对字面量调用 `toLower()`

```ct
/* 编译通过，然后写只读页 */
"abc".toLower();
```

这里**没有报错**，这正是坑：类型键丢掉了 `const`，编译器认为字面量也是合法的 `char *` 接收者。
运行期行为未定义（常见是段错误）。正确写法是先拷贝：

```ct
    tp->checkEqInt("Hello".copy(word, 8), 0, "seed a mutable buffer");
    tp->checkEqInt(word.toLower(), 0, "toLower");
```

### 6.4 缓冲区差一个字节

```ct
    tp->checkEqInt(s.copy(buf, 9), -1, "copy needs room for the NUL");
```

`"Hello, ct"` 是 9 个字节，`cap` 必须 ≥ 10（多出来那个位置放 NUL），所以 `cap == 9` 返回 -1。
这个错误没有编译期提示，只能靠检查返回值。

### 6.5 多写的实参不会报错（编译器待修）

```ct
int n = "abc".len(1);   /* 编译通过，实测生成 std_str_len(("abc"))，那个 1 被丢弃 */
```

（片段：放进 `main` 就能编译。）映射方法的实参个数目前不校验。别依赖它，也别把它当成重载。

## 7. 下一步

- 按码点处理文本：[`std::utf8`](../utf8/README.md)
- 读写文件：[`std::fs`](../fs/README.md)
- 需要动态字符串数组：[`std::vec`](../vec/README.md) 的 `VecStr`
