# 验收映射

| ACC | 对应要求 | 判据 | 验证命令 / 证据 |
| --- | --- | --- | --- |
| `ACC-SCOPE-1` | REQ-SCOPE-1 | ISO C17 头文件逐项有归属（已覆盖 / 待补 / 不适用） | PH-9 的缺口表；`docs/planning/roadmap.md` §4 |
| `ACC-SCOPE-2` | REQ-SCOPE-2 | POSIX.1-2017 头文件逐项有归属 | PH-10/PH-11 卡片；roadmap §4 |
| `ACC-SCOPE-3` | REQ-SCOPE-3 | 变参/Windows/C++ 在文档中明确排除 | `docs/requirements/scope.md` §3 |
| `ACC-REG-1` | REQ-REG-1 | `tools/check.sh all` 退出 0 | `bash tools/check.sh all` |
| `ACC-REG-2` | REQ-REG-2 | 存量清单存在且 D-001 签字；新文档不在清单 | `tools/legacy-docs.txt` + `tools/check.sh g4` |
| `ACC-REG-3` | REQ-REG-3 | `v0-PH-8` 收口版本 `0.12.0`，tag 指向收口提交 | `ct.pkg`、`docs/planning/roadmap.md`、`git tag -n v0.12.0` |
| `ACC-PH8-CARD` | PH-8 | 卡片必填字段齐全，任务可独立验收 | `bash tools/check.sh g1` |
| `ACC-PH8-TESTS` | PH-8 | `tests/run.sh` 20/20；`ct pkg check` 通过 | `bash tests/run.sh && ct pkg check .` |

## 说明

- `ACC-SCOPE-*` 在 PH-9/PH-10 正式定稿时细化到接口级；当前只钉范围。
- 存量文档的代码块不在本表的 `ACC` 覆盖内（D-001 豁免）；新文档必须引用 `docs/snippets/`。
