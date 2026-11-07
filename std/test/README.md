# `std::test` — 断言计数器（`Test`）

一个极小的断言工具：每次 `check*` 自增计数，失败时往 stderr 打一行 `FAIL: ...`，
`report()` 打印汇总并返回进程状态码。

这是 **CT 自有类型**，走原生 OO：`pub trait` + `struct ... use Trait` + `impl` 全在
[`Test.h.ct`](Test.h.ct) 里，没有 `.m.ct`、没有 shim。

## 1. 定位与边界

- 提供：三类断言（布尔、`int` 相等、字符串相等）、计数、汇总报告。
- 每次断言都计数；失败只记录并返回 0，**不中断执行**。
- `report()` 向 stdout 打印 `<n> checks, <m> failures`，并返回进程状态码（0 / 1）。
- 不做：测试发现、用例注册、setup / teardown、超时、mock、参数化。
- 本模块不分配内存，也不保存 `msg` / 字符串实参。

## 2. 依赖与安装

依赖 C 的 `<stdio.h>` / `<string.h>`；`ct.pkg` 的约束是 `requires ct >= 0.19.1`。

| 文件 | 内容 |
| --- | --- |
| `Test.h.ct` | `TestOps` 一个 `pub trait`、`Test` 一个 `pub struct ... use`、一个 `impl` |

引入方式：`import std::test::Test;`。

包安装后（详见[包根 README](../../README.md#安装)）：

```sh
ct get std@^0.1.0
ct build src/main.ct -I vendor/std -o main --cflags "-Wall -Wextra"
```

在包根内开发时：

```sh
ct build -I . tests/test/first.ct -o /tmp/std-test --cflags "-Wall -Wextra"
```

## 3. 快速开始

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

`ct build` 实际执行的 `cc` 行（本机实测，路径随检出位置变化）：

```
'cc' -std=c99 -Wall -Wextra '.ctbuild/first.c' '.ctbuild/Test.c' '-o' '/tmp/std-test'
```

输出：

```text
3 checks, 0 failures
```

本节整段就是 [`tests/test/first.ct`](../../tests/test/first.ct)；教程各段的同源程序在
[`tests/test/guide.ct`](../../tests/test/guide.ct)，负向 fixture 是
[`tests/test/main.ct`](../../tests/test/main.ct)（必须失败，gate 会核对退出码与 `FAIL` 行数）。

## 4. API 全表

### 类型与 trait

| 名字 | 种类 | 说明 |
| --- | --- | --- |
| `TestOps` | `pub trait` | 接收者 `Test *` |
| `Test` | `pub struct ... use TestOps` | `{ checks, failures }` + vptr |

### 方法

| 方法 | 签名 | 返回 / 失败 |
| --- | --- | --- |
| `init()` | `void init()` | 把 `checks` / `failures` 归零；**契约上的第一步** |
| `checkTrue(cond, msg)` | `int checkTrue(int cond, const char *msg)` | 计一次；`cond` 非零返回 1，否则记失败、打 `FAIL`、返回 0 |
| `checkEqInt(got, want, msg)` | `int checkEqInt(int got, int want, const char *msg)` | 计一次；相等返回 1，否则记失败、打 `FAIL`、返回 0 |
| `checkEqStr(got, want, msg)` | `int checkEqStr(const char *got, const char *want, const char *msg)` | `strcmp` 比较；`NULL` 当 `""`；返回 1 / 0 |
| `report()` | `int report()` | 向 stdout 打印 `<n> checks, <m> failures`；`m == 0` 返回 0，否则 1 |

失败的输出形态（每条一行，写 stderr）：

```text
FAIL: <msg>
FAIL: <msg> (got <十进制>, want <十进制>)
FAIL: <msg> (got '<字符串>', want '<字符串>')
```

`msg` 传 `NULL` 时输出 `(no message)`。

## 5. 教程

### 5.1 三种断言与返回值

```ct
import std::test::Test;

int main(void)
{
    Test t;
    Test *tp = &t;
    int ok;

    tp->init();
    ok = tp->checkTrue(1, "returns 1");
    tp->checkEqInt(ok, 1, "checkTrue return value");
    ok = tp->checkEqInt(5, 5, "returns 1");
    tp->checkEqInt(ok, 1, "checkEqInt return value");
    ok = tp->checkEqStr("a", "a", "returns 1");
    tp->checkEqInt(ok, 1, "checkEqStr return value");
    return tp->report();
}
```

输出：

```text
6 checks, 0 failures
```

三个方法都返回 `int`：成功 1，失败 0。返回值可以直接当条件用。上面把每次的返回值再断言一次，
是为了让「返回值是 1」这件事本身也被测试覆盖。

`check*` 只记数并打印，不 `abort`、不 `return`。需要「这一步不成立就别往下跑」时，自己判断
返回值，例如 `if (!tp->checkTrue(p != 0, "allocation succeeded")) return 1;`。

### 5.2 边界：`NULL` 消息与 `NULL` 字符串

```ct
import std::test::Test;

int main(void)
{
    Test t;
    Test *tp = &t;

    tp->init();
    tp->checkTrue(1, (char *)0);
    tp->checkEqStr((char *)0, "", "NULL got is empty");
    tp->checkEqStr("", (char *)0, "NULL want is empty");
    return tp->report();
}
```

输出：

```text
3 checks, 0 failures
```

`msg` 为 `NULL` 时失败输出是 `FAIL: (no message)`；字符串实参为 `NULL` 时按 `""` 处理。
所以 `NULL` 与 `""` 在 `checkEqStr` 眼里相等——要区分「没有值」和「空串」，先单独断言
指针非空。

### 5.3 失败时的输出形态

下面这个程序有四个检查，其中三个故意失败：

```ct
import std::test::Test;

int main(void)
{
    Test t;
    Test *tp = &t;

    tp->init();
    tp->checkTrue(0, "intentional failure");
    tp->checkEqInt(1, 2, "intentional int mismatch");
    tp->checkEqStr("a", "b", "intentional string mismatch");
    tp->checkTrue(1, "a passing check");
    return tp->report();
}
```

stdout：

```text
4 checks, 3 failures
```

stderr：

```text
FAIL: intentional failure
FAIL: intentional int mismatch (got 1, want 2)
FAIL: intentional string mismatch (got 'a', want 'b')
```

退出码是 1。三种断言的输出格式都能在这里看到：`checkTrue` 只有消息，两个相等断言附带
`got` / `want`。

### 5.4 `report()` 的返回值必须交给 `main`

```ct
import std::test::Test;

int main(void)
{
    Test t;
    Test *tp = &t;

    tp->init();
    tp->checkTrue(0, "this fails");
    tp->report();
    return 0;
}
```

输出：

```text
1 checks, 1 failures
```

stderr：

```text
FAIL: this fails
```

**退出码是 0。** `report()` 本身返回 1，但这里丢掉了它。CI 只看退出码，会把这段当成通过。

## 6. 常见配方

### 在循环里断言

```ct
import std::test::Test;

int main(void)
{
    Test t;
    Test *tp = &t;
    int i;

    tp->init();
    for (i = 0; i < 3; i++) tp->checkEqInt(i * 2, i + i, "double");
    return tp->report();
}
```

输出：

```text
3 checks, 0 failures
```

### 其它常用组合

| 任务 | 写法 |
| --- | --- |
| 布尔断言 | `tp->checkTrue(p != 0, "alloc");` |
| 整数相等 | `tp->checkEqInt(got, want, "len");` |
| 字符串相等 | `tp->checkEqStr(s, "ok", "status");` |
| 失败即停 | `if (!tp->checkTrue(...)) return 1;` |
| 作为进程状态 | `return tp->report();` |
| 只想看有没有失败 | 读 `report()` 的返回值 |

## 7. 排错

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

```text
main.ct:8:1: error: no method 'nope' for type 'struct std_test_Test *'
```

### 7.2 忘了 `init()`

`init()` 是契约上的第一步。栈上声明的值恰好会被编译器按 C 聚合初始化清零，所以漏掉 `init`
往往「看起来能跑」；堆对象、`memcpy` 来的对象没有这层保障。不要依赖这个巧合，计数器必须先
清零再使用。

### 7.3 忘了返回 `report()`

见 5.4：断言失败，但 `main` 返回 0，CI 会把失败当成通过。正确写法是
`return tp->report();`。

### 7.4 只读 stdout

失败详情写 stderr。把 stderr 丢进 `/dev/null` 就只剩 `N checks, M failures`，看不到是哪条
断言失败。gate 同时检查 stdout 与 stderr。

## 8. 限制与未支持

- **没有测试框架**：没有用例注册、过滤、setup / teardown、超时、并发。
- **断言不中断**：失败只记数并返回 0；需要停止就自己判断返回值。
- **只比较 `int` 与 C 字符串**：没有浮点容差、没有结构体 / 指针相等、没有数组比较。
- **`checkEqStr` 按字节比较**：对 UTF-8 只是逐字节；`NULL` 与 `""` 被判为相等。
- **`FAIL` 写 stderr，汇总写 stdout**：只看 stdout 会漏掉失败详情。
- **不分配内存**：`msg` 与字符串实参只在调用期间被读，不被保存。
- **布局是公开的**：64 位下 `sizeof(Test) == 16`（8 字节字段 + 8 字节 vptr），vptr
  （`__testops_vptr`）出现在导出的 `Test.h` 里。
- **方法不能被内联**：每次断言都是一次间接跳转；测试代码里可以忽略。
- **字面量接收者需要 0.20.0**：v0.19.1 只接受变量、元素、调用和字段链。

## 9. 版本与变更记录

| 版本 | 状态 | 变更 |
| --- | --- | --- |
| `0.1.0` | 未打 tag、未发布 | 当前版本。文档重写：教程并入本 README，删除 `tutorial.md`；修正「汇编写 stdout」为「汇总写 stdout」；API 未变。 |

编译器下限 `ct >= 0.19.1`。本文所有程序均在 v0.19.1 上编译运行通过；字面量接收者写法需要
0.20.0。
