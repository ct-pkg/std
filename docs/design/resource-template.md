# 资源对象模板（P6 收口）

| 项 | 值 |
| --- | --- |
| 性质 | 实现约定；`File` / `FileDesc` / `Dir` / `Timer` 四个 ③ 资源对象共用 |
| 依据 | P3–P5 的实现与 [资源对象裁定](decision-resource-ownership.md) |
| 状态 | 已落地；`Socket` 是 `FileDesc` 的类型别名，不加第二份模板 |

## 1. 结构

```ct
pub struct X {
    <native handle>;      /* void * 或 int；C 类型不跨边界 */
} use Drop;

/* 自由工厂：own 可用的唯一入口 */
pub struct X *make_x(...)
{
    struct X *x = malloc(sizeof(struct X));
    if (!x) return 0;
    x->native = <open>(...);
    if (!x->native) { free(x); return 0; }
    return x;
}

impl X {
    pub int close() { /* 幂等；关闭后置空/ -1；返回 0/-1 */ }
    pub ... /* 其它方法 */
}

/* 必须放在固有 impl 之后：ct 按声明顺序解析方法名。 */
impl Drop for X {
    void drop()
    {
        self->close();
    }
}
```

配套的 `.m.ct` 工厂（字符串 / 数值接收者）只做一件事：别名 `.h.ct` 生成的 C tag，
把接收者转成自由工厂的调用：

```ct
pub type XHandle = struct <ns>_X;
pub trait PathX {
    XHandle *make_x(const char *arg);
}
map trait PathX for type char * {
    make_x(path, arg) -> std_x_from_path(path, arg);
}
```

## 2. 四条硬规则

1. **`Drop` 只调用 `close()`**，绝不 `free(self)`：`delete` 负责结构体，`own` 负责 `Drop + free`。
2. **`close()` 幂等**：关闭后把句柄置 `0` / `-1`；NULL 句柄返回 0；重复 `close`、`Drop`、
   `delete` 都安全。
3. **释放协议是 `defer { delete h; }`**：两个工厂的返回值都适用；`own` 只是可选糖
   （ct 0.67.0 在方法返回值被局部捕获时会丢掉 `own` 清理）。
4. **不按值拷贝句柄、不把句柄传走后继续用、不用 `goto` 跳出作用域**：`use Drop` 不阻止这些，
   只会静默重复释放或泄漏。

## 3. 四个类型的句柄与关闭

| 类型 | 句柄字段 | `close()` | `Drop` | 特殊点 |
| --- | --- | --- | --- | --- |
| `File` | `void *fp`（`FILE *`） | `fclose`，置 0 | `close()` | 方法用 `->`；NULL 句柄安全 |
| `FileDesc` | `int fd` | `close(2)`，置 `-1` | `close()` | `raw_file_desc()` 是裸 fd 出口 |
| `Dir` | `void *dp` + `char *stored_path` | `closedir` + `free(path)`，两者置 0 | `close()` | `path()` / `enter()` 不依赖调用方字符串 |
| `Timer` | `void *native` | `timer_delete` / `setitimer` 清除，置 0 | `close()` | 到期发信号，无回调 |

## 4. `Socket` 的别名边界

`Socket` 不是第五个资源对象：

```ct
import std::io::FileDesc;
namespace std::net;
pub type Socket = FileDesc;
```

- 同一个 C tag、同一套方法、同一个 `Drop`；双向赋值不需要转换。
- `own Socket *s` 不可用（`own` 按字面结构体名匹配，别名不满足）；用
  `own FileDesc *s`，或 `defer { delete s; }`。
- `Socket` 不新增任何字段；需要 fd 时用 `raw_file_desc()`。

## 5. 每个资源对象的验收夹具

| 检查 | 夹具 |
| --- | --- |
| 工厂失败返回 `0`；NULL 句柄方法安全 | `tests/io/main.ct`、`tests/io/desc.ct`、`tests/fs/directory.ct`、`tests/time/timers.ct` |
| `close()` 幂等、关闭后方法失败 | 同上 |
| `defer { delete }` 在落出 / `return` 前执行、1000 次不泄漏 fd | `tests/io/loop.ct`、`tests/fs/directory.ct` |
| `own` 在纯语句调用下仍触发 `Drop` | `tests/io/own.ct` |
| 别名双向赋值 | `tests/net/alias.ct` |
