# `std::arena` — 块式 bump 分配器（`Arena`）

一次 `init` 拿一块内存，之后 `alloc(n)` 只做指针前进；没有逐个释放，只有整体回收
（`reset` 回收副块，`freeAll` 连第一块一起放）。

这是 **CT 自有类型**，走原生 OO：`pub trait` + `struct ... use Trait` + `impl` 全在
[`Arena.h.ct`](Arena.h.ct) 里，没有 `.m.ct`、没有 shim。

## 1. 定位与边界

- 提供：初始化一块、按字节分配、回收副块、整体释放。
- 适合「一批对象同时出生、同时死亡」的场景：解析树、临时字符串、一次请求的中间结果。
- `alloc` 返回 8 字节对齐、**不初始化**的内存；调用方自己写。
- 不做：逐个释放、扩容已有分配、清零、线程安全、统计用量。
- `init(block)` 的 `block` 只是第一块的大小，不是总内存上限；大请求会另开块。

## 2. 依赖与安装

依赖 C 的 `<stdlib.h>`（`malloc` / `free`）；`ct.pkg` 的约束是 `requires ct >= 0.20.3`。

| 文件 | 内容 |
| --- | --- |
| `Arena.h.ct` | `ArenaOps` 一个 `pub trait`、`Arena` 一个 `pub struct ... use`、一个 `impl` |

`ARENA_ALIGN` 是 `.h.ct` 里的 `#define`，只落在本单元生成的 `.c`，不在导出的 `Arena.h` 里。

引入方式：`import std::arena::Arena;`。

包安装后（详见[包根 README](../../README.md#安装)）：

```sh
ct get std@^0.2.0
ct build src/main.ct -I vendor/std -o main --cflags "-Wall -Wextra"
```

在包根内开发时：

```sh
ct build -I . tests/arena/first.ct -o /tmp/std-arena --cflags "-Wall -Wextra"
```

## 3. 快速开始

```ct
import std::arena::Arena;
import std::test::Test;

int main(void)
{
    Test t;
    Test *tp = &t;
    Arena a;
    Arena *ap = &a;
    char *p;

    tp->init();

    tp->checkEqInt(ap->init(32), 0, "init");
    p = (char *)ap->alloc(8);
    tp->checkTrue(p != 0, "alloc");
    p[0] = 'A';
    tp->checkEqInt(p[0], 'A', "the slice is writable");

    ap->reset();
    p = (char *)ap->alloc(8);
    tp->checkTrue(p != 0, "alloc after reset");
    ap->freeAll();
    tp->checkTrue(ap->alloc(8) == 0, "alloc after freeAll fails");

    return tp->report();
}
```

`ct build` 实际执行的 `cc` 行（本机实测，路径随检出位置变化）：

```
'cc' -std=c99 -Wall -Wextra '.ctbuild/first.c' '.ctbuild/Arena.c' '.ctbuild/Test.c' '-o' '/tmp/std-arena'
```

输出：

```text
5 checks, 0 failures
```

本节整段就是 [`tests/arena/first.ct`](../../tests/arena/first.ct)；教程各段的同源程序在
[`tests/arena/guide.ct`](../../tests/arena/guide.ct)，回归矩阵在
[`tests/arena/main.ct`](../../tests/arena/main.ct)。

## 4. API 全表

### 类型与 trait

| 名字 | 种类 | 说明 |
| --- | --- | --- |
| `ArenaOps` | `pub trait` | 接收者 `Arena *` |
| `Arena` | `pub struct ... use ArenaOps` | `{ next, mem, cap, used }` + vptr |

### 方法

| 方法 | 签名 | 返回 / 失败 |
| --- | --- | --- |
| `init(block)` | `int init(int block)` | 分配第一块（`block < 16` 时按 16）；成功 0，分配失败 `-1` |
| `alloc(n)` | `void *alloc(int n)` | 分配 `n` 字节，8 字节对齐；`n <= 0`、未 init 或拿不到块返回 `NULL` |
| `reset()` | `void reset()` | 释放第一块之后的所有块，`used` 归零；第一块保留并复用 |
| `freeAll()` | `void freeAll()` | 释放包括第一块在内的所有块；之后 `alloc` 返回 `NULL`，要再用就重新 `init` |

`alloc` 返回的内存不初始化，内容任意。`reset()` / `freeAll()` 都可重复调用。

## 5. 教程

### 5.1 对齐：1 字节也占 8 字节

```ct
import std::arena::Arena;
import std::test::Test;

int main(void)
{
    Test t;
    Test *tp = &t;
    Arena a;
    Arena *ap = &a;
    char *p;
    char *q;

    tp->init();
    tp->checkEqInt(ap->init(32), 0, "init");
    p = (char *)ap->alloc(1);
    tp->checkTrue(p != 0, "alloc a byte");
    tp->checkEqInt((int)((long)p % 8), 0, "alloc is 8-byte aligned");
    q = (char *)ap->alloc(8);
    tp->checkTrue(q != 0, "alloc eight bytes");
    tp->checkEqInt((int)(q - p), 8, "a one-byte request still takes eight bytes");
    ap->freeAll();
    return tp->report();
}
```

输出：

```text
5 checks, 0 failures
```

每个请求向上取整到 8 的倍数，所以连续两次分配之间的最小距离是 8。返回值永远 8 字节对齐，
可以放心转成任何对齐要求 ≤ 8 的类型。

### 5.2 零、负数与大请求

```ct
import std::arena::Arena;
import std::test::Test;

int main(void)
{
    Test t;
    Test *tp = &t;
    Arena a;
    Arena *ap = &a;
    char *q;
    int i;

    tp->init();
    tp->checkEqInt(ap->init(32), 0, "init");
    tp->checkTrue(ap->alloc(0) == 0, "alloc of zero bytes is refused");
    tp->checkTrue(ap->alloc(-1) == 0, "alloc of a negative size is refused");
    q = (char *)ap->alloc(40);
    tp->checkTrue(q != 0, "alloc across a block boundary");
    for (i = 0; i < 40; i++) q[i] = (char)i;
    tp->checkEqInt(q[39], (char)39, "the overflow block is writable");
    ap->freeAll();
    return tp->report();
}
```

输出：

```text
5 checks, 0 failures
```

`n <= 0` 直接返回 `NULL`。这个返回值和「内存不够」一样，调用方无法区分；不要用 0 字节分配
试探剩余空间。

当前块剩余空间不够时，`alloc` 自己申请一块新的（容量取「本次请求」与「当前块容量」的较大
者），挂在第一块后面，然后从新块里切。初始化时给的 `block` 不是总内存上限，只是第一块的
大小。

### 5.3 `reset()` 与 `freeAll()`

```ct
import std::arena::Arena;
import std::test::Test;

int main(void)
{
    Test t;
    Test *tp = &t;
    Arena a;
    Arena *ap = &a;
    char *first;
    char *p;

    tp->init();
    tp->checkEqInt(ap->init(32), 0, "init");
    first = (char *)ap->alloc(8);
    tp->checkTrue(first != 0, "alloc");
    ap->reset();
    p = (char *)ap->alloc(8);
    tp->checkTrue(p != 0, "alloc after reset");
    tp->checkTrue(p == first, "reset recycles the first block");
    ap->freeAll();
    tp->checkTrue(ap->alloc(8) == 0, "alloc after freeAll fails");
    tp->checkEqInt(ap->init(32), 0, "init makes the arena usable again");
    p = (char *)ap->alloc(8);
    tp->checkTrue(p != 0, "alloc after re-init");
    ap->freeAll();
    return tp->report();
}
```

输出：

```text
7 checks, 0 failures
```

| 操作 | 副块 | 第一块 | 之后能 `alloc` 吗 |
| --- | --- | --- | --- |
| `reset()` | 全部释放 | 保留、`used` 归零 | 能，且从第一块开头重新切 |
| `freeAll()` | 全部释放 | 也释放 | 不能，必须先 `init` |

`reset()` 之后拿到的指针等于第一次分配时的地址（`p == first`），说明第一块被复用了。
上一轮的指针虽然仍指向同一块内存，但内容随时会被覆盖——逻辑上已经失效。

### 5.4 堆上的 arena

```ct
import std::arena::Arena;
import std::test::Test;

#include <stdlib.h>

int main(void)
{
    Test t;
    Test *tp = &t;
    Arena seed;
    Arena *heap;
    char *p;

    tp->init();
    seed.init(16);
    heap = (Arena *)malloc(sizeof(Arena));
    if (!heap) return 2;
    *heap = seed;
    p = (char *)heap->alloc(8);
    tp->checkTrue(p != 0, "alloc through a heap pointer");
    heap->freeAll();
    free(heap);
    return tp->report();
}
```

输出：

```text
1 checks, 0 failures
```

方法调用展开成 `p->__vptr->alloc(p, ...)`，而 `malloc` 不会写 vptr。先让 `seed` 在声明处拿到
vptr，再 `*heap = seed` 整体复制。此时 `seed` 与 `heap` 共享同一块 `mem`，只释放一次
（`heap->freeAll()`），之后不要再碰 `seed`。

## 6. 常见配方

### 一轮请求内的临时字符串

```ct
import std::arena::Arena;
import std::io::Io;

int main(void)
{
    Arena a;
    Arena *ap = &a;
    char *buf;
    int i;

    ap->init(64);
    for (i = 0; i < 3; i++) {
        buf = (char *)ap->alloc(8);
        buf[0] = (char)('a' + i);
        buf[1] = 0;
        std::io::printf("round %d: %s\n", i, buf);
    }
    ap->reset();
    ap->freeAll();
    return 0;
}
```

输出：

```text
round 0: a
round 1: b
round 2: c
```

### 其它常用组合

| 任务 | 写法 |
| --- | --- |
| 分配结构体 | `Node *n = (Node *)ap->alloc(sizeof(Node));`，自己初始化字段 |
| 分配字符串副本 | `char *p = (char *)ap->alloc(len + 1);`，自己 `copy` 并补 NUL |
| 一轮结束 | `ap->reset()`，第一块保留 |
| 整个 arena 结束 | `ap->freeAll()` |
| 分阶段回收 | 每阶段一个 `Arena`，各自 `reset()` |

## 7. 排错

### 7.1 方法名不存在

```ct
import std::arena::Arena;

int main(void)
{
    Arena a;
    Arena *ap = &a;

    return ap->nope();
}
```

```text
main.ct:8:1: error: no method 'nope' for type 'struct std_arena_Arena *'
```

### 7.2 忘了 `init()`

```ct
import std::arena::Arena;
import std::io::Io;

int main(void)
{
    Arena a;
    Arena *ap = &a;

    std::io::printf("alloc(8) is null: %d\n", ap->alloc(8) == 0);
    return 0;
}
```

输出：

```text
alloc(8) is null: 1
```

`cap == 0`，`alloc` 一律返回 `NULL`，不崩也不报错。`init` 的返回值同样要检查：分配第一块
失败时它返回 `-1`。

### 7.3 对活着的 arena 再 `init()`

`init` 不做「先释放旧块」这件事。对已经 `init` 过的 arena 再 `init`，上一块的 `mem` 不会被
释放——直接泄漏。要复用，先 `freeAll()` 再 `init`。

### 7.4 `reset()` 之后继续用旧指针

`reset()` 的语义是「这些分配全部作废」，不是「暂时保留但不能再分配」。第一块里的地址会分给
下一次 `alloc`，内容会被覆盖；副块的内存已被 `free`，解引用是未定义行为。

## 8. 限制与未支持

- **没有逐个释放**：没有 `free(ptr)`。想「只释放一部分」就分多个 arena，各自 `reset` /
  `freeAll`。
- **`alloc` 不初始化、不清零**：内存内容任意。
- **`alloc(0)` / `alloc(-1)` 与分配失败同返回值**：都是 `NULL`，调用方无法区分。
- **`init` 不幂等**：对活着的 arena 再 `init` 会泄漏上一块，见 7.3。
- **没有用量统计**：没有 `used` / `remaining` 查询；容量不是 API。
- **布局是公开的**：64 位下 `sizeof(Arena) == 32`（24 字节字段 + 8 字节 vptr），vptr
  （`__arenaops_vptr`）出现在导出的 `Arena.h` 里。
- **方法不能被内联**：`alloc()` 这种热路径也走一次间接跳转；这是 `use` 结构体的机制属性。
- **裸 `malloc` 的 arena 不能直接调方法**：vptr 是垃圾，第一次 `p->alloc(...)` 就通过它跳转
  （本机实测 exit 139）。先声明值，或把初始化过的值整体赋进去——见 5.4。
- **字面量接收者从 0.20.0 起可用**（本包下限 0.20.3）：v0.19.1 只接受变量、元素、调用和字段链。

## 9. 版本与变更记录

| 版本 | 状态 | 变更 |
| --- | --- | --- |
| `0.1.0` | 未打 tag、未发布 | 当前版本。文档重写：教程并入本 README，删除 `tutorial.md`；API 未变。 |

编译器下限 `ct >= 0.20.3`。本文所有程序均在 v0.20.3 上编译运行通过；字面量接收者写法从
0.20.0 起可用。
