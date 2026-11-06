# `std::vec` — 动态数组（`VecInt` / `VecStr`）

两个单态动态数组：`VecInt` 存 `int`，`VecStr` 存 `char *`。CT 没有泛型，所以 v1 直接给这两个
形状；调用方不要依赖字段布局。

这是 **CT 自有类型**，所以走原生 OO：`pub trait` + `struct ... use Trait` + `impl` 全在
`Vec.h.ct` 里，没有 `.m.ct`、没有 shim。代价是每个对象多一个 vptr（+8 字节）和一次间接跳转，
收益是统一的 `p->push(x)` 形态。

- 教程：[tutorial.md](tutorial.md)
- 完整可运行示例：[`test/std/vec/guide.ct`](../../tests/vec/guide.ct)（gate 每次编译并运行）
- 回归矩阵：[`test/std/vec/main.ct`](../../tests/vec/main.ct)

## import

```ct
import std::vec::Vec;
```

`Vec` 是无状态的占位类型，只为满足「`.h.ct` 文件名 = 主类型名」的检查；真正要用的是
`VecIntOps` / `VecStrOps`，import 同一个文件就全部可见。

## 类型与 trait

| 名字 | 种类 | 说明 |
| --- | --- | --- |
| `Vec` | `pub struct` | 无状态占位，没有实例化价值 |
| `VecIntOps` | `pub trait` | 接收者 `VecInt *`，元素是 `int` |
| `VecInt` | `pub struct ... use VecIntOps` | `{ int *data; int n; int cap; }` + vptr |
| `VecStrOps` | `pub trait` | 接收者 `VecStr *`，元素是借用的 `char *` |
| `VecStr` | `pub struct ... use VecStrOps` | `{ char **data; int n; int cap; }` + vptr |

文件内顺序固定为 `trait → struct(use) → impl`：trait 必须声明在 `use` 它的 struct 之前，
否则报 `struct 'VecInt' uses undefined trait 'VecIntOps'`。

## 方法

`VecIntOps` 与 `VecStrOps` 名字和形状相同，只有元素类型不同（下表以 `VecInt` 为例，
`VecStr` 把 `int` 换成 `char *`）。

| 方法 | 参数 | 返回 | 语义 |
| --- | --- | --- | --- |
| `init()` | — | — | 把三个字段置零。**契约上的第一步** |
| `len()` | — | `int` | 当前元素个数 |
| `push(x)` | `int x` / `char *x` | `int` | 追加；容量不足时倍增；分配失败返回 -1，向量不变 |
| `pop(out)` | `int *out` / `char **out` | `int` | 弹出最后一个并写进 `out`；空向量返回 -1 |
| `at(i, out)` | `int i` + 出参 | `int` | 读第 `i` 个；越界返回 -1（不 assert、不崩溃） |
| `free()` | — | — | 释放元素数组并把字段归零；可重复调用 |

`at` / `pop` 的出参可以是 NULL（只取值不写）。`push` 的返回值必须检查：它是唯一会分配的方法。

## 内存与所有权

- **元素数组由向量持有，元素本身不归它管。** `free()` 只释放数组；`VecStr` 存的是调用方给的
  指针，字符串的生命周期由调用方负责。
- 初始容量为 0，第一次 `push` 才分配（`realloc(NULL, ...)`），之后按倍增扩容。
- 扩容失败返回 -1 且保持原数组有效；不存在「一半成功」的中间状态。
- `free()` 之后 `len()` 为 0，可以再 `init()` / `push()`，也可以重复 `free()`。

## 布局与代价

- 64 位下 `sizeof(VecInt) == sizeof(VecStr) == 24`：三个字段占 16 字节（12 字节数据 + 对齐），
  vptr 追加在末尾占 8 字节。裸结构体是 16 字节。
- vptr 出现在导出的 `Vec.h` 里（`__vecintops_vptr`），这是公开布局的一部分，不是内部细节。
- **方法不能被内联**：`at()` / `len()` 这种访问器，直接函数调用在 `-O2` 下会被折叠成 0；
  经 vtable 分派则固定要付一次间接跳转（本机实测约 1 ns/次）。间接跳转本身的开销 ≈0，
  真正失去的是内联机会。这是原生 OO 的固有属性，不要为个别方法做「退回直接调用」的旁路。

## 坑

1. **`init()` 是第一步。** 栈上声明的值会被编译器补上 vptr、其余字段按 C 聚合初始化清零，所以
   漏掉 `init()` 往往「看起来能跑」——不要依赖这个巧合。堆对象、`memcpy` 来的对象没有这层保障。
2. **裸 `malloc` 之后直接调方法会段错误。** `malloc` 返回的内存里 vptr 是垃圾，第一次
   `p->push(...)` 就通过它跳转（实测 exit 139）。正确写法是先把一个初始化过的值赋进去：

   ```ct
   VecInt seed;
   seed.init();
   VecInt *p = (VecInt *)malloc(sizeof(VecInt));
   *p = seed;          /* 复制 vptr 与字段 */
   p->push(1);
   ```

3. **`VecStr` 只存指针。** 把临时缓冲区或即将离开作用域的局部数组 `push` 进去，之后取出来就是
   悬垂指针；向量不复制、不释放。
4. **`at` 越界是返回值，不是断言。** 忘记检查返回值会把上一次 `out` 的内容当成数据用。
5. **一个 `VecInt` 只能有一个 vptr。** 如果把 `VecInt` 放进另一个 `use` 结构体、或对它做
   `memcpy`，要确认 vptr 跟着一起过去了；跨编译单元传递值类型没有额外保障。
