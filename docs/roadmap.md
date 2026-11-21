> **历史路线图**：P0–P7 的交付记录；新路线图见 [docs/planning/roadmap.md](planning/roadmap.md)。

# `std` Roadmap

| 项 | 值 |
| --- | --- |
| 性质 | 路线图；分期、版本位、里程碑均已定 |
| 依据 | [设计正文 §7 施工顺序](design/std-scope-and-presentation.md)、[评审与阶段文档](design/std-open-items-review-and-staging.md) |
| 当前版本 | `0.11.0`（已打 `v0.11.0`）；`P0`–`P7` 全部完成 |

## 1. 分期总表

| 期 | 主题 | 版本位 | 依赖 | 是否可并行 |
| --- | --- | --- | --- | --- |
| P0 | 改名与删除（破坏性） | `0.4.0` | 无 | 起点 |
| P1 | `str` 内存方法、`num` ctype、`math` 新族 | `0.5.0` | P0 | 与 P3 可并行 |
| P2 | `env` / `process` / `errno` / `locale` / `wchar` | `0.6.0` | P1 | 与 P3 可并行 |
| P3 | stdio：`File` / `FileDesc` | `0.7.0` | P0 | 与 P1、P2 可并行 |
| P4 | `fs` 补全与 `Dir` | `0.8.0` | P2 | 与 P5 可并行 |
| P5 | `time` 与 `Timer` | `0.9.0` | P2 | 与 P4 可并行 |
| P6 | 资源对象 `Drop` 模板收口 | `0.10.0` | P3、P4、P5 | 串行汇合点 |
| P7 | `net`（TCP + UDP） | `0.11.0` | P6 | 串行末段 |

版本规则：每期一个 MINOR 位，PATCH 留给期内的修复；`0.4.0` 起按上表递增。**已定。**

`P0`–`P7` 已全部落地；`0.6.0`–`0.10.0` 只作为开发里程碑，**未发布、未打 tag**，唯一的发布点是
`v0.11.0`。设计正文 §2.9 的 `pthread_*` / `aio_*` / `mmap` / `poll` / `select` 等表外 ① 直连项
按 2026-11-19 裁定不包装，消费者直接调 libc；不在本表内，也不影响 P0–P7 验收。

## 2. 各期交付与验收

通用验收：构建通过、`tests/run.sh` 通过、`ct pkg check` 通过；平台证据见 [platform-validation.md](design/platform-validation.md)。下表只列该期特有检查。

### P0 — 改名与删除（`0.4.0`）

- 交付：`String` → `CStrOps`（`0.20.0` 起为 `CharStr`）；`len` → `length`；`open` → `open_file`；删除 `to_string`（只留 `to_str`）；`to_upper`/`to_lower` → `uppercase`/`lowercase`；删除 `printf`。
- 验收：旧名零命中；`printf` 相关样例改为 `to_str`/`concat` + `write` 并跑通。

### P1 — `str` / `num` / `math`（`0.5.0`）

- 交付：`char` 的 7 个 `is_*`；`char *` 的 `copy_from`/`move_from`/`fill`/`compare_with`/`find_byte`/`compare_prefix` 与 `split_into`；`math` 新族（7 个生僻名改全称，其余保留原名）；`sort`/`binary_search`、`scale_by_power_of_two`/`split_exponent`/`split_fraction`。
- 验收：每条 M0/M1 转发；无宿主重名；重叠/不重叠拷贝、三态比较、指针返回各有夹具；`split_into` 缓冲不足返回所需数量且不写入。

### P2 — `env` / `process` / `errno` / `locale` / `wchar`（`0.6.0`）

- 交付：`get_env`/`Env.set`/`unset`；`Process.get_pid` 等与 `fork`/`execute*`/`system`/`pipe_open`/`wait*`、`signal`/`alarm`；`Errno.get_last`/`error_message`；`"zh_CN.UTF-8".make_locale()` → `own Locale` + `apply`/`conventions`；宽字符模块。
- 验收：借用指针语义写明；`fork` 后状态注意事项；`Locale.apply` 全局状态与 `LC_NUMERIC` 浮点往返说明；宽字符转换夹具。

### P3 — stdio（`0.7.0`）

- 交付：`File`（`FILE *` 宿主，③）与 `FileDesc`（`open_as_file_desc`，③）；`seek`/`tell`/`rewind_start`/`at_end_of_file`/`has_error`/`clear_error`/`read_char`/`write_char`/`unread_char`/`read_line`/`write_line`/`flush`；变参替代（`to_str`/`concat`、`parse_*`、格式化 + `write`）。
- 验收：`own` 关闭与幂等 `close` 夹具；`stdout`/`stderr`/`stdin` 裸宿主不归包管；EOF/错误位覆盖。

### P4 — `fs` 与 `Dir`（`0.8.0`）

- 交付：`access`/`stat(&st)`/`read_link`/`resize`/`set_mode`/`resolve_path`/`is_readable`/`is_writable`/`size`/`mode`/`modification_time`；`Dir.current()`/`dir.path`/`dir.enter`/`open_dir`；`scan_directory`。
- 验收：`struct stat` 原样暴露；三个便捷方法不替代 `stat`；`is_readable`/`is_writable` 的真实身份限制；大目录流式列举。

### P5 — `time` 与 `Timer`（`0.9.0`）

- 交付：`now_seconds`/`now_nanoseconds`/`diff_seconds`/`sleep_milliseconds`；`Time.year`/`month`/`day`/`hour`/`minute`/`second`/`format`/`from_parts`；`Timer`（`make_timer`/`make_timer_once` + `start`/`stop`/`remaining`）。
- 验收：不挂 `long`；不做 `TimeParts` 值类型；`tm` 只在 shim 内；时区行为写明；`Timer` 通知走 `signal`、不含回调。

### P6 — 资源对象收口（`0.10.0`）

- 交付：`File` / `Dir` / `FileDesc` / `Timer` 的 `Drop` 模板统一；`Socket` 为 `FileDesc` 别名（在包头文件声明）。
- 验收：`own` 样例、`close` 幂等、别名边界；四类资源释放路径一致。

### P7 — `net`（`0.11.0`）

- 交付：`Socket`（TCP + UDP）+ `"host:80".connect()` + `bind_to`/`listen_backlog`/`accept`/`send`/`receive`/`shutdown_both`/`close`；`resolve_host`；`set_option`；`parse_ip`；`receive_from`；`send_message`/`receive_message`。
- 验收：TCP/UDP loopback 回环；`Drop` 关闭；`receive` 字节数出参；超时与 `set_option` 行为；`iovec` 构造/所有权写明。

## 3. 外部依赖

`Socket` 别名已在 **0.10.0** 落地：`std/net/Socket.h.ct` 声明 `pub type Socket = FileDesc;`，
与 `FileDesc` 同一 C tag、同一套方法、双向赋值。普通 `.ct` 的文件作用域别名仍属语言侧能力，但
不影响 std 交付，也不阻塞任何期次。**外部依赖已清零。**

| 依赖 | 影响期 | 状态 | 处理 |
| --- | --- | --- | --- |
| 语言侧 `Str` 内置化 | `Str` 删除作业 | **不适用（Q10）** | `Str` 归语言侧，本仓不定义、不提供；删除作业单已转为历史文档 |

`Socket` 与 `FileDesc` 的别名声明已在包头文件完成；使用者若要在自己的普通 `.ct` 里写文件作用域
别名，仍取决于语言侧，但不影响 std 交付。

## 4. `Str` 删除的挂载

- 作业单：[`Str` 删除作业单](design/str-removal-work-order.md)。
- 挂载期次：**P2 之后、与 P4/P5 并行**的文档清理项；不单独占版本位。**已定。**
- 触发条件：语言侧 `Str` 内置化落地并可在 std 正常使用；之前不许删。
- 执行方式：按作业单四批（代码注释、周边文档、设计文档、历史记录）各自独立提交，按批回退。

## 5. 关键路径

```
P0 ── P1 ── P2 ──┬── P4 ──┐
 │                └── P5 ──┼── P6 ── P7
 └── P3 ───────────────────┘
```

- **串行主干**：P0 → P1 → P2 → P4/P5 → P6 → P7。
- **可并行**：P3 在 P0 之后即可与 P1/P2 同时进行；P4 与 P5 在 P2 之后可并行。
- **汇合点**：P6 必须等 P3、P4、P5 全部完成。
- **整体可用里程碑**：**P6（已定）**。此时字符串、数值、IO、文件系统、时间与资源释放均可按 CT 方式直接书写，C 程序员可照着写日常程序；P7 在其上补网络，达成全库覆盖（已定）。
