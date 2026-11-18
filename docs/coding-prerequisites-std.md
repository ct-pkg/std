# 编码前必读（std 篇）

通用条款见工程规范 `CG-1`…`CG-7`；本文只列本包专有内容，不重复通用条款正文。

## 1. `char *` 名字账本

完整名字与逐条说明见 [设计正文 §5.2](design/std-scope-and-presentation.md)。当前按宿主计：

| 宿主 | 名字数 |
| --- | --- |
| `char *` | 53 |
| `char` | 14 |
| `long` | 7 |
| `float` | 25 |
| `double` | 29 |

名字总量不设上限；准入仍按 §1.2 的便利性判据。

## 2. 模块、方法与映射表

| 模块 | 方法清单 | 查证索引 |
| --- | --- | --- |
| `std::str` | [§2.1](design/std-scope-and-presentation.md) | [§6.2](design/std-scope-and-presentation.md) |
| `std::num` | [§2.2](design/std-scope-and-presentation.md) | [§6.2](design/std-scope-and-presentation.md) |
| `std::math` | [§2.3](design/std-scope-and-presentation.md) | [§6.2](design/std-scope-and-presentation.md) |
| `std::io` | [§2.4](design/std-scope-and-presentation.md) | [§6.2](design/std-scope-and-presentation.md) |
| `std::fs` | [§2.5](design/std-scope-and-presentation.md) | [§6.2](design/std-scope-and-presentation.md) |
| `std::utf8` / `std::arena` / `std::test` | [§2.6](design/std-scope-and-presentation.md) | [§6.2](design/std-scope-and-presentation.md) |
| `time` / `env` / `process` / `errno` / `locale` / `net` | [§2.7](design/std-scope-and-presentation.md) | [§6.2](design/std-scope-and-presentation.md) |
| 原排除项 | [§2.9](design/std-scope-and-presentation.md) | [§6.2](design/std-scope-and-presentation.md) |

数学全名清单见 [`std-math-full-names-draft.md`](std-math-full-names-draft.md)。

## 3. 工厂命名清单

| 工厂 | 产出 | 位置 |
| --- | --- | --- |
| `open_file(mode)` | `File *`（字符串工厂；释放用 `defer { delete }`） | §2.4 |
| `open_dir()` | `Dir *`（字符串工厂；释放用 `defer { delete }`） | §2.5 |
| `open_as_file_desc(flags)` | `FileDesc *`（同一类型） | §2.4 |
| `make_locale()` | `LocaleHandle *` | §2.7 |
| `make_timer(ms, sig)` | `Timer *` | §2.5 |
| `make_timer_once(ms, sig)` | `Timer *` | §2.5 |
| `connect()` | `Socket *`（`FileDesc` 别名） | §2.7 |
| `current_dir()` | `Dir *`（自由函数） | §2.5 |

## 4. Roadmap

分期、版本位、依赖与验收见 [`docs/roadmap.md`](../roadmap.md)。
