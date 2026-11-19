applies-to: v0.12.0

# v0-PH-9：ISO C17 补全

| 项 | 值 |
| --- | --- |
| 编号 | `v0-PH-9` |
| 目标 | 按 ISO C17 头文件逐项补齐 std 的缺口，不引入 CT 值类型，变参函数除外 |
| 规范版本 | 1.17.0 |
| version-minor | 9（收口 `0.13.0`，含永久偏移 `+4`） |
| 依赖 | `v0-PH-8` 已收口；D-002 |
| 状态 | 待开工 |

## 范围

**做**：

- 先产出 `docs/iso-c-gap.md`：ISO C17 头文件 × 现状 × 缺口（已覆盖 / 直调 / 待补 / 不适用）。
- `string.h` 缺口：`strspn` / `strcspn` / `strpbrk` / `strcoll` / `strxfrm`。
- `stdio.h` 缺口：`tmpfile` / `tmpnam` / `setvbuf` / `freopen` / `perror` / `putc` / `getc` / `fgets` / `fputs`。
- `stdlib.h` 缺口：`div` / `ldiv` / `lldiv`（出参）/ `aligned_alloc` / `at_quick_exit` / `quick_exit` / `rand` / `srand`。
- `time.h` 缺口：`gmtime` 字段 / `timegm` / `timespec_get`。
- `uchar.h` / `wctype.h`：UTF-16/32 转换与宽字符分类。
- `fenv.h`：舍入模式与异常标志。
- `complex.h` / `stdatomic.h` / `threads.h`：先探针；能做则做，做不了在卡片写 `deferred` + 目标 phase，不静默跳过。

**不做**：

- 变参 `scanf` / `printf` / `v*` 族；`setjmp` / `stdarg` / `tgmath` 只写直调说明。
- 不建 CT 值类型；`div_t` / `ldiv_t` / `complex` 用出参或两数表示。

## 交付物

| 交付物 | 路径 |
| --- | --- |
| 缺口表 | `docs/iso-c-gap.md` |
| string/stdio/stdlib/time/uchar/wctype/fenv 补丁 | `std/str`、`std/io`、`std/fs`、`std/time`、`std/wchar`、`std/num` 下对应单元 |
| 测试 | `tests/iso_c/*.ct`（新建） |
| 文档 | 各模块 README 的新增接口段 |

## 任务清单

| id | deliverable | verify | effort |
| --- | --- | --- | --- |
| T1 | `docs/iso-c-gap.md` | `tools/check.sh g4` | S |
| T2 | `string.h` 六个缺口 | `bash tests/run.sh` 含 `tests/iso_c/string.ct` | M |
| T3 | `stdio.h` 九个缺口 | `bash tests/run.sh` 含 `tests/iso_c/stdio.ct` | M |
| T4 | `stdlib.h` 七个缺口 | `bash tests/run.sh` 含 `tests/iso_c/stdlib.ct` | M |
| T5 | `time.h` UTC/timespec | `bash tests/run.sh` 含 `tests/iso_c/time.ct` | M |
| T6 | `uchar.h` / `wctype.h` | `bash tests/run.sh` 含 `tests/iso_c/wide.ct` | M |
| T7 | `fenv.h` | `bash tests/run.sh` 含 `tests/iso_c/fenv.ct` | M |
| T8 | `complex.h` / `stdatomic.h` / `threads.h` 探针与结论 | `docs/iso-c-gap.md` 的对应行有 `done` 或 `deferred: <phase>` | M |

## 退出判据

- [ ] `docs/iso-c-gap.md` 覆盖全部 ISO C17 头文件，无 `todo` 未处置。
- [ ] T2–T8 每项的测试通过；`bash tests/run.sh` 全绿。
- [ ] `bash tools/check.sh all` 退出码 0；`ct pkg check .` 通过。
- [ ] 版本落点全部为 `0.13.0`；annotated tag `v0.13.0` 指向收口提交并推送。
- [ ] **任务清单每项可独立验收（6.6 T-6）**：收口对照表逐项贴 verify 输出。
- [ ] 无法实现的 ISO C 项在缺口表标 `deferred: <phase>`，不得写“以后再说”。

## 进度

| 任务 | 状态 | 证据 |
| --- | --- | --- |
| T1 | done | `docs/iso-c-gap.md` |
| T2 | done | `tests/iso_c/string.ct` 6 checks |
| T3 / T4 / T6 / T7 / T8 | pending | 18:05 继续 |
| T5 | done | `tests/iso_c/time.ct` 7 checks |

## 已知风险

| 风险 | 触发信号 | 处置 |
| --- | --- | --- |
| CT 没有 complex/atomic 类型 | 探针编译失败 | 缺口表标 `deferred`，目标 PH-10 或单独立项 |
| `div_t` 结构体无法返回值 | 只能出参 | 用 `div_quot`/`div_rem` 两函数 |
| 平台差异 | macOS/Linux 声明不同 | shim 条件编译 + Podman 复验 |
