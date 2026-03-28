#pragma once

/* =========================================================
 * TCP platform port layer
 *
 * Supports:
 *   - Windows (WinSock)
 *   - POSIX (Linux/macOS)
 *   - lwIP socket API
 *
 * This file is the ONLY place allowed to include
 * platform socket / inet / errno headers.
 * ========================================================= */

#include <stdint.h>

/* =========================================================
 * Platform selection
 * ========================================================= */

#ifdef _WIN32

/* ================= Windows ================= */

#include <winsock2.h>
#include <ws2tcpip.h>

typedef SOCKET tcp_socket_t;
#define tcp_close closesocket

static inline int tcp_last_error(void)
{
    return WSAGetLastError();
}

#define TCP_ERR_WOULD_BLOCK WSAEWOULDBLOCK
#define TCP_ERR_IN_PROGRESS WSAEWOULDBLOCK

static inline int tcp_set_nonblock(tcp_socket_t sock)
{
    u_long mode = 1;
    return ioctlsocket(sock, FIONBIO, &mode);
}

#elif defined(LWIP_SOCKET)

/* ================= lwIP ================= */

#include "lwip/errno.h"
#include "lwip/fcntl.h"
#include "lwip/inet.h"
#include "lwip/sockets.h"

typedef int tcp_socket_t;
#define tcp_close lwip_close

static inline int tcp_last_error(void)
{
    return errno;
}

#define TCP_ERR_WOULD_BLOCK EWOULDBLOCK
#define TCP_ERR_IN_PROGRESS EINPROGRESS

static inline int tcp_set_nonblock(tcp_socket_t s)
{
    int flags = lwip_fcntl(s, F_GETFL, 0);
    return lwip_fcntl(s, F_SETFL, flags | O_NONBLOCK);
}

#else

/* ================= POSIX (Linux/macOS) ================= */

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

typedef int tcp_socket_t;
#define tcp_close close

static inline int tcp_last_error(void)
{
    return errno;
}

#define TCP_ERR_WOULD_BLOCK EAGAIN
#define TCP_ERR_IN_PROGRESS EINPROGRESS

static inline int tcp_set_nonblock(tcp_socket_t s)
{
    int flags = fcntl(s, F_GETFL, 0);
    return fcntl(s, F_SETFL, flags | O_NONBLOCK);
}

#endif

/* =========================================================
 * Common helpers (platform independent)
 * ========================================================= */

static inline int tcp_would_block(void)
{
    return tcp_last_error() == TCP_ERR_WOULD_BLOCK;
}

static inline int tcp_in_progress(void)
{
    return tcp_last_error() == TCP_ERR_IN_PROGRESS;
}

/* =========================================================
 * Address abstraction
 *
 * transport_tcp.c MUST NOT include sockaddr headers.
 * ========================================================= */

#include <string.h>

typedef struct
{
    struct sockaddr_storage storage;
    socklen_t len;
} tcp_addr_t;

static inline void tcp_addr_make(tcp_addr_t* out, const char* ip, uint16_t port)
{
    memset(out, 0, sizeof(*out));

    struct sockaddr_in* a = (struct sockaddr_in*)&out->storage;

    a->sin_family = AF_INET;
    a->sin_port   = htons(port);

    if (ip)
    {
        a->sin_addr.s_addr = inet_addr(ip);
    }
    else
    {
        a->sin_addr.s_addr = INADDR_ANY;
    }

    out->len = sizeof(struct sockaddr_in);
}