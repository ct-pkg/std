# `std::env` — 环境变量的复制式读写

`name.get_env(out, cap)` 把 `getenv` 的借用指针拷进调用方缓冲；`std::env::Env.set(name, value)` /
`unset(name)` 走 namespace。不分配、不返回借用指针。

## 1. 定位与边界

- **只读拷贝**：`get_env` 返回字节数；名字不存在或缓冲放不下（含 NUL）返回 `-1` 并清空缓冲。
  空值是成功的 `0`，不是未命中——这两件事必须能区分。
- `set` 总是覆盖（`setenv` 的 `overwrite=1`）；`unset` 删除。二者返回 `0` / `-1`。
- 不包 `putenv`：它接管调用方字符串的所有权，与本包“借用/拥有分清”的规则冲突。
- `environ` 不暴露：遍历环境变量请直接读 C 侧，或逐个名字查询。

## 2. 依赖与安装

依赖 C 的 `<stdlib.h>` / `<string.h>`；`requires ct >= 0.49.0`。

| 文件 | 内容 |
| --- | --- |
| `Env.m.ct` | `EnvOps`（`char *` 宿主，`get_env`）+ `Env`（namespace 宿主，`set` / `unset`） |
| `Env.shim.h` | `std_env_get` / `set` / `unset` 的 `static inline` 实现 |

引入方式：`import std::env::Env;`。

## 3. 快速开始

```ct
import std::env::Env;
import std::str::String;
import std::test::Test;

int main(void)
{
    char value[64];

    std::env::Env.set("CT_DEMO", "hello");
    if ("CT_DEMO".get_env(value, 64) == 5) {
        /* value is a private copy of "hello" */
    }
    std::env::Env.unset("CT_DEMO");
    return 0;
}
```

## 4. API 全表

| 方法 | 签名 | 语义 | 层 | 失败 |
| --- | --- | --- | --- | --- |
| `get_env(out, cap)` | `int name.get_env(char *out, int cap)` | 拷贝环境变量值 | ② | 未设置或放不下 `-1`；空值 `0` |
| `set(name, value)` | `int std::env::Env.set(const char *name, const char *value)` | `setenv(..., 1)` | ① | `-1` |
| `unset(name)` | `int std::env::Env.unset(const char *name)` | `unsetenv` | ① | `-1` |

空名字（`""`）在 `get_env` / `set` / `unset` 里都返回 `-1`。

## 5. 与相邻模块的关系

- `std::process` 的 `execute_with_environment` 接收 `char **envp`，与 `set` / `unset` 是两层：
  前者是子进程镜像的环境数组，后者是当前进程的环境。
- `std::locale` 的 `make_locale` 只保存名字，不读环境变量；需要跟随 `LANG` / `LC_*` 时由调用方
  自己 `get_env`。

## 6. 限制与未支持

- **不提供路径列表切分**：`PATH` 一类值请用 `std::str` 的 `split_into`。
- **值长度无上限**：只受 `cap` 限制；`get_env` 不返回所需容量，缓冲不足时请放大后重试。
- **线程安全**：`getenv` / `setenv` 的线程语义随平台；本模块不加锁。
- **不复制名字**：`set` / `unset` 只在调用期间使用 `name`。

## 7. 版本与变更记录

| 版本 | 状态 | 变更 |
| --- | --- | --- |
| `0.6.0` | 本轮新增 | 新建模块：`get_env` / `set` / `unset`。 |

编译器下限 `ct >= 0.49.0`。
