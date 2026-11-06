# 教程：用 `std::vec` 存一串数

目标：从零写出一个能增长、能按下标读、能正确释放的 `VecInt`，并且知道 `->` 与 `.` 的区别、
为什么 `malloc` 出来的向量不能直接调方法。

模块参考（方法全集、布局、所有权）：[README.md](README.md)。

> 标了「可运行」的代码块都来自 [`test/std/vec/guide.ct`](../../tests/vec/guide.ct)，第 2 节的
> 第一个程序整段就是 [`test/std/vec/first.ct`](../../tests/vec/first.ct)。拆段时只重复了变量
> 声明，逻辑与顺序和文件一致。gate（`test/std/run.sh`）每次都编译并运行这两个文件。
> 带 `error:` 输出或标「错误示意」的块则是故意不通过的。

## 1. 准备

```sh
cd /path/to/ct
ct build -I . hello.ct -o hello
```

```ct
import std::vec::Vec;
```

`Vec` 是文件名对应的占位类型；`VecInt` / `VecStr` 和两个 trait 都在同一个单元里，一次 import
全部可见。

## 2. 最小程序

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

三件事按顺序发生：

1. `vp->init()` 把 `data` / `n` / `cap` 归零；
2. `push` 负责分配与扩容，返回值 0 / -1；
3. `free()` 释放元素数组。之后 `len()` 回到 0。

`vp` 不是必需的花招：`VecInt *vp = &v` 只是把「接收者是指针」这件事写明白。

## 3. 读与删

```ct
    tp->checkEqInt(vp->at(1, &x), 0, "at");
    tp->checkEqInt(x, 20, "at value");
    tp->checkEqInt(vp->at(2, &x), -1, "at past the end");
    tp->checkEqInt(vp->at(-1, &x), -1, "at before the start");
    tp->checkEqInt(vp->pop(&x), 0, "pop");
    tp->checkEqInt(x, 20, "pop returns the last element");
    tp->checkEqInt(vp->len(), 1, "len after pop");
```

`at` / `pop` 把结果写进你给的出参，并用返回值报告成败。越界是 `-1`，不会断言也不会崩溃——
所以**必须检查返回值**，否则 `x` 还是上一次的旧值。

## 4. 增长

```ct
    for (i = 0; i < 100; i++) vp->push(i);
    tp->checkEqInt(vp->len(), 101, "len after growth");
    tp->checkEqInt(vp->at(100, &x), 0, "the last element survives growth");
    tp->checkEqInt(x, 99, "the last element value");
```

容量按倍增走，`push` 是唯一会分配的方法。分配失败时返回 -1 且向量保持原样，不会半个元素进去。
容量本身不暴露在 API 里，也不该依赖。

## 5. 存字符串：`VecStr` 只存指针

```ct
    VecStr names;
    VecStr *np = &names;
    char *name;

    np->init();
    tp->checkEqInt(np->push("alpha"), 0, "push a borrowed string");
    tp->checkEqInt(np->at(0, &name), 0, "at");
    tp->checkEqStr(name, "alpha", "the pointer comes back unchanged");
    np->free();
```

`VecStr` **不复制**字符串，也不在 `free()` 时释放它们。存进去的指针必须活得比向量久：

```ct
/* 错误示意（这段故意不可运行）：buf 出了作用域，向量里留下悬垂指针 */
{
    char buf[8];
    "abc".copy(buf, 8);
    np->push(buf);
}
```

需要拥有的字符串，要在 push 之前自己 `malloc` / 复制，并在 `free()` 之后自己释放。
可运行的对照见 guide.ct 第 4 段：那里 push 的是字面量 `"alpha"`，它的生命期是整个程序。

## 6. 堆上的向量：不要裸 `malloc` 之后直接调方法

```ct
VecInt *p = (VecInt *)malloc(sizeof(VecInt));
p->init();      /* 段错误：p 里的 vptr 是垃圾（实测 exit 139） */
```

方法调用展开成 `p->__vptr->push(p, ...)`，第一步就是读 vptr。`malloc` 不会替你写它。
正确写法是先准备一个初始化过的值，再整体赋进去：

```ct
    seed.init();
    heap = (VecInt *)malloc(sizeof(VecInt));
    if (!heap) return 2;
    *heap = seed;
    tp->checkEqInt(heap->push(7), 0, "push through a heap pointer");
    tp->checkEqInt(heap->at(0, &x), 0, "at through a heap pointer");
    tp->checkEqInt(x, 7, "heap value");
    heap->free();
    free(heap);
```

`seed` 在声明处由编译器补上 vptr，`*heap = seed` 把 vptr 和字段一起复制过去，之后才安全。
（想避免这一步就根本别 `malloc` 结构体：声明一个值，把它的地址传出去。）

## 7. `.` 和 `->` 的分工

```ct
    {
        VecInt local;

        local.init();
        local.push(3);
        tp->checkEqInt(local.len(), 1, "value receiver");
        local.free();
    }
```

值接收者也合法：编译器在声明处注入 vptr，并把 `&local` 传进方法。区别只在于**对象是怎么来的**：

| 写法 | 适用 | 原因 |
| --- | --- | --- |
| `vp->push(x)` | 指针、堆对象、任何 `VecInt *` | 显式，vptr 必须已经就位 |
| `v.push(x)` | 本函数内声明的值 | 声明处的初始化器负责 vptr |

文档和 fixture 统一用 `->`，因为它在两种情况下都成立。反过来，对值用 `->` 是硬错误：

```ct
VecInt v;
v->push(1);
```

```
main.ct:8:6: error: member reference type 'struct std_vec_VecInt' is not a pointer; did you mean to use '.'?
    8 |     v->__std_vec_vecintops_vptr->push(v, 1);
      |     ~^~
      |      .
main.ct:8:39: error: passing 'struct std_vec_VecInt' to parameter of incompatible type 'void *'
    8 |     v->__std_vec_vecintops_vptr->push(v, 1);
      |                                       ^
2 errors generated.
```

（这是 clang 对生成 C 的报错；`v->...` 已经被改写成 vtable 调用，所以信息里能看到内部名字。）

## 8. 常见错误

### 8.1 方法名不存在（故意编译失败）

```ct
VecInt v;
v.nope();
```

```
main.ct:7:1: error: no method 'nope' for type 'struct std_vec_VecInt'
```

### 8.2 忘了检查 `at` / `push` 的返回值

不报错，但会读到旧数据或漏掉一次分配失败。失败时出参**不被写**，`x` 保持调用前的值：

```ct
    tp->checkEqInt(vp->at(1, &x), 0, "at");
    tp->checkEqInt(x, 20, "at value");
    tp->checkEqInt(vp->at(2, &x), -1, "at past the end");
    tp->checkEqInt(vp->at(-1, &x), -1, "at before the start");
    tp->checkEqInt(x, 20, "a failed at() leaves the out-param alone");
```

如果丢掉最后的检查，`x` 看起来「有值」，其实是上一次读出来的 20。

### 8.3 把 `VecInt` 当裸结构体用

`VecInt` 的布局是 16 字节字段 + 8 字节 vptr。手写 `malloc(sizeof(VecInt) - 8)`、
按 `{ data, n, cap }` 三个字段做聚合初始化，都会破坏 vptr（后者的编译器会补 vptr，前者不会）。
需要自定义存储时，用 `*p = <一个初始化过的值>`，不要自己算大小。

## 9. 为什么方法不能被内联

`at()` / `len()` 这种访问器，如果直接调用，`-O2` 会把整个调用折叠掉（实测 0.00 ns）；
走 vtable 时编译器看不到被调用者是谁，只能保留间接调用，本机实测约 1 ns/次。
这是 `use` 结构体的固有属性，不是实现写差了——所以本模块不会为这两个方法开「直接调用」的旁路，
那会让同一个类型有两种语义。

## 10. 下一步

- 键值表：[`std::map`](../map/README.md)
- 临时分配器：[`std::arena`](../arena/README.md)
- 字符串本身：[`std::str`](../str/README.md)
