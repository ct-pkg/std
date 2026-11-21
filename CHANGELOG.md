applies-to: v0.20.1

# Changelog

## 0.20.1 — 2026-11-21

门禁 PATCH 修复（issue 0006）：`g8` 接受按 V-7 未打 tag 的 fix 版本，
只有 PATCH=0 的 phase 发行仍要求 annotated tag；删除 `g6` 的 `0.19.x`
硬编码，改由 D-004 记录。无 API 变化，无 tag。

## 0.20.0 — 2026-11-21

`v0-PH-16` 破坏性改名：`std::str` 的单元与 trait 统一为 `CharStr`（原单元 `String`、原 trait
`CStrOps`），`import std::str::String` 改为 `import std::str::CharStr`，单元文件改为
`CharStr.m.ct` / `CharStr.shim.h`。方法名与 C 符号 `std_str_*` 不变，不留兼容别名（项目未正式发布）。
同时记录内建层边界：只删语言级类型重复，C 资源句柄与 libc/POSIX 映射保留。

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
