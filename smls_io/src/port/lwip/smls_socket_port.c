/**
 * @file smls_socket_port_lwip.c
 * @brief lwIP implementation for socket port.
 */

#include "smls_socket_port.h"

#ifdef SMLS_USE_LWIP

#include <errno.h>
#include <stdint.h>
#include <string.h>

#include <lwip/inet.h>
#include <lwip/ioctl.h>
#include <lwip/sockets.h>
#include <lwip/tcp.h>

/* =========================================================
 * Native mirror wrappers
 * ========================================================= */

smls_socket_t smls_socket(int domain, int type, int protocol)
{
    return (smls_socket_t)lwip_socket(domain, type, protocol);
}

int smls_bind(smls_socket_t sock, const smls_sockaddr_t* addr, smls_socklen_t addrlen)
{
    return lwip_bind((int)sock, (const struct sockaddr*)addr, addrlen);
}

int smls_listen(smls_socket_t sock, int backlog)
{
    return lwip_listen((int)sock, backlog);
}

smls_socket_t smls_accept(smls_socket_t sock, smls_sockaddr_t* addr, smls_socklen_t* addrlen)
{
    return (smls_socket_t)lwip_accept((int)sock, (struct sockaddr*)addr, addrlen);
}

int smls_connect(smls_socket_t sock, const smls_sockaddr_t* addr, smls_socklen_t addrlen)
{
    return lwip_connect((int)sock, (const struct sockaddr*)addr, addrlen);
}

ssize_t smls_send(smls_socket_t sock, const void* buf, size_t len, int flags)
{
    return lwip_send((int)sock, buf, len, flags);
}

ssize_t smls_recv(smls_socket_t sock, void* buf, size_t len, int flags)
{
    return lwip_recv((int)sock, buf, len, flags);
}

ssize_t smls_sendto(smls_socket_t sock, const void* buf, size_t len, int flags,
                    const smls_sockaddr_t* addr, smls_socklen_t addrlen)
{
    return lwip_sendto((int)sock, buf, len, flags, (const struct sockaddr*)addr, addrlen);
}

ssize_t smls_recvfrom(smls_socket_t sock, void* buf, size_t len, int flags, smls_sockaddr_t* addr,
                      smls_socklen_t* addrlen)
{
    return lwip_recvfrom((int)sock, buf, len, flags, (struct sockaddr*)addr, addrlen);
}

int smls_close(smls_socket_t sock)
{
    return lwip_close((int)sock);
}

int smls_setsockopt(smls_socket_t sock, int level, int optname, const void* optval,
                    smls_socklen_t optlen)
{
    return lwip_setsockopt((int)sock, level, optname, optval, optlen);
}

int smls_getsockopt(smls_socket_t sock, int level, int optname, void* optval,
                    smls_socklen_t* optlen)
{
    return lwip_getsockopt((int)sock, level, optname, optval, optlen);
}

/* =========================================================
 * Helper APIs
 * ========================================================= */

int smls_set_nonblock(smls_socket_t sock, int enable)
{
    int on = (enable != 0) ? 1 : 0;

    return lwip_ioctl((int)sock, FIONBIO, &on);
}

int smls_set_options(smls_socket_t sock, uint32_t options)
{
    const int enable = 1;

    if ((options & SMLS_SOCKET_OPT_REUSEADDR) != 0u)
    {
        if (smls_setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0)
        {
            return -1;
        }
    }

    if ((options & SMLS_SOCKET_OPT_KEEPALIVE) != 0u)
    {
        if (smls_setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &enable, sizeof(enable)) < 0)
        {
            return -1;
        }
    }

    if ((options & SMLS_SOCKET_OPT_BROADCAST) != 0u)
    {
        if (smls_setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &enable, sizeof(enable)) < 0)
        {
            return -1;
        }
    }

    if ((options & SMLS_SOCKET_OPT_TCP_NODELAY) != 0u)
    {
        if (smls_setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &enable, sizeof(enable)) < 0)
        {
            return -1;
        }
    }

    return 0;
}

int smls_set_timeout(smls_socket_t sock, int recv_timeout_ms, int send_timeout_ms)
{
#if LWIP_SO_SNDRCVTIMEO_NONSTANDARD
    int timeout_ms;
#else
    struct timeval tv;
#endif

    if (recv_timeout_ms >= 0)
    {
#if LWIP_SO_SNDRCVTIMEO_NONSTANDARD
        timeout_ms = recv_timeout_ms;
        if (smls_setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout_ms, sizeof(timeout_ms)) < 0)
        {
            return -1;
        }
#else
        tv.tv_sec  = recv_timeout_ms / 1000;
        tv.tv_usec = (recv_timeout_ms % 1000) * 1000;

        if (smls_setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
        {
            return -1;
        }
#endif
    }

    if (send_timeout_ms >= 0)
    {
#if LWIP_SO_SNDRCVTIMEO_NONSTANDARD
        timeout_ms = send_timeout_ms;
        if (smls_setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout_ms, sizeof(timeout_ms)) < 0)
        {
            return -1;
        }
#else
        tv.tv_sec  = send_timeout_ms / 1000;
        tv.tv_usec = (send_timeout_ms % 1000) * 1000;

        if (smls_setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0)
        {
            return -1;
        }
#endif
    }

    return 0;
}

int smls_socket_errno(void)
{
    return errno;
}

#endif
