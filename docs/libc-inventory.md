# libc 覆盖盘点与 std OO 化分档

> **历史文档**：本文是 0.3.x 的 libc 盘点，结论已被 [roadmap](roadmap.md) 与 [资源裁定](design/decision-resource-ownership.md) 取代；保留作证据，不再更新。

| 项 | 值 |
| --- | --- |
| 性质 | 只读盘点：不写实现、不动版本、不开相卡 |
| 依据 | 负责人结论：按「资源 / 纯函数 / 热路径」三档决定是否包装 |
| 用途 | 后续排期底稿；每条都落到具体函数名 |
| 现状基线 | `0.3.1`（`ct.pkg`），`std` 八个模块 |
| 日期 | 2026-11-15 |

## 1. 分档口径与命名规则

三档的定义（按负责人给的结论展开）：

| 档 | 判据 | 包装形态 | 价值 |
| --- | --- | --- | --- |
| 第一档 | 有资源与生命周期（`FILE *`、目录遍历、socket、定时器、locale 之类） | trait + 句柄/值类型，生命周期交给 `Drop` | 泄漏与忘记 `close` 由类型兜住 |
| 第二档 | 纯函数族（math、字符分类、进制转换、时间格式化） | namespace 静态方法，或类型宿主方法 | 命名统一、调用点不必记 libc 拼写 |
| 第三档 | 热路径与纯内存操作（`memcpy` / `memmove` / `memset` / `strcmp` 等） | **不包装**，保留 libc 原名直映射 | 零成本；包装只会多一层 |

命名规则（README §11 / `README.md:505-506`）：

- 新名字一律 `snake_case`、动词开头；`get_` / `set_` / `has_` / `is_` / `to_` / `from_` 前缀合规。
- 新名字**不能**占用同一宿主上已有的方法名（宿主方法名全局唯一，见 §3）。
- 第三档不新增任何名字：调用点直接用 libc 原名。
- 数学 / 字符这类「直映射」沿用既有 `Math.sqrt` / `Math.fabs` 的先例，保留 libc 原拼写
  （`README.md:505-506` 的保留名豁免）。**若负责人要求连直映射也动词化**，需要整族改名并先改
  §11 豁免条款——本清单不预设这个决定，表中按现状拼写给出。
- namespace 宿主的方法写作 `namespace::Trait.method(...)`（如 `std::math::Math.sqrt(x)`，
  `std/math/Math.m.ct:4`）；变参或一次性入口可以写成普通函数（如 `std::io::printf`，
  `std/io/Io.h.ct:25`）。表中两种形态都注明了。

## 2. 逐头文件盘点

### 2.1 `stdio.h`

| 函数 | 现状 | 结论 | 落地细节 |
| --- | --- | --- | --- |
| `printf` | `std::io::printf`（`Io.h.ct:25`，namespace 宿主） | 已在 std | — |
| `puts` / `putchar` / `getchar` / `fflush` | `Stdout.puts` / `putchar` / `getchar` / `fflush`（`Stdout.m.ct:5-12`） | 已在 std | — |
| `fopen` / `fclose` / `fread` / `fwrite` | `Path.open` + `File.read` / `write` / `close`（`File.m.ct:11-29`） | 已在 std | 细节见 [issue 0001 配套梳理](../issues/0001-path-file-traits.md) |
| `fseek` / `ftell` / `rewind` | 无 | 建议包·第一档 | `seek(offset, origin)` / `tell()` / `rewind_start()`，宿主 `IoFile *`；失败 `-1` |
| `feof` / `ferror` / `clearerr` | 无 | 建议包·第一档 | `at_eof()` / `has_error()` / `clear_error()`，宿主 `IoFile *`；前两个 `0/1`，`clear_error` 无返回 |
| `fgetc` / `getc` / `ungetc` | 无 | 建议包·第一档 | `read_char()` / `unread_char(c)`，宿主 `IoFile *`；`-1` 失败或 EOF（二者不可分） |
| `fputc` / `putc` | 无 | 建议包·第一档 | `write_char(c)`，宿主 `IoFile *`；`-1` 失败 |
| `fgets` / `fputs` | 无 | 建议包·第一档（可选） | `read_line(out, cap)` / `write_line(s)`，宿主 `IoFile *`；`-1` 失败，`0` EOF |
| `scanf` / `fscanf` / `sscanf` | 无 | **不建议包** | 变参 + 无长度约束，解析失控；数值解析走 `std::num`（§2.2） |
| `sprintf` | 无 | **不建议包** | 无界写；用 `num.to_str` / `to_base` 拼接 |
| `snprintf` / `vsnprintf` | 无（`num.to_str` 内部用到 `snprintf`） | 不建议包 | 变参无法经 map recipe 透传；格式化限制在 `std::num` 的定点/进制接口里 |
| `fprintf` / `vprintf` 族 | 无 | 不建议包 | 任意格式串 + 变参，`printf` 已是唯一例外 |
| `fileno` | 无 | 不建议包 | 暴露 fd，破坏 `IoFile` 抽象 |
| `fdopen` / `freopen` | 无 | 不建议包 | 句柄所有权转移在 CT 侧无法表达 |
| `setvbuf` / `setbuf` | 无 | 不建议包 | 缓冲区所有权 + 低频 |
| `perror` | 无 | 不建议包 | 直接写 stderr；错误码由 `FsEnv.get_last_error` 给 |
| `remove` / `rename` | `Fs.remove` / `Fs.rename`（`Fs.m.ct:37-38`） | 已在 std | — |
| `tmpfile` / `tmpnam` / `tempnam` | 无 | 不建议包 | 临时文件安全性 / 路径语义 |
| `popen` / `pclose` | 无 | 不建议包 | 进程控制 + 安全 |

### 2.2 `stdlib.h`

| 函数 | 现状 | 结论 | 落地细节 |
| --- | --- | --- | --- |
| `malloc` / `calloc` / `realloc` / `free` | 无（仅 shim 内部使用） | **不建议包**（第三档） | 裸内存直用；池化归 `std::arena` |
| `atoi` / `atof` | `String.to_int` / `to_double`（`CharStr.m.ct:62-63`） | 已在 std | 无错误报告，溢出是 libc 未定义行为（`CharStr.shim.h:208-219`） |
| `atol` / `atoll` | 无 | 建议包·第二档 | `parse_long(s, out)` / `parse_llong(s, out)`，宿主 `char *`；`0` 成功 / `-1` 失败，值写 `out` |
| `strtol` / `strtoll` / `strtoul` / `strtoull` | 无 | 建议包·第二档 | `parse_long` / `parse_llong` / `parse_ulong`（同上），范围错误映射为 `-1` |
| `strtod` / `strtof` / `strtold` | 无 | 建议包·第二档 | `parse_real(s, out)`，`double` 出参；`-1` 失败 |
| `abs` / `labs` / `llabs` | `Int.abs` / `Long.abs` / `LLong.abs`（`README.md:164-166`） | 已在 std | — |
| `div` / `ldiv` / `lldiv` | 无 | 不建议包 | 返回 struct，调用点极少；拆成两个方法更贵 |
| `rand` / `srand` / `random` / `srandom` | 无 | 不建议包 | 全局状态 + 弱 PRNG；可复现随机应自建类型（另开 issue） |
| `qsort` / `bsearch` | 无 | 不建议包 | 需要比较器回调；CT 方法层没有函数指针通道 |
| `getenv` | 无 | 建议包·第二档 | namespace `std::env`（trait `Env`）：`std::env::Env.get(name, out, cap)`；`-1` 未找到或放不下 |
| `setenv` / `unsetenv` | 无 | 建议包·第二档 | `std::env::Env.set(name, value)` / `.unset(name)`；`-1` 失败；`putenv` 不包（引用语义） |
| `realpath` | 无 | 建议包·第二档 | `resolve_path(out, cap)`，宿主 `char *`；`-1` 失败 |
| `system` / `exit` / `abort` / `atexit` / `quick_exit` | 无 | 不建议包 | 进程控制流 + 安全 |
| `mblen` / `mbtowc` / `wctomb` / `mbstowcs` / `wcstombs` | 无 | 不建议包 | 编码归 `std::utf8`，且依赖 locale |
| `aligned_alloc` / `posix_memalign` | 无 | 不建议包 | 裸内存（第三档） |

### 2.3 `string.h`

| 函数 | 现状 | 结论 | 落地细节 |
| --- | --- | --- | --- |
| `memcpy` / `memmove` / `memset` / `memcmp` / `memchr` | `String.copy_to` 只覆盖固定 `n` 的 memcpy 形态（`CharStr.m.ct:61`） | **第三档** | 直映射，不新增名字 |
| `strlen` | `String.len` | 已在 std | — |
| `strcmp` | `String.compare` | 已在 std | — |
| `strncmp` | 无 | 第三档 | 直映射（热路径、语义同 `strcmp`） |
| `strcpy` / `strncpy` | `String.copy`（带 `cap`，安全形态） | 已在 std（安全形态） | 原形不建议包（无界） |
| `strcat` / `strncat` | `String.concat`（带 `cap`） | 已在 std（安全形态） | `strncat` 原形不建议包 |
| `strchr` | `String.find_char`（返回索引） | 已在 std | — |
| `strrchr` | 无 | 建议包·第二档 | `rfind_char(c)`，宿主 `char *`；`-1` 未命中 |
| `strstr` | `String.find` | 已在 std | — |
| `strrstr` | 无 | 不建议包 | 非标准（GNU），需求低 |
| `strspn` / `strcspn` / `strpbrk` | 无 | 不建议包 | 低频，语义易与 `find` 混淆 |
| `strtok` / `strtok_r` | 无 | 不建议包 | 有状态分词；应由自建游标类型承担 |
| `strdup` / `strndup` | 无 | 不建议包 | 拥有字符串归语言侧内置 `Str`；`char *` 层不引入分配 |
| `strcoll` / `strxfrm` | 无 | 不建议包 | locale 排序 |
| `strerror` | 无 | 建议包·第二档 | `std::errno::Errno.message(code, out, cap)`；`-1` 放不下（§2.7） |
| `strcasecmp` / `strncasecmp`（`strings.h`） | 无 | 建议包·第二档 | `compare_ignore_case(other)`，宿主 `char *`；返回 `<0` / `0` / `>0` |

### 2.4 `ctype.h`

| 函数 | 现状 | 结论 | 落地细节 |
| --- | --- | --- | --- |
| `isalpha` / `isdigit` / `isspace` / `isupper` / `islower` | `Char.is_alpha` / `is_digit` / `is_space` / `is_upper` / `is_lower`（`Num.m.ct:94-112`） | 已在 std | — |
| `toupper` / `tolower` | `Char.to_upper` / `to_lower` | 已在 std | — |
| `isalnum` | 无 | 建议包·第二档 | `is_alnum()`，宿主 `char`；`0/1` |
| `ispunct` | 无 | 建议包·第二档 | `is_punct()` |
| `isprint` | 无 | 建议包·第二档 | `is_print()` |
| `isgraph` | 无 | 建议包·第二档 | `is_graph()` |
| `iscntrl` | 无 | 建议包·第二档 | `is_cntrl()` |
| `isblank` | 无 | 建议包·第二档 | `is_blank()` |
| `isxdigit` | 无 | 建议包·第二档 | `is_xdigit()` |

### 2.5 `math.h`

| 函数 | 现状 | 结论 | 落地细节 |
| --- | --- | --- | --- |
| `sqrt` / `fabs` | `Math.sqrt` / `Math.fabs`（`Math.m.ct:18-23`）；`Float.sqrt` / `Real.sqrt`、`Float.abs` / `Real.abs` | 已在 std | — |
| `floor` / `ceil` / `round` | `Float` / `Real` 上的 `floor` / `ceil` / `round`（`README.md:169-170`） | 已在 std | — |
| `trunc` | 无 | 建议包·第二档 | `truncate()`，宿主 `Float` / `Real`；返回同型 |
| `pow` | 无 | 建议包·第二档 | namespace `std::math`（trait `Math`）：`std::math::Math.pow(x, y)`；失败无（DOMAIN 时按 libc 返回 NaN） |
| `exp` / `exp2` / `expm1` | 无 | 建议包·第二档 | namespace `std::math`，同名；同上 |
| `log` / `log2` / `log10` / `log1p` | 无 | 建议包·第二档 | 同上 |
| `sin` / `cos` / `tan` | 无 | 建议包·第二档 | 同上 |
| `asin` / `acos` / `atan` / `atan2` | 无 | 建议包·第二档 | 同上；`atan2` 两实参 |
| `sinh` / `cosh` / `tanh` | 无 | 建议包·第二档 | 同上 |
| `hypot` / `cbrt` | 无 | 建议包·第二档 | 同上 |
| `fmod` / `remainder` | 无 | 建议包·第二档 | 同上 |
| `fmin` / `fmax` | 无 | 建议包·第二档（优先） | 宿主 `Float` / `Real` 的 `min` / `max`，与 `Int.max/min` 同形（不同宿主，允许） |
| `fdim` / `fma` / `copysign` | 无 | 建议包·第二档 | namespace `std::math`，同名 |
| `isnan` / `isinf` | `Real.is_nan` / `is_inf`（`README.md:170`） | 已在 std（仅 `Real`） | 建议补 `Float.is_nan` / `Float.is_inf`（第二档） |
| `ldexp` / `frexp` / `modf` | 无 | 不建议包 | 需要出参/结构体，收益低 |
| `nearbyint` / `rint` / `lround` / `llround` | 无 | 不建议包 | 依赖舍入模式（fenv）或隐式类型收窄 |
| `erf` / `erfc` / `tgamma` / `lgamma` | 无 | 不建议包 | 低频、平台差异 |
| `signbit` / `fpclassify` | 无 | 不建议包 | 宏枚举；`signbit` 可随 `is_nan` 一起补（另议） |

### 2.6 `time.h`

| 函数 | 现状 | 结论 | 落地细节 |
| --- | --- | --- | --- |
| `time` | 无 | 建议包·第二档 | namespace `std::time`（trait `Time`）：`std::time::Time.now_sec()`；`-1` 失败 |
| `clock` | 无 | 建议包·第二档 | `get_cpu_ticks()`；`-1` 失败 |
| `difftime` | 无 | 建议包·第二档 | `diff_seconds(a, b)` → `double` |
| `clock_gettime` | 无 | 建议包·第二档 | `now_ns()`；`-1` 失败（POSIX；C11 的 `timespec_get` 同义并入） |
| `mktime` | 无 | 建议包·第二档 | `make_epoch(parts)`，需值类型 `TimeParts`；`-1` 失败 |
| `localtime` / `gmtime` | 无 | 建议包·第二档 | `to_local(epoch, parts)` / `to_utc(epoch, parts)`；`-1` 失败 |
| `strftime` | 无 | 建议包·第二档 | `format_time(parts, fmt, out, cap)`；返回写入字节数，`<0` 失败 |
| `asctime` / `ctime` | 无 | 不建议包 | 静态缓冲、非线程安全 |
| `strptime` | 无 | 不建议包 | POSIX 扩展，失败语义弱 |
| `sleep` / `usleep` / `nanosleep` | 无 | 建议包·第二档 | `std::time::Time.sleep_ms(ms)`；`-1` 被信号打断；无资源，故不入第一档 |
| `timer_create` / `timer_settime` / `timer_delete` | 无 | 建议包·第一档 | 新类型 `Timer` + `Drop`：`open_timer()` / `start(ms)` / `stop()` / `close()`；失败 `NULL` / `-1` |
| `gettimeofday`（`sys/time.h`） | 无 | 不建议包 | `clock_gettime` 已取代 |

### 2.7 `errno.h`

| 函数 / 名字 | 现状 | 结论 | 落地细节 |
| --- | --- | --- | --- |
| `errno` | `FsEnv.get_last_error`（`Fs.m.ct:41-43`，每翻译单元一份快照） | 已在 std（按模块快照） | 不引入全局 `errno` 读写；失败约定见 `std/fs/README.md` |
| 通用访问器 | 无 | 建议包·第二档 | namespace `std::errno`（trait `Errno`）：`std::errno::Errno.get_last()`；与各模块快照并存，需先统一口径（本清单只登记） |
| `strerror` | 无 | 建议包·第二档 | `std::errno::Errno.message(code, out, cap)`；`-1` 放不下 |
| `E*` 常量 | 无 | 非函数 | 需随 `message` 导出常量表，另议 |

### 2.8 `dirent.h`

| 函数 | 现状 | 结论 | 落地细节 |
| --- | --- | --- | --- |
| `opendir` | 无（`Fs.list` 是一次性列举，不是句柄） | 建议包·第一档 | 新类型 `Dir` + `Drop`；宿主 `char *`：`path.open_dir()`（不能叫 `open`，已被 `Path.open` 占）；失败 `NULL` |
| `readdir` | 无 | 建议包·第一档 | `Dir.next(out, cap)`；`1` 有、`0` 结束、`-1` 错误 |
| `closedir` | 无 | 建议包·第一档 | `Dir.close()`；`0` 成功或已关（幂等）/ `-1` 失败；`Drop` 兜底 |
| `rewinddir` | 无 | 建议包·第一档 | `Dir.rewind()`；无返回 |
| `telldir` / `seekdir` | 无 | 建议包·第一档（可选） | `Dir.tell()` / `Dir.seek(pos)`；`-1` 失败 |
| `scandir` / `alphasort` | 无 | 不建议包 | 需要比较器与分配数组；`FsOps.list` 已覆盖一次性列举 |

### 2.9 `unistd.h`

| 函数 | 现状 | 结论 | 落地细节 |
| --- | --- | --- | --- |
| `read` / `write` / `close` | `File.read` / `write` / `close`（`IoFile *`） | 已在 std（`stdio` 句柄层） | 无 fd 层 |
| `lseek` | 无 | 建议包·第一档 | 新类型 `Fd` + `Drop`：`Fd.seek(offset, origin)` / `tell()`；`-1` 失败 |
| fd 上的 `read` / `write` / `close` | 无 | 建议包·第一档 | 同一 `Fd` 上 `read` / `write` / `close`（与 `IoFile *` 不同宿主，允许重名） |
| `open` | 无 | 建议包·第一档 | 宿主 `char *`：`path.open_fd(flags, mode)` → `Fd *`；失败 `NULL`（不能叫 `open`，`Path.open` 已占） |
| `pread` / `pwrite` | 无 | 建议包·第一档 | `Fd.read_at(off, buf, n)` / `write_at(off, buf, n)`；返回字节数，`-1` 失败 |
| `fsync` / `fdatasync` | 无 | 建议包·第一档 | `Fd.sync()`；`-1` 失败 |
| `ftruncate` | 无 | 建议包·第一档 | `Fd.truncate(len)`；`-1` 失败 |
| `truncate` | 无 | 建议包·第一档 | 宿主 `char *`：`path.resize(len)`；`-1` 失败 |
| `isatty` | 无 | 建议包·第一档 | `Fd.is_tty()`；`0/1` |
| `access` | `Fs.exists` 覆盖 `F_OK` | 已在 std（部分） | `W_OK` / `X_OK` 未覆盖；需要时补 `is_readable()` / `is_writable()`（第二档） |
| `unlink` / `rmdir` / `rename` | `Fs.remove` / `Fs.rename` | 已在 std | — |
| `getcwd` | 无 | 建议包·第二档 | namespace `std::fs` 普通函数 `std::fs::get_dir(out, cap)`（无路径接收者）；`-1` 放不下 |
| `chdir` | 无 | 建议包·第二档 | 宿主 `char *`：`path.change_dir()`；`-1` 失败 |
| `getpid` / `getppid` | 无 | 建议包·第二档 | namespace `std::process`（trait `Process`）：`std::process::Process.get_pid()` / `.get_parent_pid()` |
| `getuid` / `geteuid` / `getgid` | 无 | 建议包·第二档 | `get_uid()` / `get_euid()` / `get_gid()` |
| `readlink` | 无 | 建议包·第二档 | `read_link(out, cap)`；`-1` 失败（非链接或放不下） |
| `symlink` / `link` | 无 | 不建议包 | 低频、语义易错 |
| `chown` / `fchown` / `lchown` | 无 | 不建议包 | 权限语义，通常需 root |
| `pipe` / `dup` / `dup2` | 无 | 不建议包 | 需要 fd 生命周期分析，后续另议 |
| `fork` / `exec*` / `waitpid` / `_exit` | 无 | 不建议包 | 进程控制；CT 无并发/进程模型 |
| `sysconf` / `alarm` | 无 | 不建议包 | 参数宽泛 / 信号依赖 |

### 2.10 `fcntl.h`

| 函数 | 现状 | 结论 | 落地细节 |
| --- | --- | --- | --- |
| `open` | 无 | 建议包·第一档 | 与 §2.9 同一入口 `path.open_fd(flags, mode)` |
| `fcntl` | 无 | 不建议包 | 变参 + 命令码宽泛 |
| `creat` | 无 | 不建议包 | `open_fd` 用 `O_CREAT` 覆盖 |
| `flock` | 无 | 建议包·第一档（可选） | `Fd.lock(mode)` / `unlock()`；`-1` 失败 |
| `posix_fadvise` / `fallocate` | 无 | 不建议包 | 平台相关 + 低频 |
| `O_*` 常量 | 无 | 非函数 | 随 `open_fd` 导出；建议先只支持 `O_RDONLY` / `O_WRONLY` / `O_RDWR` / `O_CREAT` / `O_TRUNC` / `O_APPEND` |

### 2.11 `sys/stat.h`

| 函数 | 现状 | 结论 | 落地细节 |
| --- | --- | --- | --- |
| `stat` / `lstat` | 无（`Fs.exists` 内部用了 `stat`，但不暴露字段） | 建议包·第一档 | 新值类型 `FileStat`；`read_stat(out)` / `read_link_stat(out)`，宿主 `char *`；`-1` 失败 |
| `fstat` | 无 | 建议包·第一档 | `Fd.stat(out)` / `IoFile.stat(out)` |
| `chmod` | 无 | 建议包·第一档 | `set_mode(mode)`，宿主 `char *`；`-1` 失败 |
| `fchmod` | 无 | 建议包·第一档 | `Fd.set_mode(mode)`；`-1` 失败 |
| `mkdir` | `Fs.mkdir` | 已在 std | — |
| `rmdir` | `Fs.remove`（仅空目录） | 已在 std | — |
| `S_ISDIR` / `S_ISREG` / `S_ISLNK` | 无 | 建议包·第一档 | `FileStat.is_dir()` / `is_reg()` / `is_link()`；`0/1` |
| `umask` | 无 | 不建议包 | 进程全局状态 |
| `mkfifo` | 无 | 不建议包 | 低频 |
| `utime` / `utimes` | 无 | 不建议包 | 低频；`FileStat` 已带 `mtime` |

### 2.12 `sys/socket.h`

| 函数 | 现状 | 结论 | 落地细节 |
| --- | --- | --- | --- |
| `socket` | 无 | 建议包·第一档 | 新类型 `Socket` + `Drop`；namespace `std::net`（trait `Net`）：`std::net::Net.open_socket(domain, type, proto)`；失败 `NULL` |
| `bind` | 无 | 建议包·第一档 | `Socket.bind_to(host, port)`；`-1` 失败 |
| `listen` | 无 | 建议包·第一档 | `Socket.listen_backlog(n)`；`-1` 失败 |
| `accept` | 无 | 建议包·第一档 | `Socket.accept_one()` → `Socket *`；失败 `NULL` |
| `connect` | 无 | 建议包·第一档 | `Socket.connect_to(host, port)`；`-1` 失败 |
| `send` / `recv` | 无 | 建议包·第一档 | `Socket.send_bytes(buf, n)` / `recv_bytes(buf, n)`；返回字节数，`-1` 失败，`0` 对端关闭 |
| `shutdown` | 无 | 建议包·第一档 | `Socket.shutdown_both()`；`-1` 失败 |
| `close` | 无 | 建议包·第一档 | `Socket.close()`（幂等）+ `Drop` |
| `sendto` / `recvfrom` | 无 | 建议包·第一档（后续） | 需要 `Address` 值类型；先做 UDP 最小闭环 |
| `setsockopt` / `getsockopt` | 无 | 建议包·第一档（后续） | `Socket.set_option(opt, val)`；先只支持 `SO_REUSEADDR` / `TCP_NODELAY` |
| `getaddrinfo` / `freeaddrinfo` / `gethostbyname`（`netdb.h`） | 无 | 建议包·第一档（独立一相） | 新类型 `Address`；`resolve_host(name, out)`；涉及分配与超时，单独排期 |
| `inet_pton` | 无 | 建议包·第二档 | `parse_ip(out)`，宿主 `char *`；`-1` 非法 |
| `inet_ntop` | 无 | 建议包·第二档 | namespace `std::net` 普通函数 `std::net::format_ip(family, out, cap)`（无接收者）；`-1` 放不下 |
| `inet_ntoa` | 无 | 不建议包 | 静态缓冲、非线程安全（用 `format_ip`） |
| `socketpair` / `sendmsg` / `recvmsg` / `accept4` | 无 | 不建议包 | 低频或需要 `iovec`/平台扩展；已被上面的族覆盖 |

> 未逐条列出的名字（`gets`、`a64l`、`l64a`、`mktemp`、`getopt` 族、`ftw`、`glob` 等）归入
> 「不建议包」：要么已被同族行覆盖，要么需要回调/结构体/平台扩展，本项目当前表达不了。

## 3. 同名冲突与余量

规则原文（`README.md:141-143`）：

> 同一接收者类型上，**方法名不能重复**：任意两个可见 trait 声明同名方法，调用点就报
> `ambiguous type-host map for method 'x' on type 'char *'`。按名字判定，不看参数个数。
> 给 `char *` 新增 trait 前先查这张表。

### 3.1 `char *` 宿主（最拥挤，25 个名字）

| 来源 | 已占名字 |
| --- | --- |
| `String`（现 `CharStr`，14） | `len` `find` `find_char` `starts_with` `ends_with` `copy` `slice` `concat` `to_lower` `to_upper` `compare` `copy_to` `to_int` `to_double` |
| `FsOps`（7） | `exists` `read_file` `write_file` `list` `mkdir` `remove` `rename` |
| `Utf8Ops`（3） | `decode` `count_codepoints` `encode` |
| `Path`（1） | `open` |

合计 25（`README.md:145-153`）。余量：**没有数量上限，但每个新名字都要先查这张表**；下面的候选
已逐条比对，全部不撞车：

| 候选（本清单建议） | 撞车？ |
| --- | --- |
| `rfind_char`、`compare_ignore_case` | 否 |
| `parse_long` / `parse_llong` / `parse_ulong` / `parse_real` | 否（`to_int` / `to_double` 是既有解析入口，不重名） |
| `open_dir`、`open_fd`、`read_stat`、`read_link_stat`、`read_link`、`resize`、`set_mode`、`resolve_path` | 否（**不能用 `open`**：已被 `Path` 占） |
| `change_dir`、`parse_ip`、`is_readable`、`is_writable` | 否（`get_dir` / `format_ip` 没有路径接收者，走 namespace 普通函数） |
| `read` / `write` / `close` | 在 `char *` 上空闲，但**不建议**占用：会与 `IoFile *` 上的同名方法读起来混淆 |

### 3.2 其它宿主已占名字

| 宿主 | 已占 | 备注 |
| --- | --- | --- |
| `IoFile *` | `read` `write` `close`（`README.md:163`） | 本清单的 `seek` / `tell` / `at_eof` / `has_error` / `read_char` / `read_line` 均未占用 |
| namespace `std::io` | `puts` `putchar` `getchar` `fflush`（`Stdout`）、`printf`（`Io`） | 本清单没有新增该宿主方法；`open_fd` 挂 `char *`（§3.1） |
| namespace `std::fs` | `get_last_error`（`FsEnv`） | — |
| namespace `std::math` | `sqrt` `fabs`（`Math`） | 新增 `pow` / `exp` / `log` 等不撞 |
| `int` / `long` / `long long` | 各 7：`abs` `clamp` `max` `min` `to_str` `to_string` `to_base` | `Float` / `Real` 上补 `min` / `max` 属不同宿主，允许 |
| `char` | 7：`is_digit` `is_alpha` `is_space` `is_upper` `is_lower` `to_upper` `to_lower` | 新增 `is_alnum` 等不撞 |

### 3.3 结论

`char *` 已经 25 个名字，是冲突热点。后续新 API 优先落在**新的句柄类型**（`Dir` / `Fd` / `Socket` /
`Timer`）或 **namespace 宿主**（`std::time` / `std::net` / `std::env` / `std::process` / `std::errno`）
上，尽量不再往 `char *` 堆名字。

## 4. 落地顺序（排期底稿）

按「先无类型、后新类型，先本机、后网络」排序；每相可以独立发布，新增接口按既有实践走 minor
（`0.1.0` 新增 `std::io` 是先例）。与 issue 0001 的 `0.4.0` 撞车时由负责人排序，本文不预设。

| 序 | 相 | 档 | 内容 | 新类型 | 粗估 |
| --- | --- | --- | --- | --- | --- |
| 1 | A | 第二档 | `ctype.h` 补全（8 个 `is_*`）+ `num` 解析族（`parse_long` / `parse_real` 等）+ `rfind_char` / `compare_ignore_case` | 无 | 1 相 |
| 2 | B | 第二档 | `math.h` 补全（`std::math` + `Float` / `Real`，含 `Float.is_nan`） | 无 | 1 相 |
| 3 | C | 第一档 | `stdio.h` 句柄补全：`IoFile *` 上的 `seek` / `tell` / `at_eof` / `has_error` / `read_char` / `read_line` | 无 | 1 相 |
| 4 | D | 第一档 | 目录遍历：`Dir` + `next` / `rewind` / `close` | `Dir` | 1 相 |
| 5 | E | 第一档 | `Fd` + `FileStat`：`open_fd`、`read` / `write` / `seek`、`read_stat` / `set_mode` | `Fd`、`FileStat` | 1–2 相 |
| 6 | F | 第一档 + 第二档 | 时间：`TimeParts` + `now_sec` / `now_ns` / `format_time` / `sleep_ms`；`Timer` 可选 | `TimeParts`、`Timer` | 1 相 |
| 7 | G | 第一档 | socket：`Socket` + `Address`，先 TCP 最小闭环，再 UDP / DNS | `Socket`、`Address` | 1–2 相 |

「不做会怎样」各一句：

- A 不做：字符判断与数值解析继续手写 `c >= '0' && c <= '9'` / `atoi`，命名与失败约定不统一。
- B 不做：幂、对数、三角只能直接调 libc，或自己写近似。
- C 不做：句柄只能顺序读写，回头读、探测 EOF 都要调用方自己记账。
- D 不做：大目录只能用 `FsOps.list` 一次性塞进定长缓冲，放不下直接 `-1`（`std/fs/README.md:77`）。
- E 不做：文件大小 / 权限 / 类型只能 shell 出去问，或另抄一层 libc。
- F 不做：时间只有 epoch 数字，格式化要调用方手写。
- G 不做：网络程序直接调 libc，句柄无 `Drop`，泄漏与半关闭风险留在每个调用方。

## 5. 非目标

- 不写任何实现、不动 shim、不新增 trait / 类型到代码里。
- 不动版本：`0.3.1` 已发布，`0.4.0` 留给 issue 0001 的改名；本文只提供排期输入。
- 不开相卡、不排具体日期。
- 不批量改既有 API 名（`String` → `CharStr` 由 issue 0004 在 `0.20.0` 处理）。
- 不修改 `README.md:505-506` 的 libc 原名豁免；若要全动词化，先改 §11 再谈本文条目。
