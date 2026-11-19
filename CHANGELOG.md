applies-to: v0.13.0

# Changelog

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
