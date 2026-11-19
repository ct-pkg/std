applies-to: v0.19.0

# Changelog

## 0.19.0 — 2026-11-19

官方包身份归一：`publisher = std` → `publisher = ct`，规范名 `ct/std`；版本 0.19.0，
API 无变化。旧 tag 与旧 store 不移动、不删除。

## 0.18.0 — 2026-11-19

`v0-PH-14` POSIX mqueue：Linux 真实 mqueue，macOS stub。

## 0.17.0 — 2026-11-19

`v0-PH-13` POSIX 异步：aio 句柄与 posix_spawn；mq 归 PH-14。

## 0.16.0 — 2026-11-19

`v0-PH-12` POSIX 服务续：pwd/grp、sched、shm、termios、glob；aio/mq/posix_spawn
归 PH-13。

## 0.15.0 — 2026-11-19

`v0-PH-11` POSIX 服务：uname、rlimit、statvfs、fnmatch、dlfcn；其余服务归 PH-12。

## 0.14.0 — 2026-11-19

`v0-PH-10` POSIX 核心：mmap/munmap/mprotect/msync、poll、select、pthread
thread/mutex/cond、匿名 semaphore；句柄为 `void *` + 显式释放。

## 0.13.0 — 2026-11-19

`v0-PH-9` ISO C17 补全：string（5）、stdio（5）、stdlib（4）、time（UTC/timespec）、
wctype、fenv；complex/atomic/threads/uchar 探针后 deferred 到 PH-10。

## 0.12.0 — 2026-11-19

`v0-PH-8` 工程规范纳管，**API 无变化**。

- G0–G9 门禁脚本 `tools/check.sh` 与负例 `tools/tests/selftest.sh`。
- `docs/session-map.md`、`docs/phases/`、`docs/requirements/`、`docs/decisions/`、`docs/acceptance.md`。
- D-001：采用 1.17.0；存量文档/issue/历史提交豁免；版本永久偏移 `d=-4`。
- D-002：std 范围定为 ISO C17 + POSIX.1-2017。
- 路线图：`v0-PH-9` ISO C、`v0-PH-10` POSIX 核心、`v0-PH-11` POSIX 服务。

## 0.11.0 — 2026-11-18

P7 `net`：TCP/UDP、`resolve_host`、`set_option`、超时；`Socket = FileDesc` 别名；补齐 P1 漏项与 `exp2`。
