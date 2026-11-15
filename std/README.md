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
| `std::arena` | `std::arena::Arena` | [arena/README.md](arena/README.md) |
| `std::test` | `std::test::Test` | [test/README.md](test/README.md) |

八个模块的教程都已并入各自的 README，包内不再有独立的 `tutorial.md`。

容器不在本包：`Vec<T>` / `Map<K,V>` 自 ct 0.30.0 起是编译器的内置层（`lib/std/Vec.h.ct`，
自动加载、不需要 import）。本包 0.2.1 删掉了与它们重名的 `std::vec` / `std::map`。
