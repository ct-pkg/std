# `std::vec` — 动态数组（`VecInt` / `VecStr`）

两个单态动态数组：`VecInt` 存 `int`，`VecStr` 存 `char *`。CT 没有泛型，所以 v1 直接给这两个
形状；调用方不要依赖字段布局。

这是 **CT 自有类型**，走原生 OO：`pub trait` + `struct ... use Trait` + `impl` 全在
[`Vec.h.ct`](Vec.h.ct) 里，没有 `.m.ct`、没有 shim。代价是每个对象多一个 vptr（+8 字节）和
一次间接跳转，收益是统一的 `p->push(x)` 形态。

## 1. 定位与边界

- 提供：追加、按下标读、弹出末尾、长度、释放。
- 元素类型只有两种：`int`（`VecInt`）与借用的 `char *`（`VecStr`）。
- **不拥有元素**：`VecStr` 只保存指针，不复制字符串，`free()` 也不释放它们。
- 不做：去重、排序、按值查找、按下标删除、`reserve` / `shrink`。
- 容量是内部状态，不暴露、也不该依赖；扩容策略（倍增）不是 API 承诺。

## 2. 依赖与安装

依赖 C 的 `<stdlib.h>`（`realloc` / `free`）；`ct.pkg` 的约束是 `requires ct >= 0.20.3`。

| 文件 | 内容 |
| --- | --- |
| `Vec.h.ct` | `Vec` 占位类型、`VecIntOps` / `VecStrOps` 两个 `pub trait`、两个 `pub struct ... use`、两个 `impl` |

`VEC_MIN_CAP` 是 `.h.ct` 里的 `#define`，只落在本单元生成的 `.c`，不在导出的 `Vec.h` 里。

引入方式：`import std::vec::Vec;`。`Vec` 只是满足「`.h.ct` 文件名 = 主类型名」的占位类型，
真正要用的是 `VecInt` / `VecStr` 和两个 trait，一次 import 全部可见。

包安装后（详见[包根 README](../../README.md#安装)）：

```sh
ct get std@^0.2.0
ct build src/main.ct -I vendor/std -o main --cflags "-Wall -Wextra"
```

在包根内开发时：

```sh
ct build -I . tests/vec/first.ct -o /tmp/std-vec --cflags "-Wall -Wextra"
```

## 3. 快速开始

```ct
import std::vec::Vec;
import std::test::Test;

int main(void)
{
    Test t;
    Test *tp = &t;
    VecInt v;
    VecInt *vp = &v;
    int x;

    tp->init();

    vp->init();
    tp->checkEqInt(vp->push(10), 0, "push");
    tp->checkEqInt(vp->push(20), 0, "push");
    tp->checkEqInt(vp->len(), 2, "len");
    tp->checkEqInt(vp->at(1, &x), 0, "at");
    tp->checkEqInt(x, 20, "at value");
    vp->free();

    return tp->report();
}
```

`ct build` 实际执行的 `cc` 行（本机实测，路径随检出位置变化）：

```
'cc' -std=c99 -Wall -Wextra '.ctbuild/first.c' '.ctbuild/Vec.c' '.ctbuild/Test.c' '-o' '/tmp/std-vec'
```

输出：

```text
5 checks, 0 failures
```

本节整段就是 [`tests/vec/first.ct`](../../tests/vec/first.ct)；教程各段的同源程序在
[`tests/vec/guide.ct`](../../tests/vec/guide.ct)，回归矩阵在
[`tests/vec/main.ct`](../../tests/vec/main.ct)。

## 4. API 全表

### 类型与 trait

| 名字 | 种类 | 说明 |
| --- | --- | --- |
| `Vec` | `pub struct` | 无状态占位，没有实例化价值 |
| `VecIntOps` | `pub trait` | 接收者 `VecInt *`，元素 `int` |
| `VecInt` | `pub struct ... use VecIntOps` | `{ int *data; int n; int cap; }` + vptr |
| `VecStrOps` | `pub trait` | 接收者 `VecStr *`，元素 `char *` |
| `VecStr` | `pub struct ... use VecStrOps` | `{ char **data; int n; int cap; }` + vptr |

### 方法

两个 trait 名字与形状相同，只有元素类型不同。下表以 `VecInt` 为准，`VecStr` 把 `int` 换成
`char *`。

| 方法 | 签名（`VecInt`） | 返回 / 失败 |
| --- | --- | --- |
| `init()` | `void init()` | 把 `data` / `n` / `cap` 归零；**契约上的第一步** |
| `len()` | `int len()` | 当前元素个数 |
| `push(x)` | `int push(int x)` | 成功 0；扩容失败 `-1`，向量保持原样 |
| `pop(out)` | `int pop(int *out)` | 成功 0 并写 `*out`；空向量 `-1` |
| `at(i, out)` | `int at(int i, int *out)` | 成功 0 并写 `*out`；越界 `-1` |
| `free()` | `void free()` | 释放元素数组并把字段归零；可重复调用 |

`VecStr` 的对应签名是 `push(char *s)`、`pop(char **out)`、`at(int i, char **out)`。

`at` / `pop` 的出参可以是 `NULL`（只取值不写），但返回值仍然报告成败。`push` 是唯一会分配
的方法，返回值必须检查。

## 5. 教程

### 5.1 读与删：越界是返回值

```ct
import std::vec::Vec;
import std::test::Test;

int main(void)
{
    Test t;
    Test *tp = &t;
    VecInt v;
    VecInt *vp = &v;
    int x;

    tp->init();
    vp->init();
    tp->checkEqInt(vp->push(10), 0, "push 10");
    tp->checkEqInt(vp->push(20), 0, "push 20");
    tp->checkEqInt(vp->at(1, &x), 0, "at(1)");
    tp->checkEqInt(x, 20, "at(1) value");
    tp->checkEqInt(vp->at(2, &x), -1, "at past the end");
    tp->checkEqInt(vp->at(-1, &x), -1, "at before the start");
    tp->checkEqInt(x, 20, "a failed at() leaves the out-param alone");
    tp->checkEqInt(vp->pop(&x), 0, "pop");
    tp->checkEqInt(x, 20, "pop returns the last element");
    tp->checkEqInt(vp->len(), 1, "len after pop");
    tp->checkEqInt(vp->pop(&x), 0, "pop the last element");
    tp->checkEqInt(vp->pop(&x), -1, "pop on an empty vector");
    vp->free();
    tp->checkEqInt(vp->len(), 0, "free empties the vector");
    return tp->report();
}
```

输出：

```text
13 checks, 0 failures
```

`at` / `pop` 把结果写进出参，用返回值报告成败。越界是 `-1`，不会断言也不会崩溃——所以**必须
检查返回值**，否则 `x` 还是上一次的旧值。上面第 7 条断言就是在验证这一点：两次失败的 `at`
之后，`x` 仍然是 `20`。

### 5.2 增长：倍增，容量不是 API

```ct
import std::vec::Vec;
import std::test::Test;

int main(void)
{
    Test t;
    Test *tp = &t;
    VecInt v;
    VecInt *vp = &v;
    int i;
    int x;

    tp->init();
    vp->init();
    for (i = 0; i < 100; i++) tp->checkEqInt(vp->push(i), 0, "push");
    tp->checkEqInt(vp->len(), 100, "len after growth");
    tp->checkEqInt(vp->at(100, &x), -1, "at(100) is out of range");
    tp->checkEqInt(vp->at(99, &x), 0, "at(99)");
    tp->checkEqInt(x, 99, "the last element value");
    vp->free();
    return tp->report();
}
```

输出：

```text
104 checks, 0 failures
```

初始容量为 0，第一次 `push` 才分配；之后按倍增扩容。分配失败时 `push` 返回 `-1` 且向量保持
原样，不存在「半个元素进去」的中间状态。容量本身不暴露，也不该依赖。

### 5.3 `VecStr` 只存指针

```ct
import std::vec::Vec;
import std::test::Test;

int main(void)
{
    Test t;
    Test *tp = &t;
    VecStr names;
    VecStr *np = &names;
    char *name;

    tp->init();
    np->init();
    tp->checkEqInt(np->push("alpha"), 0, "push a borrowed string");
    tp->checkEqInt(np->push("beta"), 0, "push another");
    tp->checkEqInt(np->len(), 2, "len");
    tp->checkEqInt(np->at(0, &name), 0, "at(0)");
    tp->checkEqStr(name, "alpha", "the pointer comes back unchanged");
    tp->checkEqInt(np->pop(&name), 0, "pop");
    tp->checkEqStr(name, "beta", "pop returns the last pointer");
    np->free();
    tp->checkEqInt(np->len(), 0, "free empties the vector");
    return tp->report();
}
```

输出：

```text
8 checks, 0 failures
```

`VecStr` **不复制**字符串，也不在 `free()` 时释放它们。存进去的指针必须活得比向量久：
用局部数组 `char buf[8]` 做 `push`，`buf` 出了作用域后向量里留下的就是悬垂指针；插入之后
修改字符串内容，取回来的也是新内容（向量只保存指针）。需要拥有的字符串，要在 `push` 之前
自己 `malloc` / 复制，并在 `free()` 之后自己释放。上面运行示例 push 的是字面量 `"alpha"`，
它的生命期是整个程序。

### 5.4 堆对象：不要裸 `malloc` 之后直接调方法

```ct
import std::vec::Vec;
import std::test::Test;

#include <stdlib.h>

int main(void)
{
    Test t;
    Test *tp = &t;
    VecInt seed;
    VecInt *heap;
    int x;

    tp->init();
    seed.init();
    heap = (VecInt *)malloc(sizeof(VecInt));
    if (!heap) return 2;
    *heap = seed;
    tp->checkEqInt(heap->push(7), 0, "push through a heap pointer");
    tp->checkEqInt(heap->at(0, &x), 0, "at through a heap pointer");
    tp->checkEqInt(x, 7, "heap value");
    heap->free();
    free(heap);
    return tp->report();
}
```

输出：

```text
3 checks, 0 failures
```

方法调用展开成 `p->__vptr->push(p, ...)`，第一步就是读 vptr。`malloc` 不会替你写它，
下面这个完整程序在 v0.19.1 上**本机实测 exit 139**：

```ct
import std::vec::Vec;

#include <stdlib.h>

int main(void)
{
    VecInt *p = (VecInt *)malloc(sizeof(VecInt));

    p->init();
    return 0;
}
```

正确写法就是上面的 `seed` + `*heap = seed`：先让一个值在声明处拿到 vptr，再整体复制。
想避免这一步，就根本别 `malloc` 结构体——声明一个值，把它的地址传出去。

### 5.5 `.` 和 `->`

```ct
import std::vec::Vec;
import std::test::Test;

int main(void)
{
    Test t;
    Test *tp = &t;
    VecInt local;

    tp->init();
    local.init();
    local.push(3);
    tp->checkEqInt(local.len(), 1, "value receiver");
    local.free();
    return tp->report();
}
```

输出：

```text
1 checks, 0 failures
```

值接收者也合法：编译器在声明处注入 vptr，并把 `&local` 传进方法。区别只在于对象怎么来的：

| 写法 | 适用 | 原因 |
| --- | --- | --- |
| `vp->push(x)` | 指针、堆对象、任何 `VecInt *` | 显式；vptr 必须已经就位 |
| `v.push(x)` | 本函数内声明的值 | 声明处的初始化器负责 vptr |

库和本文统一用 `->`，因为它在两种情况下都成立。对值用 `->` 是编译错误（见 7.2）。

## 6. 常见配方

### 遍历并求和

```ct
import std::vec::Vec;
import std::io::Io;
import std::num::Num;

int main(void)
{
    VecInt v;
    VecInt *vp = &v;
    int i;
    int x;
    int n;
    int sum = 0;

    vp->init();
    vp->push(3);
    vp->push(7);
    vp->push(5);
    for (i = 0; i < vp->len(); i++) {
        vp->at(i, &x);
        sum = sum + x;
    }
    n = vp->len();
    std::io::printf("n=%s sum=%s\n", n.toString(), sum.toString());
    vp->free();
    return 0;
}
```

输出：

```text
n=3 sum=15
```

`n` 先赋给变量，是因为 v0.19.1 不接受字面量接收者，也不接受把 OO 方法调用的结果直接当另一个
方法的接收者；`vp->len()` 的返回值要先落到 `int n`。

### 其它常用组合

| 任务 | 写法 |
| --- | --- |
| 追加 | `if (vp->push(x) != 0) { /* 分配失败 */ }` |
| 读第 `i` 个 | `if (vp->at(i, &x) != 0) { /* 越界 */ }` |
| 弹出末尾 | `if (vp->pop(&x) == 0) { /* 得到 x */ }` |
| 清空但保留对象 | `vp->free()` 后重新 `vp->init()` |
| 清空并释放 | `vp->free()`（可重复调用） |
| 临时字符串列表 | `VecStr`，但保证每个 `char *` 活得够久 |

## 7. 排错

### 7.1 方法名不存在

```ct
import std::vec::Vec;

int main(void)
{
    VecInt v;
    VecInt *vp = &v;

    vp->init();
    return vp->nope();
}
```

```text
main.ct:9:1: error: no method 'nope' for type 'struct std_vec_VecInt *'
```

方法按名字绑定到接收者类型；不存在的名字直接报在调用点。注意接收者类型打印的是生成的 C 名
`struct std_vec_VecInt *`。

### 7.2 对值用 `->`

```ct
import std::vec::Vec;

int main(void)
{
    VecInt v;

    v->push(1);
    return 0;
}
```

```text
main.ct:8:6: error: member reference type 'struct std_vec_VecInt' is not a pointer; did you mean to use '.'?
    8 |     v->__std_vec_vecintops_vptr->push(v, 1);
      |     ~^~
      |      .
main.ct:8:39: error: passing 'struct std_vec_VecInt' to parameter of incompatible type 'void *'
    8 |     v->__std_vec_vecintops_vptr->push(v, 1);
      |                                       ^
2 errors generated.
```

这是 clang 对生成 C 的报错；`v->...` 已经被改写成 vtable 调用，所以信息里能看到内部名字。
本函数内声明的值用 `v.push(1)`，指针用 `vp->push(1)`。

### 7.3 裸 `malloc` 后直接调方法

`malloc` 返回的内存里 vptr 是垃圾，第一次 `p->init()` / `p->push(...)` 就通过它跳转，
**本机实测 exit 139**。正确写法见 5.4：先声明值拿到 vptr，再 `*heap = seed`。

### 7.4 忘了检查 `at` / `push` 的返回值

不报错，但会读到旧数据或漏掉一次分配失败。失败时出参**不被写**，`x` 保持调用前的值；
分配失败时向量保持原样，不会部分追加。见 5.1 的断言。

## 8. 限制与未支持

- **没有泛型**：只有 `VecInt` / `VecStr` 两个单态形状，没有 `Vec<T>`、没有自定义元素类型。
- **没有按值操作**：没有 `find` / `remove` / `contains` / `sort` / `reverse`。
- **没有容量控制**：没有 `reserve` / `shrinkToFit`；容量不暴露。
- **`VecStr` 不拥有元素**：不复制、不释放；字符串生命周期由调用方负责。
- **`free()` 之后再 `push` 必须先 `init()`**：`free()` 把字段归零，但不会自动重新准备对象。
- **布局是公开的**：64 位下 `sizeof(VecInt) == sizeof(VecStr) == 24`（16 字节字段 + 8 字节
  vptr），vptr（`__vecintops_vptr` / `__vecstroops_vptr`）出现在导出的 `Vec.h` 里。
- **方法不能被内联**：`at()` / `len()` 经 vtable 分派，固定付出一次间接跳转；这是 `use`
  结构体的机制属性，不要为个别方法退回直接调用。
- **字面量接收者从 0.20.0 起可用**（本包下限 0.20.3）：v0.19.1 只接受变量、元素、调用和字段链。
- **没有析构**：`VecInt` 放在别的结构体里时，谁持有谁负责调用 `free()`。

## 9. 版本与变更记录

| 版本 | 状态 | 变更 |
| --- | --- | --- |
| `0.1.0` | 未打 tag、未发布 | 当前版本。文档重写：教程并入本 README，删除 `tutorial.md`；API 未变。 |

编译器下限 `ct >= 0.20.3`。本文所有程序均在 v0.20.3 上编译运行通过；字面量接收者写法从
0.20.0 起可用。
