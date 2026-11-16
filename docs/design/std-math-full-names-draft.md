# 数学函数命名清单（已定稿，并入 §2.3）

| 项 | 值 |
| --- | --- |
| 状态 | **已由负责人裁定**；§2.3、§5.1、§6.2 已按此同步 |
| 前提 | 数学函数一律只提供方法形态；`Math` namespace 入口删除 |
| 范围 | 只列 `Float` / `Real` 方法层；整数宿主的 `abs`/`min`/`max` 归 `num` |

## 改名的 7 项（生僻名 → 全称）

| 原名 | 定名 | libc |
| --- | --- | --- |
| `expm1` | `exponential_minus_one` | `expm1` |
| `log1p` | `logarithm_one_plus` | `log1p` |
| `fdim` | `positive_difference` | `fdim` |
| `fma` | `fused_multiply_add` | `fma` |
| `cbrt` | `cube_root` | `cbrt` |
| `fmod` | `floating_modulo` | `fmod` |
| `copysign` | `copy_sign` | `copysign` |

## 保留原名（只做 snake_case 化）

`abs`、`sqrt`、`pow`、`log`、`sin`、`cos`、`max`、`min`、`floor`、`round`、`remainder`、
`truncate`、`is_nan`、`is_infinite`、`exp`、`exp2`、`log2`、`log10`、`tan`、`asin`、`acos`、
`atan`、`atan2`、`sinh`、`cosh`、`tanh`、`hypot`、`ceil`。

## 备注

- 保留项不再设“全名主名 + 短名别名”的双层命名；名字就是原名。
- 改名项不再保留旧名作别名。
- `Math.sqrt` / `Math.fabs` 已删除，调用改为 `x.sqrt()` / `x.abs()` 形态。
