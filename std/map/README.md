# `std::map` — 有界哈希表（`MapStrInt` / `MapIntPtr`）

两个单态键值表：`MapStrInt` 是 `char *` → `int`，`MapIntPtr` 是 `int` → `void *`。
开放寻址、固定容量：`init` 一次定大小，`put` 装满就拒绝，**从不 rehash、从不自动扩容**。

这是 **CT 自有类型**，走原生 OO：`pub trait` + `struct ... use Trait` + `impl` 全在
[`Map.h.ct`](Map.h.ct) 里，没有 `.m.ct`、没有 shim。代价是每个对象多一个 vptr（+8 字节）
和一次间接跳转。

- 教程：[tutorial.md](tutorial.md)
- 完整可运行示例：[`test/std/map/guide.ct`](../../tests/map/guide.ct)（gate 每次编译并运行）
- 回归矩阵：[`test/std/map/main.ct`](../../tests/map/main.ct)

## import

```ct
import std::map::Map;
```

`Map` 是无状态的占位类型，只为满足「`.h.ct` 文件名 = 主类型名」的检查；真正要用的是
`MapStrInt` / `MapIntPtr` 和两个 trait，import 同一个文件就全部可见。

## 类型与 trait

| 名字 | 种类 | 说明 |
| --- | --- | --- |
| `Map` | `pub struct` | 无状态占位，没有实例化价值 |
| `MapStrIntOps` | `pub trait` | 接收者 `MapStrInt *`，键 `char *`，值 `int` |
| `MapStrInt` | `pub struct ... use MapStrIntOps` | `{ cap, n, keys, vals, used }` + vptr |
| `MapIntPtrOps` | `pub trait` | 接收者 `MapIntPtr *`，键 `int`，值 `void *` |
| `MapIntPtr` | `pub struct ... use MapIntPtrOps` | `{ cap, n, keys, vals, used, free_fn }` + vptr |

文件内顺序固定为 `trait → struct(use) → impl`：trait 必须在 `use` 它的 struct 之前，否则报
`struct 'MapStrInt' uses undefined trait 'MapStrIntOps'`。

## 方法

两个 trait 形状相同，只有键 / 值类型和 `init` 的参数不同。

### `MapStrIntOps`

| 方法 | 参数 | 返回 | 语义 |
| --- | --- | --- | --- |
| `init(want)` | `int want` | `int` | 分配容量：不小于 16、不小于 `want` 的 2 的幂；成功 0，分配失败 -1 |
| `put(key, value)` | `char *key, int value` | `int` | 插入或覆盖；表满 / `key` 为 NULL / 未 init 返回 -1 |
| `get(key, out)` | `const char *key, int *out` | `int` | 找到返回 0 并写 `*out`；没有返回 -1；`out` 可为 NULL |
| `len()` | — | `int` | 当前条目数 |
| `clear()` | — | — | 清空所有条目，表仍可用（不释放数组） |
| `free()` | — | — | `clear()` 后释放数组并把字段归零；可重复调用 |

### `MapIntPtrOps`

| 方法 | 参数 | 返回 | 语义 |
| --- | --- | --- | --- |
| `init(want, free_fn)` | `int want, void (*free_fn)(void *)` | `int` | 同上；`free_fn` 被记住，`clear()` 对每个值调用一次（可为 NULL） |
| `put(key, value)` | `int key, void *value` | `int` | 插入或覆盖；表满 / 未 init 返回 -1 |
| `get(key, out)` | `int key, void **out` | `int` | 找到返回 0 并写 `*out`；没有返回 -1；`out` 可为 NULL |
| `len()` / `clear()` / `free()` | — | — | 同 `MapStrIntOps`，区别见内存一节 |

## 内存与所有权

- **键不被复制（`MapStrInt`）。** `put` 只保存 `char *` 指针，比较用 `strcmp`。键必须活得比条目久；
  插入后修改键缓冲区会让它再也查不到（哈希按插入时的内容算过）。
- **值不被复制（`MapIntPtr`）。** 表只存 `void *`；所有权规则由 `init` 的 `free_fn` 决定：
  给了就在 `clear()` / `free()` 时对每个值调一次，没给就完全不管。
- **`clear()` 与 `free()` 不同。** `clear()` 清空条目、保留数组，表可以继续 `put`；`free()` 连数组
  一起释放。两者都可重复调用。
- 表本身分配三个数组（`keys` / `vals` / `used`），`init` 失败时已分配的会回滚，字段归零。
- **没有 `delete`**：`Drop` 在生成物里是 `static`，库单元无法销毁调用方的值类型，所以显式 `free()`。

## 布局与代价

- 64 位下裸字段 `MapStrInt` 是 32 字节、`MapIntPtr` 是 40 字节；`use` 追加 vptr 后分别是
  **40 / 48 字节**（vptr 在末尾）。
- vptr（`__mapstrintops_vptr` / `__mapintptrops_vptr`）出现在导出的 `Map.h` 里，是公开布局的一部分。
- **方法不能被内联**：`len()` / `get()` 直接调用在 `-O2` 下会被折叠，走 vtable 固定要付一次
  间接跳转（本机实测约 1 ns）。这是原生 OO 的固有属性，不要为个别方法做直接调用旁路。
- 固定容量换来的是可预测的内存：`put` 不会在背后扩容，也没有 rehash 停顿。

## 坑

1. **`init()` 是第一步。** 未 `init` 的表 `cap == 0`：`put` / `get` 一律返回 -1，`len()` 是 0。
   栈上声明的值会被编译器补 vptr、其余字段按 C 聚合初始化清零，所以漏掉 `init` 往往「看起来
   没崩」——不要依赖这个巧合。
2. **容量有下限，且不会增长。** `init(8)` 实际得到 16 个槽；`init(4)` 也是 16。装满后再 `put`
   返回 -1，表保持原样。
3. **满载时连覆盖已有键也会 -1。** 满表检查发生在查找之前，所以 `put` 一个已存在的键同样返回
   -1，而不是更新它。要更新就先 `clear()` 或留出余量。
4. **裸 `malloc` 之后直接调方法会段错误。** `malloc` 返回的内存里 vptr 是垃圾，第一次
   `p->put(...)` 就通过它跳转。正确写法是先把一个初始化过的值赋进去：

   ```ct
   MapStrInt seed;
   seed.init(4);
   MapStrInt *p = (MapStrInt *)malloc(sizeof(MapStrInt));
   *p = seed;          /* 复制 vptr 与字段 */
   p->put("k", 1);
   ```

5. **`MapStrInt` 的键是借用的。** 用局部数组或临时缓冲区当键，出了作用域就是悬垂指针；
   表在 `clear()` 时也不会释放键。
6. **`MapIntPtr` 的值归 `free_fn` 管。** 自己再 `free` 一遍就是双重释放；反过来，`init` 时传
   NULL 而值确实需要释放，就会漏。
7. **`get` 的 `out` 失败时不写。** 返回 -1 之后出参保持调用前的值，别把它当「查到了 0」。
