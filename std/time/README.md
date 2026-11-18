# `std::time` — 时钟、日历与定时器

`std::time::Time` 是 namespace 宿主（`std::time::Time.now_seconds()` / `.year(t)` / `.format(t, ...)`），
`Timer` 是 ③ 资源对象：`(100).make_timer(sig)` / `make_timer_once(sig)`，到期发信号。

## 1. 定位与边界

- **时间不挂 `long`**（Q8）：时间戳与文件大小都是 `long`，`size.year()` 会返回 2023。所有日历
  函数走 namespace：`std::time::Time.year(t)`，`struct tm` 只在 shim 内。
- **本地时区**：`year` / `month` / `day` / `hour` / `minute` / `second` / `day_of_week` /
  `format` 都走 `localtime_r`，读进程当前时区；不是 UTC。`TZ` 在进程启动时确定。
- **`from_parts` 是工厂**：返回 `long` 时间值，`mktime` 会规范化越界字段（13 月 → 下一年）。
- **`Timer` 只管理生命周期**：到期通过 `signal` 通知，本模块不含回调；处理器用
  `std::process::Process.signal(...)` 安装。
- **平台差异**：有 POSIX 定时器的平台用 `timer_create`，到期发传入的信号；没有的平台
  （macOS）回退到 `setitimer(ITIMER_REAL)`，只能发 `SIGALRM`，且同一进程同时只有一个回退
  定时器准确。README 与测试都按 `SIGALRM` 写。
- **释放协议**：`Timer *t = ...; defer { delete t; }`。`own` 是可选糖，ct 0.65.1 在方法返回值
  被局部捕获时会丢掉释放（见 [`decision-resource-ownership.md`](../../docs/design/decision-resource-ownership.md)）。

## 2. 依赖与安装

依赖 `<time.h>` / `<signal.h>` / `<sys/time.h>` / `<unistd.h>`；`requires ct >= 0.64.0`。

| 文件 | 内容 |
| --- | --- |
| `Time.m.ct` | `Time` trait + `map trait Time for namespace std::time`（时钟与日历函数） |
| `Timer.h.ct` | `pub struct Timer { void *native; } use Drop`、自由工厂 `make_timer` / `make_timer_once`、`impl Timer` |
| `TimerFactory.m.ct` | `int` 宿主上的 `(ms).make_timer(sig)` / `(ms).make_timer_once(sig)` |
| `Time.shim.h` / `Timer.shim.h` / `TimerFactory.shim.h` | 各自的 `static inline` C 实现 |

引入：`import std::time::Time;`（时钟/日历）与 `import std::time::Timer;`（定时器）。

## 3. 快速开始

```ct
import std::time::Time;
import std::process::Process;

int main(void)
{
    char buf[32];
    long t = std::time::Time.from_parts(2024, 3, 15, 12, 0, 0);

    std::time::Time.year(t);                            /* 2024 */
    std::time::Time.format(t, "%Y-%m-%d", buf, 32);     /* "2024-03-15" */
    std::time::Time.sleep_milliseconds(10);
    return 0;
}
```

定时器：

```ct
import std::time::Timer;
import std::process::Process;

int g_hits = 0;

void on_timer(int signum) { g_hits = g_hits + 1; }

int main(void)
{
    int sig = std::process::Process.sigalrm();
    Timer *t;

    std::process::Process.signal(sig, on_timer);
    t = (20).make_timer_once(sig);
    defer { delete t; }
    t->start();
    std::time::Time.sleep_milliseconds(100);
    return g_hits == 1 ? 0 : 1;
}
```

## 4. API 全表

### 4.1 namespace `std::time::Time`

| 方法 | 签名 | 语义 | 失败 |
| --- | --- | --- | --- |
| `now_seconds` | `long now_seconds()` | `time(NULL)` | `-1` |
| `now_nanoseconds` | `long now_nanoseconds()` | `clock_gettime(CLOCK_REALTIME)` 纳秒 | `-1` |
| `diff_seconds` | `double diff_seconds(long a, long b)` | `difftime(a, b)` | — |
| `sleep_milliseconds` | `int sleep_milliseconds(int ms)` | `nanosleep`，`EINTR` 重试 | `-1` |
| `get_cpu_ticks` | `long get_cpu_ticks()` | `clock()` | — |
| `year` / `month` / `day` / `hour` / `minute` / `second` | `int x(long when)` | `localtime_r` 字段 | `-1` |
| `day_of_week` | `int day_of_week(long when)` | 0 = 周日 | `-1` |
| `format` | `int format(long when, const char *pattern, char *out, int cap)` | `strftime`，写调用方缓冲 | `-1`（放不下） |
| `from_parts` | `long from_parts(y, m, d, h, mi, s)` | `mktime`，本地时区 | `-1` |

`format` 的空结果返回 0；`cap` 含 NUL。字段函数对非法时间戳返回 `-1`。

### 4.2 `Timer`（自由工厂 + `impl Timer`）

| 入口 | 签名 | 语义 | 失败 |
| --- | --- | --- | --- |
| `make_timer` | `Timer *make_timer(int ms, int sig)` | 周期定时器 | `0` |
| `make_timer_once` | `Timer *make_timer_once(int ms, int sig)` | 一次性定时器 | `0` |
| `(ms).make_timer` | `Timer *ms.make_timer(int sig)` | 数字工厂，同一类型 | `0` |
| `(ms).make_timer_once` | `Timer *ms.make_timer_once(int sig)` | 同上 | `0` |
| `start` | `int t->start()` | 武装；周期自动重装 | `-1` |
| `stop` | `int t->stop()` | 解除；幂等 | `-1` |
| `remaining` | `int t->remaining()` | 距下次到期毫秒；停止为 0 | `-1` |

`ms <= 0` 或 `sig <= 0` 的工厂返回 `0`。

## 5. 与相邻模块的关系

- `std::process::Process.signal` / `sigalrm` / `sigrtmin` 是定时器的通知通道；没有回调层。
- `std::fs::Stat` 的时间字段是秒级 `long`，可直接喂给 `Time.year` / `Time.format`。
- `std::locale` 的 `LC_TIME` 影响 `format` 的月份/星期文本；数字格式不受影响。

## 6. 限制与未支持

- **不做时区换算**：只有本地时区（`localtime_r` / `mktime`），没有 UTC ↔ 本地、也没有时区数据库。
- **没有 `TimeParts` / `Timestamp` 值类型**：`Time` namespace + `long` 时间值；`tm` 只在 shim 内。
- **`Timer` 没有回调**：到期是信号；信号处理器受 async-signal-safe 限制。
- **macOS 回退**：只用 `SIGALRM`，且同一进程一个回退定时器有效；`sigrtmin()` 在 macOS 返回
  `SIGUSR2`，不要拿它配回退定时器。
- **`now_nanoseconds` 是 `long`**：2262 年后溢出；需要更长时间跨度时用秒。
- **`sleep_milliseconds` 不保证实时**：调度延迟、信号处理后仍会补足请求的睡眠。

## 7. 版本与变更记录

| 版本 | 状态 | 变更 |
| --- | --- | --- |
| `0.9.0` | 本轮新增 | 新建模块：`Time` namespace（时钟、日历、格式化、`from_parts`）与 `Timer`（周期/一次性、`own` 自由工厂 + 数字工厂、`defer delete`）。 |

编译器下限 `ct >= 0.64.0`。
