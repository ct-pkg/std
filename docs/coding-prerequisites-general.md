# 编码前必读（通用篇）

本清单只写对任何 CT 包都成立的条款，不绑定具体模块、方法表或本包名字账本。

## 1. 命名

`snake_case`；默认全称，只有扫一眼不用想就知道的缩写才留；数学记号保留原样；方法名不重复宿主或类型已交代的信息。

```ct
/* 正例 */
device.timeout();
circle.area();
connection.reconnect();
path.mode();
angle.sin();
```

```ct
/* 反例 */
device.tmo();              // 错：缩写
circle.area_of_circle();   // 错：重复宿主
path.file_mode();          // 错：重复对象的类型信息
angle.sine();              // 错：sin 是通用数学记号，保留原名
```

## 2. 只有对象式调用，没有裸函数入口

能力一律挂宿主；同一能力不另设自由函数入口。

```ct
/* 正例 */
buffer.copy_from(src, n);
item.refresh();
```

```ct
/* 反例 */
copy_from(buffer, src, n);  // 错：裸函数入口，与对象式重复
```

## 3. 能力寄生在基础类型上（含工厂寄生）

先寄生既有基础类型；工厂从字符串起头，不新增对象包装。

```ct
/* 正例 */
"config".load();
"dir".open_dir();
```

```ct
/* 反例 */
Resource r = resource_new("config");  // 错：为字符串另造类型
load(r);                              // 错：工厂没有从字符串起头
```

## 4. ①/②/③ 三层与便利层准入

① 零代码映射；② 便利方法；③ 只给真持有资源。② 的门槛是“明显更短、读着更自然”，不是“能包就包”。

```ct
/* 正例 */
native_call(&state);                 // ① 直连
"abc".starts_with("a");              // ② 明显更短
own Handle h = "res".open();         // ③ 真持有资源
```

```ct
/* 反例 */
struct Byte { int v; };              // 错：基础类型足够，不必建 struct
"abc".lowercase_text();              // 错：没有比 lowercase 更短或更自然
```

## 5. 资源对象由工厂出 `own`，释放留包内

工厂返回 `own` 句柄；离开作用域由包内关闭；显式 `close()` 只供提前关闭，幂等。

```ct
/* 正例 */
{
    own Handle h = "res".open();
    h.use();
}   /* 包内关闭，调用方不 free */
```

```ct
/* 反例 */
Handle *h = open_raw("res");   // 错：裸句柄外泄
/* ... */
close_raw(h);                  // 错：释放责任丢给调用方
```

## 6. 显式失败返回、部分读写与 `EINTR`

失败用显式返回值；读写按实际字节数循环；`EINTR` 重试，不假定一次读满。

```ct
/* 正例 */
char buf[4096];
int total = 0;
while (total < want) {
    int n = h.read(buf + total, want - total);
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
h.read(buf, want);          // 错：忽略部分读
if (n < 0) return;          // 错：没有区分 EINTR 与真实错误
```

## 7. 映射就是唯一实现

每个能力只有一个实现：`shim` 转发平台函数，方法层复用 ①；便利层不复制逻辑。

```ct
/* 正例 */
int width() { return platform_width(self); }
char *to_text(out, cap) { return platform_to_text(self, out, cap); }
```

```ct
/* 反例 */
int width() {
    int n = 0;
    while (self[n]) n++;     // 错：绕过 ①，另写一套实现
    return n;
}
```
