# std — CT 标准库（已从 ct-lang 迁出）

本仓库是 CT 的标准库，从 `ct-lang` 的 `std/` 目录整体迁出，作为一个**普通 ct-pkg 包**发布。
`import std::str::String` 等 import 路径**不变**，变化的只有分发方式：不再随编译器安装，改为 `ct get` 安装。

> **当前状态：源码已迁入，未发版。** `std/` 与 `tests/` 已由 `tools/migrate-from-ct.sh --apply`
> 从 ct-lang 复制（25 个文件 + 22 个 fixture，`--check` 无漂移），包内 gate 为 10 passed / 0 failed。
> 尚未打 tag；在 `v0.1.0` 打出来之前本包不可通过索引安装。

## 安装

```
ct get std@^0.1.0        # 解析并写入 ct.lock、vendor/std/
ct verify                # 可选：核对 vendor/ 与 lock 的摘要
ct install               # 打印建议的构建命令
```

`ct install` 会打印 `-I vendor/std ...`，把这一项加进 `ct build` 即可：

```
ct build src/main.ct -I vendor/std -o main
```

`vendor/std/std/str/String.m.ct` 这样的双层目录不是笔误：包工具按包逐个打印 `-I vendor/<包名>`，
而命名空间根是 `std/`，两层合起来正好还原 `import std::str::String`（与 `clib` 包的结构一致）。

### 离线 / 钉版本

把 `vendor/` 连同 `ct.lock` 一起提交，CI 里用 `ct install --check`（或 `ct verify`）即可离线构建。
需要某个具体版本时，在 `ct.pkg` 里写 `dep = std@0.1.0` 并提交 `ct.lock`。

## 版本策略

| 项 | 约定 |
| --- | --- |
| 版本号 | 独立 semver，从 `0.1.0` 起；与编译器版本号**无关**（编译器是 `0.19.x` 时本包可以是 `0.1.x`） |
| `0.x` 期间 | 破坏性改动走 **minor**（`0.1.0` → `0.2.0`），修复走 patch；不承诺向后兼容 |
| `1.0.0` 之后 | 破坏性改动走 major，与包的 semver 常规一致 |
| tag | `v<version>`，例如 `v0.1.0`；索引只认 `v*` 且取最高一个 |
| 编译器下限 | 写在 `ct.pkg` 的 `requires`；**只告警不拒装**，所以 README 也要写。当前为 `ct >= 0.20.0`，依据见下 |

`requires` 是实测的，而且要分两层看（2026-11-06，对照编译器由 tag `v0.19.1` 源码引导）：

| 对象 | v0.19.1 | 当前 ct-lang `main` |
| --- | --- | --- |
| 七个模块单元本体 | 通过 | 通过 |
| 22 个 fixture + 文档示例 | **5 passed / 5 failed** | 10 passed / 0 failed |

- **库本体兼容 0.19.1。** 四个 `.h.ct` 用 `ct build --ns-root . --emit-c -o <dir>` 逐个产出 C；
  三个 `.m.ct` 不能单独出 C，改用一个只 import 七个模块、以**变量**作接收者各调一个 API 的探针程序
  编译并运行，结果 `9 checks, 0 failures`。`std/**/*.ct` 里没有任何字面量接收者。
- **文档与示例不兼容 0.19.1。** fixture 与 tutorial 大量写 `"hi".len()`、`"std".codepointCount()`，
  v0.19.1 报：

  ```
  error: method receiver must be a variable, an element, a call or a field chain
  ```

  这条语言特性是 v0.19.1 之后才加的。

所以包级 `requires = ct >= 0.20.0`：在 0.19.1 上**装得上、库也能用**，但照抄 README / tutorial 的示例
会编译失败。0.20.0 是第一个带该特性的发行版（尚未发布）；发版后把这里和 `ct.pkg` 一起钉成实际 tag 复核。

## ⚠️ 索引限制：只能拿到最新版

`index/index` 的质量约定是**每个仓库只收录最高的 `v*` tag**，一行一个包，不带历史版本。
后果对使用者是明确的：

- `ct get std@^0.1.0` 只会解析到索引里的那一个版本；
- **老编译器无法从索引取到与它兼容的旧版 std**。`requires` 只打印告警，装完之后才可能报编译错误；
- 需要旧版时只能绕过索引：`git clone` 后在本地用 `file://` 路径安装，或手工 checkout 对应 tag 的 `vendor/std`。

这条限制在包模型升级（索引支持多版本）之前不会消失，因此本包**不承诺**「任意编译器版本都能装上匹配的 std」。

## 目录结构

```
std/                      ← 仓库根 = 包根（ct.pkg 所在处）
├── ct.pkg
├── LICENSE               ← 从 ct-lang 复制，含 GPL-3.0 与 GCC 运行时例外
├── LICENSE.EXCEPTION
├── README.md
├── std/                  ← 命名空间根，-I vendor/std 指向这里
│   ├── README.md         ← 七个模块的总览
│   ├── str/   String.m.ct + String.shim.h + README + tutorial
│   ├── fs/    Fs.m.ct + Fs.shim.h + README + tutorial
│   ├── utf8/  Utf8Ops.m.ct + Utf8Ops.shim.h + README + tutorial
│   ├── vec/   Vec.h.ct + README + tutorial
│   ├── map/   Map.h.ct
│   ├── arena/ Arena.h.ct
│   └── test/  Test.h.ct
├── tests/                ← 原 ct-lang 的 test/std 门禁，改为本包自己的 CI
│   ├── run.sh
│   └── <mod>/…           ← fixture，由迁移脚本复制
├── tools/
│   ├── bootstrap-ct.sh   ← 从 ct-lang 源码引导出 ct 编译器（CI 用）
│   └── migrate-from-ct.sh
└── .github/workflows/test.yml
```

`std/` 内的 7 个模块目录与文件内容**逐字来自 ct-lang**，迁移脚本只做复制，不做改写。

## 测试

```
CT=/path/to/ct bash tests/run.sh          # 或把 ct 放进 PATH 后直接 bash tests/run.sh
CC=gcc bash tests/run.sh                  # 指定编译器
```

`tests/run.sh` 是 ct-lang 第 10 道门禁 `test/std/run.sh` 的移植版，保留同样的检查：

1. 每个 `.h.ct` 的 `namespace std::<目录>;` 与「`pub struct` 名 = 文件名」；
2. 每个 fixture 用 `-Wall -Wextra` 编译、运行，stdout 必须含 ` 0 failures`；
3. `test` 模块的负向 fixture 必须返回非零并打印 3 条 `FAIL`。

### CI 覆盖（迁出后唯一的防线）

ct-lang 侧删掉 `std/` 与 `test/std` 之后，编译器仓库不再有任何 std 覆盖。
本包的 CI 因此承担全部信号，触发条件刻意做成「PR 必跑 + 每晚对编译器 main 跑」：

| Job | 触发 | 编译器来源 | 矩阵 |
| --- | --- | --- | --- |
| `host` | `pull_request`、push `main`、schedule、手动 | `CT_REF`（当前 `main`，编译器发版后钉到 `v0.20.0`） | ubuntu / gcc，macOS / clang |
| `compiler-head` | schedule、手动 | 强制 `main` | ubuntu / gcc |

`compiler-head` 的意义：语言仓库的破坏性改动通常落在 `main`，它会在下一个自然日把这个包打红，
而不是等到下一次包发布才发现。

**仓库可见性（已核实）**：`ct-lang` 当前是**公开仓库**，所以工作流用内置 `github.token` 就能跨仓
checkout，不需要任何 secret。工作流仍写成 `secrets.CT_LANG_TOKEN || github.token`：仓库哪天转私有，
配一个对 `ct-lang` 有 read 权限的 `CT_LANG_TOKEN` 即可恢复，无需改 YAML。

## 从 ct-lang 迁移（已执行，2026-11-06）

```
bash tools/migrate-from-ct.sh                 # 只打印计划，不写任何文件
bash tools/migrate-from-ct.sh --check         # 只检查源/目标是否漂移（有漂移则非零退出）
CT_SRC=/path/to/ct bash tools/migrate-from-ct.sh --apply   # 真正复制
```

脚本行为：

- 复制 `std/{str,fs,utf8,vec,map,arena,test}` → `std/`（`rsync -a --delete`，排除 `.DS_Store`）；
- 复制 `std/README.md` → `std/README.md`（七个模块的总览，不属于任何模块目录）；
- 复制 `test/std/*` → `tests/`（同样 `--delete`，但**排除 `tests/run.sh`**，它是本包维护的门禁脚本）；
- 只读 ct 仓库，不改、不删、不提交；不执行任何 git 命令；
- 幂等：重复执行结果一致；`--check` 可在 CI 里防止包内容与 ct 源头静默漂移。

本轮**不属于**本包、由 ct 侧另行安排的动作（脚本一律不碰）：
删除 ct-lang 的 `std/`、删除 `test/std`、修改 `install.sh`、修改 `check-all.sh` 的第 10 道门禁。

### 跨仓库相对链接（脚本改写）

模块文档是按 ct-lang 的目录写的，直接搬会有三类链接指向包外，共 39 条。迁移脚本在复制时做**确定性改写**，
不手工改包内文案（否则下一次 `--apply` 会覆盖）：

| 原形态 | 条数 | 改写为 |
| --- | --- | --- |
| `../../test/std/<mod>/…` | 29 | `../../tests/<mod>/…`（fixture 目录随包搬迁） |
| `../../issues/selfhost/…` | 8 | `https://github.com/samxxu/ct-lang/blob/main/issues/selfhost/…` |
| `../selfhost/ct-ast/…` | 2 | `https://github.com/samxxu/ct-lang/blob/main/selfhost/ct-ast/…` |

台账和编译器源码留在 ct-lang，所以指向绝对 URL：既不会在包内过期，也不用把副本拖进来。改写规则写在
[`tools/migrate-from-ct.sh`](tools/migrate-from-ct.sh) 顶部的注释里；`--check` 比对的就是改写后的结果，
因此规则可复现。当前包内相对链接 100 条，**断链 0**。

## 摘要与发布注意

`refresh-index` 计算摘要时会遍历包目录下除 `.git`、`vendor`、`.ctcache` 之外的所有文件，
因此 `README.md`、`tests/`、`.github/` 也计入 `sha256`。这意味着：

- 打 tag 之前必须让所有文件（包括 CI 工作流）定稿；
- tag 之后任何文件改动都需要新版本号 + 新 tag，否则 `ct verify` 会报摘要不一致。

### 打 tag 后必须做（清单）

索引里 `std` 目前只有一行注释（无 tag 时 `refresh-index.sh` 会 `exit 1`，打断夜间任务），
注释不会提醒任何人，所以按顺序做完这四步才算发布完成：

1. 把 `index/packages.txt` 里 `std` 那行的 `#` 去掉（`refresh-index.sh` 从这里读仓库列表）；
2. 跑 `index/tools/refresh-index.sh`（或手动触发索引仓库的 workflow），生成带**真实 sha256** 的
   `index/index` 行——`name version url tag sha256:<64 hex>`，不能手写占位摘要；
3. 起一个干净目录跑 `ct get std@0.1.0 && ct verify`，确认摘要与安装链路都成立；
4. `CT_SRC=<ct-lang> bash tools/migrate-from-ct.sh --check` 必须输出 `in sync`，否则说明包内文件
   与源头不一致，先对齐再重新打 tag。

发布顺序：`git tag v0.1.0` → `bash tools/publish-package.sh std` → 上面四步。

## License

与 ct-lang 一致：GPL-3.0-or-later，附 GCC 运行时例外（见 `LICENSE` 与 `LICENSE.EXCEPTION`）。
你写的 `.ct` 源码与编译器生成物不受 GPL 约束。
