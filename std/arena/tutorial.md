# 教程：用 `std::arena` 批量分配

目标：从空文件开始，写出一个能分配、能整体回收的 arena，分清 `reset()` 与 `freeAll()`，
并知道分配出来的内存归谁、什么时候失效。

模块参考（方法全集、布局、所有权）：[README.md](README.md)。

> 标了「可运行」的代码块都来自 [`test/std/arena/guide.ct`](../../tests/arena/guide.ct)，
> 第 2 节的第一个程序整段就是 [`test/std/arena/first.ct`](../../tests/arena/first.ct)。
> 拆段时只重复了必要的变量声明，逻辑与顺序和文件一致。gate（`test/std/run.sh`）每次都编译
> 并运行这两个文件。带 `error:` 输出或标「错误示意」的块则是故意不通过的。

## 1. 准备

```sh
cd /path/to/ct
ct build -I . hello.ct -o hello
```

```ct
import std::arena::Arena;
```

## 2. 第一个程序：分配、写、回收

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

`alloc` 返回 `void *`，转成你要的类型即可；内存不初始化。`freeAll()` 之后 `alloc` 返回 `NULL`，
这不是错误，是「arena 已经空了」。

## 3. 对齐：1 字节也占 8 字节

```ct
    p = (char *)ap->alloc(1);
    tp->checkTrue(p != 0, "alloc a byte");
    tp->checkEqInt((int)((long)p % 8), 0, "alloc is 8-byte aligned");
    first = p;
    q = (char *)ap->alloc(8);
    tp->checkTrue(q != 0, "alloc eight bytes");
    tp->checkEqInt((int)(q - p), 8, "a one-byte request still takes eight bytes");
```

每个请求向上取整到 8 的倍数，所以连续两次分配之间的最小距离是 8。返回值永远 8 字节对齐，
可以放心转成任何对齐要求 ≤ 8 的类型。

## 4. 零与负数：`NULL`，不是断言

```ct
    tp->checkTrue(ap->alloc(0) == 0, "alloc of zero bytes is refused");
    tp->checkTrue(ap->alloc(-1) == 0, "alloc of a negative size is refused");
```

`n <= 0` 直接返回 `NULL`。这个返回值和「内存不够」一样，调用方无法区分；不要用 0 字节分配
试探剩余空间。

## 5. 大请求：另开一块

```ct
    q = (char *)ap->alloc(40);
    tp->checkTrue(q != 0, "alloc across a block boundary");
    for (i = 0; i < 40; i++) q[i] = (char)i;
    tp->checkEqInt(q[39], (char)39, "the overflow block is writable");
```

当前块剩余空间不够时，`alloc` 自己申请一块新的（容量取「本次请求」和「当前块容量」的较大者），
挂在第一块后面，然后从新块里切。初始化时给的 `block` 不是总内存上限，只是第一块的大小。

## 6. `reset()` 与 `freeAll()`

```ct
    ap->reset();
    p = (char *)ap->alloc(8);
    tp->checkTrue(p != 0, "alloc after reset");
    tp->checkTrue(p == first, "reset recycles the first block");
    tp->checkEqInt((int)((long)p % 8), 0, "reset keeps the alignment");
```

```ct
    ap->freeAll();
    tp->checkTrue(ap->alloc(8) == 0, "alloc after freeAll fails");
    tp->checkEqInt(ap->init(32), 0, "init makes the arena usable again");
    p = (char *)ap->alloc(8);
    tp->checkTrue(p != 0, "alloc after re-init");
    ap->freeAll();
```

| 操作 | 副块 | 第一块 | 之后能 `alloc` 吗 |
| --- | --- | --- | --- |
| `reset()` | 全部释放 | 保留、`used` 归零 | 能，且从第一块开头重新切 |
| `freeAll()` | 全部释放 | 也释放 | 不能，必须先 `init` |

`reset()` 之后拿到的指针等于第一次分配时的地址（`p == first`），说明第一块被复用了。
上一轮的指针虽然仍指向同一块内存，但内容随时会被覆盖——逻辑上已经失效。

## 7. 堆上的 arena

```ct
    seed.init(16);
    heap = (Arena *)malloc(sizeof(Arena));
    if (!heap) return 2;
    *heap = seed;
    p = (char *)heap->alloc(8);
    tp->checkTrue(p != 0, "alloc through a heap pointer");
    heap->freeAll();
    free(heap);
```

方法调用展开成 `p->__vptr->alloc(p, ...)`，而 `malloc` 不会写 vptr。先让 `seed` 在声明处拿到
vptr，再 `*heap = seed` 整体复制。此时 `seed` 与 `heap` 共享同一块 `mem`，只释放一次
（`heap->freeAll()`），之后不要再碰 `seed`。

## 8. 常见错误

### 8.1 方法名不存在

```ct
import std::arena::Arena;

int main(void)
{
    Arena a;
    Arena *ap = &a;

    return ap->nope();
}
```

```
main.ct:8:1: error: no method 'nope' for type 'struct std_arena_Arena *'
```

### 8.2 对活着的 arena 再 `init()`

```ct
/* 错误示意：第一次 init 分配的 mem 被覆盖，直接泄漏 */
Arena a;
Arena *ap = &a;
ap->init(32);
ap->init(64);   /* 上一块 mem 没人释放 */
```

`init` 不做「先释放旧块」这件事。要复用，先 `freeAll()` 再 `init`。

### 8.3 忘了 `init()`

```ct
/* 错误示意：cap == 0，alloc 一律返回 NULL，不崩也不报错 */
Arena a;
Arena *ap = &a;
void *p = ap->alloc(8);
```

### 8.4 `reset()` 之后继续用旧指针

```ct
/* 错误示意：reset 之后第一块会被重新切分，旧内容随时被覆盖 */
ap->reset();
p[0] = 'x';     /* p 指向的槽位可能已经分给别人 */
```

`reset()` 的语义是「这些分配全部作废」，不是「暂时保留但不能再分配」。

## 9. 下一步

- 动态数组：[`std::vec`](../vec/README.md)（元素数组自己管理，不借用 arena）
- 键值表：[`std::map`](../map/README.md)
- 字符串：[`std::str`](../str/README.md)
