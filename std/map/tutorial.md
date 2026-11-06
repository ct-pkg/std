# 教程：用 `std::map` 存键值对

目标：从空文件开始，写出一个能插入、能覆盖、能查、能正确释放的 `MapStrInt`，再换成带
`free_fn` 的 `MapIntPtr`，并且知道容量为什么不会自己长、堆对象为什么要先赋值再调方法。

模块参考（方法全集、布局、所有权）：[README.md](README.md)。

> 标了「可运行」的代码块都来自 [`test/std/map/guide.ct`](../../tests/map/guide.ct)，第 2 节的
> 第一个程序整段就是 [`test/std/map/first.ct`](../../tests/map/first.ct)。拆段时只重复了必要
> 的变量声明，逻辑与顺序和文件一致。gate（`test/std/run.sh`）每次都编译并运行这两个文件。
> 带 `error:` 输出或标「错误示意」的块则是故意不通过的。

## 1. 准备

```sh
cd /path/to/ct
ct build -I . hello.ct -o hello
```

```ct
import std::map::Map;
```

`Map` 是文件名对应的占位类型；`MapStrInt` / `MapIntPtr` 和两个 trait 都在同一个单元里，
一次 import 全部可见。

## 2. 第一个程序：插入、查询、释放

```ct
import std::map::Map;
import std::test::Test;

int main(void)
{
    Test t;
    Test *tp = &t;
    MapStrInt m;
    MapStrInt *mp = &m;
    int v;

    tp->init();

    tp->checkEqInt(mp->init(8), 0, "init is the first step");
    tp->checkEqInt(mp->put("one", 1), 0, "put");
    tp->checkEqInt(mp->get("one", &v), 0, "get");
    tp->checkEqInt(v, 1, "the stored value");
    tp->checkEqInt(mp->get("two", &v), -1, "a missing key");
    tp->checkEqInt(mp->len(), 1, "len");
    mp->free();

    return tp->report();
}
```

三件事按顺序发生：

1. `init` 分配三个数组并定下容量；
2. `put` / `get` 用返回值报告成败，`get` 通过出参给值；
3. `free()` 释放数组。之后表回到「未 init」状态，要再用就重新 `init`。

`mp` 不是必需的花招：`MapStrInt *mp = &m` 只是把「接收者是指针」写明白。

## 3. 覆盖与缺失

```ct
    tp->checkEqInt(mp->put("k0", 10), 0, "put");
    tp->checkEqInt(mp->get("k0", &v), 0, "get");
    tp->checkEqInt(v, 10, "the stored value");
    tp->checkEqInt(mp->put("k0", 11), 0, "overwrite");
    tp->checkEqInt(mp->get("k0", &v), 0, "get after overwrite");
    tp->checkEqInt(v, 11, "the overwritten value");
    tp->checkEqInt(mp->len(), 1, "an overwrite does not grow the table");
    tp->checkEqInt(mp->get("missing", &v), -1, "a missing key");
```

`put` 对已存在的键是**覆盖**，`len()` 不变。`get` 找不到返回 -1，此时 `v` 不被写；`out`
本身可以是 `NULL`（只关心有没有）。

## 4. 键按内容比较，但指针不被复制

```ct
    tp->checkEqInt("kk".copy(key, 8), 0, "seed a mutable key buffer");
    tp->checkEqInt(mp->put(key, 20), 0, "put with a buffer key");
    tp->checkEqInt(mp->get("kk", &v), 0, "lookup through another pointer");
    tp->checkEqInt(v, 20, "equal content finds the entry");
```

`MapStrInt` 用 `strcmp` 比较键，所以 put 用的指针和 get 用的指针不要求相同，内容相等即可。
但**表保存的是 put 时那个指针**，不复制字符串：`key` 必须活得比条目久。插入之后修改
`key` 的内容会让哈希与存储位置不一致，这个条目就再也查不到了。

`MapIntPtr` 相反：键是 `int`，按值保存；值是 `void *`，只存指针。

## 5. 固定容量：不会自己长

```ct
    tp->checkEqInt(pm->init(4, drop), 0, "init sizes up to the 16 minimum");
    for (i = 0; i < 16; i++) {
        int *p = (int *)malloc(sizeof(int));
        if (!p) return 1;
        *p = i * 10;
        tp->checkEqInt(pm->put(i, p), 0, "fill a slot");
    }
    tp->checkEqInt(pm->len(), 16, "the table is full");
    tp->checkEqInt(pm->put(99, 0), -1, "a full table refuses a new key");
```

`init(want)` 取「不小于 16、不小于 `want` 的 2 的幂」：`init(4)` 与 `init(8)` 都是 16 个槽。
满了之后 `put` 返回 -1，表保持原样，不会扩容、不会 rehash。

注意：满表检查发生在查找之前，所以**满载时覆盖一个已存在的键也会返回 -1**。需要更新就先
`clear()`，或者一开始留出余量。

## 6. `clear()` 与 `free()`

```ct
    mp->clear();
    tp->checkEqInt(mp->len(), 0, "clear empties the table");
    tp->checkEqInt(mp->get("k0", &v), -1, "get after clear");
    tp->checkEqInt(mp->put("k0", 1), 0, "put after clear");
    mp->free();
```

`clear()` 只把槽位标记为空、保留数组，所以之后能继续 `put`；`free()` 连数组一起释放。
`MapStrInt` 的 `clear()` **不释放键**（键是借用的），`MapIntPtr` 的 `clear()` 会对每个值调用
`init` 时给的 `free_fn`。

## 7. `MapIntPtr` 与 `free_fn`

```ct
static void drop(void *p)
{
    free(p);
}
```

```ct
    tp->checkEqInt(pm->init(4, drop), 0, "init sizes up to the 16 minimum");
    for (i = 0; i < 16; i++) {
        int *p = (int *)malloc(sizeof(int));
        if (!p) return 1;
        *p = i * 10;
        tp->checkEqInt(pm->put(i, p), 0, "fill a slot");
    }
```

值是自己 `malloc` 的，就把释放规则在 `init` 时一次说清：`clear()` / `free()` 会对每个值调用
`drop`。传 `NULL` 表示表完全不碰值。**不要**在 `clear()` 之后自己再释放一遍。

```ct
    tp->checkEqInt(pm->get(15, &out), 0, "the last key is found");
    tp->checkEqInt(*(int *)out, 150, "the last value");
    tp->checkEqInt(pm->get(99, &out), -1, "a missing key");
    pm->clear();
    tp->checkEqInt(pm->len(), 0, "clear runs free_fn and drops the entries");
    pm->free();
```

`get` 的出参是 `void **`，取出来要转回真实类型（`*(int *)out`）——表只保证它存的是你放进去
的那个指针。

## 8. 堆上的表：不要裸 `malloc` 之后直接调方法

```ct
MapStrInt *p = (MapStrInt *)malloc(sizeof(MapStrInt));
p->put("k", 1);     /* 段错误：p 里的 vptr 是垃圾（实测 exit 139） */
```

方法调用展开成 `p->__vptr->put(p, ...)`，第一步就是读 vptr。正确写法是先准备一个初始化过的值，
再整体赋进去：

```ct
    seed.init(4);
    heap = (MapStrInt *)malloc(sizeof(MapStrInt));
    if (!heap) return 2;
    *heap = seed;
    tp->checkEqInt(heap->put("h", 7), 0, "put through a heap pointer");
    tp->checkEqInt(heap->get("h", &v), 0, "get through a heap pointer");
    tp->checkEqInt(v, 7, "heap value");
    heap->free();
    free(heap);
```

`seed` 在声明处由编译器补上 vptr，`*heap = seed` 把 vptr 和字段一起复制过去。注意此时
`seed` 与 `heap` 共享同一组数组，只能释放一次（`heap->free()`），之后不要再碰 `seed`。

## 9. `.` 和 `->`

和 [`std::vec`](../vec/README.md) 一样：库里的写法统一用 `->`，栈上值和堆对象都成立；
本函数内声明的值也接受 `m.put(...)`（编译器在声明处注入 vptr 并传 `&m`）。
对值写 `m->...` 是编译错误。

## 10. 常见错误

### 10.1 方法名不存在

```ct
import std::map::Map;

int main(void)
{
    MapStrInt m;
    MapStrInt *mp = &m;

    return mp->nope();
}
```

```
main.ct:8:1: error: no method 'nope' for type 'struct std_map_MapStrInt *'
```

### 10.2 忘了 `init()`

```ct
/* 错误示意：没有 init，put 返回 -1，表里一个元素都没有 */
MapStrInt m;
MapStrInt *mp = &m;
mp->put("k", 1);
```

`cap == 0`，`put` / `get` 全部返回 -1。不崩、不报错，只是什么都没发生——所以 `init` 的返回值
也要检查。

### 10.3 裸 `malloc` 后直接调方法

```ct
/* 错误示意：vptr 是垃圾，第一次调用就段错误（实测 exit 139） */
MapStrInt *p = (MapStrInt *)malloc(sizeof(MapStrInt));
p->put("k", 1);
```

正确写法见第 8 节。

### 10.4 满载时想覆盖

```ct
/* 错误示意：表满后连覆盖已有键都返回 -1 */
tp->checkEqInt(pm->put(99, 0), -1, "a full table refuses a new key");
```

满表检查在查找之前。要更新已有键，先 `clear()` 或留出容量。

## 11. 下一步

- 存字符串数组：[`std::vec`](../vec/README.md) 的 `VecStr`
- 批量分配的临时对象：[`std::arena`](../arena/README.md)
- 字符串本身的字节操作：[`std::str`](../str/README.md)
