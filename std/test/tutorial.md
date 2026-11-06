# 教程：用 `std::test` 写断言

目标：从空文件开始，写出一个会数检查、会在失败时打印原因、退出码正确的测试程序，
并知道 `FAIL` 输出长什么样。

模块参考（方法全集、语义、布局）：[README.md](README.md)。

> 标了「可运行」的代码块都来自 [`test/std/test/guide.ct`](../../tests/test/guide.ct)，第 2 节的
> 第一个程序整段就是 [`test/std/test/first.ct`](../../tests/test/first.ct)。gate
> （`test/std/run.sh`）每次都编译并运行这两个文件；同目录的 `main.ct` 是**负向 fixture**，
> 必须失败，gate 会核对它的退出码与 `FAIL` 行数。
> 带 `error:` 输出或标「错误示意」的块则是故意不通过的。

## 1. 准备

```sh
cd /path/to/ct
ct build -I . hello.ct -o hello
```

```ct
import std::test::Test;
```

## 2. 第一个程序：三条断言

```ct
import std::test::Test;

int main(void)
{
    Test t;
    Test *tp = &t;

    tp->init();
    tp->checkTrue(1, "a passing check");
    tp->checkEqInt(2 + 2, 4, "int equality");
    tp->checkEqStr("abc", "abc", "string equality");

    return tp->report();
}
```

跑起来：

```sh
ct build -I . test/std/test/first.ct -o /tmp/first && /tmp/first
# 3 checks, 0 failures
```

`init()` 清零，三条断言各计一次，`report()` 打印汇总并返回 0。`main` 把 `report()` 的返回值
直接交出去——这一步不能省。

## 3. 三种断言

| 方法 | 用来比较 | 失败输出 |
| --- | --- | --- |
| `checkTrue(cond, msg)` | 条件是否为真 | `FAIL: <msg>` |
| `checkEqInt(got, want, msg)` | 两个 `int` | `FAIL: <msg> (got <got>, want <want>)` |
| `checkEqStr(got, want, msg)` | 两个字符串（`strcmp`） | `FAIL: <msg> (got '<got>', want '<want>')` |

```ct
    tp->checkTrue(1, "checkTrue");
    tp->checkEqInt(7, 7, "checkEqInt");
    tp->checkEqStr("hello", "hello", "checkEqStr");
```

三个方法都返回 `int`：成功 1，失败 0。返回值可以直接当条件用。

## 4. 断言失败不会中断执行

```ct
    ok = tp->checkTrue(1, "returns 1");
    tp->checkEqInt(ok, 1, "checkTrue return value");
    ok = tp->checkEqInt(5, 5, "returns 1");
    tp->checkEqInt(ok, 1, "checkEqInt return value");
    ok = tp->checkEqStr("a", "a", "returns 1");
    tp->checkEqInt(ok, 1, "checkEqStr return value");
```

`check*` 只记数并打印，不 `abort`、不 `return`。需要「这一步不成立就别往下跑」时，自己判断：

```ct
/* 错误示意（故意失败，所以不放进 gate）：一失败就收工 */
if (!tp->checkTrue(p != 0, "allocation succeeded")) return 1;
p[0] = 'x';
```

第二个断言拿 `ok` 再检查一次，是为了让「返回值是 1」这件事本身也被测试覆盖。

## 5. 边界：`NULL` 消息与 `NULL` 字符串

```ct
    tp->checkTrue(1, (char *)0);
    tp->checkEqStr((char *)0, "", "NULL got is empty");
    tp->checkEqStr("", (char *)0, "NULL want is empty");
```

`msg` 为 `NULL` 时失败输出是 `FAIL: (no message)`；字符串实参为 `NULL` 时按 `""` 处理。
所以 `NULL` 与 `""` 在 `checkEqStr` 眼里相等——要区分「没有值」和「空串」，先单独断言
指针非空。

## 6. 失败时的输出形态

负向 fixture [`test/std/test/main.ct`](../../tests/test/main.ct) 里有四条断言，其中三条
故意失败。gate 编译并运行它，要求退出码非零且 stderr 恰好三行 `FAIL:`：

```sh
ct build -I . test/std/test/main.ct -o /tmp/failing && /tmp/failing
```

stdout：

```
4 checks, 3 failures
```

stderr：

```
FAIL: intentional failure
FAIL: intentional int mismatch (got 1, want 2)
FAIL: intentional string mismatch (got 'a', want 'b')
```

退出码是 1。三种断言的输出格式都能在这里看到：`checkTrue` 只有消息，两个相等断言附带
`got` / `want`。

## 7. 常见错误

### 7.1 方法名不存在

```ct
import std::test::Test;

int main(void)
{
    Test t;
    Test *tp = &t;

    return tp->nope();
}
```

```
main.ct:8:1: error: no method 'nope' for type 'struct std_test_Test *'
```

### 7.2 忘了 `init()`

```ct
/* 错误示意：计数器带着声明时的值开始，report 的数字不可信 */
Test t;
Test *tp = &t;
tp->checkEqInt(1, 1, "one check");
return tp->report();
```

### 7.3 忘了返回 `report()`

```ct
/* 错误示意：断言失败了，但进程仍然退出 0 */
tp->init();
tp->checkTrue(0, "this fails");
tp->report();
return 0;
```

`report()` 本身返回 1，但这里丢掉了它。CI 只看退出码，会把这段当成通过。

### 7.4 只读 stdout

失败详情写 stderr。把 stderr 丢进 `/dev/null` 就只剩 `N checks, M failures`，看不到是哪条
断言失败。

## 8. 下一步

- 字符串断言之外，用 [`std::str`](../str/README.md) 处理文本
- 用 [`std::vec`](../vec/README.md) / [`std::map`](../map/README.md) 组织测试数据
- 回到 [`std/README.md`](../README.md) 看七个模块的全貌
