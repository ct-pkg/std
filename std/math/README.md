# `std::math` — 浮点数值方法（`Math` / `Float` / `Real`）

`math.h` 的一层方法映射：命名空间上的 `std::math::Math.sqrt(x)` / `fabs(x)`，
以及值上的 `x.abs()` / `x.sqrt()` / `x.floor()` / `x.ceil()` / `x.round()`；
`double` 另有 `isNan` / `isInf` / `toStr` / `toString`。

## 1. 定位与边界

- 只覆盖 `sqrt` / `fabs` / `floor` / `ceil` / `round`（以及 `double` 的 `isNan` / `isInf` / 文本化）。
- 没有 `pow` / `exp` / `log` / 三角函数；需要时直接调 C 的 `math.h`。
- `Float`（`float`）保持 `float` 宽度，走 `sqrtf` / `fabsf` / `floorf` / `ceilf` / `roundf`；
  `Real`（`double`）走无后缀版本。
- **只有 `double` 有 `toStr` / `toString`**；`float` 没有文本化方法。
- 不分配：`toStr` 写调用方缓冲区；`toString` 用静态环形槽（非线程安全）。
- 链接需要 `-lm`（包 `ldflags` 已带；libm 并入 libc 的平台上是空操作）。

## 2. 依赖与安装

依赖 C 的 `<math.h>` / `<stdio.h>` / `<stdlib.h>`；`ct.pkg` 的约束是 `requires ct >= 0.20.3`。

| 文件 | 内容 |
| --- | --- |
| `Math.m.ct` | `pub trait Math` + `map trait Math for namespace std::math`（`sqrt` / `fabs`） |
| `Real.m.ct` | `pub trait Float`（`float` 宿主）+ `pub trait Real`（`double` 宿主） |
| `Real.shim.h` | `std_math_double_to_str` / `std_math_double_to_string` 的 `static inline` 实现 |

| 宿主 | trait | 调用形态 |
| --- | --- | --- |
| namespace `std::math` | `Math` | `std::math::Math.sqrt(x)` |
| `float` | `Float` | `f.sqrt()` |
| `double` | `Real` | `d.sqrt()` / `d.toString()` |

引入方式：`import std::math::Math;`（命名空间函数）与 `import std::math::Real;`（`Float` /
`Real` 值方法）。

包安装后（详见[包根 README](../../README.md#安装)）：

```sh
ct get std@^0.2.0
ct build src/main.ct -I vendor/std -o main --cflags "-Wall -Wextra" --ldflags "-lm"
```

## 3. 快速开始

```ct
import std::math::Math;
import std::math::Real;
import std::io::Io;

int main(void)
{
    double x = 16.0;
    double d = 2.6;
    float f = -2.5f;
    float f26 = 2.6f;
    double zero = 0.0;
    double nan = zero / zero;
    double inf = 1.0 / zero;

    std::io::printf("math sqrt=%g fabs=%g\n",
                    std::math::Math.sqrt(x), std::math::Math.fabs(-3.5));
    std::io::printf("float abs=%g floor=%g ceil=%g round=%g\n",
                    (double)f.abs(), (double)f.floor(),
                    (double)f26.ceil(), (double)f26.round());
    std::io::printf("double sqrt=%g round=%g\n", d.sqrt(), d.round());
    std::io::printf("nan=%d inf=%d\n", nan.isNan(), inf.isInf());
    return 0;
}
```

`ct build` 实际执行的 `cc` 行（本机实测，路径随检出位置变化）：

```
'cc' -std=c99 -Wall -Wextra -I '/Users/samxx/vscspace/ct-pkg/std/std/math' '.ctbuild/main.c' '.ctbuild/Io.c' -lm '-o' '/tmp/std-math'
```

输出：

```text
math sqrt=4 fabs=3.5
float abs=2.5 floor=-3 ceil=3 round=3
double sqrt=1.61245 round=3
nan=1 inf=1
```

`%g` 只保留 6 位有效数字，所以 `sqrt(2.6)` 显示为 `1.61245`；这不是精度丢失，
`toString` 才是可往返的文本（见 5.3）。

字面量接收者（`(2.5).floor()`）从 **0.20.0** 起可用（本包下限 0.20.3）；v0.19.1 先赋给变量。

## 4. API 全表

### 命名空间 `std::math`（trait `Math`）

| 方法 | 签名 | 返回 |
| --- | --- | --- |
| `sqrt(x)` | `double sqrt(double x)` | 平方根；负数返回 NaN |
| `fabs(x)` | `double fabs(double x)` | 绝对值 |

调用必须带命名空间：`std::math::Math.sqrt(x)`，不能写成裸 `sqrt(x)`。

### 宿主 `float`（trait `Float`）

| 方法 | 签名 | 返回 |
| --- | --- | --- |
| `abs()` | `float abs()` | `fabsf` |
| `sqrt()` | `float sqrt()` | `sqrtf`；负数返回 NaN |
| `floor()` | `float floor()` | `floorf` |
| `ceil()` | `float ceil()` | `ceilf` |
| `round()` | `float round()` | `roundf`，四舍五入到最近整数（半值远离 0） |

`Float` 没有 `isNan` / `isInf` / `toStr` / `toString`；需要文本时先转 `double`。

### 宿主 `double`（trait `Real`）

| 方法 | 签名 | 返回 / 失败 |
| --- | --- | --- |
| `abs()` | `double abs()` | `fabs` |
| `sqrt()` | `double sqrt()` | `sqrt`；负数返回 NaN |
| `floor()` | `double floor()` | 向下取整 |
| `ceil()` | `double ceil()` | 向上取整 |
| `round()` | `double round()` | 四舍五入到最近整数（半值远离 0） |
| `isNan()` | `int isNan()` | 是 NaN 返回非 0 |
| `isInf()` | `int isInf()` | 是 ±Inf 返回非 0 |
| `toStr(out, cap)` | `char *toStr(char *out, int cap)` | 可往返文本；缓冲区太小会**静默截断**并失去往返保证 |
| `toString()` | `char *toString()` | 可往返文本；四个静态槽轮转，**非线程安全** |

`toStr` / `toString` 的文本规则：先用 `%g`（6 位有效数字），如果 `strtod` 读回来不等于原值，
改用 `%.17g`。所以 `2.6` 输出 `"2.6"`，`1.0/3.0` 输出 `"0.33333333333333331"`，
两者都能被 `strtod` 精确还原。

## 5. 教程

### 5.1 命名空间方法与值方法

- `std::math::Math.sqrt(x)`：函数式，参数是 `double`，结果也是 `double`；
- `x.sqrt()`：值方法，按 `x` 的宿主选择 `float` 或 `double` 版本。

`double x = 16.0;` 时，`std::math::Math.sqrt(x)` 与 `x.sqrt()` 都得到 `4`：前者是命名空间
函数，后者是 `Real` 的值方法。

两者语义相同，区别只在调用形态。`Math` 的宿主是命名空间，所以必须写全
`std::math::Math.`；`Float` / `Real` 的宿主是值类型。

### 5.2 `float` 与 `double` 不互相匹配

`float` 变量匹配 `Float`，`double` 变量匹配 `Real`：

`float f = 2.6f;` 的 `f.sqrt()` 走 `Float.sqrt`（`sqrtf`），结果是 `float`；
`double d = 2.6;` 的 `d.sqrt()` 走 `Real.sqrt`（`sqrt`），结果是 `double`。

把 `float` 传给 `Real` 的方法不会自动发生；需要 `double` 时显式写 `(double)f`。
相反，`double` 上的 `isNan` / `toString` 在 `float` 上不存在。

### 5.3 文本化：可往返

```ct
#include <stdlib.h>

import std::math::Math;
import std::math::Real;
import std::io::Io;

int main(void)
{
    char buf[32];
    double d = 2.6;
    double third = 1.0 / 3.0;
    double precise = 1.23456789;

    std::io::printf("short=%s long=%s\n", d.toStr(buf, 32), third.toString());
    std::io::printf("precise=%s\n", precise.toString());
    std::io::printf("roundtrip=%d\n", atof(third.toString()) == third);
    return 0;
}
```

输出：

```text
short=2.6 long=0.33333333333333331
precise=1.2345678899999999
roundtrip=1
```

- `2.6` 用 `%g` 就够了，保持短文本；
- `1.0/3.0` 六位不够，自动切到 `%.17g`；
- `atof` / `strtod` 读回来与原值逐位相等，所以 `roundtrip=1`；
- `toStr` 的 `cap` 太小会截断，截断后不再保证可往返。

`toString` 的四个槽与 `std::num` 的整数环是**两个独立的环**，互不覆盖；但同样非线程安全。

### 5.4 NaN 与 Inf

`double zero = 0.0; double nan = zero / zero; double inf = 1.0 / zero;` 时，
`zero / zero` 是 NaN，`1.0 / zero` 是 Inf。

- `nan.isNan()` 返回非 0；NaN 与任何值（包括自己）比较都不相等；
- `inf.isInf()` 返回非 0，`-1.0 / zero` 也是 Inf；
- `sqrt(-1.0)` 得到 NaN，不是错误码；
- 本模块不提供 `isFinite`。

### 5.5 链接 `-lm`

`Math.m.ct` 的注释明确要求链接 libm。`ct install` 会把包的 `ldflags` 带进命令；
手工 `ct build` 时要加：

```sh
ct build -I . main.ct -o /tmp/std-math --cflags "-Wall -Wextra" --ldflags "-lm"
```

在 libm 已并入 libc 的平台（例如 macOS）上 `-lm` 是空操作，但保留它可以让命令跨平台。

## 6. 常见配方

### 勾股定理与取整

```ct
import std::math::Math;
import std::math::Real;
import std::io::Io;

int main(void)
{
    double a = 3.0;
    double b = 4.0;
    double h = std::math::Math.sqrt(a * a + b * b);
    double pi = 3.14159;

    std::io::printf("hypot=%g floor=%g ceil=%g\n", h, pi.floor(), pi.ceil());
    return 0;
}
```

输出：

```text
hypot=5 floor=3 ceil=4
```

### 其它常用组合

| 任务 | 写法 |
| --- | --- |
| 平方根 | `std::math::Math.sqrt(x)` 或 `x.sqrt()` |
| 绝对值 | `std::math::Math.fabs(x)` 或 `x.abs()` |
| 向下 / 向上取整 | `x.floor()` / `x.ceil()` |
| 四舍五入 | `x.round()` |
| 判断 NaN / Inf | `x.isNan()` / `x.isInf()`（仅 `double`） |
| 可往返文本 | `x.toString()` 或 `x.toStr(buf, sizeof buf)` |
| `float` 转文本 | `((double)f).toString()` |

## 7. 排错

### 7.1 在 `float` 上调用 `isNan`

```ct
import std::math::Real;

int main(void)
{
    float f = 1.0f;
    return f.isNan();
}
```

```text
main.ct:6:1: error: no method 'isNan' for type 'float'
```

`isNan` / `isInf` / `toStr` / `toString` 只属于 `Real`（`double`）。
先写 `double d = (double)f;` 再调用。

### 7.2 忘了 import

```ct
int main(void)
{
    double d = 16.0;
    double r = d.sqrt();
    return (int)r;
}
```

```text
main.ct:4:1: error: no method 'sqrt' for type 'double'
```

`double` 的 `sqrt` 来自 `std::math::Real`；命名空间版本还要
`import std::math::Math;`。

### 7.3 忘了 `-lm`

如果平台把 libm 单独提供，编译会失败在链接阶段（找不到 `sqrt` / `fabs` 等符号）。
在 `ct build` 后加 `--ldflags "-lm"`，或使用 `ct install` 打印的命令。

## 8. 限制与未支持

- **函数集很小**：只有 `sqrt` / `fabs` / `floor` / `ceil` / `round` / `isNan` / `isInf`；
  没有 `pow` / `exp` / `log` / 三角函数 / `hypot` / `isFinite`。
- **`float` 没有文本化**：`toStr` / `toString` 只在 `double` 上。
- **`Math` 必须带命名空间**：没有裸 `sqrt` 入口。
- **`toString` 非线程安全**：四个静态槽，第五个活结果覆盖第一个；与 `std::num` 的环相互独立。
- **`toStr` 截断会破坏往返**：容量不够时得到短文本，不报错。
- **`%g` 只有 6 位有效数字**：直接 `printf("%g", ...)` 不等于 `toString`；要可往返文本用
  `toString` / `toStr`。
- **链接可能需要 `-lm`**：取决于平台。
- **字面量接收者从 0.20.0 起可用**（本包下限 0.20.3）；0.19.1 只接受变量、元素、调用和字段链。
- **多给实参不报错**：与其它映射模块相同的编译器行为（SH-56）。

## 9. 版本与变更记录

| 版本 | 状态 | 变更 |
| --- | --- | --- |
| `0.1.0` | 未打 tag、未发布 | 当前版本。文档首次成文：此前 `std/math` 只有源码与模块表，没有 README 或教程。 |

编译器下限 `ct >= 0.20.3`。
