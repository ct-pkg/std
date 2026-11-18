# `std::math` — 浮点数值方法（`Math` / `Float` / `Real`）

`math.h` 的一层方法映射：命名空间上的 `std::math::Math.sqrt(x)` / `fabs(x)`，
以及值上的 `x.abs()` / `x.sqrt()` / `x.floor()` / `x.ceil()` / `x.round()`；
`double` 另有 `is_nan` / `is_inf` / `to_str`。

## 1. 定位与边界

- 只覆盖 `sqrt` / `fabs` / `floor` / `ceil` / `round`（以及 `double` 的 `is_nan` / `is_inf` / 文本化）。
- 没有 `pow` / `exp` / `log` / 三角函数；需要时直接调 C 的 `math.h`。
- `Float`（`float`）保持 `float` 宽度，走 `sqrtf` / `fabsf` / `floorf` / `ceilf` / `roundf`；
  `Real`（`double`）走无后缀版本。
- **只有 `double` 有 `to_str`**；`float` 没有文本化方法。
- 不分配：`to_str` 写调用方缓冲区，容量不足会静默截断。
- 链接需要 `-lm`（包 `ldflags` 已带；libm 并入 libc 的平台上是空操作）。

## 2. 依赖与安装

依赖 C 的 `<math.h>` / `<stdio.h>` / `<stdlib.h>`；`ct.pkg` 的约束是 `requires ct >= 0.30.0`。

| 文件 | 内容 |
| --- | --- |
| `Math.m.ct` | `pub trait Math` + `map trait Math for namespace std::math`（`sqrt` / `fabs`） |
| `Real.m.ct` | `pub trait Float`（`float` 宿主）+ `pub trait Real`（`double` 宿主） |
| `Real.shim.h` | `std_math_double_to_str` 的 `static inline` 实现 |

| 宿主 | trait | 调用形态 |
| --- | --- | --- |
| namespace `std::math` | `Math` | `std::math::Math.sqrt(x)` |
| `float` | `Float` | `f.sqrt()` |
| `double` | `Real` | `d.sqrt()` / `d.to_str(buf, cap)` |

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
import std::num::Num;
import std::str::String;
import std::io::Stdout;

int main(void)
{
    double x = 16.0;
    double d = 2.6;
    float f = -2.5f;
    float f26 = 2.6f;
    double zero = 0.0;
    double nan = zero / zero;
    double inf = 1.0 / zero;
    double v;
    char buf[32];
    char num[16];
    char line[64];

    v = std::math::Math.sqrt(x);
    "math sqrt=".concat(v.to_str(buf, 32), line, 64);
    std::io::Stdout.puts(line);

    v = std::math::Math.fabs(-3.5);
    "math fabs=".concat(v.to_str(buf, 32), line, 64);
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
    "inf=".concat(inf.is_inf().to_str(num, 16), line, 64);
    std::io::Stdout.puts(line);
    return 0;
}
```

`ct build` 实际执行的 `cc` 行（本机实测，路径随检出位置变化）：

```
'cc' -std=c99 -Wall -Wextra -I '/Users/samxx/vscspace/ct-pkg/std/std/math' -I '/Users/samxx/vscspace/ct-pkg/std/std/num' -I '/Users/samxx/vscspace/ct-pkg/std/std/str' -I '/Users/samxx/vscspace/ct-pkg/std/std/io' '.ctbuild/main.c' -lm '-o' '/tmp/std-math'
```

输出：

```text
math sqrt=4
math fabs=3.5
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

`Float` 没有 `is_nan` / `is_inf` / `to_str`；需要文本时先转 `double`。

### 宿主 `double`（trait `Real`）

| 方法 | 签名 | 返回 / 失败 |
| --- | --- | --- |
| `abs()` | `double abs()` | `fabs` |
| `sqrt()` | `double sqrt()` | `sqrt`；负数返回 NaN |
| `floor()` | `double floor()` | 向下取整 |
| `ceil()` | `double ceil()` | 向上取整 |
| `round()` | `double round()` | 四舍五入到最近整数（半值远离 0） |
| `is_nan()` | `int is_nan()` | 是 NaN 返回非 0 |
| `is_inf()` | `int is_inf()` | 是 ±Inf 返回非 0 |
| `to_str(out, cap)` | `char *to_str(char *out, int cap)` | 可往返文本；缓冲区太小会**静默截断**并失去往返保证 |

`to_str` 的文本规则：先用短格式（6 位有效数字），如果 `strtod` 读回来不等于原值，
改用完整精度格式。所以 `2.6` 输出 `"2.6"`，`1.0/3.0` 输出 `"0.33333333333333331"`，
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
相反，`double` 上的 `is_nan` / `to_str` 在 `float` 上不存在。

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
- `inf.is_inf()` 返回非 0，`-1.0 / zero` 也是 Inf；
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
import std::str::String;
import std::io::Stdout;

int main(void)
{
    double a = 3.0;
    double b = 4.0;
    double h = std::math::Math.sqrt(a * a + b * b);
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
| 平方根 | `std::math::Math.sqrt(x)` 或 `x.sqrt()` |
| 绝对值 | `std::math::Math.fabs(x)` 或 `x.abs()` |
| 向下 / 向上取整 | `x.floor()` / `x.ceil()` |
| 四舍五入 | `x.round()` |
| 判断 NaN / Inf | `x.is_nan()` / `x.is_inf()`（仅 `double`） |
| 可往返文本 | `x.to_str(buf, sizeof buf)` |
| `float` 转文本 | 先 `double d = (double)f;`，再 `d.to_str(buf, sizeof buf)` |

## 7. 排错

### 7.1 在 `float` 上调用 `is_nan`

```ct
import std::math::Real;

int main(void)
{
    float f = 1.0f;
    return f.is_nan();
}
```

```text
main.ct:6:1: error: no method 'is_nan' for type 'float'
```

`is_nan` / `is_inf` / `to_str` 只属于 `Real`（`double`）。
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

- **函数集很小**：只有 `sqrt` / `fabs` / `floor` / `ceil` / `round` / `is_nan` / `is_inf`；
  没有 `pow` / `exp` / `log` / 三角函数 / `hypot` / `isFinite`。
- **`float` 没有文本化**：`to_str` 只在 `double` 上。
- **`Math` 必须带命名空间**：没有裸 `sqrt` 入口。
- **`to_str` 截断会破坏往返**：容量不够时得到短文本，不报错。
- **短格式只有 6 位有效数字**：`to_str` 只在短格式能精确回读时使用它，否则自动加长；
  自己用固定格式拼接不保证往返。
- **链接可能需要 `-lm`**：取决于平台。
- **字面量接收者从 0.20.0 起可用**（本包下限 0.30.0）；0.19.1 只接受变量、元素、调用和字段链。
- **多给实参不报错**：与其它映射模块相同的编译器行为（SH-56）。

## 9. 版本与变更记录

| 版本 | 状态 | 变更 |
| --- | --- | --- |
| `0.1.0` | 未打 tag、未发布 | 当前版本。文档首次成文：此前 `std/math` 只有源码与模块表，没有 README 或教程。 |

编译器下限 `ct >= 0.30.0`。
