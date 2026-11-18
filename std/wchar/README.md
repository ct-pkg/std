# `std::wchar` — 宽字符与多字节的互转

`<wchar.h>` 的三件事：宽字符串长度、多字节转宽、宽转多字节。CT 没有 `wchar_t`，宽字符串在
边界上用 `int *` 表示，宽度一致性由 shim 的编译期检查保证。

## 1. 定位与边界

- 宽字符串类型是 `int *`：本包在 Linux / macOS / BSD 上构建，`wchar_t` 与 `int` 同宽；
  shim 里的 `sizeof(wchar_t) == sizeof(int)` 检查不通过就编不过。
- 三个能力各一个入口，另有方法拼写：
  - `std::wchar::Wchar.wide_string_length(ws)` / `ws.wide_length()`；
  - `"text".to_wide(out, cap)`（`cap` 是宽单元数）；
  - `ws.to_multibyte(out, cap)`（`cap` 是字节数）。
- 转换先数一遍再写：装不下（含终止符）返回 `-1` 并清空目标，不截断、不分配。
- 编码跟随当前 locale；`"C"` 下只支持 ASCII。需要 UTF-8 时先 `std::locale` 的 `apply`。
- 不做 `wprintf` / `wscanf` 族（变参），也不建宽字符串类型。

## 2. 依赖与安装

依赖 C 的 `<wchar.h>` / `<stdlib.h>`；`requires ct >= 0.49.0`。

| 文件 | 内容 |
| --- | --- |
| `Wchar.m.ct` | `Wchar`（namespace 宿主）、`WideText`（`char *` 宿主）、`WideString`（`int *` 宿主） |
| `Wchar.shim.h` | `std_wchar_*` 的 `static inline` 实现与宽度检查 |

引入方式：`import std::wchar::Wchar;`。

## 3. 快速开始

```ct
import std::test::Test;
import std::wchar::Wchar;

int main(void)
{
    int wbuf[16];
    char out[16];
    int *wp = wbuf;

    "hello".to_wide(wbuf, 16);   /* 5, wbuf[5] == 0 */
    wp.wide_length();            /* 5 */
    wp.to_multibyte(out, 16);    /* 5, out == "hello" */
    return 0;
}
```

## 4. API 全表

| 方法 | 签名 | 语义 | 层 | 失败 |
| --- | --- | --- | --- | --- |
| `wide_string_length(ws)` | `int std::wchar::Wchar.wide_string_length(int *ws)` | `wcslen` | ① | 空指针 `-1` |
| `multibyte_to_wide_string(text, out, cap)` | `int ...(const char *text, int *out, int cap)` | `mbstowcs` | ① | 非法序列或放不下 `-1` |
| `wide_to_multibyte_string(ws, out, cap)` | `int ...(int *ws, char *out, int cap)` | `wcstombs` | ① | 同上 |
| `to_wide(out, cap)` | `int text.to_wide(int *out, int cap)` | 同上，`char *` 接收者 | ② | 同上 |
| `wide_length()` | `int ws.wide_length()` | 同上，`int *` 接收者 | ② | `-1` |
| `to_multibyte(out, cap)` | `int ws.to_multibyte(char *out, int cap)` | 同上，`int *` 接收者 | ② | 同上 |

**容量单位**：`to_wide` / `multibyte_to_wide_string` 的 `cap` 数宽单元；`to_multibyte` /
`wide_to_multibyte_string` 的 `cap` 数字节。两者都包含终止符位置。

## 5. 与相邻模块的关系

- `std::utf8` 处理 UTF-8 字节序列（不依赖 locale）；本模块走 libc 的 locale 编码转换。
  同一段文本通常只用其中一条路径。
- `std::locale` 的 `apply` 改变转换所用的编码；`std::wchar` 不自带 `setlocale`。
- `std::str` 的 `length` 数字节；`wide_length` 数宽单元，两者不可互换。

## 6. 限制与未支持

- **只支持 `wchar_t` 与 `int` 同宽的平台**；2 字节 `wchar_t` 的平台编译期拒绝。
- **编码依赖 locale**：默认 `"C"` 只认 ASCII；非 ASCII 需要先 `std::locale` 的 `apply`。
- **不做宽字符分类 / 大小写 / 格式化**：需要时用 libc 的 `<wctype.h>` 与 `<wchar.h>` 直调。
- **不分配**：所有结果写调用方缓冲，容量不足即 `-1`。
- **非法序列没有部分结果**：`mbstowcs` 失败时目标被清空。

## 7. 版本与变更记录

| 版本 | 状态 | 变更 |
| --- | --- | --- |
| `0.6.0` | 本轮新增 | 新建模块：宽字符串长度与双向转换（namespace + 两种接收者）。 |

编译器下限 `ct >= 0.49.0`。
