# `std/` — 命名空间根（导航）

这里是 `-I vendor/std` 指向的目录，`import std::<mod>::<Unit>` 的根。包级说明（定位、安装、
版本策略、跨模块用法）在[包根 README](../README.md)；本页只做索引，不再复制 API 表。

| 模块 | 入口 import | 文档 |
| --- | --- | --- |
| `std::str` | `std::str::String` | [str/README.md](str/README.md) |
| `std::io` | `std::io::Io` / `Stdout` / `File` | [io/README.md](io/README.md) |
| `std::fs` | `std::fs::Fs` | [fs/README.md](fs/README.md) |
| `std::utf8` | `std::utf8::Utf8Ops` | [utf8/README.md](utf8/README.md) |
| `std::num` | `std::num::Num` | [num/README.md](num/README.md) |
| `std::math` | `std::math::Math` / `Real` | [math/README.md](math/README.md) |
| `std::vec` | `std::vec::Vec` | [vec/README.md](vec/README.md) |
| `std::map` | `std::map::Map` | [map/README.md](map/README.md) |
| `std::arena` | `std::arena::Arena` | [arena/README.md](arena/README.md) |
| `std::test` | `std::test::Test` | [test/README.md](test/README.md) |

十个模块的教程都已并入各自的 README，包内不再有独立的 `tutorial.md`。
