/*
 * Socket.shim.h - declarations for the std::net C core.
 *
 * The implementation is in Net.shim.c (a separate translation unit) so the
 * POSIX feature-test macro is set before any system header. This header is
 * included by Socket.h.ct and Net.shim.h; it deliberately exposes no system
 * types.
 *
 * License: MIT (see LICENSE).
 */

/* The POSIX.1-2008 feature set: shims are the first headers in a generated
   consumer, so defining it here makes mkdir/readlink/telldir/fileno/setenv and
   friends visible under strict -std=c99 on glibc. */
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#ifndef CT_STD_NET_SOCKET_SHIM_H
#define CT_STD_NET_SOCKET_SHIM_H

int std_net_connect_fd(const char *host_port);
int std_net_stream_fd(void);
int std_net_datagram_fd(void);
int std_net_bind_any(void *sock, int port);
int std_net_bind_address(void *sock, const char *ip, int port);
int std_net_listen(void *sock, int backlog);
int std_net_accept_fd(void *sock);
int std_net_local_address(void *sock, char *out, int cap, int *port);
int std_net_shutdown(void *sock, int how);
int std_net_set_option(void *sock, int level, int name, void *value, int length);
int std_net_set_reuse_address(void *sock);
int std_net_set_receive_timeout(void *sock, int milliseconds);
int std_net_set_send_timeout(void *sock, int milliseconds);
int std_net_send_to(void *sock, const char *data, int n, const char *ip, int port);
void *std_net_receive_from(void *sock, char *buf, int cap, char *ip_out, int ip_cap, int *port, int *count);
void *std_net_send_message(void *sock, void *iov, int count);
void *std_net_receive_message(void *sock, void *iov, int count, int *bytes);
int std_net_resolve_host(const char *host, char *out, int cap);
int std_net_parse_ip(const char *text, char *out);

#endif
