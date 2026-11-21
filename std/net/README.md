# `std::net` — TCP / UDP 套接字（`Socket` 是 `FileDesc` 的别名）

`Socket` 与 `std::io::FileDesc` 是同一类型：同一个 C tag、同一套方法、同一个 `Drop`。
本模块补上创建、绑定、监听、连接、收发与地址解析；不引入第二个套接字类型。

## 1. 定位与边界

- **`Socket` 不是新类型**：`pub type Socket = FileDesc;`。`accept` / `connect` 的返回值与
  `open_as_file_desc` 是同一类型，双向赋值不需要转换。
- **创建入口**：
  - `"host:port".connect()`（字符串起头）与 `std::net::connect("host:port")`（自由函数）；
  - `std::net::open_stream()`（未连接的 TCP）、`std::net::open_datagram()`（UDP）；
  - `server->accept()` / `std::net::accept_connection(server)`。
- **收发**：`FileDesc` 已有 `send` / `receive`（连接型，`receive` 字节数出参）；本模块补
  `send_to` / `receive_from`（UDP，来源地址出参）与 `send_message` / `receive_message`
  （调用方构造 `iovec`，仅连接型套接字）。`send` / `send_message` 返回接收者本身以便链式续接，
  失败留给 `std::errno`。
- **阻塞模型**：所有套接字都是阻塞的；`set_receive_timeout` / `set_send_timeout` 设置
  `SO_RCVTIMEO` / `SO_SNDTIMEO`，超时后调用返回 `-1` 且 `errno` 为 `EAGAIN` / `EWOULDBLOCK`。
  `getaddrinfo` 也是阻塞的，没有解析超时。
- **地址形态**：主机用 `char *`、端口用 `int`，不建 `Address` 类型。`"host:port"` 在最后一个
  `:` 处切分；IPv4 字面量与主机名都支持，IPv6 字面量不走这个拼写。
- **不做 SIGPIPE 自杀**：创建套接字时在支持的平台设 `SO_NOSIGPIPE`，Linux 的 `send` 走
  `MSG_NOSIGNAL`；对端关闭不会杀掉进程。
- **释放协议**：`Socket *s = ...; defer { delete s; }`。`own Socket *s` 不可用（别名的字面名
  不满足 `own` 检查），用 `own FileDesc *s`。

## 2. 依赖与安装

依赖 `<sys/socket.h>` / `<netinet/in.h>` / `<arpa/inet.h>` / `<netdb.h>` / `<sys/uio.h>`；
`requires ct >= 0.67.0`。

| 文件 | 内容 |
| --- | --- |
| `Socket.h.ct` | `pub type Socket = FileDesc;` + 自由工厂 `connect` / `open_stream` / `open_datagram` / `accept_connection` |
| `Net.m.ct` | `Net`（namespace：`resolve_host`）、`ConnectText`（`"h:p".connect`）、`IpText`（`parse_ip`）、`SocketOps`（绑定/监听/accept/超时/收发/消息） |
| `Socket.shim.h` | C 核心：`getaddrinfo` / `connect` / `bind` / `listen` / `accept` / `sendto` / `recvfrom` / `sendmsg` / `recvmsg` / `setsockopt` |
| `Net.shim.h` | 映射配方用的 typed 包装（返回 `FileDesc *` 的 connect / accept / receive_from 等） |

引入：`import std::net::Socket;`（`Net.m.ct` 由它带入）。

## 3. 快速开始

TCP 回环：

```ct
import std::net::Socket;
import std::num::Num;
import std::str::CharStr;

int main(void)
{
    Socket *server = std::net::open_stream();
    Socket *client;
    Socket *conn;
    char addr[64];
    char port[16];
    char buf[32];
    int n = 0;
    int p;

    defer { delete server; }
    server->set_reuse_address();
    server->bind_to(0);
    p = server->local_port();
    p.to_str(port, 16);
    "127.0.0.1:".concat(port, addr, 64);

    client = addr.connect();
    defer { delete client; }
    conn = server->accept();
    defer { delete conn; }

    client->send("ping", 4);
    conn->receive(buf, 32, &n);
    return n == 4 ? 0 : 1;
}
```

UDP 回环（片段，省略错误检查）：

```ct
import std::net::Socket;

Socket *a = std::net::open_datagram();
Socket *b = std::net::open_datagram();
char ip[32];
int port;
char buf[32];
int n = 0;

defer { delete a; }
defer { delete b; }
a->bind_to(0);
b->bind_to(0);
b->send_to("udp", 3, "127.0.0.1", a->local_port());
a->receive_from(buf, 32, ip, 32, &port, &n);
```

## 4. API 全表

### 4.1 自由工厂 / 地址

| 入口 | 签名 | 语义 | 失败 |
| --- | --- | --- | --- |
| `std::net::connect` | `Socket *connect(const char *host_port)` | TCP 连接，`own` 可用 | `0` |
| `std::net::open_stream` | `Socket *open_stream()` | 未连接 TCP 套接字 | `0` |
| `std::net::open_datagram` | `Socket *open_datagram()` | UDP 套接字 | `0` |
| `std::net::accept_connection` | `Socket *accept_connection(Socket *server)` | 接受一个连接 | `0` |
| `path.connect` | `Socket *path.connect()` | 字符串工厂，同一类型 | `0` |
| `path.parse_ip` | `int path.parse_ip(char *out)` | 校验并规范化 IPv4 文本 | `-1` |
| `std::net::Net.resolve_host` | `int resolve_host(const char *host, char *out, int cap)` | 解析首个 IPv4 地址 | `-1` |

`parse_ip` 的 `out` 至少 16 字节；`resolve_host` 的 `cap` 含 NUL。

### 4.2 `Socket *` 上的网络方法（`SocketOps`）

| 方法 | 签名 | 语义 | 失败 |
| --- | --- | --- | --- |
| `bind_to` | `int s->bind_to(int port)` | 绑 `INADDR_ANY:port`；0 为临时端口 | `-1` |
| `bind_to_address` | `int s->bind_to_address(const char *ip, int port)` | 绑指定 IPv4 | `-1` |
| `listen_backlog` | `int s->listen_backlog(int n)` | `listen` | `-1` |
| `accept` | `Socket *s->accept()` | 接受连接，返回新句柄 | `0` |
| `local_address` | `int s->local_address(char *out, int cap, int *port)` | `getsockname` | `-1` |
| `local_port` | `int s->local_port()` | 本地端口 | `-1` |
| `shutdown_both` / `shutdown_read` / `shutdown_write` | `int s->shutdown_*()` | `shutdown(2)` | `-1` |
| `set_option` | `int s->set_option(int level, int name, void *value, int len)` | 原生 `setsockopt` | `-1` |
| `set_reuse_address` | `int s->set_reuse_address()` | `SO_REUSEADDR` | `-1` |
| `set_receive_timeout` | `int s->set_receive_timeout(int ms)` | `SO_RCVTIMEO`；0 清除 | `-1` |
| `set_send_timeout` | `int s->set_send_timeout(int ms)` | `SO_SNDTIMEO`；0 清除 | `-1` |
| `send_to` | `int s->send_to(const char *data, int n, const char *ip, int port)` | UDP 发送，返回字节数 | `-1` |
| `receive_from` | `Socket *s->receive_from(char *buf, int cap, char *ip_out, int ip_cap, int *port, int *count)` | UDP 接收，来源地址/端口/字节数出参 | `count = -1` |
| `send_message` | `Socket *s->send_message(void *iov, int count)` | `sendmsg`，仅连接型 | 链式，错误看 `errno` |
| `receive_message` | `Socket *s->receive_message(void *iov, int count, int *bytes)` | `recvmsg`，仅连接型 | `bytes = -1` |

连接型收发复用 `FileDesc`：`send(data, n)` 返回接收者、`receive(buf, cap, &n)` 返回接收者并写字节数。

## 5. 与相邻模块的关系

- `std::io::FileDesc` 是类型定义处；`Socket` 只声明别名。
- `raw_file_desc()` 把 fd 交给 `poll` / `select` / `mmap` / 原生 `setsockopt`。
- 错误码用 `std::errno::Errno.get_last()`；超时是 `EAGAIN` / `EWOULDBLOCK`。
- `std::time::Timer` 可用于给阻塞收发设上限（信号 + 超时），但套接字本身不感知定时器。

## 6. 限制与未支持

- **阻塞 + 无 DNS 超时**：`getaddrinfo` 不可中断；`connect` 没有 Happy-Eyeballs、没有非阻塞模式。
- **IPv4 为主**：`"host:port"` 不支持 IPv6 字面量；`parse_ip` / `resolve_host` 只处理 IPv4。
- **`send_message` / `receive_message` 仅连接型**：未连接的 UDP 套接字用 `send_to` /
  `receive_from`；`iovec` 由调用方构造并负责其缓冲区生命周期。
- **`set_option` 是原生入口**：`level` / `name` 常量来自平台头文件，`.ct` 不能 `#include`；
  常用项走 `set_reuse_address` / `set_*_timeout`。
- **`accept` 的返回值需要释放**：`defer { delete c; }`；`accept` 不接受 `sockaddr` 出参。
- **别名不能用于 `own`**：`own FileDesc *s = std::net::connect(...)` 或 `defer delete`。
- **不做 TLS / HTTP / DNS 缓存**：这些都是上层包的事。

## 7. 版本与变更记录

| 版本 | 状态 | 变更 |
| --- | --- | --- |
| `0.11.0` | 本轮变更 | 补全 TCP + UDP：`connect` / `open_stream` / `open_datagram` / `accept_connection`，`bind_to` / `bind_to_address` / `listen_backlog` / `accept` / `local_address` / `local_port`，`shutdown_*`、`set_option` / `set_reuse_address` / `set_*_timeout`，`send_to` / `receive_from`、`send_message` / `receive_message`、`resolve_host`、`parse_ip`。 |
| `0.10.0` | 已发布 | `Socket = FileDesc` 别名。 |

编译器下限 `ct >= 0.67.0`。
