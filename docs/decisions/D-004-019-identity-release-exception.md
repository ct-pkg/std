applies-to: v0.20.1

# D-004：`0.19.0` 官方身份发行不占 phase 号

| 项 | 值 |
| --- | --- |
| id | D-004 |
| date | 2026-11-21 |
| anchor | `ae8dd94` |
| status | accepted |
| trigger | 用户指出 phase/MINOR 与 fix/PATCH 不一致，并在修正方案中选择 A / A |
| judge | USER |
| signer | USER |
| seats | REQ / EDITOR |
| scope | `0.19.0` 官方身份发行与 `v0-PH-15` 空号 |
| target | `v0-PH-16` |

## 决定

1. `0.19.0` 是**官方包身份归一发行**，不是 phase；`v0-PH-15` 不补卡、
   永久空号，不追溯开卡。
2. 版本序列保留既有事实：

   ```text
   v0-PH-14 → 0.18.0
   0.19.0    → 官方身份发行（非 phase，D-004 例外）
   v0-PH-16 → 0.20.0
   ```

   这是 V-1“MINOR 与 phase 一一对应”的**已签字例外**，不是无记录的跳号。
3. 后续 phase 继续按 `d = -4` 的永久偏移：`v0-PH-17` 收口 `0.21.0`。
   不回填 `0.19.0`、不重编号、不移动已发布的 `v0.19.0` / `v0.20.0`。
4. `g6` 不再硬编码 `0.19.x` 放行；若未来在历史提交上核对 `0.19.x`，
   必须能读到本 decision 才放行。

## 理由

- `v0.19.0` 与 `v0.20.0` 已推送，tag 不可回改；
- 0.19.0 没有 phase 交付物，但确实产生了对外可见的包身份变更；
- 追溯开 `v0-PH-15` 会违反“先开卡后开工”，比登记例外更糟；
- 规范允许以用户签字的 `deviation:` 登记存量落差；本条按同一机制登记。

## 授权

- 签字：USER（2026-11-21，会话指令“A。A。”：0.19 走签 D-004；gate fix 走 issue + 0.20.1）。
- 登记位置：`AGENTS.md` 0.3 偏离项、`docs/planning/phase-id-ledger.md`、`docs/planning/roadmap.md`。
