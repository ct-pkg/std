# `std::search` — 带比较器的通用算法（`Search`）

C 的 `qsort` / `bsearch` 需要比较器；`std::search` 只做这一层转发，不引入新类型、不分配内存。

## 1. 定位与边界

- 覆盖两个算法：`sort`（`qsort`）与 `binary_search`（`bsearch`）。
- 数组以 `char *` + 元素字节数传入，因此对任意元素类型都适用。
- 比较器是 C 的函数指针：`int cmp(const void *a, const void *b)`。CT 侧把函数名当 `void *` 传入。
- **不做**：闭包、带上下文的比较器、稳定排序、自定义容器。需要这些时应在使用者侧实现。

## 2. 依赖与安装

入口：`import std::search::Search;`。无第三方依赖，只需要 C 标准库。

## 3. 快速开始

```ct
import std::search::Search;
import std::test::Test;

int cmp_int(const void *a, const void *b)
{
    int x = *(const int *)a;
    int y = *(const int *)b;
    return (x > y) - (x < y);
}

int main(void)
{
    int v[3];
    char *base = (char *)v;
    int key = 20;

    v[0] = 30; v[1] = 10; v[2] = 20;
    std::search::Search.sort(base, 3, 4, cmp_int);
    if (std::search::Search.binary_search(base, 3, 4, &key, cmp_int) != 1) return 1;
    return 0;
}
```

## 4. API

| 方法 | 签名 | 返回 |
| --- | --- | --- |
| `sort` | `void sort(char *base, int count, int elem_size, void *cmp)` | 无；原地排序 |
| `binary_search` | `int binary_search(char *base, int count, int elem_size, void *key, void *cmp)` | 元素下标；找不到或空数组返回 `-1` |

- `count` 是元素个数，`elem_size` 是单个元素的字节数（用 `sizeof` 或字面量）。
- `binary_search` 要求数组已用**同一个**比较器排好序。
- `count <= 1`、`elem_size <= 0` 或比较器为空时，`sort` 什么也不做。
- 比较器必须满足 `qsort` 的三态约定：负数、0、正数。

## 5. 回调契约

| 项 | 约定 |
| --- | --- |
| 签名 | `int cmp(const void *a, const void *b)` |
| 传参 | 直接写函数名，CT 把它当 `void *` |
| 解引用 | 比较器内部先转成真实元素指针再读 |
| 上下文 | 没有 `void *ctx` 参数；需要外部状态时用文件作用域变量（非线程安全） |

## 6. 限制

- 排序不稳定；相等元素的相对顺序不保证。
- 不检查数组边界之外的任何内容，`count` / `elem_size` 由调用方保证正确。
- 比较器为 `NULL` 时 `binary_search` 返回 `-1`，`sort` 不动数组。
- 本模块不占用 `char *` 上的方法名：两个方法挂在 namespace 宿主上。
