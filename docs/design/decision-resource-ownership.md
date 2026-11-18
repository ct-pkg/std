# 资源对象的形态裁定：CT struct + `Drop`，但释放协议用 `defer { delete }`

| 项 | 值 |
| --- | --- |
| 性质 | 裁定记录；解决 P2–P7 施工中「`own` 能不能作为资源释放协议」的争议 |
| 依据 | 正席 / 反席两轮对抗实测（ct v0.65.1，macOS arm64），探针分别在 `/tmp/battle-pro`、`/tmp/battle-con` |
| 结论 | 资源类型用 **CT struct + `use Drop` + 幂等 `close()` + 双工厂**；**释放协议定为 `defer { delete h; }`**，`own` 不再是文档默认 |
| 影响 | P2 `LocaleHandle`、P3 `File` / `FileDesc`、P4 `Dir`、P5 `Timer`、P7 `Socket`（`FileDesc` 别名） |

## 1. 争议

设计正文（[§2.4 / §2.7 / §2.8 / §5.4](std-scope-and-presentation.md)）把 `File` / `Dir` / `FileDesc` /
`Timer` / `Socket` 定为 ③ 资源对象；`own X *h = "path".open_file(...)` 是设计里的默认写法。
反证文档（[§2.3](../review/std-scope-con-argument.md)）在 ct v0.53.0 上记录过 `own` 不接受方法调用（F6）、
`Drop` 不关 C 资源（F7）、拷贝/逃逸重复释放（F8）。两条结论必须在 v0.65.1 上重新实测后裁定。

## 2. 实测（两席独立，命令与结果都保留在探针目录）

| # | 事实 | 证据 |
| --- | --- | --- |
| F-1 | `own` 只接受 `malloc` / `calloc` / **自由函数**工厂；方法调用和 namespace 静态调用都报 `'own' requires a malloc, calloc or factory call` | 两席一致；本仓 `std/io/Path.m.ct` 的字符串工厂因此不能用于 `own` |
| F-2 | `.h.ct` 可以 `import` 同目录 `.m.ct`；反向（`.m.ct` import `.h.ct`）是循环导入硬错。`.m.ct` 用 `pub type H = struct <ns>_<T>;` 命名 `.h.ct` 生成的 C tag，方法工厂与自由工厂返回**同一类型**、双向可赋值 | 正席 probe 5/6；本仓 probe `/tmp/ownprobe3` |
| F-3 | `use Drop` + 固有 `impl X` **不加 vptr**、不改变 `sizeof`（8 字节仍是 8 字节）；只有 `use Trait` 才追加 vptr（8 → 16） | 正席 probe 7 |
| F-4 | 正确使用时 `own` 在正常落出、`return`、`break` 前都会 `Drop`；`delete` 取消 autodrop，不重复释放；`own` 循环 200 次 0 leak | 正席 probe 1/8；反席 probe 17 |
| F-5 | **v0.65.1 的编译器缺陷**：`own` 句柄一旦作为方法接收者且返回值被局部初始化/赋值捕获（`int n = f->read(...)`、`int rc = f->close()`、`File *p = f;`），生成的 C **完全不插入** Drop/释放，且无任何告警 | 正席「Critical extra finding」；本仓 `/tmp/ownbug/m.c` 复现：只有 `open_file`，没有 `Drop_drop`/`free` |
| F-6 | 值拷贝 `File g = *f;` 会取消 `own f` 的 autodrop，`g` 从不 Drop（0 次、静默泄漏）；字段拷贝进另一个 `use Drop` struct 会 2 次 Drop/双 close；`delete &g`（栈值）编过但 abort | 反席 probe 5/6/7 |
| F-7 | `goto` 跳出 `own` 作用域、对 `own` 重新赋值：静默泄漏；把 `own` 句柄作为函数实参传走：Drop 后全局悬垂（F8 未修） | 反席 probe 9/10 |
| F-8 | `defer { delete h; }` 在正常落出、`return`、`break`、`continue` 都执行；1000 次循环 0 leak（`leaks` 报告 0）；`goto` 跨 defer 是编译期硬错 | 正席 probe 4/8；反席 probe 13；本仓 `/tmp/ownbug/loop.ct` 1000 次通过 |
| F-9 | 只 `close()` 不释放包装体：每次 open 漏 16 B（1000 次 15,984 B），fd 正确关闭 | 反席 probe 1/14 |
| F-10 | 跨单元 `Drop` 需要 ct ≥ 0.48.0，`own` 工厂初始化需要 ct ≥ 0.49.0；`defer` 从 0.38.0 起 | ct-lang `docs/history/phase-log.md` |

## 3. 两席立场

- **正席**：A（CT struct + Drop + 双工厂）在类型安全、零 vptr、同一 C tag 三点上成立；但 `own` 的
  自动释放不可靠，必须改成「自由工厂 + `defer { delete }`」或出参式方法。
- **反席**：拒绝 A，理由是 `own` 与方法工厂不兼容、F8 一整套静默错误、Drop 不是 RAII、成本高于
  16 B/句柄的包装体泄漏；主张退回不透明别名 + 显式 `close()` + `defer`。

## 4. 裁定

**采用正席的类型与工厂结构，采用反席的释放协议。**

1. **类型**：资源类型是 CT struct + `use Drop` + 固有 `impl`（无 vptr）。`Drop` 只调用显式
   `close()`（`File` / `FileDesc` / `Dir` / `Timer`），绝不 `free(self)`——结构体由 `delete` 或
   `own` 释放。
2. **双工厂**：`.h.ct` 里的自由函数工厂（`std::io::open_file`、`std::io::open_as_file_desc`、
   `std::fs::open_dir`、`std::fs::current_dir`、`std::time::make_timer`、`std::locale::make_locale`、
   `std::net::connect`）返回同一类型，供 `own` 使用；`.m.ct` 里的字符串 / 数值工厂
   （`path.open_file`、`path.open_as_file_desc`、`"d".open_dir`、`(100).make_timer`、
   `"host:80".connect`、`"zh".make_locale`）保持 CT 的「字符串起头」写法。
3. **释放协议（文档默认）**：拿到句柄后**下一行**写 `defer { delete h; }`。它覆盖正常落出、
   `return`、`break`、`continue`，在 v0.65.1 上 0 leak；`close()` 幂等，提前 close 后 defer 的
   `delete` 仍是安全的两步（Drop 看到已关闭的句柄直接返回）。
4. **`own` 的地位**：保留双工厂，使 `own` 在「只用语句形式调用」时可用；但 README 不把它写成
   默认，也不承诺「`own` 一定释放」——F-5 是编译器缺陷，修复前 `own` 只是可选糖。
5. **禁止项**（写进每个资源模块 README）：不按值拷贝句柄；不把句柄作为函数实参传走后继续使用；
   不对句柄用 `goto` 跳出作用域；不在 `Drop` 里 `free(self)`。
6. **测试门槛**：每个资源类型至少一个 1000 次 open/delete 循环夹具（证明 fd 不泄漏），
   一个幂等 close 夹具，一个 `defer` 提前返回夹具。

## 5. 与设计正文的差异（需在 P6 汇总）

| 设计写法 | 实际落地 | 原因 |
| --- | --- | --- |
| `own File *f = "p".open_file("r")` | `File *f = "p".open_file("r"); defer { delete f; }` 或 `own File *f = std::io::open_file("p", "r");` | F-1 / F-5 |
| `Dir.current()` | 自由函数 `std::fs::current_dir()`；字符串工厂 `"d".open_dir()` | 类型静态工厂不能初始化 `own`，也不在 CT 的 `map trait` 形态里 |
| `Time.year(t)` | namespace 形态 `std::time::Time.year(t)`（trait `Time` + `for namespace std::time`） | CT 的 namespace 静态方法就是 `namespace::Trait.method` |
| `"zh".make_locale()` → `own Locale` | 同一 `LocaleHandle`，方法工厂需 `delete`；自由工厂 `std::locale::make_locale(name)` 才有 `own` | F-1 |
| `Socket` 为 `FileDesc` 同义别名 | `pub type Socket = FileDesc;`（同一 C tag，方法一套） | 实测跨单元别名可用；`own Socket` 受 F-1 限制时退回 `FileDesc` |

## 6. 版本下限

`ct.pkg` 的 `requires` 从 `ct >= 0.30.0` 抬到 `ct >= 0.49.0`：跨单元 `Drop` 需要 0.48.0，
`own` 工厂初始化需要 0.49.0；本仓只在 0.65.1 上完整验证。低于 0.49.0 的编译器不满足
`Drop` / `own` 的基本前提，不能消费本包的资源对象。

## 7. 复核

```sh
grep -n "defer { delete\|F-5\|own" std/io/README.md std/fs/README.md std/time/README.md
bash tests/run.sh
```
