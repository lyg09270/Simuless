#pragma once

/* =========================================================
 * UDP platform port layer
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

typedef SOCKET udp_socket_t;
#define udp_close closesocket

static inline int udp_last_error(void)
{
    return WSAGetLastError();
}

#define UDP_ERR_WOULD_BLOCK WSAEWOULDBLOCK
#define UDP_ERR_IN_PROGRESS WSAEWOULDBLOCK

static inline int udp_set_nonblock(udp_socket_t sock)
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

typedef int udp_socket_t;
#define udp_close lwip_close

static inline int udp_last_error(void)
{
    return errno;
}

#define UDP_ERR_WOULD_BLOCK EWOULDBLOCK
#define UDP_ERR_IN_PROGRESS EINPROGRESS

static inline int udp_set_nonblock(udp_socket_t s)
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

typedef int udp_socket_t;
#define udp_close close

static inline int udp_last_error(void)
{
    return errno;
}

#define UDP_ERR_WOULD_BLOCK EAGAIN
#define UDP_ERR_IN_PROGRESS EINPROGRESS

static inline int udp_set_nonblock(udp_socket_t s)
{
    int flags = fcntl(s, F_GETFL, 0);
    return fcntl(s, F_SETFL, flags | O_NONBLOCK);
}

#endif

/* =========================================================
 * Common helpers (platform independent)
 * ========================================================= */

static inline int udp_would_block(void)
{
    return udp_last_error() == UDP_ERR_WOULD_BLOCK;
}

static inline int udp_in_progress(void)
{
    return udp_last_error() == UDP_ERR_IN_PROGRESS;
}

/* =========================================================
 * Address abstraction
 *
 * transport_udp.c MUST NOT include sockaddr headers.
 * ========================================================= */

#include <string.h>

typedef struct
{
    struct sockaddr_storage storage;
    socklen_t len;
} udp_addr_t;

static inline int udp_addr_make(udp_addr_t* out, const char* ip, uint16_t port)
{
    memset(out, 0, sizeof(*out));

    struct sockaddr_in* a = (struct sockaddr_in*)&out->storage;

    a->sin_family = AF_INET;
    a->sin_port   = htons(port);

    if (ip)
    {
#if defined(_WIN32)
        if (InetPtonA(AF_INET, ip, &a->sin_addr) != 1)
            return -1;
#else
        if (inet_pton(AF_INET, ip, &a->sin_addr) != 1)
            return -1;
#endif
    }
    else
    {
        a->sin_addr.s_addr = htonl(INADDR_ANY);
    }

    out->len = sizeof(struct sockaddr_in);
    return 0;
}

static inline int udp_addr_equal(const udp_addr_t* lhs, const udp_addr_t* rhs)
{
    const struct sockaddr* la;
    const struct sockaddr* ra;

    if (lhs == NULL || rhs == NULL)
    {
        return 0;
    }

    la = (const struct sockaddr*)&lhs->storage;
    ra = (const struct sockaddr*)&rhs->storage;

    if (la->sa_family != ra->sa_family)
    {
        return 0;
    }

    if (la->sa_family == AF_INET)
    {
        const struct sockaddr_in* l4 = (const struct sockaddr_in*)la;
        const struct sockaddr_in* r4 = (const struct sockaddr_in*)ra;

        return (l4->sin_port == r4->sin_port) && (l4->sin_addr.s_addr == r4->sin_addr.s_addr);
    }

    return 0;
}