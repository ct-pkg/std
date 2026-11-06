# 教程：用 `std::utf8` 按码点处理文本

目标：从空文件开始，分清「字节」和「码点」，能解一个码点、能遍历、能把码点编码回缓冲区，
并且知道失败时返回什么。

模块参考（方法全集、参数、语义）：[README.md](README.md)。

> 标了「可运行」的代码块都来自 [`test/std/utf8/guide.ct`](../../tests/utf8/guide.ct)，第 2 节的
> 第一个程序整段就是 [`test/std/utf8/first.ct`](../../tests/utf8/first.ct)。拆段时只重复了
> 必要的变量声明，逻辑与顺序和文件一致。gate（`test/std/run.sh`）每次都编译并运行这两个文件。
> 带 `error:` 输出或标「错误示意」的块则是故意不通过的。

## 1. 准备

`std` 是源码库，编译器没有默认 include 根（D17-2）：

```sh
cd /path/to/ct        # 仓库根，里面有 std/
ct build -I . hello.ct -o hello
```

```ct
import std::utf8::Utf8Ops;
```

通常还会同时 import `std::str::String`：两个模块共享 `char *` 宿主，一个数字节、一个数码点。

## 2. 第一个程序：`é` 的两副面孔

```ct
import std::utf8::Utf8Ops;
import std::str::String;
import std::test::Test;

int main(void)
{
    Test t;
    Test *tp = &t;
    char buf[8];
    int cp;
    int n;

    tp->init();

    tp->checkEqInt("é".len(), 2, "len counts bytes");
    tp->checkEqInt("é".codepointCount(), 1, "codepointCount counts code points");

    tp->checkEqInt("é".decode(&cp), 2, "decode returns the bytes consumed");
    tp->checkEqInt(cp, 233, "decode gives the code point");

    n = buf.encode(cp, 8);
    tp->checkEqInt(n, 2, "encode returns the bytes written");
    buf[n] = 0;
    tp->checkEqInt(buf.decode(&cp), 2, "round trip");
    tp->checkEqInt(cp, 233, "round-trip value");

    return tp->report();
}
```

跑起来：

```sh
ct build -I . test/std/utf8/first.ct -o /tmp/first && /tmp/first
# 9 checks, 0 failures
```

`é` 是一个码点、两个字节。`len()` 报 2，`codepointCount()` 报 1，`decode` 返回 2，
`encode` 也返回 2——四个数字分成两组，单位不同。

## 3. 字节与码点

```ct
    tp->checkEqInt("é".len(), 2, "String.len counts bytes");
    tp->checkEqInt("é".codepointCount(), 1, "codepointCount counts code points");
    tp->checkEqInt("héllo".len(), 6, "héllo is six bytes");
    tp->checkEqInt("héllo".codepointCount(), 5, "héllo is five code points");
```

UTF-8 是变长编码：ASCII 1 字节，`é` 这类 2 字节，欧元符号 3 字节，emoji 4 字节。
所以「长度」有两个答案，`std` 用两个名字区分，不再引入第三个叫 `length` 的东西。

## 4. `decode`：一次一个码点

```ct
    tp->checkEqInt("A".decode(&cp), 1, "decode one byte");
    tp->checkEqInt(cp, 65, "the ASCII code point");
    tp->checkEqInt("é".decode(&cp), 2, "decode two bytes");
    tp->checkEqInt(cp, 233, "the two-byte code point");
    tp->checkEqInt("\xE2\x82\xAC".decode(&cp), 3, "decode three bytes");
    tp->checkEqInt(cp, 8364, "the euro sign");
    tp->checkEqInt("\xF0\x9F\x98\x80".decode(&cp), 4, "decode four bytes");
    tp->checkEqInt(cp, 128512, "the emoji code point");
    tp->checkEqInt("A".decode((int *)0), 1, "the out-param may be NULL");
```

返回值是**消费的字节数**，不是码点值，也不是剩余长度。码点值写进 `&cp`；`cp` 传 `NULL`
表示只想知道「下一个字符占几个字节」。无效输入返回 -1，此时 `cp` 不被写。

## 5. 遍历：用返回值前进

```ct
    s = "héllo";
    i = 0;
    n = 0;
    while (s[i]) {
        p = s + i;
        step = p.decode(&cp);
        if (step < 0) break;
        i += step;
        n++;
    }
    tp->checkEqInt(n, 5, "the walk visits five code points");
    tp->checkEqInt(i, 6, "the walk consumes six bytes");
```

关键在 `i += step`。写 `i++` 会在 `é` 的第二个字节上重新开始解码，那是一个孤立续字节，
`decode` 返回 -1。`while (s[i])` 只看当前字节是不是 NUL，所以字符串本身仍然以 NUL 结尾。

## 6. `encode`：接收者是输出缓冲区

```ct
    n = buf.encode(233, 8);
    tp->checkEqInt(n, 2, "encode returns the byte width");
    tp->checkEqInt(buf[0], (char)0xC3, "first byte of é");
    tp->checkEqInt(buf[1], (char)0xA9, "second byte of é");
    buf[n] = 0;
    tp->checkEqStr(buf, "é", "the buffer spells é once terminated");
    tp->checkEqInt(buf.decode(&cp), 2, "round trip");
    tp->checkEqInt(cp, 233, "round-trip value");
```

`encode` 是唯一「接收者不是文本」的方法：`buf.encode(cp, cap)` 里 `buf` 是**写到哪里**，
`cap` 是它的字节容量。语义与 `decode` 对称：

| 方向 | 写法 | 返回 | 谁提供空间 |
| --- | --- | --- | --- |
| 字节 → 码点 | `s.decode(&cp)` | 消费的字节数 | 不需要 |
| 码点 → 字节 | `buf.encode(cp, cap)` | 写入的字节数 | 调用方：`buf` + `cap` |

**`encode` 不写 NUL。** 上面 `buf[n] = 0;` 这一行是调用方的责任，不是示例的装饰。
少了它，`buf` 只是若干个字节，不是 C 字符串：

```ct
    buf[0] = '?';
    buf[1] = 'X';
    n = buf.encode('A', 8);
    tp->checkEqInt(n, 1, "one byte for ASCII");
    tp->checkEqInt(buf[0], 'A', "the byte was written");
    tp->checkEqInt(buf[1], 'X', "encode leaves the following byte alone");
```

容量按字节算：`encode(233, 2)` 刚好成功，`encode(233, 1)` 返回 -1。

```ct
    tp->checkEqInt(buf.encode(233, 2), 2, "cap equal to the width is enough");
    tp->checkEqInt(buf.encode(233, 1), -1, "cap below the width fails");
```

## 7. 严格解码：无效输入是错误

```ct
    tp->checkEqInt("\xC0\x80".decode(&cp), -1, "decode rejects an overlong");
    tp->checkEqInt("\xED\xA0\x80".decode(&cp), -1, "decode rejects a surrogate");
    tp->checkEqInt("\x80".decode(&cp), -1, "decode rejects a lone continuation");
    tp->checkEqInt("\xC3".decode(&cp), -1, "decode rejects a truncated sequence");
    tp->checkEqInt("\x80".codepointCount(), -1, "codepointCount rejects invalid text");
```

`decode` 只接受合法序列：续字节必须 `10xxxxxx`，不允许过长编码，不允许代理区
（`U+D800`–`U+DFFF`），不允许超过 `U+10FFFF`。不合法就返回 -1，**不**替换成 `U+FFFD`——
替换会让两段不同的字节得到同一段文本。

编码侧对称：

```ct
    tp->checkEqInt(buf.encode(0xD800, 8), -1, "encode rejects a surrogate");
    tp->checkEqInt(buf.encode(0x110000, 8), -1, "encode rejects past U+10FFFF");
```

## 8. 常见错误

### 8.1 方法名不存在

```ct
import std::utf8::Utf8Ops;

int main(void)
{
    "abc".nope();
    return 0;
}
```

```
main.ct:5:1: error: no method 'nope' for type 'char *'
```

### 8.2 忘了 import

```ct
int main(void)
{
    int cp;
    return "abc".decode(&cp);
}
```

```
main.ct:4:1: error: no method 'decode' for type 'char *'
```

### 8.3 把 `encode` 的输出当字符串用

```ct
/* 错误示意：encode 不写 NUL，这里读出的是缓冲区里的旧内容 */
n = buf.encode(cp, 8);
int len = buf.len();
```

`encode` 只写 `n` 个字节；`buf.len()` 会一直读到第一个 NUL 为止，可能是越界读。
正确写法是 `buf[n] = 0;` 之后再当字符串用（见第 6 节）。

### 8.4 用 `i++` 遍历多字节文本

```ct
/* 错误示意：按 1 前进，第二个字节是续字节，decode 返回 -1 */
char *s = "é";
int i = 0;
while (s[i]) {
    char *p = s + i;
    int step = p.decode(&cp);   /* 第 2 轮返回 -1 */
    i++;                        /* 应该写 i += step */
}
```

步进必须用 `step`，不是 1。正确写法见第 5 节。

### 8.5 多写的实参不会报错（编译器待修）

```ct
import std::utf8::Utf8Ops;

int main(void)
{
    int cp;
    return "abc".decode(&cp, 1);
}
```

**编译通过**，那个 `1` 被静默丢弃。少给实参则会报
`a map call has fewer arguments than its binding`。台账见
[SH-56](https://github.com/samxxu/ct-lang/blob/main/issues/selfhost/sh56-map-method-arity-unchecked.md)。

## 9. 下一步

- 字节级操作：[`std::str`](../str/README.md)
- 读写文件：[`std::fs`](../fs/README.md)
- 存一串码点：[`std::vec`](../vec/README.md) 的 `VecInt`
