/*
 * Socket.shim.h - the C core of std::net.
 *
 * Everything here works on raw fds or strings; the CT layer wraps the result
 * with `adopt_file_desc`. The generated FileDesc.h is included for that
 * prototype (and for the struct tag used by Net.shim.h).
 *
 * Portability and failure notes:
 *   - `SO_NOSIGPIPE` is set on every socket where the platform has it (macOS);
 *     on Linux `send` uses MSG_NOSIGNAL. A peer closing must not kill us.
 *   - `getaddrinfo` is blocking and has no timeout; only the first address is
 *     tried after a successful connect, and the whole call fails on the first
 *     error (no Happy-Eyeballs).
 *   - `host:port` is split at the last ':'. IPv6 literals are not supported in
 *     that spelling (use bind_to_address for IPv4).
 *   - `set_receive_timeout` / `set_send_timeout` take milliseconds; a timed-out
 *     call returns -1 and leaves EAGAIN/EWOULDBLOCK in errno.
 *
 * License: MIT (see LICENSE).
 */

#ifndef CT_STD_NET_SOCKET_SHIM_H
#define CT_STD_NET_SOCKET_SHIM_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "FileDesc.h"

static inline int std_net_nosigpipe(int fd)
{
#ifdef SO_NOSIGPIPE
    int one = 1;

    setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &one, (socklen_t)sizeof one);
#endif
    (void)fd;
    return 0;
}

/* Split "host:port" at the last ':'. Returns 0 on success. */
static inline int std_net_split_host_port(const char *text, char *host, int host_cap, int *port)
{
    const char *colon;
    size_t host_len;
    long value;
    char *end;

    if (!text || !*text || !host || host_cap <= 0 || !port) return -1;
    colon = strrchr(text, ':');
    if (!colon || colon == text || !colon[1]) return -1;
    host_len = (size_t)(colon - text);
    if ((int)host_len + 1 > host_cap) return -1;
    memcpy(host, text, host_len);
    host[host_len] = 0;
    value = strtol(colon + 1, &end, 10);
    if (!end || *end || value <= 0 || value > 65535) return -1;
    *port = (int)value;
    return 0;
}

static inline int std_net_connect_fd(const char *host_port)
{
    struct addrinfo hints;
    struct addrinfo *result = NULL;
    struct addrinfo *it;
    char host[256];
    char service[16];
    int port;
    int fd = -1;

    if (std_net_split_host_port(host_port, host, (int)sizeof host, &port) != 0) {
        errno = EINVAL;
        return -1;
    }
    snprintf(service, sizeof service, "%d", port);
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host, service, &hints, &result) != 0) return -1;
    for (it = result; it; it = it->ai_next) {
        fd = socket(it->ai_family, it->ai_socktype, it->ai_protocol);
        if (fd < 0) continue;
        std_net_nosigpipe(fd);
        if (connect(fd, it->ai_addr, it->ai_addrlen) == 0) break;
        close(fd);
        fd = -1;
    }
    freeaddrinfo(result);
    return fd;
}

static inline int std_net_stream_fd(void)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    if (fd >= 0) std_net_nosigpipe(fd);
    return fd;
}

static inline int std_net_datagram_fd(void)
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (fd >= 0) std_net_nosigpipe(fd);
    return fd;
}

static inline int std_net_fd_of(void *sock)
{
    return std_io_FileDesc_raw_file_desc((struct std_io_FileDesc *)sock);
}

static inline int std_net_bind_any(void *sock, int port)
{
    struct sockaddr_in addr;
    int fd = std_net_fd_of(sock);

    if (fd < 0 || port < 0 || port > 65535) return -1;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((unsigned short)port);
    return bind(fd, (struct sockaddr *)&addr, sizeof addr) == 0 ? 0 : -1;
}

static inline int std_net_bind_address(void *sock, const char *ip, int port)
{
    struct sockaddr_in addr;
    int fd = std_net_fd_of(sock);

    if (fd < 0 || !ip || port < 0 || port > 65535) return -1;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1) {
        errno = EINVAL;
        return -1;
    }
    return bind(fd, (struct sockaddr *)&addr, sizeof addr) == 0 ? 0 : -1;
}

static inline int std_net_listen(void *sock, int backlog)
{
    int fd = std_net_fd_of(sock);

    if (fd < 0 || backlog < 0) return -1;
    return listen(fd, backlog) == 0 ? 0 : -1;
}

static inline int std_net_accept_fd(void *sock)
{
    int fd = std_net_fd_of(sock);
    int client;

    if (fd < 0) return -1;
    client = accept(fd, NULL, NULL);
    if (client >= 0) std_net_nosigpipe(client);
    return client;
}

/* Local address of a bound socket. */
static inline int std_net_local_address(void *sock, char *out, int cap, int *port)
{
    struct sockaddr_in addr;
    socklen_t len = sizeof addr;
    int fd = std_net_fd_of(sock);
    char text[INET_ADDRSTRLEN];

    if (fd < 0 || !out || cap <= 0 || !port) return -1;
    if (getsockname(fd, (struct sockaddr *)&addr, &len) != 0) return -1;
    if (!inet_ntop(AF_INET, &addr.sin_addr, text, sizeof text)) return -1;
    if ((int)strlen(text) + 1 > cap) return -1;
    strcpy(out, text);
    *port = (int)ntohs(addr.sin_port);
    return 0;
}

static inline int std_net_shutdown(void *sock, int how)
{
    int fd = std_net_fd_of(sock);

    if (fd < 0) return -1;
    return shutdown(fd, how) == 0 ? 0 : -1;
}

static inline int std_net_set_option(void *sock, int level, int name, void *value, int length)
{
    int fd = std_net_fd_of(sock);

    if (fd < 0 || !value || length < 0) return -1;
    return setsockopt(fd, level, name, value, (socklen_t)length) == 0 ? 0 : -1;
}

static inline int std_net_set_reuse_address(void *sock)
{
    int fd = std_net_fd_of(sock);
    int one = 1;

    if (fd < 0) return -1;
    return setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, (socklen_t)sizeof one) == 0 ? 0 : -1;
}

static inline int std_net_set_timeout(void *sock, int which, int milliseconds)
{
    int fd = std_net_fd_of(sock);
    struct timeval tv;

    if (fd < 0 || milliseconds < 0) return -1;
    tv.tv_sec = milliseconds / 1000;
    tv.tv_usec = (milliseconds % 1000) * 1000;
    return setsockopt(fd, SOL_SOCKET, which, &tv, (socklen_t)sizeof tv) == 0 ? 0 : -1;
}

static inline int std_net_set_receive_timeout(void *sock, int milliseconds)
{
    return std_net_set_timeout(sock, SO_RCVTIMEO, milliseconds);
}

static inline int std_net_set_send_timeout(void *sock, int milliseconds)
{
    return std_net_set_timeout(sock, SO_SNDTIMEO, milliseconds);
}

static inline int std_net_send_to(void *sock, const char *data, int n, const char *ip, int port)
{
    struct sockaddr_in addr;
    int fd = std_net_fd_of(sock);
    int flags = 0;

    if (fd < 0 || !data || n < 0 || !ip || port <= 0 || port > 65535) return -1;
    memset(&addr, 0, sizeof addr);
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1) {
        errno = EINVAL;
        return -1;
    }
#ifdef MSG_NOSIGNAL
    flags = MSG_NOSIGNAL;
#endif
    return (int)sendto(fd, data, (size_t)n, flags, (struct sockaddr *)&addr, sizeof addr);
}

static inline void *std_net_receive_from(void *sock, char *buf, int cap,
                                         char *ip_out, int ip_cap, int *port, int *count)
{
    struct sockaddr_in addr;
    socklen_t len = sizeof addr;
    char text[INET_ADDRSTRLEN];
    int fd = std_net_fd_of(sock);
    int n;

    if (count) *count = -1;
    if (fd < 0 || !buf || cap < 0) return sock;
    memset(&addr, 0, sizeof addr);
    n = (int)recvfrom(fd, buf, (size_t)cap, 0, (struct sockaddr *)&addr, &len);
    if (n < 0) return sock;
    if (count) *count = n;
    if (port) *port = (int)ntohs(addr.sin_port);
    if (ip_out && ip_cap > 0) {
        if (inet_ntop(AF_INET, &addr.sin_addr, text, sizeof text) &&
            (int)strlen(text) + 1 <= ip_cap) {
            strcpy(ip_out, text);
        } else {
            ip_out[0] = 0;
        }
    }
    return sock;
}

static inline void *std_net_send_message(void *sock, void *iov, int count)
{
    struct msghdr msg;
    int fd = std_net_fd_of(sock);
    int flags = 0;

    if (fd < 0 || !iov || count <= 0) return sock;
    memset(&msg, 0, sizeof msg);
    msg.msg_iov = (struct iovec *)iov;
    msg.msg_iovlen = (size_t)count;
#ifdef MSG_NOSIGNAL
    flags = MSG_NOSIGNAL;
#endif
    sendmsg(fd, &msg, flags);
    return sock;
}

static inline void *std_net_receive_message(void *sock, void *iov, int count, int *bytes)
{
    struct msghdr msg;
    int fd = std_net_fd_of(sock);
    int n;

    if (bytes) *bytes = -1;
    if (fd < 0 || !iov || count <= 0) return sock;
    memset(&msg, 0, sizeof msg);
    msg.msg_iov = (struct iovec *)iov;
    msg.msg_iovlen = (size_t)count;
    n = (int)recvmsg(fd, &msg, 0);
    if (n >= 0 && bytes) *bytes = n;
    return sock;
}

/* Resolve to the first IPv4 address as text. 0 on success, -1 otherwise. */
static inline int std_net_resolve_host(const char *host, char *out, int cap)
{
    struct addrinfo hints;
    struct addrinfo *result = NULL;
    struct addrinfo *it;
    char text[INET_ADDRSTRLEN];
    int rc = -1;

    if (!host || !*host || !out || cap <= 0) return -1;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host, NULL, &hints, &result) != 0) return -1;
    for (it = result; it; it = it->ai_next) {
        struct sockaddr_in *addr = (struct sockaddr_in *)it->ai_addr;
        if (!inet_ntop(AF_INET, &addr->sin_addr, text, sizeof text)) continue;
        if ((int)strlen(text) + 1 > cap) continue;
        strcpy(out, text);
        rc = 0;
        break;
    }
    freeaddrinfo(result);
    return rc;
}

/* Normalise an IPv4 literal into dotted text: 0 on success, -1 if invalid. */
static inline int std_net_parse_ip(const char *text, char *out)
{
    struct in_addr addr;

    if (!text || !out) return -1;
    if (inet_pton(AF_INET, text, &addr) != 1) return -1;
    if (!inet_ntop(AF_INET, &addr, out, INET_ADDRSTRLEN)) return -1;
    return 0;
}

#endif
