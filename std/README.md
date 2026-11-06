# `std/` — ct 标准库

> 中文 · [English](#english)

`std` 是**源码库**，不是 `ct` 二进制的一部分：`import std::str::String` 只在显式 `-I` 下解析，
编译器**没有**默认搜索路径（D17-2）。仓库里它是顶层 `std/`；`install.sh` 把它装到
`$(dirname "$prefix")/share/ct/std`，并打印建议的 `-I` 行。

```sh
ct build -I . main.ct -o main         # 在仓库根
ct build -I ~/.local/share/ct main.ct -o main   # 安装后
```

## 模块

每个模块只有一种调用形态（OO），没有并存的函数式入口。

| 单元 | namespace | 方法 |
| --- | --- | --- |
| `std/str/String.m.ct` + `String.shim.h` | `std::str` | `len` / `find` / `findChar` / `startsWith` / `endsWith` / `copy` / `slice` / `concat` / `toLower` / `toUpper` / `compare` |
| `std/fs/Fs.m.ct` + `Fs.shim.h` | `std::fs` | `exists` / `readFile` / `writeFile` / `list` |
| `std/utf8/Utf8Ops.m.ct` + `Utf8Ops.shim.h` | `std::utf8` | `decode` / `codepointCount` / `encode` |
| `std/vec/Vec.h.ct` | `std::vec` | `VecIntOps` / `VecStrOps`：`init` / `len` / `push` / `pop` / `at` / `free` |
| `std/map/Map.h.ct` | `std::map` | `MapStrIntOps` / `MapIntPtrOps`：`init` / `put` / `get` / `len` / `clear` / `free` |
| `std/arena/Arena.h.ct` | `std::arena` | `init` / `alloc` / `reset` / `freeAll` |
| `std/test/Test.h.ct` | `std::test` | `init` / `checkTrue` / `checkEqInt` / `checkEqStr` / `report` |

每个模块的工作文档（参考 + 从零到能用的教程）：
[str/README.md](str/README.md) · [str/tutorial.md](str/tutorial.md) ·
[vec/README.md](vec/README.md) · [vec/tutorial.md](vec/tutorial.md) ·
[fs/README.md](fs/README.md) · [fs/tutorial.md](fs/tutorial.md) ·
[utf8/README.md](utf8/README.md) · [utf8/tutorial.md](utf8/tutorial.md) ·
[map/README.md](map/README.md) · [map/tutorial.md](map/tutorial.md) ·
[arena/README.md](arena/README.md) · [arena/tutorial.md](arena/tutorial.md) ·
[test/README.md](test/README.md) · [test/tutorial.md](test/tutorial.md)。

## 调用形态

接收者是 `char *` 的模块用 `.`：

```ct
import std::str::String;
import std::fs::Fs;
import std::utf8::Utf8Ops;

char buf[64];
char *s = "hello";

int n = s.len();                 // 字节数，不是码点数
s.copy(buf, 64);                 // 输出缓冲区 + 容量仍由调用方提供
s.toLower();                     // 原地改，返回 0 / -1
int i = s.findChar('l');

char *path = "a.txt";
int ok = path.exists();
path.writeFile("hi");
path.readFile(buf, 64);
path.list(buf, 64);

char *u = "héllo";
int cp;
u.decode(&cp);
int cps = u.codepointCount();    // 码点数；String.len 数的是字节
buf.encode(cp, 16);              // 接收者是写入的缓冲区
```

结构体模块在库里统一写成 `p->method()`，栈上值和堆对象都成立。声明出来的值也接受 `v.method()`：
编译器会在该声明的初始化器里补上 vptr 并把 `&v` 传进去。但 `malloc` 出来的对象在赋入一个初始化过的
值之前 vptr 是垃圾，直接调方法会段错误，所以下面一律先有值再取地址：

```ct
import std::vec::Vec;
import std::map::Map;
import std::arena::Arena;
import std::test::Test;

VecInt v;
VecInt *vp = &v;
vp->init();
vp->push(1);
int n = vp->len();
vp->free();

MapStrInt m;
MapStrInt *mp = &m;
mp->init(8);
mp->put("k", 1);
int got;
mp->get("k", &got);
mp->free();

Arena a;
Arena *ap = &a;
ap->init(32);
void *p = ap->alloc(8);
ap->freeAll();

Test t;
Test *tp = &t;
tp->init();
tp->checkEqInt(n, 1, "one element");
return tp->report();
```

## 两类实现

`std` 的模块分两类，机制不同，不要互相套用。

### 一、映射外来能力：`<Trait>.m.ct` + `<Trait>.shim.h`

宿主本来就是 C 已有类型（`char *`）的，用**静态类型宿主映射**：

- `<Trait>.m.ct` 只写 `pub trait` 和 `map trait ... for type char *`，把它展开成 C 调用；
- `<Trait>.shim.h` 与 `.m.ct` **同目录同基名**，装着 `static inline` 的实现。编译器自动探测到它，
  在每个消费方生成的 `.c` 顶部 include 一次，并把该目录加进 `cc` 行的 `-I`。

`str` / `fs` / `utf8` 都是这一形态，没有 `.h.ct`，也没有 `.shim.c`。映射是零开销的：`s.len()`
在调用点直接生成 `std_str_len(s)`，没有 wrapper、没有 vtable、结构体布局不变；`const char *`
接收者也匹配同一个宿主（类型键会丢 `const`）。

两个代价照实写：

- **实现按消费方复制一份**。`String.shim.h` 约 190 行 / 5.5K，每个 import 它的编译单元各有一份；
  断点要下在每个消费者里。这是 header-only 的固有代价。
- **`std_*` 符号不再是外部符号**（`static inline` 只在本编译单元可见），跨语言的 FFI 需要自己
  再导出一层。之所以用 `static inline` 而不是普通 `static`：后者在某消费方未调用时会触发
  `-Wunused-function`，前者不会（未使用的 `inline` 直接被丢掉）。

**为什么不能压成一个文件**：`.m.ct` 的 prologue 白名单只允许 `#include` / `#define`，写 `#ct-raw`
会直接报 `a .m.ct unit allows only #include and #define in its prologue`
（[Parser.h.ct:790-796](https://github.com/samxxu/ct-lang/blob/main/selfhost/ct-ast/frontend/parse/Parser.h.ct#L790-L796)，先写在 namespace 后、
再写在 map 之后都同样被拒）。所以「一个映射模块只留一个文件」目前做不到；两个文件（`.m.ct` +
shim 头）是理论下限。记在这里，免下一个人再试一次。

### 二、CT 自有类型：原生 `.h.ct`

宿主是 CT 自己定义的类型（`VecInt` / `MapStrInt` / `Arena` / `Test`）的，走原生 OO：
`pub trait` + `struct ... use Trait` + `impl Trait for Type`，全部写在同一个 `.h.ct` 里，
不建 `.m.ct`、不建 shim。

```ct
pub trait BoxOps {
    int get();
}

pub struct Box {
    int v;
} use BoxOps;

impl BoxOps for Box {
    int get()
    {
        return self->v;
    }
}
```

它和映射式有三处硬区别，都是编译器行为，不是风格建议：

1. **文件内顺序必须是 `trait → struct(use) → impl`**。trait 声明在 struct 之前，否则报
   `struct 'Box' uses undefined trait 'BoxOps'`。`use` 会给结构体尾部追加 `__<trait>_vptr`，
   分派是 `p->__vptr->get(p)`。
2. **`sizeof` 变大，且 vptr 出现在导出头里**。实测容器 `VecInt` / `VecStr` 16 → 24、
   `MapStrInt` 32 → 40、`MapIntPtr` 40 → 48（vptr 在末尾）。`Vec.h` / `Map.h` 里能直接看到
   `struct std_vec_VecIntOps_vtbl *__vecintops_vptr;`。这是硬约束：不给 `use` 就没有方法分派。
3. **堆对象不能裸 `malloc` 后直接调方法**。`malloc` 出来的内存里 vptr 是垃圾，第一次方法调用
   就会通过它跳转，实测段错误（exit 139）。先声明一个值（推荐），或用复合字面量赋值
   （`*p = (T){ .field = ... };`，编译器会一并注入 vptr），之后才能调方法。

还有一个固有属性，必须认下来：**`use` 结构体的方法永远不能被内联**。`at()` / `len()` 这种微小
访问器，直接调用在 `-O2` 下会被折叠成 0；走 vtable 固定约 1 ns/次。这是原生 OO 的属性，不是
实现瑕疵——不要为它做「特殊情况退回直接调用」的旁路，那会把一致性做没。机制本身的间接调用开销
实测 ≈0（`-O0` 1.73 vs 1.70 ns，`-O2` 1.03 vs 1.01 ns）；真正失去的只是内联。

## 约定

- **目录 = namespace**：`std/<mod>/<unit>.ct` 的 namespace 必须是 `std::<mod>`（`--ns-root` 会校验）；
  `.h.ct` 另要求 `pub struct <base>` 与文件名同名。每个单元自包含（自己的 `#include`、
  自己的辅助 `static`）。
- **失败值是返回值，不是 abort**：缓冲区不够一律 `-1` / `NULL`，绝不静默截断；`alloc` 拿不到块返回 `NULL`。
- **不依赖 `delete`**：`Drop` / `Handback` 在生成物里是 `static`，库单元无法销毁调用方的值类型，
  所以每个持有资源的类型都提供显式 `free()`（`Vec*` / `Map*`）、`reset()` / `freeAll()`（`Arena`）
  或 `clear()`（`Map*`）。容器**不**自动释放元素：`VecStr` 的字符串是借用的指针。
- **容器是有界实现**：`Vec` 容量倍增、溢出即 `-1`；`Map` 固定容量（`init` 取整到 2 的幂，最小 16），
  满载拒绝新键，不自动 rehash。
- **`MapIntPtr` 的所有权**：`init` 时传入 `free_fn`（可为 `NULL`），`clear()` / `free()` 对每个
  value 调用一次。键按指针保存，生命周期由调用方负责。
- **`toLower()` / `toUpper()` 是原地修改**，返回 0 / -1 状态码，**不返回新字符串**；不要对字符串
  字面量调用（类型键会丢掉 `const`，编译器不拦）。`copy` / `slice` / `concat` 保持短名：它们只写
  调用方给的 `out + cap`，`toXxx` 会被读成「返回新值」。
- **`String.len()` 数字节，`Utf8Ops.codepointCount()` 数码点**。同一段 `"é"` 是 `len() == 2`、
  `codepointCount() == 1`。两个名字不能互换，也不要再引入叫 `length` 的第三个。
- **`char *` 上的方法名是全局共享的**：同一个接收者类型上，任何两个可见 trait 只要声明了同名方法，
  调用点就报 `ambiguous type-host map for method 'x' on type 'char *'`（列出两个 `.m.ct`）。
  它按名字判定，不做参数个数重载，也不会静默挑一个；只 import 不调用则不报错。所以给 `char *`
  新增 trait 前先 grep 现有名字：
  `String` 的 `len` / `find` / `findChar` / `startsWith` / `endsWith` / `copy` / `slice` /
  `concat` / `toLower` / `toUpper` / `compare`；`FsOps` 的 `exists` / `readFile` / `writeFile` /
  `list`；`Utf8Ops` 的 `decode` / `codepointCount` / `encode`。
- **类型键会丢掉 `const`**：`s.toLower()`、`p->push()` 在 `const` 接收者上也会匹配。原地修改的
  方法不要对字符串字面量或 `const` 指针调用，编译器不会替你拦。

**取舍（D17-7）**：v1 只发单态类型（`VecInt` / `VecStr` / `MapStrInt` / `MapIntPtr`），
没有泛型；`void *` 擦除的通用接口可替换，但会失去编译期的键/值类型安全，因此不随 v1 发布。

## English

`std` is a **source library**, not part of the `ct` binary: `import std::str::String` resolves only
through an explicit `-I` (there is no default search path, D17-2). In the repository it is the
top-level `std/`; `install.sh` places it at `$(dirname "$prefix")/share/ct/std` and prints the
suggested `-I` line.

Every module has exactly one calling form (OO); there is no functional entry point left beside it.

| Unit | namespace | Methods |
| --- | --- | --- |
| `std/str/String.m.ct` + `String.shim.h` | `std::str` | `len` / `find` / `findChar` / `startsWith` / `endsWith` / `copy` / `slice` / `concat` / `toLower` / `toUpper` / `compare` |
| `std/fs/Fs.m.ct` + `Fs.shim.h` | `std::fs` | `exists` / `readFile` / `writeFile` / `list` |
| `std/utf8/Utf8Ops.m.ct` + `Utf8Ops.shim.h` | `std::utf8` | `decode` / `codepointCount` / `encode` |
| `std/vec/Vec.h.ct` | `std::vec` | `VecIntOps` / `VecStrOps`: `init` / `len` / `push` / `pop` / `at` / `free` |
| `std/map/Map.h.ct` | `std::map` | `MapStrIntOps` / `MapIntPtrOps`: `init` / `put` / `get` / `len` / `clear` / `free` |
| `std/arena/Arena.h.ct` | `std::arena` | `init` / `alloc` / `reset` / `freeAll` |
| `std/test/Test.h.ct` | `std::test` | `init` / `checkTrue` / `checkEqInt` / `checkEqStr` / `report` |

Working docs per module (reference plus a from-scratch tutorial):
[str/README.md](str/README.md) · [str/tutorial.md](str/tutorial.md) ·
[vec/README.md](vec/README.md) · [vec/tutorial.md](vec/tutorial.md) ·
[fs/README.md](fs/README.md) · [fs/tutorial.md](fs/tutorial.md).

A `char *` receiver uses `.`:

```ct
import std::str::String;
import std::fs::Fs;
import std::utf8::Utf8Ops;

char buf[64];
char *s = "hello";

int n = s.len();                 // bytes, not code points
s.copy(buf, 64);                 // the caller still owns the output buffer and capacity
s.toLower();                     // in place, returns 0 / -1
int i = s.findChar('l');

char *path = "a.txt";
int ok = path.exists();
path.writeFile("hi");
path.readFile(buf, 64);
path.list(buf, 64);

char *u = "héllo";
int cp;
u.decode(&cp);
int cps = u.codepointCount();    // code points; String.len counts bytes
buf.encode(cp, 16);              // the receiver is the buffer being written
```

Struct modules spell calls `p->method()` throughout, which works for a stack value and a heap object
alike. A declared value also accepts `v.method()`: the compiler appends the vptr to that declaration's
initializer and passes `&v`. An object from `malloc` has a garbage vptr until an initialised value is
assigned to it, and dispatch through it segfaults, so the examples below take the address first:

```ct
import std::vec::Vec;
import std::map::Map;
import std::arena::Arena;
import std::test::Test;

VecInt v;
VecInt *vp = &v;
vp->init();
vp->push(1);
int n = vp->len();
vp->free();

MapStrInt m;
MapStrInt *mp = &m;
mp->init(8);
mp->put("k", 1);
int got;
mp->get("k", &got);
mp->free();

Arena a;
Arena *ap = &a;
ap->init(32);
void *p = ap->alloc(8);
ap->freeAll();

Test t;
Test *tp = &t;
tp->init();
tp->checkEqInt(n, 1, "one element");
return tp->report();
```

### Two kinds of module

**Mapping a foreign capability: `<Trait>.m.ct` + `<Trait>.shim.h`.** When the host already exists
in C (`char *`), a static type-host map spells the OO calls. The `.m.ct` holds only the trait and
`map trait ... for type char *`; the same-base `<Trait>.shim.h` holds `static inline` definitions,
is auto-detected by the compiler, included at the top of every consumer's generated `.c`, and its
directory becomes an `-I` root. `str` / `fs` / `utf8` are this shape: no `.h.ct`, no `.shim.c`.
The mapping is zero-cost (`s.len()` emits `std_str_len(s)` at the call site, no wrapper, no vtable,
no layout change), and a `const char *` receiver matches because the type key drops `const`.

Two costs, stated plainly. The implementation is **copied into every consumer** (`String.shim.h`
is ~190 lines / 5.5K; set breakpoints in the consumer, not in one place), and the `std_*` symbols
stop being external symbols, so cross-language FFI needs its own export layer. The functions are
`static inline` rather than plain `static` because an unused plain `static` triggers
`-Wunused-function`, while an unused `inline` is simply dropped.

**Why not one file:** a `.m.ct` prologue allows only `#include` / `#define`; `#ct-raw` is a hard
parse error (`a .m.ct unit allows only #include and #define in its prologue`,
[Parser.h.ct:790-796](https://github.com/samxxu/ct-lang/blob/main/selfhost/ct-ast/frontend/parse/Parser.h.ct#L790-L796) — the position does
not matter). So a mapping module cannot be self-contained today; two files are the floor. This is
recorded so the next person does not try again.

**A CT-owned type: native `.h.ct`.** When the host is CT's own type (`VecInt` / `MapStrInt` /
`Arena` / `Test`), write `pub trait` + `struct ... use Trait` + `impl Trait for Type` in one
`.h.ct`, with no `.m.ct` and no shim. Three hard differences follow, all compiler behaviour:

1. **In-file order is `trait → struct(use) → impl`**; a trait declared after its struct reports
   `struct 'Box' uses undefined trait 'BoxOps'`. `use` appends `__<trait>_vptr` to the struct and
   dispatch becomes `p->__vptr->get(p)`.
2. **`sizeof` grows and the vptr is published** in the exported header: `VecInt` / `VecStr` 16 → 24,
   `MapStrInt` 32 → 40, `MapIntPtr` 40 → 48. This is a hard constraint — without `use` there is no
   method dispatch.
3. **A heap object must not be dispatched through after a bare `malloc`**: its vptr is garbage and
   the first call segfaults (measured exit 139). Declare a value, or assign a compound literal
   (`*p = (T){ .field = ... };`, which also injects the vptr), before calling a method.

One inherent property to accept: **a method on a `use` struct can never be inlined**. A tiny
accessor such as `at()` / `len()` folds away entirely at `-O2` when called directly, but through
the vtable always costs the indirect jump (about 1 ns here). Do not add a "special case, call
directly" bypass for such methods — that trades consistency for nothing. The mechanism itself is
free (measured 1.73 vs 1.70 ns at `-O0`, 1.03 vs 1.01 ns at `-O2`); what is lost is inlining.

Failure is a return value (`-1` / `NULL`), never an abort and never a silent truncation. There is no
`delete`: `Drop` / `Handback` are emitted `static`, so every resource type ships an explicit
`free()` (`Vec*` / `Map*`), `reset()` / `freeAll()` (the arena) or `clear()` (the maps). Containers
never free their elements — `VecStr` strings are borrowed pointers, and `MapIntPtr` calls the
`free_fn` supplied at `init`.

Containers are bounded: `Vec` doubles and returns `-1` on overflow; `Map` is fixed-capacity
(`init` rounds up to a power of two, at least 16) and refuses new keys when full — it never rehashes.

`String.len()` counts bytes; `Utf8Ops.codepointCount()` counts code points. `"é"` is `len() == 2`
and `codepointCount() == 1`.

Method names on `char *` are globally shared: if two visible traits declare the same method for the
same receiver type, the call site fails with `ambiguous type-host map for method 'x' on type 'char *'`
(naming both `.m.ct` files). The check is by name only — different arities do not disambiguate, and
nothing is picked silently; merely importing both is fine. Grep the existing names before adding
another `char *` trait: `String` uses `len` / `find` / `findChar` / `startsWith` / `endsWith` /
`copy` / `slice` / `concat` / `toLower` / `toUpper` / `compare`, `FsOps` uses `exists` / `readFile` /
`writeFile` / `list`, and `Utf8Ops` uses `decode` / `codepointCount` / `encode`.

The type key drops `const`, so in-place methods (`toLower`, `push`, `put`, ...) also match a `const`
receiver; the compiler will not stop you from mutating through one.

**Trade-off (D17-7)**: v1 ships monomorphic types only (`VecInt` / `VecStr` / `MapStrInt` /
`MapIntPtr`), no generics. A `void *`-erased interface is possible but loses compile-time key / value
typing, so it is not part of v1.
