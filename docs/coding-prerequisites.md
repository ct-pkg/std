# 编码前必读

| 项 | 值 |
| --- | --- |
| 用途 | 写 `std` 代码前的硬约束清单；规则从设计正文抽取，不另立新规 |
| 详规 | 命名 [§5.1](design/std-scope-and-presentation.md)、失败返回 [§5.3](design/std-scope-and-presentation.md)、所有权 [§5.4](design/std-scope-and-presentation.md)、三层与准入 [§1.1](design/std-scope-and-presentation.md)、③ 准入 [§1.5](design/std-scope-and-presentation.md)、指针不外泄 [§1.8](design/std-scope-and-presentation.md)、映射表 [§6.2](design/std-scope-and-presentation.md) |

## 1. 命名

`snake_case`；默认全称，只有扫一眼不用想就知道的缩写才留；数学记号保留原样；方法名不重复宿主或类型已交代的信息。

```ct
/* 正例 */
"abc".length();
9.0.sqrt();
"a".concat("b");
path.size();
path.modification_time();
```

```ct
/* 反例 */
"abc".len();             // 错：len 已整条替换为 length
9.0.square_root();       // 错：sqrt 是通用数学记号，保留原名
path.file_size();        // 错：宿主已是路径，file_ 重复类型信息
```

## 2. 只有对象式调用，没有裸函数入口

能力一律挂宿主；同一能力不另设自由函数入口。

```ct
/* 正例 */
buf.copy_from(src, n);
"abc".starts_with("a");
```

```ct
/* 反例 */
copy_from(buf, src, n);  // 错：裸函数入口，与对象式重复
```

## 3. 能力寄生在基础类型上（含工厂寄生）

先寄生 `char *`、`int`、`long`、`double` 等既有宿主；工厂从字符串起头，不新增路径/句柄包装。

```ct
/* 正例 */
"f".open_file("r");
"d".open_dir();
"zh_CN.UTF-8".make_locale();
```

```ct
/* 反例 */
FilePath p = file_path_new("f");   // 错：为路径另造类型
open_file(p, "r");                 // 错：工厂没有从字符串起头
```

## 4. ①/②/③ 三层与便利层准入

① 零代码映射；② 便利方法；③ 只给真持有资源。② 的门槛是“明显更短、读着更自然”，不是“能包就包”。

```ct
/* 正例 */
stat(&st);                         // ① 直连
"abc".starts_with("a");            // ② 明显更短
own File f = "a.txt".open_file("r"); // ③ 真持有 FILE *
```

```ct
/* 反例 */
struct Char { char c; };           // 错：①/② 足够，A1 已定不建 struct
"abc".is_lowercase();              // 错：ctype 名只做 snake_case 化，保留 is_lower
```

## 5. 资源对象由工厂出 `own`，释放留包内

工厂返回 `own` 句柄；离开作用域由包内关闭；显式 `close()` 只供提前关闭，幂等。

```ct
/* 正例 */
{
    own File f = "a.txt".open_file("r");
    char buf[64];
    f.read(buf, sizeof buf);
}   /* 包内关闭，调用方不 free */
```

```ct
/* 反例 */
FILE *f = fopen("a.txt", "r");     // 错：裸指针外泄
/* ... */
fclose(f);                         // 错：释放责任丢给调用方
```

## 6. 显式失败返回、部分读写与 `EINTR`

失败用显式返回值；读写按实际字节数循环；`EINTR` 重试，不假定一次读满。

```ct
/* 正例 */
char buf[4096];
int total = 0;
while (total < want) {
    int n = fd.read(buf + total, want - total);
    if (n < 0) {
        if (errno == EINTR) continue;
        break;
    }
    if (n == 0) break;
    total += n;
}
```

```ct
/* 反例 */
fd.read(buf, want);                // 错：忽略部分读
if (n < 0) return;                 // 错：没有区分 EINTR 与真实错误
```

## 7. 映射就是唯一实现

每个能力只有一个实现：`shim` 转发 libc，方法层复用 ①；便利层不复制逻辑。查证索引见 [§6.2](design/std-scope-and-presentation.md)。

```ct
/* 正例 */
int length() { return std_str_length(self); }
char *to_str(out, cap) { return std_num_to_str(self, out, cap); }
```

```ct
/* 反例 */
int length() {
    int n = 0;
    while (self[n]) n++;           // 错：绕过 ①，另写一套实现
    return n;
}
```
