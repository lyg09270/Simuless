/**
 * @file smls_socket_port_posix.c
 * @brief POSIX implementation for socket port.
 */

#include "smls_socket_port.h"

#if !defined(_WIN32) && !defined(SMLS_USE_LWIP)

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>

/* =========================================================
 * Native mirror wrappers
 * ========================================================= */

smls_socket_t smls_socket(int domain, int type, int protocol)
{
    return (smls_socket_t)socket(domain, type, protocol);
}

int smls_bind(smls_socket_t sock, const smls_sockaddr_t* addr, smls_socklen_t addrlen)
{
    return bind((int)sock, (const struct sockaddr*)addr, (socklen_t)addrlen);
}

int smls_listen(smls_socket_t sock, int backlog)
{
    return listen((int)sock, backlog);
}

smls_socket_t smls_accept(smls_socket_t sock, smls_sockaddr_t* addr, smls_socklen_t* addrlen)
{
    return (smls_socket_t)accept((int)sock, (struct sockaddr*)addr, (socklen_t*)addrlen);
}

int smls_connect(smls_socket_t sock, const smls_sockaddr_t* addr, smls_socklen_t addrlen)
{
    return connect((int)sock, (const struct sockaddr*)addr, (socklen_t)addrlen);
}

ssize_t smls_send(smls_socket_t sock, const void* buf, size_t len, int flags)
{
    return send((int)sock, buf, len, flags);
}

ssize_t smls_recv(smls_socket_t sock, void* buf, size_t len, int flags)
{
    return recv((int)sock, buf, len, flags);
}

ssize_t smls_sendto(smls_socket_t sock, const void* buf, size_t len, int flags,
                    const smls_sockaddr_t* addr, smls_socklen_t addrlen)
{
    return sendto((int)sock, buf, len, flags, (const struct sockaddr*)addr, (socklen_t)addrlen);
}

ssize_t smls_recvfrom(smls_socket_t sock, void* buf, size_t len, int flags, smls_sockaddr_t* addr,
                      smls_socklen_t* addrlen)
{
    return recvfrom((int)sock, buf, len, flags, (struct sockaddr*)addr, (socklen_t*)addrlen);
}

int smls_close(smls_socket_t sock)
{
    return close((int)sock);
}

int smls_setsockopt(smls_socket_t sock, int level, int optname, const void* optval,
                    smls_socklen_t optlen)
{
    return setsockopt((int)sock, level, optname, optval, (socklen_t)optlen);
}

int smls_getsockopt(smls_socket_t sock, int level, int optname, void* optval,
                    smls_socklen_t* optlen)
{
    return getsockopt((int)sock, level, optname, optval, (socklen_t*)optlen);
}

/* =========================================================
 * POSIX helper APIs
 * ========================================================= */

int smls_set_nonblock(smls_socket_t sock, int enable)
{
    const int flags = fcntl((int)sock, F_GETFL, 0);

    if (flags < 0)
    {
        return -1;
    }

    if (enable != 0)
    {
        return fcntl((int)sock, F_SETFL, flags | O_NONBLOCK);
    }

    return fcntl((int)sock, F_SETFL, flags & (~O_NONBLOCK));
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
    struct timeval tv;

    if (recv_timeout_ms >= 0)
    {
        tv.tv_sec  = recv_timeout_ms / 1000;
        tv.tv_usec = (recv_timeout_ms % 1000) * 1000;

        if (smls_setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
        {
            return -1;
        }
    }

    if (send_timeout_ms >= 0)
    {
        tv.tv_sec  = send_timeout_ms / 1000;
        tv.tv_usec = (send_timeout_ms % 1000) * 1000;

        if (smls_setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0)
        {
            return -1;
        }
    }

    return 0;
}

int smls_socket_make_ipv4_addr(smls_sockaddr_t* addr, const char* ip, uint16_t port)
{
    if (addr == NULL || ip == NULL)
    {
        return -1;
    }

    struct sockaddr_in* ipv4 = (struct sockaddr_in*)addr;

    memset(ipv4, 0, sizeof(*ipv4));

    ipv4->sin_family = AF_INET;
    ipv4->sin_port   = htons(port);

    if (inet_pton(AF_INET, ip, &ipv4->sin_addr) != 1)
    {
        return -1;
    }

    return 0;
}

int smls_socket_poll(smls_socket_t sock, uint32_t timeout_ms)
{
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET((int)sock, &readfds);

    struct timeval tv;
    struct timeval* ptv = NULL;

    if (timeout_ms > 0u)
    {
        tv.tv_sec  = (long)(timeout_ms / 1000u);
        tv.tv_usec = (long)((timeout_ms % 1000u) * 1000u);
        ptv        = &tv;
    }
    else if (timeout_ms == 0u)
    {
        tv.tv_sec  = 0;
        tv.tv_usec = 0;
        ptv        = &tv;
    }

    return select((int)sock + 1, &readfds, NULL, NULL, ptv);
}

int smls_socket_errno(void)
{
    return errno;
}

#endif
