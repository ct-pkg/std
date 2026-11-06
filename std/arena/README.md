# `std::arena` — 块式 bump 分配器（`Arena`）

一次 `init` 拿一块内存，之后 `alloc(n)` 只做指针前进；没有逐个释放，只有整体回收
（`reset` 回收副块，`freeAll` 连第一块一起放）。

这是 **CT 自有类型**，走原生 OO：`pub trait` + `struct ... use Trait` + `impl` 全在
[`Arena.h.ct`](Arena.h.ct) 里，没有 `.m.ct`、没有 shim。

- 教程：[tutorial.md](tutorial.md)
- 完整可运行示例：[`test/std/arena/guide.ct`](../../tests/arena/guide.ct)（gate 每次编译并运行）
- 回归矩阵：[`test/std/arena/main.ct`](../../tests/arena/main.ct)

## import

```ct
import std::arena::Arena;
```

## 类型与 trait

| 名字 | 种类 | 说明 |
| --- | --- | --- |
| `ArenaOps` | `pub trait` | 接收者 `Arena *` |
| `Arena` | `pub struct ... use ArenaOps` | `{ next, mem, cap, used }` + vptr |

文件内顺序固定为 `trait → struct(use) → impl`：trait 必须在 `use` 它的 struct 之前，否则报
`struct 'Arena' uses undefined trait 'ArenaOps'`。

## 方法

| 方法 | 参数 | 返回 | 语义 |
| --- | --- | --- | --- |
| `init(block)` | `int block` | `int` | 分配第一块（`block < 16` 时按 16）；成功 0，分配失败 -1 |
| `alloc(n)` | `int n` | `void *` | 分配 `n` 字节，8 字节对齐；`n <= 0` 或拿不到块返回 `NULL` |
| `reset()` | — | — | 释放第一块之后的所有块，`used` 归零；第一块保留并复用 |
| `freeAll()` | — | — | 释放包括第一块在内的所有块；之后 `alloc` 返回 `NULL`，要再用就重新 `init` |

`alloc` 的返回内存不初始化，内容任意；调用方自己写。

## 内存与所有权

- **分配出来的内存生命周期跟着 arena 走。** 没有逐个释放的 API：要么 `reset()` 一次回收全部
  副块，要么 `freeAll()` 回收全部。
- `reset()` 之后，第一块的内存仍然有效（会被后续 `alloc` 重新切分），副块的内存已经
  `free`——指向副块的指针变成悬垂指针。
- `alloc` 拿不到内存返回 `NULL`，**不 abort**；调用方必须检查。
- 第一块的容量由 `init` 决定，`reset()` 不会缩小它；请求大于当前块时，`alloc` 会另开一块，
  容量取「本次请求」与「当前块容量」的较大者。
- 8 字节对齐：请求 1 字节也会占用 8 字节的槽位。

## 布局与代价

- 64 位下裸字段 24 字节（`next` / `mem` / `cap` / `used`），`use` 追加 vptr 后 **32 字节**。
- vptr（`__arenaops_vptr`）出现在导出的 `Arena.h` 里，是公开布局的一部分。
- **方法不能被内联**：`alloc()` 这种热路径也走一次间接跳转（本机实测约 1 ns）。这是原生 OO
  的固有属性；不要为了省这一次跳转给它加直接调用旁路。

## 坑

1. **`init()` 是第一步，而且只该做一次。** 对已经 `init` 过的 arena 再 `init`，上一块的
   `mem` 不会被释放——直接泄漏。要复用，先 `freeAll()` 再 `init`。
2. **`reset()` ≠ `freeAll()`。** `reset` 保留第一块（`cap` 不变，`alloc` 立刻可用）；
   `freeAll` 连第一块一起释放，`alloc` 从此返回 `NULL`，必须重新 `init`。
3. **没有 `free(ptr)`。** 单个分配无法归还；想「只释放一部分」就分多个 arena，各自
   `reset` / `freeAll`。
4. **`reset()` 之后旧指针不能用。** 第一块里的地址会分给下一次 `alloc`，内容会被覆盖；
   副块的内存已被 `free`，解引用是未定义行为。
5. **`alloc(0)` / `alloc(-1)` 返回 `NULL`。** 这与「分配失败」是同一个返回值，调用方无法区分；
   不要用 0 字节分配来试探剩余空间。
6. **请求大于块会另开一块。** 此时总内存超过 `block` 的大小，这是预期行为，不是泄漏；
   所有块都会被 `reset` / `freeAll` 收走。
7. **裸 `malloc` 出来的 arena 不能直接调方法。** vptr 是垃圾，第一次 `p->alloc(...)` 就会
   通过它跳转。先声明一个值，或把初始化过的值整体赋进去（见教程第 7 节）。
