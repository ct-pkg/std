# `std::locale` — 区域设置的命名、应用与数字约定

`"zh_CN.UTF-8".make_locale()` 只保存名字；`apply(category)` 才调用 `setlocale`；
`conventions(...)` 把 `localeconv()` 的小数点与千位分隔符拷进调用方缓冲。

## 1. 定位与边界

- **工厂不改变进程**：`make_locale` 只复制名字；`apply` 才改进程全局 locale。
- **`apply` 是全局副作用**：它改的是整个进程的 `setlocale` 状态，线程不安全；调用点必须
  自己保证串行。`LC_NUMERIC` 会影响 `std::num` / `std::math` 的浮点文本与解析。
- **不做值拷贝**：`LocaleHandle` 持有名字指针，复制结构体值会产生两个“所有者”；只传指针，
  释放用一个 `delete`（或 `own` 自动释放）。
- **句柄拥有名字**：`LocaleHandle` 是 CT struct + `use Drop`；拿到句柄后写
  `defer { delete l; }`，出口释放名字副本。字符串工厂返回同一类型；方法调用不能初始化
  `own`，而且 ct 0.67.0 会在方法返回值被局部捕获时丢掉 `own` 的释放，所以 `own` 只是可选糖。
- **`conventions` 拷贝而不是返回借用指针**：`struct lconv *` 是静态对象，本包只暴露
  `decimal_point` 与 `thousands_sep` 两个字段，写调用方缓冲。

## 2. 依赖与安装

依赖 C 的 `<locale.h>`；`requires ct >= 0.67.0`。

| 文件 | 内容 |
| --- | --- |
| `LocaleHandle.h.ct` | `pub struct LocaleHandle`（`use Drop`）、自由工厂 `make_locale`、分类常量、`current_locale`、`impl LocaleHandle` |
| `LocaleFactory.m.ct` | `char *` 宿主上的字符串工厂 `"name".make_locale()` |
| `LocaleHandle.shim.h` / `LocaleFactory.shim.h` | `std_locale_*` 的 `static inline` 实现 |

**为什么句柄叫 `LocaleHandle` 而不是 `Locale`**：`.h.ct` 单元会生成同名导出头。`Locale.h.ct`
生成 `.ctbuild/Locale.h`，而 `ct build` 把 `.ctbuild` 放进 `cc -I`；在大小写不敏感的文件系统
（macOS）上它会顶替系统的 `<locale.h>`，导致 shim 里的 `setlocale` / `lc_*` 常量全部失效。
改名后生成的 `LocaleHandle.h` 不再与任何系统头重名。`Time.h.ct` 同理改用 `Timer.h.ct` +
`Time.m.ct`。

引入方式：`import std::locale::LocaleHandle;`（它 `import` 了工厂单元，字符串方法一并可见）。

## 3. 快速开始

```ct
import std::locale::LocaleHandle;

int main(void)
{
    char dp[8];
    char ts[8];
    LocaleHandle *c = "C".make_locale();
    defer { delete c; }

    c->apply(std::locale::lc_all());
    c->conventions(dp, 8, ts, 8);   /* "." and "" in the C locale */
    return 0;
}
```

自由工厂（`own` 可选；默认仍是 `defer { delete }`）：

```ct
LocaleHandle *l = std::locale::make_locale("C");
defer { delete l; }
l->apply(std::locale::lc_numeric());
```

## 4. API 全表

| 方法 | 签名 | 语义 | 层 | 失败 |
| --- | --- | --- | --- | --- |
| `make_locale(name)` | `LocaleHandle *std::locale::make_locale(const char *name)` | 复制名字，不调用 `setlocale` | ② | 空名/内存不足 `0` |
| `"name".make_locale()` | `LocaleHandle *name.make_locale()` | 同上，字符串工厂 | ② | `0` |
| `name_to_str(out, cap)` | `int l->name_to_str(char *out, int cap)` | 取保存的名字 | ③ | 放不下 `-1` |
| `apply(category)` | `int l->apply(int category)` | `setlocale(category, name)` | ③ | 平台拒绝 `-1` |
| `conventions(dp, dp_cap, ts, ts_cap)` | `int l->conventions(char *dp, int dp_cap, char *ts, int ts_cap)` | 拷贝小数点/千位分隔符 | ③ | 任一缓冲不足 `-1` |
| `lc_all()` 等 | `int std::locale::lc_all()` | 分类常量 | ① | 无 |
| `current_locale(out, cap)` | `int std::locale::current_locale(char *out, int cap)` | 读进程当前 locale 名 | ① | 放不下 `-1` |

分类常量：`lc_all` / `lc_numeric` / `lc_time` / `lc_collate` / `lc_ctype` / `lc_monetary`。

**句柄释放**：`defer { delete l; }` 在正常落出、`return`、`break`、`continue` 前执行 `Drop`
（释放名字）并回收结构体。两个工厂的句柄同一类型、同一协议。不要按值拷贝句柄。

## 5. 与相邻模块的关系

- `std::wchar` 的多字节转换跟随 locale；先 `apply` 再转。
- `std::num::to_str` / `std::math::to_str` 的浮点文本受 `LC_NUMERIC` 影响；整数不受。
- `std::env` 的 `LANG` / `LC_*` 不会被 `make_locale` 读取：名字必须显式给出，
  想跟随环境就自己 `get_env`。

## 6. 限制与未支持

- **`apply` 全局且线程不安全**：没有 per-thread locale；多线程程序应在启动时固定 `"C"`，
  或自己串行化所有 `apply`。
- **`conventions` 只给小数字段**：`grouping`、货币符号等仍需要直接调 C 的 `localeconv`。
- **不校验名字**：`make_locale` 接受任何非空字符串；合法性只在 `apply` 时由平台判定。
- **不存在 `Locale` 类型别名**：CT 的 `own` 初始化式按字面结构体名匹配，别名不能用于 `own`；
  统一用 `LocaleHandle`。
- **不提供 `setlocale` 的全局直调**：只能经句柄的 `apply`，避免两套写法。

## 7. 版本与变更记录

| 版本 | 状态 | 变更 |
| --- | --- | --- |
| `0.6.0` | 本轮新增 | 新建模块：`LocaleHandle`（`own` / `Drop`）、字符串工厂、分类常量、`conventions` 拷贝式。 |

编译器下限 `ct >= 0.67.0`。
