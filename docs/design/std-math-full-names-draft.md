# 数学函数全名清单（草案，待确认）

| 项 | 值 |
| --- | --- |
| 性质 | 草案。**不作为定稿**；确认后再并入设计正文 §2.3、§5.1、§6.2 |
| 依据 | 负责人裁定：数学函数一律只提供方法形态；全名 `snake_case` 为主，现有短名保留为别名；`Math` namespace 入口删除 |
| 边界 | 只列 `Float` / `Real` 方法层；`Math.sqrt` / `Math.fabs` 不再保留；整数宿主的 `abs`/`min`/`max` 归 `num`，不在本表 |

| 短名（别名） | 全名（主名） | libc | 备注 |
| --- | --- | --- | --- |
| `abs` / `fabs` | `absolute_value` | `fabs` | 浮点宿主 |
| `sqrt` | `square_root` | `sqrt` | |
| `floor` | `floor` | `floor` | 本身已是完整词 |
| `ceil` | `ceiling` | `ceil` | |
| `round` | `round` | `round` | 本身已是完整词 |
| `truncate` | `truncate` | `trunc` | |
| `min` | `minimum` | `fmin` | 只登记浮点宿主 |
| `max` | `maximum` | `fmax` | 只登记浮点宿主 |
| `is_nan` | `is_nan` | `isnan` | `NaN` 按通用缩写保留 |
| `is_infinite` | `is_infinite` | `isinf` | 已是完整词 |
| `pow` | `power` | `pow` | |
| `exp` | `exponential` | `exp` | |
| `expm1` | `exponential_minus_one` | `expm1` | |
| `log` | `natural_logarithm` | `log` | |
| `log2` | `logarithm_base_two` | `log2` | |
| `log10` | `logarithm_base_ten` | `log10` | |
| `log1p` | `logarithm_one_plus` | `log1p` | |
| `sin` | `sine` | `sin` | |
| `cos` | `cosine` | `cos` | |
| `tan` | `tangent` | `tan` | |
| `asin` | `arc_sine` | `asin` | |
| `acos` | `arc_cosine` | `acos` | |
| `atan` | `arc_tangent` | `atan` | |
| `atan2` | `arc_tangent_two` | `atan2` | |
| `sinh` | `hyperbolic_sine` | `sinh` | |
| `cosh` | `hyperbolic_cosine` | `cosh` | |
| `tanh` | `hyperbolic_tangent` | `tanh` | |
| `hypot` | `hypotenuse` | `hypot` | |
| `cbrt` | `cube_root` | `cbrt` | |
| `fmod` | `floating_remainder` | `fmod` | |
| `remainder` | `remainder` | `remainder` | 本身已是完整词 |
| `fdim` | `positive_difference` | `fdim` | |
| `fma` | `fused_multiply_add` | `fma` | |
| `copysign` | `copy_sign` | `copysign` | |

## 待确认点

1. `floor` / `round` / `truncate` / `remainder` 已是完整词，是否保留原名（本草案按保留处理）。
2. `is_nan` 中的 `nan` 是否按通用缩写保留（本草案按保留处理）。
3. `min` / `max` 的全名取 `minimum` / `maximum`，是否与其他宿主的同名方法冲突（`num` 侧仍叫 `min`/`max`，宿主不同、可共存）。
4. 全名与短名并存期间，README 只推荐全名，短名标注为别名；确认后统一回填 §6.2 查证索引。
