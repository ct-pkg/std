# `std::test` — 断言计数器（`Test`）

一个极小的断言工具：每次 `check*` 自增计数，失败时往 stderr 打一行 `FAIL: ...`，
`report()` 打印汇总并返回进程状态码。

这是 **CT 自有类型**，走原生 OO：`pub trait` + `struct ... use Trait` + `impl` 全在
[`Test.h.ct`](Test.h.ct) 里，没有 `.m.ct`、没有 shim。

- 教程：[tutorial.md](tutorial.md)
- 完整可运行示例：[`test/std/test/guide.ct`](../../tests/test/guide.ct)（gate 每次编译并运行）
- 负向 fixture：[`test/std/test/main.ct`](../../tests/test/main.ct)（故意失败，gate 校验它的输出）

## import

```ct
import std::test::Test;
```

## 类型与 trait

| 名字 | 种类 | 说明 |
| --- | --- | --- |
| `TestOps` | `pub trait` | 接收者 `Test *` |
| `Test` | `pub struct ... use TestOps` | `{ checks, failures }` + vptr |

文件内顺序固定为 `trait → struct(use) → impl`。

## 方法

| 方法 | 参数 | 返回 | 语义 |
| --- | --- | --- | --- |
| `init()` | — | — | 把 `checks` / `failures` 归零。**契约上的第一步** |
| `checkTrue(cond, msg)` | `int cond, const char *msg` | `int` | 计一次检查；`cond` 非零返回 1，否则记一次失败、打 `FAIL` 并返回 0 |
| `checkEqInt(got, want, msg)` | `int, int, const char *` | `int` | 计一次检查；相等返回 1，否则记失败、打 `FAIL` 并返回 0 |
| `checkEqStr(got, want, msg)` | `const char *, const char *, const char *` | `int` | `strcmp` 比较；`NULL` 当 `""`；返回 1 / 0 |
| `report()` | — | `int` | 向 stdout 打印 `<n> checks, <m> failures`；`m == 0` 返回 0，否则 1 |

失败的输出形态（每条一行，写 stderr）：

```
FAIL: <msg>
FAIL: <msg> (got <十进制>, want <十进制>)
FAIL: <msg> (got '<字符串>', want '<字符串>')
```

`msg` 传 `NULL` 时输出 `(no message)`。

## 内存与所有权

- **本模块不分配任何内存。** `msg` 与字符串实参只在调用期间被读，不被保存。
- `checkEqStr` 用 `strcmp`，对 UTF-8 只是逐字节比较；`NULL` 一律当空串。
- 没有资源需要释放；`Test` 是纯计数器。

## 布局与代价

- 64 位下裸字段 8 字节（两个 `int`），`use` 追加 vptr 后 **16 字节**。
- vptr（`__testops_vptr`）出现在导出的 `Test.h` 里，是公开布局的一部分。
- **方法不能被内联**：每次断言都是一次间接跳转（约 1 ns），测试代码里可以忽略。

## 坑

1. **`init()` 是第一步。** 计数器不清零就调用 `report()`，失败数会从上一次的状态接着算。
2. **`report()` 的返回值必须交给 `main`。** 只调用不返回，进程照样退出 0，CI 会把失败的测试
   当成通过。
3. **检查不会中断执行。** `check*` 失败只记数并返回 0，后面的语句照常跑；需要「一失败就停」
   就自己判断返回值：`if (!tp->checkTrue(...)) return 1;`。
4. **`FAIL` 写 stderr，汇编写 stdout。** 只看 stdout 会漏掉失败详情；gate 同时检查两者。
5. **`checkEqStr` 是按字节比较，且把 `NULL` 当 `""`。** `NULL` 与 `""` 会被判为相等；
   想区分「没有值」和「空串」，先用 `checkTrue(s != 0, ...)`。
6. **断言宏不能跨单元。** 这是本模块提供真实方法而不是 `#define ASSERT(...)` 的原因：
   宏在另一个编译单元里不可见，方法可以。
