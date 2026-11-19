applies-to: v0.12.0

# `std` Roadmap（规范版）

| 项 | 值 |
| --- | --- |
| 性质 | 路线图；由主编维护，phase 卡片从本表立项 |
| 当前版本 | `0.12.0`（`v0.12.0` 已发布） |
| 当前 phase | 无（`v0-PH-8` 已收口） |
| 版本偏移 | 历史 `P0`–`P7` 与 `v0-PH-0…7` 错位，`MINOR = n + 4`；见 `AGENTS.md` 的 deviation |
| 规范版本 | 1.17.0 |

## 1. 范围

`std` 的交付面是 **ISO C17 标准库 + POSIX.1-2017**（变参函数除外），按 CT 的调用形态呈现：

- **ISO C17 头文件**：`assert` / `complex` / `ctype` / `errno` / `fenv` / `float` / `inttypes` /
  `iso646` / `limits` / `locale` / `math` / `setjmp` / `signal` / `stdalign` / `stdarg` / `stdatomic` /
  `stdbool` / `stddef` / `stdint` / `stdio` / `stdlib` / `stdnoreturn` / `string` / `tgmath` /
  `threads` / `time` / `uchar` / `wchar` / `wctype`。
- **POSIX.1-2017 头文件**：`aio` / `dirent` / `dlfcn` / `fcntl` / `fnmatch` / `glob` / `grp` /
  `iconv` / `langinfo` / `mqueue` / `netdb` / `poll` / `pthread` / `pwd` / `regex` / `sched` /
  `semaphore` / `spawn` / `sys/mman` / `sys/resource` / `sys/select` / `sys/socket` /
  `sys/statvfs` / `sys/time` / `sys/utsname` / `termios` / `unistd` 等。
- **不在范围**：变参函数（`scanf` / `printf` / `sprintf` 族）、Windows 原生 API、C++ 标准库、
  Unicode 规范化 / 字素簇（归语言侧或上层包）。

> 该范围由 `docs/decisions/D-002-scope-iso-c-posix.md` 裁定；此前的 §2.9 “POSIX 长尾不包装”
> 由本表取代。

## 2. 已完成

| 期 | 版本 | 内容 | 状态 |
| --- | --- | --- | --- |
| `P0` | `0.4.0` | 改名与删除 | 已发布（`v0.4.0`） |
| `P1` | `0.5.0` | `str` 内存方法、`num` ctype、`math` 新族 | 已发布（`v0.5.0`） |
| `P2` | `0.6.0` | `env` / `process` / `errno` / `locale` / `wchar` | 开发里程碑，未打 tag |
| `P3` | `0.7.0` | stdio `File` / `FileDesc` | 开发里程碑，未打 tag |
| `P4` | `0.8.0` | `fs` 补全与 `Dir` | 开发里程碑，未打 tag |
| `P5` | `0.9.0` | `time` 与 `Timer` | 开发里程碑，未打 tag |
| `P6` | `0.10.0` | 资源对象 `Drop` 收口、`Socket` 别名 | 开发里程碑，未打 tag |
| `P7` | `0.11.0` | `net` TCP + UDP | 已发布（`v0.11.0`） |

历史阶段保留 `P0`–`P7` 标签（存量豁免，不迁移为 `v0-PH-*`）。

## 3. 新 phase

| phase | 主题 | 版本 | 状态 | 卡片 |
| --- | --- | --- | --- | --- |
| `v0-PH-8` | 工程规范纳管（G0–G9） | `0.12.0` | 已收口 | [card](../phases/v0-PH-8/card.md) |
| `v0-PH-9` | ISO C17 补全 | `0.13.0` | 待开工 | [card](../phases/v0-PH-9/card.md) |
| `v0-PH-10` | POSIX 核心：线程 / 同步 / 内存映射 / 多路复用 | `0.14.0` | 草案 | [draft](../phases/draft-posix-core/card.md) |
| `v0-PH-11` | POSIX 服务：aio / mq / sem / shm / sched / 资源与信息 | `0.15.0` | 草案 | [draft](../phases/draft-posix-services/card.md) |

草案卡不排期、不派工、不开工；定稿时按 PN-13 一次完成改名、卡头与编号账本。
`v0-PH-9` 之后的编号在正式定稿时写入账本；`v0-PH-8` 是下一个可执行 phase。

## 4. 各期目标

### v0-PH-8 工程规范纳管

把本仓纳入 1.17.0 规范：G0 落地、G0–G9 可执行、phase 卡片/验收/决策/台账可用；存量文档与
历史提交按 `AGENTS.md` 的 deviation 豁免。**不改 std 的 API。**

### v0-PH-9 ISO C17 补全

先产出“ISO C17 头文件 × 现状”的逐项缺口表，再按缺口实现：

- 缺失头文件族：`fenv` / `complex` / `uchar` / `wctype` / `stdatomic` / `threads`（C11）/ `inttypes`+`stdint`。
- `string.h` 缺口：`strspn` / `strcspn` / `strpbrk` / `strcoll` / `strxfrm` / `strtok_r`（POSIX）。
- `stdio.h` 缺口：`tmpfile` / `tmpnam` / `setvbuf` / `freopen` / `perror` / `putc` / `getc` / `fgets` / `fputs`。
- `stdlib.h` 缺口：`div` / `ldiv` / `lldiv` / `aligned_alloc` / `at_quick_exit` / `quick_exit` / `rand` 族。
- `time.h` 缺口：UTC 侧（`gmtime` / `timegm`）、`timespec_get`、`clock_getres`（POSIX）。
- `math.h` / `float.h` / `limits.h`：宏与边界值的登记，不新增运行时类型。

### v0-PH-10 POSIX 核心

`pthread`（线程、mutex、cond、rwlock、once、key）、`semaphore`、`sys/mman`（`mmap` / `munmap` /
`mprotect` / `msync` / `madvise`）、`poll` / `sys/select`。C 结构体（`pthread_mutex_t`、
`pthread_cond_t`、`pollfd`、`fd_set`）用**不透明句柄 + 显式 `*_free`/`destroy`**，不建 CT 值类型；
回调（线程启动）走 `void *`。

### v0-PH-11 POSIX 服务

`aio`、`mqueue`、`shm`、`sched`、`sys/resource`、`sys/utsname`、`unistd` 的 `sysconf` / `getrusage` /
`times`、`glob` / `fnmatch`、`regex`（可选）、`dlfcn`、`spawn` / `posix_spawn`、`pwd` / `grp`、
`termios`、`statvfs`。`aiocb` 同样用不透明句柄 + 访问器。

## 5. 版本规则

- 新 phase 的 `version-minor` 写 `n`（PH-8 写 8）；收口版本按 `MINOR = n + 4` 计算（偏移 deviation）。
- 每个 phase 收口一个 MINOR；PATCH 只记 fix；纯文档不 bump。
- phase 收口打 annotated tag `v<版本>` 并推送；分支推送仍须用户指令。

## 6. 外部依赖

无。`POSIX` 与 `ISO C` 都由宿主 libc 提供；平台差异（macOS 无 `timer_t`、无 `SO_NOSIGPIPE` 等）
按 phase 卡片的“已知风险”处理，必要时不实现并写明原因（`deferred` 需写触发条件）。
