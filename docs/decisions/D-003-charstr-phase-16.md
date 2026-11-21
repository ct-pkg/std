applies-to: v0.20.0

# D-003：`v0-PH-16` 立项与 `0.20.0` 发行

| 项 | 值 |
| --- | --- |
| id | D-003 |
| date | 2026-11-21 |
| anchor | `e613880` |
| status | accepted |
| trigger | 用户指令「按 issue 0004 发版 0.20.0」 |
| judge | USER |
| seats | REQ |
| scope | `std::str` 的单元与 trait 改名；内建层边界的记录与 `lib/std` 清单 |
| target | `v0-PH-16` |

## 决定

1. `std::str` 的单元文件与 trait 统一为 `CharStr`（issue [0004](../../issues/0004-str-unit-name-charstr.md)）。
   保留 `std::str` 模块；不新建 `std::char`。
2. 内建层边界按方案 B：只删语言级类型重复（核查 0 处），C 资源句柄与 libc/POSIX 映射保留
   （[builtin-layer-boundary.md](../planning/builtin-layer-boundary.md)）。
3. 本项目尚未正式发布，改名不留兼容窗口，旧 `import` 直接硬错。
4. 本 phase 收口版本 `0.20.0`（`v0-PH-16`，含永久偏移 `+4`）。

## 理由

- `0.4.0` 只改了 trait 名，单元与 import 仍是 `String`，名字与语义不符（见 issue 0004 的证据）。
- 语言级类型归内建层后，本仓的边界需要成文，避免后续再次出现重复定义。
- `0.19.0` 是包身份归一发行、未开 phase，故本 phase 取 `v0-PH-16`，`0.19.0` 对应的号位不复用。

## 授权

- 签字：USER（2026-11-21，会话指令）
