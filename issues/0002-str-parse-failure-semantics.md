# `std::str` 需要一组严格数值解析：`parse_*` 的失败语义

| 项 | 值 |
| --- | --- |
| 状态 | **已解决（0.11.0；契约按设计 D4 改为出参 + 0/-1，见 §12）** |
| 影响 | 新增 4 个方法；不改现有方法的行为 |
| 关联 | [设计提案 §2.1](file:///Users/samxx/trae-ws/std/docs/design/std-scope-and-presentation.md) |
| 验证情况 | **未实现、未运行**；下文所有行为均为设计口径，不是实测结果 |

## 1. 问题

`std::str` 现在只有宽松解析：`to_int()` / `to_double()` 直接落到 `atoi` / `atof`（`std/str/String.shim.h:211-220`），
没有错误信号。调用方拿到 `0` 时无法区分「输入就是 0」「输入不是数字」「溢出」，而 `atoi` 的溢出还是
未定义行为。需要一组严格解析方法，失败可判定、与 libc 同构。

## 2. 现状

- `String` trait 现有 14 个方法（`std/str/String.m.ct:50-63`），其中 `to_int()` 返回 `int`、
  `to_double()` 返回 `double`。
- 二者 shim 均为 `atoi` / `atof`（`String.shim.h:211-220`），空指针返回 0。
- 本仓目前没有 `parse_*`，也没有任何解析错误报告机制。

## 3. 为什么不能继续用 `atoi` / `atof`

1. **无失败信号**：非数字输入返回 0，与合法的 `"0"` 不可区分。
2. **溢出是未定义行为**：`atoi` 对超出 `int` 的输入没有定义；`strtol` 才有 `ERANGE`。
3. **吞掉尾部**：`"12abc"` 静默返回 12，调用方无法知道输入不合法。
4. **无部分/全量区分**：`atoi` 不提供 `endptr`，无法判断是否消费到结尾。

## 4. 期望语义

新增 4 个方法，全部为 `char *` 宿主，模块 `std::str`：

| 方法 | 返回 | 底层 |
| --- | --- | --- |
| `parse_long()` | `long` | `strtol` |
| `parse_llong()` | `long long` | `strtoll` |
| `parse_ulong()` | `unsigned long` | `strtoul` |
| `parse_real()` | `double` | `strtod` |

统一规则：

- **必须**使用 `endptr` + `errno`，禁止退化为 `atoi` / `atof`。
- **全量解析**：允许前导空白与尾随空白；出现其它未消费字符即失败。
- **失败返回 0**，并设置 `errno`：
  - `EINVAL`：没有解析到数字（空串、纯空白、非法字符、仅符号），或尾部有非空字符；
  - `ERANGE`：数值超出目标类型范围（含下溢，按 libc 行为）。
- **成功不保证 `errno` 为零**：与 libc 同构，调用方必须先把 `errno` 置 0，再根据「返回值 + errno」判定。
  因此合法的 `"0"` 与失败的区别只能靠 `errno`。
- `parse_ulong()` 拒绝前导 `-`（返回 0 + `EINVAL`），不做补码回绕。
- `parse_real()` 接受 `strtod` 能完整识别的写法（含 `inf` / `nan`）；尾部规则同上。
- 不引入新类型、不出参、不改变 `to_int` / `to_double`。

## 5. 与 `to_int` / `to_double` 的区别（不许混用）

| 输入 | `to_int()` | `parse_long()` |
| --- | --- | --- |
| `"12abc"` | `12`（宽松，尾随忽略） | `0`，`errno = EINVAL` |
| `"12"` | `12` | `12` |
| `""` | `0` | `0`，`errno = EINVAL` |
| `"9223372036854775808"` | 未定义 | `0`，`errno = ERANGE` |

文档必须写清：`to_*` 是 `atoi`/`atof` 语义（永远「成功」、无错误），`parse_*` 是严格语义（会失败）；
需要判断输入合法性时只能用 `parse_*`。

## 6. 可执行验收判据

新增 `tests/str/` fixture，至少覆盖：

1. `"42"` → `42`，`errno == 0`；
2. `" 42 "` → `42`，`errno == 0`（前导/尾随空白允许）；
3. `"12abc"` → `0`，`errno == EINVAL`；
4. `""` 与 `"   "` → `0`，`errno == EINVAL`；
5. `"9223372036854775808"`（`LONG_MAX + 1`）→ `0`，`errno == ERANGE`；
6. `parse_ulong("-1")` → `0`，`errno == EINVAL`；
7. `parse_real("3.5")` → `3.5`，`errno == 0`；`parse_real("3.5x")` → `0`，`errno == EINVAL`。

每个用例调用前先 `errno = 0`。fixture 可直接 `#include <errno.h>` 读取 `errno`（本仓测试已有直接调用
libc 的先例）。`tests/run.sh` 全绿是本条完成的唯一判据。

## 7. 影响面

- `std/str/String.m.ct`：新增 4 条声明与映射（`CStrOps` 改名落地后同样要加）。
- `std/str/String.shim.h`：新增 4 个 helper，`#include <errno.h>`、`<limits.h>`。
- `std/str/README.md`：API 表新增 4 行，并加「宽松 vs 严格」对照与 `errno` 使用说明。
- `tests/str/main.ct`：新增上述用例。
- `README.md` §4 的 `char *` 名字表：新增 4 个名字。
- 设计提案 §2.1 的「新增」行同步。

## 8. 命名合规

`parse_long` / `parse_llong` / `parse_ulong` / `parse_real` 均为 `snake_case`，动词前缀 `parse_`，
与既有 `to_*` 区分明确；与 libc 名字（`strtol` 等）不冲突，不占用任何现有 `char *` 名字。

## 9. 顺带登记（纯补齐，不涉及解析语义）

- `rfind_char(int c)`：反向查找，未命中返回 `-1`，与 `find_char` 对称。
- `compare_ignore_case(const char *other)`：忽略 ASCII 大小写比较，返回 `<0` / `0` / `>0`，与
  `compare` 同构。

两者都只补方法，不引入新类型，不改变现有行为；验收为各自 fixture 用例 + `tests/run.sh` 全绿。

## 10. 非目标

- 不引入 `Result` 之类的新类型，不改 `to_int` / `to_double` 的宽松语义。
- 不做进制参数化（`to_base` 只负责输出）；如需要 `parse_base`，另开条目。
- 不改 `std::num` 的任何方法。

## 11. 状态与阻塞（历史；已被 §12 取代）

- 状态：**待排期**（登记时）。
- 阻塞：无外部依赖，不依赖发布机制或任何未具备的能力；只等本仓排期。
- 注意：本条所有语义均未实现、未运行，不得在文档或提交信息中写成「已通过」。

## 12. 处理结果（2026-11-19）

**已实现**（0.11.0，commit `918a9b0`），但契约按设计正文 §2.1 / D4 定为
**出参 + 状态码**，不是本文 §4 的「返回值 + errno」：

| 方法 | 实际签名 | 成功 | 失败 |
| --- | --- | --- | --- |
| `parse_long` | `int s.parse_long(long *out)` | `0`，写 `*out` | `-1`，不写 |
| `parse_long_long` | `int s.parse_long_long(long long *out)` | 同上 | 同上 |
| `parse_unsigned_long` | `int s.parse_unsigned_long(unsigned long *out)` | 同上 | 同上 |
| `parse_real` | `int s.parse_real(double *out)` | 同上 | 同上 |

失败条件：空串/纯空白、`out == NULL`、无可解析前缀、尾随非空白字符、`ERANGE`；unsigned 形式
额外拒绝前导 `-`。**允许前导与尾随空白**。成功/失败不读写 `errno`（内部清零只用于检测
`ERANGE`，不作为对外契约）。

与 §4 的差异及理由：

1. **出参 + 0/-1**：设计正文 §2.1 已把 `parse_long(out)` 定为出参形态（D4「失败必须可报；
   链式返回值无法同时表达」）。它避开「合法 0 与失败 0 不可区分」和 `errno` 全局状态，
   与本包 `copy` / `read_line` 一类状态码一致。
2. **方法名取设计名**：`parse_long_long` / `parse_unsigned_long`，不用本文 §4 的
   `parse_llong` / `parse_ulong`；设计正文 §5.1 的展开名优先。
3. **unsigned 拒绝 `-`**：与本文 §4 一致（`strtoul` 的补码回绕是坑），已由
   `tests/str/parse_extra.ct` 钉住。
4. **不承诺 `errno`**：调用方只看返回值；需要 errno 的调用方应直接用 libc。

**§9 顺带登记也已完成**：`reverse_find_char` 与 `compare_ignore_case` 已在 0.11.0 落地
（设计名；`tests/str/more.ct`），不是本文写的 `rfind_char`。

**验收**：`tests/str/parse_extra.ct`（21 项）与 `tests/str/more.ct`（9 项）通过；
`tests/run.sh` 全绿。API 表与「宽松 vs 严格」对照已写入 `std/str/README.md`；
`README.md` §4.2 的 `char *` 名字表已增 6 行（4 个 parse + 2 个 §9 方法）。

**仍然不做**：不引入 `Result`，不改 `to_int` / `to_double`，不做 `parse_base`。
