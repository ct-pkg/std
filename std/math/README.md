# `std::math` — 浮点数值方法（`Float` / `Real`）

`math.h` 的一层方法映射，**只有方法形态**：`float` 宿主是 `Float`（34 个方法），`double`
宿主是 `Real`（38 个方法），调用一律写成 `x.sqrt()` / `x.pow(2.0)` 这样的值方法。
namespace 入口已删除，`math/Math.m.ct` 已物理移除。

## 1. 定位与边界

- 数学函数只提供方法形态，接收者决定宽度：`float` 走 `Float`，`double` 走 `Real`。
- `Float` 有 34 个方法，`Real` 有 38 个：同名方法语义相同，`Real` 另有
  `scale_by_power_of_two` / `split_exponent` / `split_fraction` / `to_str` 四个。
- `Float`（`float`）保持 `float` 宽度，走 `sqrtf` / `fabsf` / `powf` 等带 `f` 后缀的入口；
  `Real`（`double`）走无后缀版本。
- 七个生僻 libm 名在 CT 侧写成全名（见 §4.3），其余保留 C 原名。
- `sqrt` 负参数得 NaN，不是错误码；`min` / `max` 的 NaN 行为按 C 的 `fmin` / `fmax`
  （一个参数是 NaN 时返回另一个）。
- **只有 `Real` 有 `to_str`**；`Float` 没有文本化方法，需要文本时先转 `double`。
- 不分配：`to_str` 写调用方缓冲区，容量不足会静默截断并失去往返保证；
  `split_exponent` / `split_fraction` 写调用方的出参指针。
- 链接需要 `-lm`（包 `ldflags` 已带；libm 并入 libc 的平台上是空操作）。

## 2. 依赖与安装

依赖 C 的 `<math.h>` / `<stdio.h>` / `<stdlib.h>`；`ct.pkg` 的约束是 `requires ct >= 0.30.0`。

| 文件 | 内容 |
| --- | --- |
| `Real.m.ct` | `pub trait Float`（`float` 宿主，34 个方法）+ `pub trait Real`（`double` 宿主，38 个方法） |
| `Real.shim.h` | `std_math_double_to_str` 的 `static inline` 实现 |

| 宿主 | trait | 方法数 | 调用形态 |
| --- | --- | --- | --- |
| `float` | `Float` | 34 | `f.sqrt()` / `f.pow(y)` |
| `double` | `Real` | 38 | `d.sqrt()` / `d.pow(y)` / `d.to_str(buf, cap)` |

引入方式：`import std::math::Real;`（一次带入 `Float` 与 `Real` 两个值方法 trait）。
namespace 入口的 import 不再是文档写法。

包安装后（详见[包根 README](../../README.md#安装)）：

```sh
ct get std@^0.2.0
ct build src/main.ct -I vendor/std -o main --cflags "-Wall -Wextra" --ldflags "-lm"
```

## 3. 快速开始

```ct
import std::math::Real;
import std::num::Num;
import std::str::String;
import std::io::Stdout;

int main(void)
{
    double x = 16.0;
    double d = 2.6;
    double neg = -3.5;
    double base = 2.0;
    double zero = 0.0;
    double nan = zero / zero;
    double inf = 1.0 / zero;
    float f = -2.5f;
    float f26 = 2.6f;
    double v;
    char buf[32];
    char num[16];
    char line[64];

    v = x.sqrt();
    "sqrt=".concat(v.to_str(buf, 32), line, 64);
    std::io::Stdout.puts(line);

    v = neg.abs();
    "abs=".concat(v.to_str(buf, 32), line, 64);
    std::io::Stdout.puts(line);

    v = base.pow(10.0);
    "pow=".concat(v.to_str(buf, 32), line, 64);
    std::io::Stdout.puts(line);

    v = (double)f.abs();
    "float abs=".concat(v.to_str(buf, 32), line, 64);
    std::io::Stdout.puts(line);
    v = (double)f.floor();
    "float floor=".concat(v.to_str(buf, 32), line, 64);
    std::io::Stdout.puts(line);
    v = (double)f26.ceil();
    "float ceil=".concat(v.to_str(buf, 32), line, 64);
    std::io::Stdout.puts(line);
    v = (double)f26.round();
    "float round=".concat(v.to_str(buf, 32), line, 64);
    std::io::Stdout.puts(line);

    v = d.sqrt();
    "double sqrt=".concat(v.to_str(buf, 32), line, 64);
    std::io::Stdout.puts(line);
    v = d.round();
    "double round=".concat(v.to_str(buf, 32), line, 64);
    std::io::Stdout.puts(line);

    "nan=".concat(nan.is_nan().to_str(num, 16), line, 64);
    std::io::Stdout.puts(line);
    "inf=".concat(inf.is_infinite().to_str(num, 16), line, 64);
    std::io::Stdout.puts(line);
    return 0;
}
```

`ct build` 展开后的 `cc` 行类似下面（路径随检出位置变化）：

```
'cc' -std=c99 -Wall -Wextra -I '<pkg>/std/math' -I '<pkg>/std/str' -I '<pkg>/std/io' '.ctbuild/main.c' -lm '-o' '/tmp/std-math'
```

输出：

```text
sqrt=4
abs=3.5
pow=1024
float abs=2.5
float floor=-3
float ceil=3
float round=3
double sqrt=1.61245154965971
double round=3
nan=1
inf=1
```

`to_str` 是可往返文本：6 位有效数字不够时自动加长，所以 `sqrt(2.6)` 显示为
`1.61245154965971`（见 5.3）。

字面量接收者（`(2.5).floor()`）从 **0.20.0** 起可用（本包下限 0.30.0）；v0.19.1 先赋给变量。

## 4. API 全表

### 4.1 宿主 `float`（trait `Float`，34 个方法）

| 方法 | 签名 | 语义 |
| --- | --- | --- |
| `abs()` | `float abs()` | `fabsf`，绝对值 |
| `sqrt()` | `float sqrt()` | `sqrtf`，平方根；负数得 NaN |
| `floor()` | `float floor()` | `floorf`，向下取整 |
| `ceil()` | `float ceil()` | `ceilf`，向上取整 |
| `round()` | `float round()` | `roundf`，四舍五入到最近整数（半值远离 0） |
| `truncate()` | `float truncate()` | `truncf`，向 0 取整 |
| `pow(y)` | `float pow(float y)` | `powf`，`x` 的 `y` 次幂；定义域外得 NaN |
| `exp()` | `float exp()` | `expf`，`e^x` |
| `exponential_minus_one()` | `float exponential_minus_one()` | `expm1f`，`e^x - 1`，小 `x` 时比先减 1 精确 |
| `log()` | `float log()` | `logf`，自然对数；`x < 0` 得 NaN，`x == 0` 得 -Inf |
| `log2()` | `float log2()` | `log2f`，以 2 为底 |
| `log10()` | `float log10()` | `log10f`，以 10 为底 |
| `logarithm_one_plus()` | `float logarithm_one_plus()` | `log1pf`，`log(1 + x)`，小 `x` 时比先加 1 精确 |
| `sin()` | `float sin()` | `sinf`，正弦 |
| `cos()` | `float cos()` | `cosf`，余弦 |
| `tan()` | `float tan()` | `tanf`，正切 |
| `asin()` | `float asin()` | `asinf`，反正弦；定义域 `[-1, 1]` 之外得 NaN |
| `acos()` | `float acos()` | `acosf`，反余弦；定义域 `[-1, 1]` 之外得 NaN |
| `atan()` | `float atan()` | `atanf`，反正切，值域 `(-π/2, π/2)` |
| `atan2(x)` | `float atan2(float x)` | `atan2f`，按 `y = 接收者`、`x = 参数` 的象限求反正切 |
| `sinh()` | `float sinh()` | `sinhf`，双曲正弦 |
| `cosh()` | `float cosh()` | `coshf`，双曲余弦 |
| `tanh()` | `float tanh()` | `tanhf`，双曲正切 |
| `hypot(y)` | `float hypot(float y)` | `hypotf`，`sqrt(x² + y²)`，中间结果不溢出 |
| `cube_root()` | `float cube_root()` | `cbrtf`，立方根；负数有实根 |
| `floating_modulo(y)` | `float floating_modulo(float y)` | `fmodf`，`x / y` 的浮点余数，符号随 `x` |
| `remainder(y)` | `float remainder(float y)` | `remainderf`，IEEE 754 余数，商按最近整数取 |
| `positive_difference(y)` | `float positive_difference(float y)` | `fdimf`，`max(x - y, 0)`，不产生负值 |
| `fused_multiply_add(b, c)` | `float fused_multiply_add(float b, float c)` | `fmaf`，`x * b + c`，只舍入一次 |
| `copy_sign(y)` | `float copy_sign(float y)` | `copysignf`，取 `y` 的符号和 `x` 的绝对值 |
| `min(y)` | `float min(float y)` | `fminf`；一个参数是 NaN 时返回另一个 |
| `max(y)` | `float max(float y)` | `fmaxf`；一个参数是 NaN 时返回另一个 |
| `is_nan()` | `int is_nan()` | `isnan`，是 NaN 返回非 0 |
| `is_infinite()` | `int is_infinite()` | `isinf`，是 ±Inf 返回非 0 |

### 4.2 宿主 `double`（trait `Real`，38 个方法）

前 34 个与 `Float` 同名同义，只是签名改为 `double`；标 **Real 独有** 的四个只在 `double` 上存在。

| 方法 | 签名 | 语义 |
| --- | --- | --- |
| `abs()` | `double abs()` | `fabs`，绝对值 |
| `sqrt()` | `double sqrt()` | `sqrt`，平方根；负数得 NaN |
| `floor()` | `double floor()` | `floor`，向下取整 |
| `ceil()` | `double ceil()` | `ceil`，向上取整 |
| `round()` | `double round()` | `round`，四舍五入到最近整数（半值远离 0） |
| `truncate()` | `double truncate()` | `trunc`，向 0 取整 |
| `pow(y)` | `double pow(double y)` | `pow`，`x` 的 `y` 次幂；定义域外得 NaN |
| `exp()` | `double exp()` | `exp`，`e^x` |
| `exponential_minus_one()` | `double exponential_minus_one()` | `expm1`，`e^x - 1`，小 `x` 时比先减 1 精确 |
| `log()` | `double log()` | `log`，自然对数；`x < 0` 得 NaN，`x == 0` 得 -Inf |
| `log2()` | `double log2()` | `log2`，以 2 为底 |
| `log10()` | `double log10()` | `log10`，以 10 为底 |
| `logarithm_one_plus()` | `double logarithm_one_plus()` | `log1p`，`log(1 + x)`，小 `x` 时比先加 1 精确 |
| `sin()` | `double sin()` | `sin`，正弦 |
| `cos()` | `double cos()` | `cos`，余弦 |
| `tan()` | `double tan()` | `tan`，正切 |
| `asin()` | `double asin()` | `asin`，反正弦；定义域 `[-1, 1]` 之外得 NaN |
| `acos()` | `double acos()` | `acos`，反余弦；定义域 `[-1, 1]` 之外得 NaN |
| `atan()` | `double atan()` | `atan`，反正切，值域 `(-π/2, π/2)` |
| `atan2(x)` | `double atan2(double x)` | `atan2`，按 `y = 接收者`、`x = 参数` 的象限求反正切 |
| `sinh()` | `double sinh()` | `sinh`，双曲正弦 |
| `cosh()` | `double cosh()` | `cosh`，双曲余弦 |
| `tanh()` | `double tanh()` | `tanh`，双曲正切 |
| `hypot(y)` | `double hypot(double y)` | `hypot`，`sqrt(x² + y²)`，中间结果不溢出 |
| `cube_root()` | `double cube_root()` | `cbrt`，立方根；负数有实根 |
| `floating_modulo(y)` | `double floating_modulo(double y)` | `fmod`，`x / y` 的浮点余数，符号随 `x` |
| `remainder(y)` | `double remainder(double y)` | `remainder`，IEEE 754 余数，商按最近整数取 |
| `positive_difference(y)` | `double positive_difference(double y)` | `fdim`，`max(x - y, 0)`，不产生负值 |
| `fused_multiply_add(b, c)` | `double fused_multiply_add(double b, double c)` | `fma`，`x * b + c`，只舍入一次 |
| `copy_sign(y)` | `double copy_sign(double y)` | `copysign`，取 `y` 的符号和 `x` 的绝对值 |
| `min(y)` | `double min(double y)` | `fmin`；一个参数是 NaN 时返回另一个 |
| `max(y)` | `double max(double y)` | `fmax`；一个参数是 NaN 时返回另一个 |
| `is_nan()` | `int is_nan()` | `isnan`，是 NaN 返回非 0 |
| `is_infinite()` | `int is_infinite()` | `isinf`，是 ±Inf 返回非 0 |
| `scale_by_power_of_two(e)` | `double scale_by_power_of_two(int e)` | **Real 独有**。`ldexp`，`x * 2^e` |
| `split_exponent(e)` | `double split_exponent(int *e)` | **Real 独有**。`frexp`，返回规格化小数，指数写入 `*e` |
| `split_fraction(ipart)` | `double split_fraction(double *ipart)` | **Real 独有**。`modf`，返回小数部分，整数部分写入 `*ipart` |
| `to_str(out, cap)` | `char *to_str(char *out, int cap)` | **Real 独有**。可往返文本；缓冲区太小会**静默截断**并失去往返保证 |

`to_str` 的文本规则：先用短格式（6 位有效数字），如果 `strtod` 读回来不等于原值，
改用完整精度格式。所以 `2.6` 输出 `"2.6"`，`1.0/3.0` 输出 `"0.33333333333333331"`，
两者都能被 `strtod` 精确还原。

### 4.3 七个全名映射

| C 名 | CT 方法 |
| --- | --- |
| `expm1` | `exponential_minus_one` |
| `log1p` | `logarithm_one_plus` |
| `fdim` | `positive_difference` |
| `fma` | `fused_multiply_add` |
| `cbrt` | `cube_root` |
| `fmod` | `floating_modulo` |
| `copysign` | `copy_sign` |

其余方法保留 C 原名（`abs` / `sqrt` / `pow` / `exp` / `log` / `sin` …）。

## 5. 教程

### 5.1 方法形态

数学只有一种写法：在值上调用方法。

```ct
double x = 16.0;
double y = x.sqrt();
double z = x.pow(2.0);
```

接收者的宿主决定宽度：`float` 变量走 `Float`（`float` 进、`float` 出），`double` 变量走
`Real`（`double` 进、`double` 出）。`x.sqrt()` 的结果仍是原宽度，不会自动升成 `double`。

namespace 入口自 **0.5.0** 起删除，`Math.m.ct` 已物理移除。
迁移时把旧的函数式调用改成 `x.sqrt()`，其它函数同理改成接收者方法。

### 5.2 `float` 与 `double` 不互相匹配

`float` 变量匹配 `Float`，`double` 变量匹配 `Real`：

`float f = 2.6f;` 的 `f.sqrt()` 走 `Float.sqrt`（`sqrtf`），结果是 `float`；
`double d = 2.6;` 的 `d.sqrt()` 走 `Real.sqrt`（`sqrt`），结果是 `double`。

把 `float` 传给 `Real` 的方法不会自动发生；需要 `double` 时显式写 `(double)f`。
两个宿主都有 `is_nan` / `is_infinite`，但 `Float` 没有 `to_str`、`split_exponent`、
`split_fraction`、`scale_by_power_of_two`——这四个只在 `Real`（`double`）上存在。

### 5.3 文本化：可往返

```ct
import std::math::Real;
import std::str::String;
import std::io::Stdout;

int main(void)
{
    char buf[32];
    char line[64];
    double d = 2.6;
    double third = 1.0 / 3.0;
    double precise = 1.23456789;

    "short=".concat(d.to_str(buf, 32), line, 64);
    std::io::Stdout.puts(line);

    "third=".concat(third.to_str(buf, 32), line, 64);
    std::io::Stdout.puts(line);

    "precise=".concat(precise.to_str(buf, 32), line, 64);
    std::io::Stdout.puts(line);
    return 0;
}
```

输出：

```text
short=2.6
third=0.33333333333333331
precise=1.2345678899999999
```

- `2.6` 用短格式就够了，保持短文本；
- `1.0/3.0` 六位不够，自动改用完整精度格式；
- 文本由 `strtod` 回读校验，读回来与原值逐位相等；
- `to_str` 的 `cap` 太小会截断，截断后不再保证可往返。

### 5.4 NaN 与 Inf

`double zero = 0.0; double nan = zero / zero; double inf = 1.0 / zero;` 时，
`zero / zero` 是 NaN，`1.0 / zero` 是 Inf。

- `nan.is_nan()` 返回非 0；NaN 与任何值（包括自己）比较都不相等；
- `inf.is_infinite()` 返回非 0，`-1.0 / zero` 也是 Inf；
- `sqrt(-1.0)` 得到 NaN，不是错误码；
- `min` / `max` 按 C 的 `fmin` / `fmax`：一个参数是 NaN 时返回另一个参数，两个都是 NaN 才得 NaN；
- 本模块不提供 `isFinite`。

### 5.5 链接 `-lm`

本模块调用 libm。`ct install` 会把包的 `ldflags` 带进命令；手工 `ct build` 时要加：

```sh
ct build -I . main.ct -o /tmp/std-math --cflags "-Wall -Wextra" --ldflags "-lm"
```

在 libm 已并入 libc 的平台（例如 macOS）上 `-lm` 是空操作，但保留它可以让命令跨平台。

## 6. 常见配方

### 勾股定理与取整

```ct
import std::math::Real;
import std::str::String;
import std::io::Stdout;

int main(void)
{
    double a = 3.0;
    double b = 4.0;
    double h = a.hypot(b);
    double pi = 3.14159;
    double v;
    char buf[32];
    char line[32];

    "hypot=".concat(h.to_str(buf, 32), line, 32);
    std::io::Stdout.puts(line);
    v = pi.floor();
    "floor=".concat(v.to_str(buf, 32), line, 32);
    std::io::Stdout.puts(line);
    v = pi.ceil();
    "ceil=".concat(v.to_str(buf, 32), line, 32);
    std::io::Stdout.puts(line);
    return 0;
}
```

输出：

```text
hypot=5
floor=3
ceil=4
```

### 其它常用组合

| 任务 | 写法 |
| --- | --- |
| 平方根 | `x.sqrt()` |
| 绝对值 | `x.abs()` |
| 幂 | `x.pow(y)` |
| `e` 指数 / 自然对数 | `x.exp()` / `x.log()` |
| 三角函数 | `x.sin()` / `x.cos()` / `x.atan2(y)` |
| 向下 / 向上 / 截断取整 | `x.floor()` / `x.ceil()` / `x.truncate()` |
| 四舍五入 | `x.round()` |
| 判断 NaN / Inf | `x.is_nan()` / `x.is_infinite()`（`Float` / `Real` 都有） |
| 可往返文本 | `x.to_str(buf, cap)`（仅 `Real`） |
| `float` 转文本 | 先 `double d = (double)f;`，再 `d.to_str(buf, cap)` |
| 二进制缩放 / 拆指数 / 拆小数 | `x.scale_by_power_of_two(e)` / `x.split_exponent(&e)` / `x.split_fraction(&ipart)`（仅 `Real`） |

## 7. 排错

### 7.1 在 `float` 上调用 `to_str`

```ct
import std::math::Real;

int main(void)
{
    float f = 1.0f;
    char buf[16];

    f.to_str(buf, 16);
    return 0;
}
```

```text
error: no method 'to_str' for type 'float'
```

`to_str` / `split_exponent` / `split_fraction` / `scale_by_power_of_two` 只属于 `Real`（`double`）。
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

`double` 的 `sqrt` 来自 `std::math::Real`；补 `import std::math::Real;`。

### 7.3 忘了 `-lm`

如果平台把 libm 单独提供，编译会失败在链接阶段（找不到 `sqrt` / `pow` 等符号）。
在 `ct build` 后加 `--ldflags "-lm"`，或使用 `ct install` 打印的命令。

## 8. 限制与未支持

- **只覆盖 `math.h` 的一部分**：没有 `is_finite` / `is_normal` / `next_after` / `erf` / `tgamma`；
  需要时直接调 C 的 `math.h`。
- **`float` 没有文本化**：`to_str` 只在 `double` 上；`Float` 也没有三个出参方法。
- **`min` / `max` 不传播 NaN**：按 C 的 `fmin` / `fmax`，一个参数是 NaN 时返回另一个。
- **namespace 入口已删除**：`Math.m.ct` 已物理移除；数学只有方法形态。
- **`to_str` 截断会破坏往返**：容量不够时得到短文本，不报错。
- **短格式只有 6 位有效数字**：`to_str` 只在短格式能精确回读时使用它，否则自动加长；
  自己用固定格式拼接不保证往返。
- **链接可能需要 `-lm`**：取决于平台。
- **字面量接收者从 0.20.0 起可用**（本包下限 0.30.0）；0.19.1 只接受变量、元素、调用和字段链。
- **多给实参不报错**：与其它映射模块相同的编译器行为（SH-56）。

## 9. 版本与变更记录

| 版本 | 状态 | 变更 |
| --- | --- | --- |
| `0.5.0` | 未打 tag、待发布 | 数学只保留方法形态：namespace 入口删除，`Math.m.ct` 物理移除。`Float` 从 5 个方法扩到 34 个，`Real` 扩到 38 个；`isInf` 改名 `is_infinite`，`Float` 也提供 `is_nan` / `is_infinite`；`Real` 新增 `scale_by_power_of_two` / `split_exponent` / `split_fraction`。 |
| `0.1.0` | 已发布 | 文档首次成文：此前 `std/math` 只有源码与模块表，没有 README 或教程。 |

编译器下限 `ct >= 0.30.0`。
