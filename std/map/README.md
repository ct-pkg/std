# `std::map` — 有界哈希表（`MapStrInt` / `MapIntPtr`）

两个单态键值表：`MapStrInt` 是 `char *` → `int`，`MapIntPtr` 是 `int` → `void *`。
开放寻址、固定容量：`init` 一次定大小，`put` 装满就拒绝，**从不 rehash、从不自动扩容**。

这是 **CT 自有类型**，走原生 OO：`pub trait` + `struct ... use Trait` + `impl` 全在
[`Map.h.ct`](Map.h.ct) 里，没有 `.m.ct`、没有 shim。代价是每个对象多一个 vptr（+8 字节）
和一次间接跳转。

## 1. 定位与边界

- 提供：插入 / 覆盖、按键查询、条目数、清空、释放。
- 容量在 `init` 时定死，满了 `put` 返回 `-1`；内存可预测，没有 rehash 停顿。
- **键不被复制**（`MapStrInt`）：只保存 `char *`，比较用 `strcmp`。
- **值不被复制**（`MapIntPtr`）：只保存 `void *`，释放规则由 `init` 的 `free_fn` 决定。
- 不做：扩容、删除单个键、遍历 / 迭代器、有序遍历、线程安全。
- 没有 `delete`：`Drop` 在生成物里是 `static`，库单元无法销毁调用方的值类型，所以显式 `free()`。

## 2. 依赖与安装

依赖 C 的 `<stdlib.h>` / `<string.h>`；`ct.pkg` 的约束是 `requires ct >= 0.20.3`。

| 文件 | 内容 |
| --- | --- |
| `Map.h.ct` | `Map` 占位类型、`MapStrIntOps` / `MapIntPtrOps` 两个 `pub trait`、两个 `pub struct ... use`、两个 `impl`，以及 `static` 哈希辅助函数 |

`MAP_MIN_CAP` 与哈希辅助函数只落在本单元生成的 `.c`，不在导出的 `Map.h` 里。

引入方式：`import std::map::Map;`。`Map` 只是满足「`.h.ct` 文件名 = 主类型名」的占位类型，
真正要用的是 `MapStrInt` / `MapIntPtr` 和两个 trait，一次 import 全部可见。

包安装后（详见[包根 README](../../README.md#安装)）：

```sh
ct get std@^0.2.0
ct build src/main.ct -I vendor/std -o main --cflags "-Wall -Wextra"
```

在包根内开发时：

```sh
ct build -I . tests/map/first.ct -o /tmp/std-map --cflags "-Wall -Wextra"
```

## 3. 快速开始

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

`ct build` 实际执行的 `cc` 行（本机实测，路径随检出位置变化）：

```
'cc' -std=c99 -Wall -Wextra '.ctbuild/first.c' '.ctbuild/Map.c' '.ctbuild/Test.c' '-o' '/tmp/std-map'
```

输出：

```text
6 checks, 0 failures
```

本节整段就是 [`tests/map/first.ct`](../../tests/map/first.ct)，回归矩阵在
[`tests/map/main.ct`](../../tests/map/main.ct)。同目录的
[`tests/map/guide.ct`](../../tests/map/guide.ct) 与本 README 同源，但它在 v0.19.1 上仍有一处
字面量接收者（`"kk".copy(key, 8)`）导致编译失败；本文对应示例已改成变量接收者。

## 4. API 全表

### 类型与 trait

| 名字 | 种类 | 说明 |
| --- | --- | --- |
| `Map` | `pub struct` | 无状态占位，没有实例化价值 |
| `MapStrIntOps` | `pub trait` | 接收者 `MapStrInt *`，键 `char *`，值 `int` |
| `MapStrInt` | `pub struct ... use MapStrIntOps` | `{ cap, n, keys, vals, used }` + vptr |
| `MapIntPtrOps` | `pub trait` | 接收者 `MapIntPtr *`，键 `int`，值 `void *` |
| `MapIntPtr` | `pub struct ... use MapIntPtrOps` | `{ cap, n, keys, vals, used, free_fn }` + vptr |

### `MapStrIntOps`

| 方法 | 签名 | 返回 / 失败 |
| --- | --- | --- |
| `init(want)` | `int init(int want)` | 容量取不小于 16、不小于 `want` 的 2 的幂；成功 0，分配失败 `-1` |
| `put(key, value)` | `int put(char *key, int value)` | 插入或覆盖；表满 / `key == NULL` / 未 init 返回 `-1` |
| `get(key, out)` | `int get(const char *key, int *out)` | 找到返回 0 并写 `*out`；没有返回 `-1`；`out` 可为 `NULL` |
| `len()` | `int len()` | 当前条目数 |
| `clear()` | `void clear()` | 清空条目，表仍可用（不释放数组） |
| `free()` | `void free()` | `clear()` 后释放数组并把字段归零；可重复调用 |

### `MapIntPtrOps`

| 方法 | 签名 | 返回 / 失败 |
| --- | --- | --- |
| `init(want, free_fn)` | `int init(int want, void (*free_fn)(void *))` | 同 `MapStrIntOps`；`free_fn` 被记住，`clear()` 对每个值调用一次（可为 `NULL`） |
| `put(key, value)` | `int put(int key, void *value)` | 插入或覆盖；表满 / 未 init 返回 `-1` |
| `get(key, out)` | `int get(int key, void **out)` | 找到返回 0 并写 `*out`；没有返回 `-1`；`out` 可为 `NULL` |
| `len()` / `clear()` / `free()` | — | 同 `MapStrIntOps`，区别见 5.3 与 5.4 |

## 5. 教程

### 5.1 覆盖与缺失

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
    tp->checkEqInt(mp->init(8), 0, "init");
    tp->checkEqInt(mp->put("k0", 10), 0, "put");
    tp->checkEqInt(mp->get("k0", &v), 0, "get");
    tp->checkEqInt(v, 10, "the stored value");
    tp->checkEqInt(mp->put("k0", 11), 0, "overwrite");
    tp->checkEqInt(mp->get("k0", &v), 0, "get after overwrite");
    tp->checkEqInt(v, 11, "the overwritten value");
    tp->checkEqInt(mp->len(), 1, "an overwrite does not grow the table");
    tp->checkEqInt(mp->get("missing", &v), -1, "a missing key");
    tp->checkEqInt(v, 11, "a failed get leaves the out-param alone");
    mp->free();
    return tp->report();
}
```

输出：

```text
10 checks, 0 failures
```

`put` 对已存在的键是**覆盖**，`len()` 不变。`get` 找不到返回 `-1`，此时出参不被写；`out`
本身可以是 `NULL`（只关心有没有）。

### 5.2 键按内容比较，但指针不被复制

```ct
import std::map::Map;
import std::str::String;
import std::test::Test;

int main(void)
{
    Test t;
    Test *tp = &t;
    MapStrInt m;
    MapStrInt *mp = &m;
    char key[8];
    char *kk = "kk";
    int v;

    tp->init();
    tp->checkEqInt(mp->init(8), 0, "init");
    tp->checkEqInt(kk.copy(key, 8), 0, "seed a mutable key buffer");
    tp->checkEqInt(mp->put(key, 20), 0, "put with a buffer key");
    tp->checkEqInt(mp->get("kk", &v), 0, "lookup through another pointer");
    tp->checkEqInt(v, 20, "equal content finds the entry");
    mp->free();
    return tp->report();
}
```

输出：

```text
5 checks, 0 failures
```

`MapStrInt` 用 `strcmp` 比较键，所以 `put` 用的指针和 `get` 用的指针不要求相同，内容相等即可。
但**表保存的是 `put` 时那个指针**，不复制字符串：`key` 必须活得比条目久。插入之后修改
`key` 的内容，会让哈希与存储位置不一致，这个条目就再也查不到了。

`MapIntPtr` 相反：键是 `int`，按值保存；值是 `void *`，只存指针。

### 5.3 固定容量与 `free_fn`

```ct
import std::map::Map;
import std::test::Test;

#include <stdlib.h>

static void drop(void *p)
{
    free(p);
}

int main(void)
{
    Test t;
    Test *tp = &t;
    MapIntPtr m;
    MapIntPtr *mp = &m;
    int i;
    void *out;

    tp->init();
    tp->checkEqInt(mp->init(4, drop), 0, "init sizes up to the 16 minimum");
    for (i = 0; i < 16; i++) {
        int *p = (int *)malloc(sizeof(int));
        if (!p) return 1;
        *p = i * 10;
        tp->checkEqInt(mp->put(i, p), 0, "fill a slot");
    }
    tp->checkEqInt(mp->len(), 16, "the table is full");
    tp->checkEqInt(mp->put(99, 0), -1, "a full table refuses a new key");
    tp->checkEqInt(mp->get(15, &out), 0, "the last key is found");
    tp->checkEqInt(*(int *)out, 150, "the last value");
    tp->checkEqInt(mp->get(99, &out), -1, "a missing key");
    mp->clear();
    tp->checkEqInt(mp->len(), 0, "clear runs free_fn and drops the entries");
    mp->free();
    return tp->report();
}
```

输出：

```text
23 checks, 0 failures
```

`init(want)` 取「不小于 16、不小于 `want` 的 2 的幂」：`init(4)` 与 `init(8)` 都是 16 个槽。
满了之后 `put` 返回 `-1`，表保持原样，不扩容、不 rehash。

注意：满表检查发生在查找之前，所以**满载时覆盖一个已存在的键也会返回 `-1`**。需要更新就先
`clear()`，或者一开始留出余量。

值是自己 `malloc` 的，就把释放规则在 `init` 时一次说清：`clear()` / `free()` 会对每个值调用
`drop`。传 `NULL` 表示表完全不碰值。**不要**在 `clear()` 之后自己再释放一遍。`get` 的出参是
`void **`，取出来要转回真实类型（`*(int *)out`）——表只保证它存的是你放进去的那个指针。

### 5.4 `clear()` 与 `free()`

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
    tp->checkEqInt(mp->init(8), 0, "init");
    tp->checkEqInt(mp->put("k0", 10), 0, "put");
    mp->clear();
    tp->checkEqInt(mp->len(), 0, "clear empties the table");
    tp->checkEqInt(mp->get("k0", &v), -1, "get after clear");
    tp->checkEqInt(mp->put("k0", 1), 0, "put after clear");
    tp->checkEqInt(mp->get("k0", &v), 0, "get after put");
    tp->checkEqInt(v, 1, "the new value");
    mp->free();
    return tp->report();
}
```

输出：

```text
7 checks, 0 failures
```

`clear()` 只把槽位标记为空、保留数组，所以之后能继续 `put`；`free()` 连数组一起释放。
`MapStrInt` 的 `clear()` **不释放键**（键是借用的），`MapIntPtr` 的 `clear()` 会对每个值调用
`init` 时给的 `free_fn`。两者都可重复调用。

### 5.5 堆上的表：不要裸 `malloc` 之后直接调方法

```ct
import std::map::Map;
import std::test::Test;

#include <stdlib.h>

int main(void)
{
    Test t;
    Test *tp = &t;
    MapStrInt seed;
    MapStrInt *heap;
    int v;

    tp->init();
    seed.init(4);
    heap = (MapStrInt *)malloc(sizeof(MapStrInt));
    if (!heap) return 2;
    *heap = seed;
    tp->checkEqInt(heap->put("h", 7), 0, "put through a heap pointer");
    tp->checkEqInt(heap->get("h", &v), 0, "get through a heap pointer");
    tp->checkEqInt(v, 7, "heap value");
    heap->free();
    free(heap);
    return tp->report();
}
```

输出：

```text
3 checks, 0 failures
```

方法调用展开成 `p->__vptr->put(p, ...)`，第一步就是读 vptr，而 `malloc` 不会写它。先让 `seed`
在声明处拿到 vptr，再 `*heap = seed` 整体复制。此时 `seed` 与 `heap` 共享同一组数组，只能
释放一次（`heap->free()`），之后不要再碰 `seed`。

## 6. 常见配方

### 带默认值的配置查询

```ct
import std::map::Map;
import std::io::Io;
import std::num::Num;

int main(void)
{
    MapStrInt conf;
    MapStrInt *cp = &conf;
    int port;
    int rc;

    cp->init(8);
    cp->put("host", 1);
    cp->put("port", 8080);
    rc = cp->get("port", &port);
    std::io::printf("port rc=%d value=%s\n", rc, port.toString());
    rc = cp->get("missing", &port);
    std::io::printf("missing rc=%d\n", rc);
    cp->free();
    return 0;
}
```

输出：

```text
port rc=0 value=8080
missing rc=-1
```

### 其它常用组合

| 任务 | 写法 |
| --- | --- |
| 插入或更新 | `if (mp->put(k, v) != 0) { /* 表满或未 init */ }` |
| 查询 | `if (mp->get(k, &out) == 0) { /* 命中 */ }` |
| 只关心是否存在 | `mp->get(k, (int *)0)` |
| 清空后复用 | `mp->clear()`，数组保留，可继续 `put` |
| 释放 | `mp->free()`，之后要再用就重新 `init` |
| 让表拥有值 | `init(want, drop)`，`drop` 里 `free` |

## 7. 排错

### 7.1 方法名不存在

```ct
import std::map::Map;

int main(void)
{
    MapStrInt m;
    MapStrInt *mp = &m;

    return mp->nope();
}
```

```text
main.ct:8:1: error: no method 'nope' for type 'struct std_map_MapStrInt *'
```

### 7.2 忘了 `init()`

```ct
import std::map::Map;
import std::io::Io;

int main(void)
{
    MapStrInt m;
    MapStrInt *mp = &m;
    int v;
    int put_rc;
    int get_rc;
    int n;

    put_rc = mp->put("k", 1);
    get_rc = mp->get("k", &v);
    n = mp->len();
    std::io::printf("put=%d get=%d len=%d\n", put_rc, get_rc, n);
    return 0;
}
```

输出：

```text
put=-1 get=-1 len=0
```

`cap == 0`，`put` / `get` 全部返回 `-1`。不崩、不报错，只是什么都没发生——所以 `init` 的返回值
也要检查。栈上声明的值会被编译器补 vptr、其余字段按 C 聚合初始化清零，所以漏掉 `init` 往往
「看起来没崩」；不要依赖这个巧合。

### 7.3 裸 `malloc` 后直接调方法

`malloc` 返回的内存里 vptr 是垃圾，第一次调用就通过它跳转，**本机实测 exit 139**：

```ct
import std::map::Map;

#include <stdlib.h>

int main(void)
{
    MapStrInt *p = (MapStrInt *)malloc(sizeof(MapStrInt));

    return p->put("k", 1);
}
```

正确写法见 5.5。

### 7.4 满载时想覆盖

满表检查在查找之前：表满后 `put` 一个已存在的键同样返回 `-1`，而不是更新它。要更新已有键，
先 `clear()` 或留出容量。见 5.3 的 16 槽示例。

## 8. 限制与未支持

- **没有泛型**：只有 `MapStrInt` / `MapIntPtr` 两个单态形状。
- **固定容量**：没有扩容、没有 rehash；满了只能 `-1`。`init` 的下限是 16 槽。
- **没有删除单个键**：只有 `clear()` 全清。
- **没有遍历**：没有迭代器、没有 `keys` / `values` 视图；需要遍历得自己维护键列表。
- **`MapStrInt` 的键是借用的**：不复制、不释放；插入后修改键内容会失配。
- **`MapIntPtr` 的值归 `free_fn` 管**：自己再 `free` 一遍就是双重释放；传 `NULL` 而值确实
  需要释放就会漏。
- **没有 `delete`**：`Drop` 是 `static`，库单元不能销毁调用方的值类型。
- **布局是公开的**：64 位下 `MapStrInt` / `MapIntPtr` 分别为 40 / 48 字节（32 / 40 字节字段
  + 8 字节 vptr），vptr 出现在导出的 `Map.h` 里。
- **方法不能被内联**：`len()` / `get()` 经 vtable 分派，固定付出一次间接跳转。
- **字面量接收者从 0.20.0 起可用**（本包下限 0.20.3）：v0.19.1 只接受变量、元素、调用和字段链。

## 9. 版本与变更记录

| 版本 | 状态 | 变更 |
| --- | --- | --- |
| `0.1.0` | 未打 tag、未发布 | 当前版本。文档重写：教程并入本 README，删除 `tutorial.md`；API 未变。 |

编译器下限 `ct >= 0.20.3`。本文所有程序均在 v0.20.3 上编译运行通过；字面量接收者写法从
0.20.0 起可用。
