/*
 * Net.shim.h - the typed wrappers around Socket.shim.h for std::net.
 *
 * Socket.shim.h has the C core (fd in, fd out); this header adds the
 * struct-returning forms the mapping recipes need (connect / accept /
 * receive_from / send_message / receive_message) and the local_port helper.
 *
 * Including Socket.shim.h pulls in the generated FileDesc.h, so the struct tag
 * and `std_io_adopt_file_desc` are visible here.
 *
 * License: MIT (see LICENSE).
 */

/* The POSIX.1-2008 feature set: shims are the first headers in a generated
   consumer, so defining it here makes mkdir/readlink/telldir/fileno/setenv and
   friends visible under strict -std=c99 on glibc. */
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 700
#endif

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#ifndef CT_STD_NET_NET_SHIM_H
#define CT_STD_NET_NET_SHIM_H

#include "Socket.shim.h"
#include "FileDesc.h"

static inline struct std_io_FileDesc *std_net_connect_path(const char *host_port)
{
    int fd = std_net_connect_fd(host_port);

    if (fd < 0) return NULL;
    return std_io_adopt_file_desc(fd);
}

static inline struct std_io_FileDesc *std_net_accept_socket(void *sock)
{
    int fd = std_net_accept_fd(sock);

    if (fd < 0) return NULL;
    return std_io_adopt_file_desc(fd);
}

static inline int std_net_local_port(void *sock)
{
    char text[46];
    int port = -1;

    if (std_net_local_address(sock, text, (int)sizeof text, &port) != 0) return -1;
    return port;
}

static inline struct std_io_FileDesc *std_net_receive_from_socket(void *sock, char *buf, int cap,
                                                                  char *ip_out, int ip_cap,
                                                                  int *port, int *count)
{
    return (struct std_io_FileDesc *)std_net_receive_from(sock, buf, cap, ip_out, ip_cap, port, count);
}

static inline struct std_io_FileDesc *std_net_send_message_socket(void *sock, void *iov, int count)
{
    return (struct std_io_FileDesc *)std_net_send_message(sock, iov, count);
}

static inline struct std_io_FileDesc *std_net_receive_message_socket(void *sock, void *iov,
                                                                     int count, int *bytes)
{
    return (struct std_io_FileDesc *)std_net_receive_message(sock, iov, count, bytes);
}

#endif
