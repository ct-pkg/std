# 反证：std 的两层范围（零代码映射底座 + 方法层铺宽）

| 项 | 值 |
| --- | --- |
| 性质 | 只读反证：不写实现、不改代码、不起版本 / tag；仅新增本文 |
| 对象 | `docs/libc-inventory.md` 三档路线、两份 `issues/0001-*` |
| 被判的路线 | **底座零代码映射（标准 C 有什么直连什么）+ 方法层铺宽（`char *` / 基础类型上挂方法，便利即可，不要求生命周期）+ 新类型加 `Drop` 只限真正持有资源** |
| 约束 | 只在 std 仓处理，不往 `ct-lang` 排任何工作；判据用 CT 自己的风格（字符串起头、方法挂既有类型、链路走到底） |
| 附加约束 | 凡是需要引入 **struct 对等类型**（`Char` / `Str` / 数组包装 …）的地方都要单列，由负责人本人评估必要性；本文 §2.6 逐项单列 |
| 基线 | 包 `0.3.1`，编译器实测 **ct v0.53.0**，`cc` = 本机 clang；探针在 `/tmp`，不改本仓 |
| 日期 | 2026-11-15 |

## 0. 结论摘要

| 编号 | 结论 | 强度 |
| --- | --- | --- |
| C1 | 方法层的**便利性收益是真的**，本仓自己的夹具就是证据（554 次方法调用 vs 24 次 libc 直调）；但它**不随方法数线性增长**，前 12 个名字吃掉约一半调用 | 强 |
| C2 | 方法层**不提供类型安全**：`char *` 上的路径、缓冲区、JSON 完全可互换；`long` 上的时间戳与文件大小完全可互换。铺宽只买到书写便利，买不到"用错会报错" | 最强（实跑） |
| C3 | 失败表达（`-1` + per-TU `errno` 快照）**跨翻译单元失效**：库函数里失败，调用方读到 `0`（成功）。这比 errno 更差，不是更安全 | 最强（实跑 0 vs 17） |
| C4 | `Drop` **连 C 资源都不会自动关**；不写显式 `impl` 时 drop 体是空的。它只能兜"包装体 free"，兜不了 `FILE *` / `DIR *` / fd | 最强（实跑 19 个 fd 后 EMFILE） |
| C5 | 零运行时开销这条**成立**：同一程序 33,464 vs 33,480 字节，编译时间无差异。我先认 | 强 |
| C6 | 清单里**大半"需要新类型"的东西可以退回零代码映射**（`Fd` / `Socket` 用 `int`，`FileStat` / `TimeParts` 用普通 struct 出参，`Dir` 沿用 `IoFile` 的不透明指针先例），代价只是"显式 close" | 中强 |
| C7 | `char *` 的稀缺不是"数量上限"，而是**三重稀缺**：语义唯一（同义词会撞）、跨包抢占（实测两个包同名调用直接报歧义）、可发现性（每个字符串变量都挂 40+ 方法） | 强 |
| C8 | `char *` 的账本**今天就是错的**（26 / 22 / 25 三种说法并存，`std::fs` 索引写 4 实为 7）。拿它当"还有余量"的依据不成立 | 强 |
| C9 | **struct 对等类型在便利性场景里一个都不必要**：`char` / `char *` 已经能挂 `map trait`（现有 `Char` / `String` 就是证明）。引入 struct 只在"拥有 / 多态 / 长度绑定"时需要；实测代价：trait 形态 16 倍内存、两种形态都断链式、与既有 trait 同名会生成非法 C | 最强（实跑） |

一句话：**底座我同意，铺宽的动机我同意，铺宽的幅度和新类型的兜底承诺我反对。** 反对的不是"方法写法"，是"用同义词占满一个全生态共享的命名空间，再宣称这是类型安全"。

## 1. 这条路线值不值得

### 1.1 先认：方法层的便利性在本仓是主用法，不是装饰

27 个夹具里：

| 指标 | 实测 |
| --- | --- |
| 方法调用 | **554 次** |
| libc 直调（`strlen` / `printf` / `stat` / `rename` …） | **24 次** |
| 出现的方法拼写 | 53 种 |

也就是说本仓自己的样例、教程、回归夹具里，**96% 的调用是方法式**。这条我完全认同：判据用 CT 风格而不是传统 C 风格，方法层不是"糖"，是默认写法。`"abc".starts_with("x")`、`n.to_string()`、`p.decode(&cp)` 确实比函数式顺手，而且 `README.md:284-285` 的链式（`s.len().to_string()`）是这个风格的关键一环。

### 1.2 但是：便利性的收益集中在少数名字上，长尾很薄

同一批夹具的方法调用频次：

| 名次 | 方法 | 次数 |
| --- | --- | --- |
| 1 | `decode` | 49 |
| 2 | `encode` | 33 |
| 3 | `to_base` | 29 |
| 4 | `to_string` | 23 |
| 5 | `to_str` | 23 |
| 6 | `abs` | 20 |
| 7 | `find` | 18 |
| 8 | `count_codepoints` | 17 |
| 9 | `to_upper` | 16 |
| 10 | `slice` | 16 |
| 11 | `exists` | 16 |
| 12 | `concat` | 14 |

前 12 个名字合计 274 次，占 554 次的 **49%**；其余 74 个公开方法分摊另一半。**对方会说**：长尾也是需求，用户会用到。**但**：本仓自己的 beforeEach 是"每个方法都要写测试"，所以长尾的调用量有相当一部分是**测试制造出来的**，不是真实使用。用"覆盖了多少 libc 函数"当 KPI，会把测试自证当需求证据。

结论：按频次而不是按头文件铺，方法层大概只需要 15–20 个名字；清单 81 条里至少一半是长尾。

### 1.3 反例 A：同一类型，三种角色——编译期一个字都不报

实测（`/tmp/conarg-probe/probe10.ct`）以下代码全部编译通过并运行：

```ct
char *a = "/tmp/conarg-probe/a.txt";
char *b = "/tmp/conarg-probe/b.txt";
char json[16] = "{}";
char buf[16];

a.write_file("x");
b.rename(a);            /* 方法式：b 是源 */
rename(a, b);           /* libc：a 是源，方向相反，也编译 */

json.exists();          /* JSON 文本被当成路径 */
json.read_file(buf, 16); /* JSON 文本被当成路径读文件 */
buf.write_file("y");    /* 读缓冲被当成路径写文件 */
json.to_int();          /* 对任意字符串都能"转数字" */
```

**对方会说**：`char *` 就是字符串，这些语义本来就都是字符串，没问题。**但**：方法层的卖点之一是"更自然、更接近 OO"，而 OO 的第一层含义是**接收者携带语义**。这里接收者不携带任何语义——`path` / `json` / `buf` 是同一个类型，方法名是唯一区分手段。于是：

- 错用不会被编译期拦住，只会在运行期得到 `-1`；
- 自动补全会把 40+ 个方法塞给每一个字符串变量，**可发现性反而下降**（你无法从候选里判断哪个属于当前语义）；
- 新人最容易犯的错（把内容当路径、把路径当缓冲）恰好是这层最鼓励的写法。

这不是"通用 C 库 vs CT 风格"的问题：CT 风格要求"字符串起头"，但没说"所有字符串共享全部语义"。要保住"起头"又能保住语义，只有一条路——**领域类型**（`path.exists()` 里的 `path` 是 `Path`，不是 `char *`）。清单把 `Path` 降级成 `char *` 上的方法层（`std/io/File.m.ct:11-16`），恰好把语义丢掉了。

### 1.4 反例 B：领域方法挂在基础类型上，等于挂给所有同类型值

用户明确举例"从时间戳直接取年月日"。这个便利我承认，但实现方式必须挑。实测按清单风格挂到 `long` 上（`/tmp/conarg-probe/probe12.ct`）：

```ct
long ts   = 1700000000;   /* 时间戳 */
long size = 1700000000;   /* 文件大小，同类型 */
printf("%d %d\n", ts.year(), size.year());
```

输出：

```
ts.year=2023 size.year=2023
```

文件大小也能问出"2023 年"。**对方会说**：用户不会对文件大小调 `.year()`。**但**：编译期不拦、补全里出现、代码评审靠人眼——这正是方法层"铺宽"的代价：便利是**给类型的**，不是给语义的。同理，`int` 上的 `to_base` / `abs` / `clamp` 对任何整数都成立（这条还算无害），但时间、货币、句柄这类"底层是标量、语义是领域"的东西，挂上去就是误用面。

CT 风格下的正解不是放弃方法，而是**建一个零资源的小值类型**：`Timestamp { long secs; }` + `.year()`。它仍然是"字符串/值起头、方法挂类型、链路走到底"，但只对 `Timestamp` 可见。代价是 `size_t` 式的隐式转换没有了，需要显式构造——这是类型安全的正常价格。

**但也要标出这条的 CT 风格代价**：新类型的方法（无论 `use Trait` 派发还是固有 `impl`）都不接受"临时值作接收者"。实测 `chain_new(1).get()` 与固有方法版 `chain2_new(9).get()` 都报 `a method call on a temporary is only allowed at the start of a statement or of an assignment`（规范 `:2030-2033` 记录了同一限制），即 `now().year()` 写不出来，必须先落到变量。所以"用领域类型换类型安全"会**局部削弱链式风格**。这是真实的取舍，不能只说好话：要么接受一次变量落地，要么接受方法挂在 `long` 上、误用面扩大。

### 1.5 反例 C：双拼写 + 参数顺序，同一个能力两套方向

`std/fs` 已经有 `rename`（方法，接收者是源）与 libc `rename(src, dst)`（函数）。实测两种写法都能编译，而参数顺序相反：

```ct
b.rename(a);     /* b -> a */
rename(a, b);    /* a -> b */
```

同理 `s.copy_to(dst, n)` 与 `memcpy(dst, src, n)`、`s.compare(other)` 与 `strcmp(a, b)`、`s.find(needle)` 与 `strstr(hay, needle)`。**对方会说**：文档写清楚就行。**但**：`char *` 到处都是，参数顺序错了编译器无法发现；一旦方法层和底座同时可用，**同一个语义就有两个拼写、两种方向**，评审必须逐条记住"这里用的是哪套"。这不是假想：`README.md:505-506` 已经明确 libc 直映射**保留原名**（`printf` / `puts` / `sqrt` / `fabs`），所以双拼写是既定政策，不是意外。

### 1.6 收益边界

| 代码 | 方法层铺宽的净收益 |
| --- | --- |
| CT 单语言、单线程、纯新项目，且字符串语义单一（CLI、文本处理） | **正**——本节 1.1 的数据就是这批人 |
| 同一程序里路径 / 内容 / 缓冲区混用（几乎所有真实程序） | **零到负**——1.3 的误用面 |
| 时间、货币、句柄等"标量底 + 领域语义" | **负**——1.4 的误用面 |
| 需要 C/FFI 互操作（shim 无导出符号） | **负**——见 §2.4 |
| 需要精确错误分支（日志、落盘、网络） | **负**——见 §2.4 与 §3.4 |

## 2. 形态选择的反面，以及 shim 把成本藏到了哪里

### 2.1 类型宿主方法（`map trait ... for type char *`）

| 失效面 | 证据 |
| --- | --- |
| 无类型安全（角色可互换） | §1.3 实跑 |
| 名字按宿主全局唯一、不看 arity | `README.md:141-143`；`mkdir(int mode)` 必须显式传参（`README.md:455`） |
| **跨包抢占**：两个包各定义 `char *.parent`，同时 import 后调用直接报错 | 实测：`ambiguous type-host map for method 'parent' on type 'char *': './pkg/a/Paths.m.ct' and './pkg/b/Dirs.m.ct'` |
| 顶层 `const` 不参与类型键 | `README.md:153`；今天安全只因为 `String` 没有写接收者的方法 |
| 方法调用结果不能初始化 `own` | 实测 `error: 'own' requires a malloc, calloc or factory call ...`（对 37 条第一档是致命的） |
| 名字与 libc 同名时两套拼写并存 | 实测：`s.strlen()` 与 `strlen(s)` 同时编译、结果一致 |

**对方会说**：跨包同名是极端情况。**但**：`char *` 是**所有** ct-pkg 包最想挂方法的宿主，`parent` / `base` / `join` / `normalize` / `trim` / `split` / `format` 都是天然候选。std 每占一个，生态里就少一个可用名，且消费者一旦同时 import 就编译失败——这不是极端，是宿主的性质。

### 2.2 namespace 静态方法

- 没有接收者，就没有"起头"的对象，链式断在第一步；`std::io::printf` 这种只能是普通函数（`std/io/Io.h.ct:25`）。
- 同样不能初始化 `own`（实测，与 2.1 同一条 L49-1 错误）。
- 命名空间宿主也共享名字：`std::io` 已有 `puts` / `putchar` / `getchar` / `fflush` / `printf`，再加 `read` / `write` 会与 `IoFile *` 上的同名方法在阅读上打架（`docs/libc-inventory.md:275` 自己承认）。
- 对纯函数（`Math.sqrt`）合适，但纯函数本来就可以直接调 libc，见 §2.5。

### 2.3 新类型 + `Drop`（只限资源类，但这一层仍然不可靠）

先给结论：CT 的 `Drop` 是**"手写 close 的自动调用点"**，不是 RAII。规范（`ct-lang/docs/language-spec.md`）：

- `:2548` `use Drop` 只置位、不进 `use[]`，无 vtable/vptr；写显式 `impl Drop` 即自负全责，不再生成字段胶水。
- `:2549` 指针字段胶水只对"有 Drop 的 CT struct"生效，**不 free 子指针**。
- `:2550` 跨单元 Drop 体由声明单元发射，`impl` 必须与 struct 同单元；需要 **ct ≥ 0.48.0**（`docs/history/phase-log.md:62`），而本包下限是 0.30.0。
- `:469-484` `own` 初始化式只能是 `malloc` / `calloc` / 返回 `struct X *` 的**自由函数**；不支持数组；`goto` 跳出不释放；重新赋值取消兜底；逃逸判定保守。
- `:460`、`:2035-2037` `break` / `continue` 出口只在 ct-ast 实现，冻结的 L2 会漏释放。

实测（探针见 §8）：

| # | 场景 | 结果 | 结论 |
| --- | --- | --- | --- |
| 1 | `struct W { FILE *fp; } use Drop;` 不写 `impl`，`ulimit -n 32` 循环开文件 | 生成的 drop 体是空的 `{ (void)_self; }`；`opened=19` 后 EMFILE | **`use Drop` 不关 C 资源** |
| 2 | 写了 `impl Drop` + 自由函数工厂 | 作用域尾、`return` 前都触发 | 这条可靠，我认 |
| 3 | 句柄存进全局后离开作用域 | 仍触发 Drop，随后读到 `g->id=7131` | 无移动/借用：**别处保存的句柄被释放** |
| 4 | `goto` 跳出 `own` 作用域 | 不触发 | 漏 |
| 5 | `own` 重新赋值 | 不触发，只有 `warning: potential leak` | 静默失效 |
| 6 | 值拷贝 + 字段胶水 | **3 次 DROP**（`Pair.a` + `Pair.b` + 显式 delete） | 浅拷贝重复释放 |
| 7 | `Vec<Res *>` + `release()` | 元素不 Drop | 容器不传播所有权 |
| 8 | 给现有 `IoFile` 补 Drop | `error: impl for undefined struct 'IoFile'` | Drop 加不到既有句柄上，只能换类型 = 破坏性 |

补两条结构性的：

- **只有接口派发才需要 vptr，但现成模板都这么做**。`use Drop` 不加 vptr（实测 struct 只有 `int id`）；固有方法（`impl S { ... }`）也不加（实测 `sizeof(struct Char2)==1`、生成头里是真函数原型）；只有 `use Trait` + `impl Trait` 才追加 vptr（`sizeof(Arena)==32` = 24 字节字段 + 8 字节 vptr，`README.md:201-203`）。所以"新类型 + Drop"不必带 vptr——但本仓 `Arena` / `Test` 的既有写法就是 trait + vptr，照抄会白付 8 字节与间接调用。
- **跨单元 Drop 把下限抬到 0.48.0**，会切断 0.30.0–0.47.x 的消费者；而 `README.md:348` 的"Drop 在包单元里是 static、消费者不可见"正是 0.48 之前的旧行为。

### 2.4 shim 作为底座，把哪些成本藏到了别处

shim 是主形态，我同意；但"零代码映射"这个描述只在**纯转发**时成立。以下成本是真实存在、且被清单一笔带过的：

| 成本 | 证据 | 为什么藏起来了 |
| --- | --- | --- |
| **per-TU 静态状态**：`errno` 快照跨 TU 读不到 | 实测：`Dep.c` 里 `mkdir` 失败返回 `-1`，`main.c` 读 `get_last_error()` 得 **0**；同 TU 得 17 | 文档只说"每翻译单元一份"（`README.md:459-460`），没说这会让**库函数返回的错误在调用方不可见** |
| **失败表达不统一** | 现有 `to_int` 是"无错误报告"（`docs/libc-inventory.md:64`），清单新提的 `parse_long` 是"出参 + `-1`" | 同一个解析族两套模型，同一份文档里并存 |
| **`exists` 三态坍缩** | `std/fs/Fs.shim.h:63-77`：`stat` 失败（如 `EACCES`）也返回 `0` | 调用方无法区分"不存在"和"看不到" |
| **无符号边界 / 无 ABI** | `nm -g` 只有两个 vtable，`std_str_*` 不可见 | 换来零开销，也换来"只有 CT 能用"；C/FFI 侧零收益 |
| **逐消费者复制** | `README.md:197-199`：断点要下在消费者里 | 调试体验与库边界消失；出问题定位在消费者代码 |
| **平台差异没有集中点** | `std/fs/Fs.shim.h:45` 直接 `#include <dirent.h>`，`:205` 用 POSIX `mkdir(path, mode)`，`:103` 用 `fstat`/`S_ISDIR`；无 feature-test 宏 | shim 被注入每个消费者 TU，编译选项/平台宏由消费者决定；`std/fs/README.md:11` 已承认 `list` 在 Windows 上不可用。正常库会把 `#ifdef` 关在实现文件里，这里关不掉 |
| **同名函数双拼写** | §1.5 | 一旦方法层用 libc 原名（`Math.sqrt`），两套入口长期并存 |

我不主张"这些今天已经报错"——本机 `-Wall -Wextra -Werror` 的消费者构建是通过的。我主张的是：**这些差异没有集中修复点**。清单把 81 个函数的平台风险默认成"照抄 libc 就行"，但 shim 的编译单元是消费者的，不是包的。

### 2.5 哪些"被包装成方法层或新类型"的部分可以退回零代码映射

这是我认为清单里最该改的一节。逐条判断（前提：不接受"自动 close"这个卖点，因为它不可靠，见 §2.3）：

| 清单项 | 现提案形态 | 可以退化成的形态 | 能力损失 |
| --- | --- | --- | --- |
| `file_stat` / `lstat` | 新类型 `FileStat` + 方法 | `struct stat` 直接可见 + 一个 out 参函数（或把 `struct stat` 定义导出） | 无；只是字段名是 C 的 |
| `mktime` / `localtime` / `strftime` | 新类型 `TimeParts` | 直接用 `struct tm` + out 参 | 无 |
| `open` / `lseek` / `pread` / `fsync` | 新类型 `Fd` + `Drop` | `int` 上的自由函数（`fd_seek(fd, off, whence)`）或 `int` 宿主方法 | 无；`close` 仍要显式 |
| socket 族 | 新类型 `Socket` + `Drop` | `int` fd 上的自由函数 | 无（地址结构本来就得单独设计） |
| `timer_create` 族 | 新类型 `Timer` + `Drop` | 不包（CT 无并发模型） | 无 |
| `opendir` / `readdir` / `closedir` | 新类型 `Dir` + `Drop` | 沿用 `IoFile` 的不透明指针先例：`path.open_dir()` + 显式 `d.close()` | 无自动 close；但 `Drop` 也保不住 C 资源 |
| `pow` / `exp` / `log` / `sin` / `cos` / `isalnum` / `isxdigit` … | namespace 方法 | **不进 std**，直接调 libc | 无；少一层名字 |
| `strtol` / `strtod` / `atol` | `parse_*` 方法 | 直接调 libc（需要错误码时用 `errno`） | 无；反而少一套 `-1` 约定 |
| `getenv` / `readlink` / `realpath` / `getcwd` | namespace / 方法 | 直接调 libc（`realpath` 的 `PATH_MAX` 问题 libc 一样有） | 无 |
| `isatty` / `getpid` / `getuid` / `chmod` | 方法 | 直接调 libc | 无 |

结论：**7 个新类型里没有一个是"必须有类型才能表达"的**。它们引入类型的唯一额外能力是"自动 close"，而这个能力在 §2.3 里已经被证明不可靠。按这一列退化，清单的 81 条能压到 15 条以内，且不损失任何"能做的事"，只损失"少写几个字符"。

### 2.6 struct 对等类型：逐个评估（按附加约束单列）

**先回答"不动 struct 能不能同样顺手"：能，而且现状已经证明。** CT 的 `map trait` 允许把方法挂到 **`char` / `char *` / `int` / `double` / namespace** 上——现有 `std::num::Char`（`std/num/Num.m.ct:94`，宿主 `char`）与 `std::str::String`（`std/str/String.m.ct:66`，宿主 `char *`）就是两例。也就是说，"给 `char` 加 `is_digit()`"和"给 `char *` 加 `starts_with()`"**今天不引入任何 struct 就能做到**。

因此 struct 对等类型的唯一正当理由是**便利之外**的三件事：

1. **拥有**（要 `Drop` 释放缓冲区）；
2. **多态**（要放进接口值 / 异构容器）；
3. **把数据与长度绑在一起**（`Str { data, len }`）。

清单里 81 条方法层需求，没有一条落在 1–3 上；它们全是第 4 种理由——"写起来顺手"，而这一种用 `map trait` 就够。

#### 2.6.1 逐个候选

| 候选 struct | 用途 | 是否必要 | 不动 struct 的等价路子 | 备注 |
| --- | --- | --- | --- | --- |
| `Char`（`char` 的对等体） | 挂 `is_digit` / `to_upper` | **不必要** | 已有 `std::num::Char` trait 映射 `char` 宿主（`Num.m.ct:94-112`） | 再造 struct 等于同一批方法两套入口 |
| `Str`（`char *` 的对等体） | 长度 + 拥有 + 方法 | **对"便利"不必要**；对"拥有"已有内置 `Str` | `std::str::String` 已映射 `char *`；内置 `Str`（`lib/std/Str.h.ct`）已拥有 | 再造第三个"字符串"是语义重叠，见 §2.6.3 |
| `Path` struct | 挂 `open` / `exists` | **不必要** | 现有 `Path` trait 已挂 `char *`（`std/io/File.m.ct:11-16`） | 变 struct 要先决定借用还是拥有，并为此加转换 |
| 数组包装 struct | 让数组"有方法" | **不必要** | 传 `T *` + `len`（本仓 `slice` / `copy_to` 就是） | CT 的数组参数本来就退化为指针 |
| `Dir`（`DIR *` 对等体） | 遍历 | **可作为不透明别名，不必是 CT struct** | 沿用 `IoFile` 先例：`pub type Dir = struct ct_dir;` + 显式 `close` | 不需要 vptr、不需要 `Drop` |
| `Fd` / `Socket` | fd 包装 | **不必要** | `int` + 自由函数，或 `int` 宿主方法 | §2.5 已列 |
| `FileStat` / `TimeParts` | 出参结构 | **可作为纯数据 struct**（无方法、无 vptr、无 `Drop`） | 直接用 `struct stat` / `struct tm` | 纯数据 struct 不触发本节的问题 |
| `Timestamp` | 时间戳方法 | **可选**，但优于挂在 `long` 上 | 零资源小值类型 + `impl` | 见 §1.4 的取舍：换来类型安全，代价是链式要在变量处落地 |

结论：**需要单列评估的是 4 项**：`Dir`（不透明别名 vs CT struct）、`Timestamp`（值类型 vs `long` 宿主）、`FileStat` 与 `TimeParts`（纯数据 struct，无方法、无所有权，成本只是字段映射）。`Char` / `Str` / `Path` / 数组包装这四类"对等体"在便利性目标下都不成立。

#### 2.6.2 引入之后的实测代价

| 代价 | 实测 | 说明 |
| --- | --- | --- |
| **内存放大（取决于形态）** | `use CharOps` 形态：`sizeof(Char)=16`、`sizeof(char)=1`，`Char[4]` = 64 B（16 倍）；固有方法 `impl Char2` 形态：`sizeof(Char2)=1`、`Char2[4]` = 4 B（零放大） | 8 字节 vptr + 对齐只在 trait 形态出现；导出头可见 `__pkg_x_char_vptr` |
| **接口值装箱** | 只有 trait 形态能放进接口 / 异构容器；固有方法形态不能 | 便利性不需要多态，所以这条不是理由 |
| **方法不再内联** | trait 形态经 `struct ..._vtbl *vt` 间接调用；固有方法形态是普通外部函数（`int pkg_x_Char2_is_digit(struct ...*)`） | 与 `.m.ct` 的 `static inline` 零开销不同，但固有方法的开销只是一次函数调用 |
| **链式在第一环就断** | `chain_new(1).get()` → `error: a method call on a temporary is only allowed at the start of a statement or of an assignment` | 这正是"字符串起头、链路走到底"要避免的 |
| **与既有 trait 同名会生成非法 C** | 同命名空间 `pub trait Char` + `pub struct Char` 编译通过，但生成的 `.c` 写 `pkg_x_Char *_self = (pkg_x_Char *)self;`（缺 `struct`），cc 报 3 个错 | `std::num::Char` 已占用该名字，再加 `struct Char` 必须改名 |
| **值拷贝复制所有权** | `Pair { Res a; Res b; }` 两次值拷贝后 `delete` → 3 次 DROP（§2.3 #6） | CT 无移动语义，struct 对等体一旦拥有资源就重复释放 |
| **每次边界都要装/拆箱** | `char` ↔ `Char` 靠 `.c`；`char *` ↔ `Str` 靠 `str_from`（分配 + 拷贝）/ `str_c_str`（再分配一份副本，调用方 `free`），共享要 `retain()`（又分配） | 每次跨边界多一次分配或一次字段访问，抵掉零代码映射的全部优势 |
| **数组 / C 互操作断裂** | C 函数要 `char`，拿到的是 `Char`；要 `char *`，拿到的是 `Str` | 每个调用点加一层转换，且借用/拥有责任要人工记 |

#### 2.6.3 多个对等体之间的重叠与转换混乱

现有与潜在的"字符串"概念已经有四个：

| 概念 | 谁分配 | 谁释放 | 长度 | 能否直接给 C |
| --- | --- | --- | --- | --- |
| `char *` | 调用方 | 调用方 | `strlen` 每次算 | 是 |
| `std::str::String`（trait，拟改 `CStrOps`） | 调用方 | 调用方 | `len()` | 是（就是 `char *`） |
| 内置 `Str` | `str_from`（分配+拷贝） | `Drop` / `release` | 字段 | `str_c_str`（再分配副本，调用方 `free`） |
| 拟新增 `std::str::Str`（对等体） | ？ | ？ | 字段 | ？ |

前三个已经让 `issues/0001-str-string-trait-rename.md:15-23` 花了一整份 issue 去解释"谁拥有"；再加第四个对等体，读者要同时判断四个概念的所有权。同理：

- `char` / `std::num::Char` trait / 拟议 `Char` struct → 三个名字一个概念；
- `char *` / `Path` trait / 拟议 `Path` struct → 三个；
- `struct stat` / `FileStat`、`struct tm` / `TimeParts` → 两两需要字段级映射（`st_size` 类型、`tm_year+1900`、`tm_mon+1`、DST 标志），每一处都是经典 off-by-one 来源。

**对方会说**：对等体只是"更 OO 的写法"，不用可以不管。**但**：一旦 std 里存在 `Str` 与 `char *`，任何 API 签名都要选一个；选了 `Str`，C 互操作要转换；选了 `char *`，`Str` 的方法用不上。这不是"多一个选择"，是**每个函数签名都要做一次不可逆的选择**，而且两个方向都要写文档与测试（Arena 密度：751 行/类型）。

#### 2.6.4 如果必须引入：门槛

一个 struct 对等类型只有在同时满足以下四条时才应进入排期，否则退回 `map trait` 或纯数据 struct：

1. 它**拥有**某个必须释放的东西，且现有 `char *` / `int` 宿主无法表达；
2. 它的释放有**显式 `impl Drop`**，且有夹具证明（按 §2.3 的 8 条逐一过）；
3. 它能给出**双向转换的明确所有权规则**（谁分配、谁释放、借用是否合法），并写进 README 的术语表；
4. 方法用**固有 `impl S { ... }`** 写，不用 `use Trait` + vptr——除非确实需要接口值（那就要按 `Arena` 的代价重新算一遍）。

不满足 1 的（`Char` / `Path` / 数组包装 / `Str` 的便利部分）一律不引入；满足 1 但过不了 2 的（`Dir` / `Fd` / `Socket`）用不透明别名 + 显式 `close`。

## 3. 成本被低估的部分

### 3.1 每个方法的固定开销（本仓实测密度）

基线：**86 个公开方法**、源码非注释 **388 行**、shim **849 行**、模块文档 **3,336 行**、测试 **1,801 行 / 27 夹具**。

| 口径 | 每方法 | 算法 |
| --- | --- | --- |
| 全包平均 | 4.5 行源码 + 9.9 行 shim + 38.8 行文档 + 20.9 行测试 | 388 / 849 / 3,336 / 1,801 ÷ 86 |
| 资源类密度（`std::fs`，7 方法） | 8.1 行 unit + **35.9 行 shim** + 47.6 行文档 + 31.3 行测试 ≈ **123 行/方法** | `Fs.m.ct` 57、`Fs.shim.h` 251、`fs/README.md` 333、`tests/fs/*` 219 |
| 纯映射密度（`std::num`，28 个宿主方法） | 4.0 + 4.7 + 11.3 + 9.4 ≈ **29 行/方法** | `Num.m.ct` 112、`Num.shim.h` 132、`num/README.md` 317、`tests/num/*` 264 |
| 自有类型密度（`Arena`，4 方法 + 1 类型） | **751 行/类型**（123 代码 + 399 文档 + 229 测试） | `Arena.h.ct`、`arena/README.md`、`tests/arena/*` |

每个方法还要额外付：查重（`README.md:143`）、`README.md:145-153` 表增行、`README.md:512` 与 §9 计数同步、失败路径夹具、一次版本 bump + tag + `ct verify` 摘要重算。

### 3.2 81 条的总量级

`docs/libc-inventory.md` 标"建议包"的：**81 条**（第一档 37、第二档 44，含可选/后续）。

| 口径 | 总量 |
| --- | --- |
| 纯映射 29 行/方法 | 81 × 29 ≈ **2,350 行** |
| 资源类 123 行/方法 | 81 × 123 ≈ **9,960 行** |
| 中位（≈70） | **≈ 5,700 行**，约等于现有仓全部内容（6,374 行）的 0.9 倍 |
| 7 个新类型（按 `Arena` 密度） | **≈ 5,200 行起**；`Socket` 有 10 个方法，还要再加 |

而本仓的既有比例是 **每 1 行源码配 2.7 行文档**（3,336 / 1,237）。按此推算，5,700 行新增会带来约 **15,000 行**总改动——现在整个仓才 6,374 行。

### 3.3 `char *` 名字：三重稀缺，而且账目已经错了

清单 §3.1 列了 18 个新增 `char *` 名字（`rfind_char`、`compare_ignore_case`、`parse_long`、`parse_llong`、`parse_ulong`、`parse_real`、`open_dir`、`open_fd`、`read_stat`、`read_link_stat`、`read_link`、`resize`、`set_mode`、`resolve_path`、`change_dir`、`parse_ip`、`is_readable`、`is_writable`），落地后 **25 → 43（+72%）**。

稀缺不是"数量上限"（`docs/libc-inventory.md:266` 自己也说没有上限），而是三层：

1. **语义唯一**：`find` 已占，`search` / `index_of` / `locate` 都只能算同义词；`open` 已占，所以目录只能叫 `open_dir`、fd 只能叫 `open_fd`（清单 §3.1 自己承认）。自然名字是有限的，同义词堆上去只会让可发现性更差。
2. **跨包抢占**：实测两个包同时定义 `parent` → 调用点直接编译失败。std 每占一个自然名，第三方就少一个。
3. **可发现性**：`char *` 上 43 个方法意味着每个字符串变量的补全都混着路径、编码、数值、文件系统四类语义；而补全无法按语义过滤（因为类型相同）。

而"先查表"依赖的表本身有三处矛盾（§7）：`README.md:139` 写 26、`README.md:358` 写 22、`README.md:512` 写 25，表内合计 25；`README.md:130` 的 `std::fs` 写 4，实际 7。**对方会说**：先把表修正就行。**但**：这说明"余量"从来不是数出来的，是估出来的；在 18 个新名字落地前必须先重算，否则下一个 issue 会继续基于错账做减法。

### 3.4 双份维护与一致性

铺宽之后，同一能力会有两种写法（§1.5），于是需要维护两套文档、两套样例、两套失败语义：

| 能力 | 底座（libc / 零代码映射） | 方法层 | 一致性风险 |
| --- | --- | --- | --- |
| 解析数字 | `strtol` + `errno` | `parse_long` + `-1`；既有 `to_int` **无错误报告** | 三种失败模型 |
| 判断存在 | `stat` + `errno` | `exists` → `0` / `1`（`EACCES` 也是 0） | 两态 vs 三态 |
| 错误查询 | `errno` | `FsEnv.get_last_error()`（per-TU） | 跨 TU 读到 0（实测） |
| 重命名 | `rename(src, dst)` | `path.rename(dst)` | 参数方向相反（实测都编译） |
| 求长度 | `strlen` | `s.len()` | 一致，但占一个名字 |
| 求平方根 | `sqrt` | `Math.sqrt` | 两套拼写长期并存（政策要求保留原名） |

`README.md:505-506` 明确"直映射保留原名"，所以双拼写不是过渡态，是**长期态**。长期态就要长期维护两套文档，而文档成本已经占本仓 2.7 倍源码（§3.1）。

### 3.5 包体积 / 编译期 / 运行时：这部分我认输

| 指标 | 实测 |
| --- | --- |
| 单调用点体积 | `strlen` 版 33,480 B vs `s.len()` 版 33,464 B |
| 单 TU 编译 | 0.31 / 0.35 / 0.38 s vs 0.34 / 0.34 / 0.37 s |
| 全门禁 27 夹具 | 19.16 s（≈0.71 s/夹具） |
| 导出符号 | 仅 2 个 vtable；`-Wall -Wextra -Werror` 消费者构建通过 |

所以"包装拖慢构建 / 撑大二进制 / 触发警告"这三条常见反对**在本仓不成立**，不应写进决策依据。真正的成本在 §3.1–3.4 与 §3.6。

### 3.6 版本演进：没有 ABI，但有 tag 税和"只留最新"

- shim 无导出符号 ⇒ 无 ABI 可谈，所有消费者都要源码重编。好处是不会二进制不兼容；代价是**没有"打补丁不重编"的路径**。
- 摘要覆盖包目录下除 `.git` / `vendor` / `.ctcache` 外所有文件，含 `README.md`、`tests/`、`docs/`；tag 后任何改动都要新版本 + 新 tag，否则 `ct verify` 报不一致（`README.md:499-501`）。`0.3.1` 就是为一句过期描述发的。
- 索引只收最高的 `v*` tag（`README.md:353`），旧版只能 `file://` 或手工 checkout ⇒ 消费者锁版本的成本很高。
- **新增一个 `char *` 方法名，对已经用了这个名字的第三方是破坏性变更**（实测歧义报错为证）。`0.x` 期间允许，但发布说明里必须逐名列出，否则消费者只会看到"突然编译失败"。

## 4. 替代方案

### 方案 A：底座 + 15 个高频方法（推荐）

- 保留现有 25 个，只按调用频次（§1.2）补 5–8 个最高频的（例如 `rfind_char`、`min`/`max`、`is_alnum`）。
- 其余 60+ 条**不做**，文档里给 libc 对照即可。
- 代价：几乎没有新增类型；`char *` 停在 30–33 个名字。
- 适用条件：认可"便利性按频次给，不按头文件给"。

### 方案 B：底座 + 资源类只做映射、不做新类型

- `Dir` 沿用 `IoFile` 的不透明指针先例（显式 `close`）；`Fd` / `Socket` 用 `int` + 自由函数；`FileStat` / `TimeParts` 直接导出 C struct（纯数据，无方法、无 vptr、无 `Drop`）。
- **不引入任何 struct 对等类型**（`Char` / `Str` / `Path` / 数组包装），方法层只挂 `map trait` 的既有宿主。
- 代价：没有自动 close；但 §2.3 已证明自动 close 本来就不可靠。约 1,500 行。
- 适用条件：真实痛点是"大目录列举"与"文件元数据"，且不接受 `Drop` 的新下限（0.48.0）。

### 方案 C：std 冻结，网络 / 进程另开瘦包

- `std` 停在 `0.3.x`；`net` / `process` 各自成包、各自 semver。
- **关键限制**：`char *` 名字空间是调用点级全局的，瘦包不能继续往 `char *` 挂方法，必须自带类型或 namespace；否则与 std 同名即歧义。
- 适用条件：网络/进程有明显独立的发布节奏与使用者。

### 方案 D：只写 recipe 对照表

- 把清单 §2 转成"libc → CT 写法"文档 + 3–5 个安全形态示例（有界拷贝、定长读、`errno` 分支）。零代码、零 tag 税。
- 代价：不修 `Fs.list` 定长缓冲、不修 `IoFile` 包装体泄漏。
- 适用条件：收益主要是"不用记拼写"。

> 注：把 `own` 放宽到接受方法调用（`language-spec.md:469-470`）是 ROI 最高的一条，但按本轮约束**不在 std 仓排期**，故不列为替代方案，只作为已知的将来项记录。

## 5. 如果必须做，怎么把风险压到最低

### 5.1 先做（4 项封顶）

| 序 | 项 | 理由 | 形态 |
| --- | --- | --- | --- |
| 1 | 修 `IoFile` 包装体回收 | `File.shim.h:43-53` 自认每次 open 漏 16 B；这是现存 bug | 映射（新增 `free` 出口）或换类型（破坏性，需单独一版） |
| 2 | `Dir` + `next` / `close` | `Fs.list` 定长缓冲是真缺陷（`fs/README.md:77`、`:194`） | 不透明指针 + 显式 close，**不加 Drop** |
| 3 | `FileStat`（大小 / 类型） | 读文件前必须猜 `cap` 的根因 | 导出 C struct + out 参函数 |
| 4 | 时间：`now_ns` / `format_time` | 真实需求，无资源，不需要生命周期 | namespace 函数（**不要挂到 `long` 上**） |

### 5.2 缓做（先给证据）

- `Fd`：与 `IoFile *` 重叠，先收集 3 个"必须 fd 层"的真实用例。
- `Socket` / DNS：独立域，按方案 C 另包。
- `Timer`：CT 无并发模型（清单 §2.6 自己也这么写）。
- `Float` / `Real` 的 `min` / `max`：搭车项，不单独排。

### 5.3 永不做

1. `ctype.h` 8 个 `is_*`（libc 已是母语，零安全增量）。
2. `math.h` 三角 / 对数 / 双曲补全（约 20 个名字，只换 import）。
3. `std::errno` 全局访问器 + `E*` 常量表（与 per-TU 快照冲突，必然非线程安全）。
4. `scanf` / `sprintf` / `fprintf` 族、`qsort` / `bsearch`、进程 / 信号 / locale / `mb*`：升格为**永久非目标**，不要留在需求池里制造"以后可能做"的幻觉。
5. 为同一 libc 调用提供第二个名字（`compare` 之后再来 `equals`）。
6. 把领域方法挂到基础类型上（`long.year()`、`char*.exists()` 之外的时间/货币类东西）。
7. 任何"为便利性引入的 struct 对等类型"（`Char` / `Str` / `Path` / 数组包装）。需要引入时按 §2.6.4 单列，由负责人评估，不与其他条目夹在同一相里。

### 5.4 合并门槛（每相都过）

- 先修 §7 的三处计数，再用一个 import 全部 trait 的 TU 编译，0 条歧义。
- 每个资源 API 三条夹具：正常、底层失败、句柄被别处保存/重新赋值。
- 用 `Drop` 的话：`requires` 抬到 `ct >= 0.48.0`；每个 wrapper 显式 `impl Drop`；必须有 `ulimit -n` 或 ASAN 夹具证明 fd / 内存回收。
- 跨 TU 错误传播必须有夹具（§2.4 的那一条）；文档不得把"`get_last_error` 可用"写成无条件承诺。
- 文档不得把"`Drop` 兜底"写成卖点，除非上一条的夹具在 CI 里跑。
- 凡引入 struct 对等类型，必须单独一卡、按 §2.6.4 三条门槛逐条给证据，且不得与"便利性方法"同相发布。

### 5.5 发布纪律

一相一 minor；文档与代码同提交；**不要**再出现 `0.3.1` 那种"一句过期文字再发一版"的补丁。

## 6. 可被推翻的条件（我就认输的清单）

| # | 条件 | 测量方法 | 判定 | 现状 |
| --- | --- | --- | --- | --- |
| F1 | 方法层的便利性确实被大量使用 | 统计真实 CT 代码（非夹具）的方法调用占比 | >50% 则铺宽有据 | 本仓夹具 554:24（96%），**倾向支持对方** |
| F2 | 新增名字不与生态冲突 | 两个包同名 `char *.parent` 同时 import | 0 条歧义 | **已失败**（实测报错） |
| F3 | `errno` 快照能跨 TU 传递 | 库函数返回 `-1`，调用方读快照 | 应得 17 | **已失败**（得 0） |
| F4 | 方法层能给出类型安全 | 路径/缓冲/JSON 互换编译 | 应被拒绝 | **已失败**（全部编过） |
| F5 | 领域方法不误伤同类型值 | `size.year()` | 应被拒绝 | **已失败**（返回 2023） |
| F6 | 每个句柄入口都能被 `own` 接管 | `own X *h = <入口>()` 编译 | 100% 通过 | **已失败**（方法/namespace 均报 L49-1） |
| F7 | `Drop` 能自动释放 C 资源 | `ulimit -n 32` + 循环开 wrapper | 200 次全成功 | **已失败**（19 次后 EMFILE） |
| F8 | `Drop` 不重复释放 / 不悬空 | 值拷贝、存全局、进容器三条夹具 | 0 次重复、0 次 UAF | **已失败**（3 次 DROP；`g->id=7131`） |
| F9 | 构建与体积不受影响 | 同程序对比、单 TU 计时 | ≤+20% | **通过**——我认输 |
| F10 | 文档 / 代码比不失控 | 行数比 | ≤4.0（现 2.70） | 未测；按 81 条估算会 >10 |
| F11 | 每个新 API 有底层失败夹具 | 统计 | 100% | 未测 |
| F12 | 发布后 3 个版本内仓外 ≥3 个消费者调用新 API | ct-pkg 索引 / 仓外检索 | <3 则该 API 应删 | 未测 |
| F13 | struct 对等类型的内存代价可接受 | `sizeof` 对比 + 数组 | 放大 ≤2 倍 | **分裂**：`use CharOps` 形态 16 B（16 倍，失败）；`impl Char2` 固有方法 1 B（通过）→ 必须规定只用固有方法 |
| F14 | struct 对等类型不破坏链式写法 | `factory().method()` | 编译通过 | **已失败**：`a method call on a temporary is only allowed at the start of a statement or of an assignment` |
| F15 | 对等 struct 与既有 trait 可同名共存 | 同命名空间声明 `trait Char` + `struct Char` | 生成的 C 可编译 | **已失败**：生成 `pkg_x_Char *_self = ...` 缺 `struct`，cc 报 3 错 |
| F16 | 对等体与底座之间的转换为零成本 | 数每个边界的分配/字段访问 | 0 次分配 | **已失败**：`char*` ↔ `Str` 需 `str_from`（分配+拷贝）或 `str_c_str`（借用），`char` ↔ `Char` 需 `.c` |

只要 F2–F8、F13–F16 保持"已失败"，**"方法层铺宽 + 新类型 + struct 对等体 + Drop 兜底"就不能作为一揽子方案进排期**；能争的是方案 B 的功能增量与方案 A 的少数高频名字。

反过来，我也给自己定了认输条件：

- 若 F1 在**真实代码**（排除夹具）里也 >50%，且 F2/F4/F5 证明不成立（即名字不冲突、用户确实不混用语义），那"铺宽"只是我的审美，我撤回 §1.3–1.5。
- 若 F6/F7/F8 被后续 ct 版本修好（`own` 接受方法调用、`use Drop` 对 C 资源生成 close、值拷贝变 move），我撤回 §2.3，且"新类型 + Drop"应成为资源类的首选形态。
- 若 CT 将来支持**无 vptr 的固有方法**或值类型方法（不再是 `use Trait` + 动态派发），F13/F14 的前提就不成立，我会重新评估 struct 对等体；但按本轮约束这不在 std 仓排期，只能登记为将来项。

## 7. 顺带发现的文档硬伤（影响论据可信度）

| 位置 | 问题 | 影响 |
| --- | --- | --- |
| `README.md:139` | 标题写 26 个方法，表内合计 25 | 余量高估 1 |
| `README.md:358` | 「避开 22 个已有名字」 | 少算 3；按它查重会漏 |
| `README.md:130` | `std::fs` 方法数 4 | 实际 7（`Fs.m.ct:31-38`）；模块索引合计 83，实际 86 |
| `README.md:348` | 「包单元里的 `Drop` 发射为 `static`，消费者不可见」 | 对 ct ≥ 0.48.0 不成立（`phase-log.md:62`；生成头带 `void <S>_Drop_drop(void *)` 真原型） |
| `README.md:245` | 「没有析构」 | 语言有 `Drop`，只是本包不用；应改为「本包不依赖 `Drop`」 |

**对方会说**：文档小错，不影响方向。**但**：本次决策的两条核心依据——"`char *` 还有余量"和"`Drop` 在包里不可用"——恰好各依赖一条错数据。方向可以不变，账本必须先重算。

## 8. 复现附录

环境：ct v0.53.0（`/Users/samxx/.local/bin/ct`），`cc` = 本机 clang；探针在 `/tmp/conarg-*`，不改本仓。

| 探针 | 代码要点 | 结果 |
| --- | --- | --- |
| 1 | `own Res *r = res_open(1);` + 无 own 对照 + `delete r` | Drop 在作用域尾 / `return` 前 / `delete` 时触发；无 own 只有 `warning: potential leak` |
| 2 | `own Res *r = tag.open_res("A");`（类型宿主） | `error: 'own' requires a malloc, calloc or factory call ...` |
| 3 | `own Res *r = pkg::res::Maker.make_res(5);`（namespace） | 同一条 L49-1 错 |
| 4 | `struct Pair { Res a; Res b; } use Drop;` + 两次值拷贝 + `delete` | DROP 打印 3 次 |
| 5 | `own Res *r = ...; store(r); }` 后读全局 | 离开块时 DROP，随后读到 7131（UAF） |
| 6 | `own Res *r = ...; r = res_open(...);` | 不 DROP，`warning: potential leak` |
| 7 | `goto` 跳出 `own` 作用域 | 不 DROP |
| 8 | `struct W { FILE *fp; } use Drop;` 无 impl；`ulimit -n 32` 循环 200 次 | drop 体为空；`opened=19` |
| 9 | `impl Drop for IoFile { ... }` | `error: impl for undefined struct 'IoFile'` |
| 10 | `Vec<Res *> v; v.push(...); v.release();` | 无 DROP |
| 11 | `char *json = "{}"; json.exists(); json.read_file(buf,16); buf.write_file("y");` | 全部编译通过 |
| 12 | `long ts = 1700000000; long size = 1700000000; ts.year(); size.year();` | 两者都返回 2023 |
| 13 | `b.rename(a);` 与 `rename(a, b);` | 都编译，方向相反 |
| 14 | 两个包各定义 `char *.parent`，同时 import | `ambiguous type-host map for method 'parent'` |
| 15 | `s.strlen()` 与 `strlen(s)` | 都编译，结果一致 |
| 16 | `Dep.c` 里 `mkdir` 失败；`main.c` 读 `get_last_error()` | cross-TU 得 **0**；同 TU 得 **17** |
| 17 | `strlen` vs `s.len()` 同程序；单 TU 计时 | 33,480 vs 33,464 B；0.31–0.38 s vs 0.34–0.37 s |
| 18 | `struct Char { char c; } use CharOps;` + `Char a[4]` | `sizeof(Char)=16`、`sizeof(char)=1`、`sizeof(a)=64` |
| 19 | `chain_new(1).get()`（工厂返回 struct 值） | `error: a method call on a temporary is only allowed at the start of a statement or of an assignment` |
| 20 | 同命名空间 `pub trait Char` + `pub struct Char` | 生成 `.c` 写 `pkg_x_Char *_self = (pkg_x_Char *)self;`（缺 `struct`），cc 报 3 错 |
| 21 | `char *` ↔ `Str`：`str_from` / `str_c_str` / `retain` | 规范口径：三者都**分配副本**（`Str.h.ct:37`、`:117-118`、`:132-133`），`str_c_str` 的返回值由调用方 `free`；未实跑 |
| 22 | `struct Char2 { char c; }` + 固有 `impl Char2 { is_digit() }` | `sizeof(Char2)=1`、`Char2[4]`=4 B；生成头是真原型、无 vptr |
| 23 | `chain2_new(9).get()`（固有方法、值返回） | 与 #19 同一条 rvalue 错误 |

关键构建命令：

```sh
cd /tmp/conarg-probe
CT=/Users/samxx/.local/bin/ct
STD_ROOT=<本仓根>
$CT build -I . probe5.ct  -o probe5.bin  --cflags "-Wall -Wextra"
$CT build -I . probe10.ct -o probe10.bin --cflags "-Wall -Wextra" -I "$STD_ROOT"

# 跨 TU 错误：
cd /tmp/conarg-errno
$CT build -I . -I "$STD_ROOT" main.ct -o main.bin --cflags "-Wall -Wextra"
./main.bin   # cross-TU first: rc=-1 err=0 / same-TU after: rc=-1 err=17
```
