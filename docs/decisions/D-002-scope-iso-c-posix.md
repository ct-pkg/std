applies-to: v0.12.0

# D-002：std 的范围 = ISO C17 + POSIX.1-2017

| 项 | 值 |
| --- | --- |
| id | D-002 |
| date | 2026-11-19 |
| anchor | `8689082` |
| status | accepted |
| trigger | 用户要求“POSIX 与 ISO C 都纳入 std 的范围，请增加相关的部分，制定 roadmap 和 phase” |
| judge | USER |
| seats | REQ / ARCH |
| scope | `std` 的全部模块与后续 phase |
| target | `v0-PH-9` |
| target-2 | `v0-PH-10` |
| target-3 | `v0-PH-11` |
| target-4 | `v0-PH-12` |
| target-5 | `v0-PH-13` |
| criteria | ISO C17 头文件与 POSIX.1-2017 头文件逐项有归属；变参函数、Windows 原生 API 与 C++ 明确排除；新 phase 按头文件分组 |
| verdict | accepted：范围定为 ISO C17 + POSIX.1-2017；PH-9 做 ISO C 缺口，PH-10 做 POSIX 核心，PH-11 做 POSIX 服务；C 结构体用不透明句柄或 `void *`，不建 CT 值类型 |
| rationale | 用户明确要求两者都纳入。ISO C17 是语言标准库；POSIX.1-2017 是宿主 libc 的系统接口，C 程序员日常使用的线程、mmap、poll、aio 都在其中。把两者合并成一条交付面，避免“哪些算标准库”反复争议 |
| invalidation | 用户重新收窄范围；或某个 POSIX 头文件在目标平台不可用且无法用条件编译隔离 |
| affected | `docs/planning/roadmap.md`、`docs/requirements/scope.md`、`docs/phases/draft-iso-c/`、`docs/phases/draft-posix-core/`、`docs/phases/draft-posix-services/` |

## 1. 纳入

- ISO C17：`assert` `complex` `ctype` `errno` `fenv` `float` `inttypes` `iso646` `limits` `locale`
  `math` `setjmp` `signal` `stdalign` `stdarg` `stdatomic` `stdbool` `stddef` `stdint` `stdio`
  `stdlib` `stdnoreturn` `string` `tgmath` `threads` `time` `uchar` `wchar` `wctype`。
- POSIX.1-2017：`aio` `dirent` `dlfcn` `fcntl` `fnmatch` `glob` `grp` `iconv` `langinfo`
  `mqueue` `netdb` `poll` `pthread` `pwd` `regex` `sched` `semaphore` `spawn` `sys/mman`
  `sys/resource` `sys/select` `sys/socket` `sys/statvfs` `sys/time` `sys/utsname` `termios`
  `unistd` 等。

## 2. 排除

- 变参函数：`scanf` / `printf` / `sprintf` / `fprintf` / `v*` 族（CT 无变参声明）。
- Windows 原生 API、C++ 标准库。
- Unicode 规范化 / 字素簇（归语言侧或上层包）。
- 语言级机制（`setjmp` 的跳转语义、`stdarg` 的变参、`tgmath` 的泛型宏）只做“直调说明”，不伪装成 CT 类型。

## 3. 呈现原则

- 能寄生基础类型的继续寄生（`char *` / `int` / `double` / namespace）。
- 只有真正持有资源、且宿主类型无法表达时才建对象；POSIX 结构体优先用**不透明句柄 + 显式
  `*_free`/`destroy`**，避免 CT 值拷贝。
- C 结构体布局不跨边界：需要构造 `struct aiocb` / `pollfd` / `pthread_mutex_t` 时由 shim 分配，
  CT 侧只拿 `void *` 并调用访问器。
