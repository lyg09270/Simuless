/**
 * @file smls_socket_port_win32.c
 * @brief WinSock implementation for socket port.
 */

#include "smls_socket_port.h"

#ifdef _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

/* =========================================================
 * Native mirror wrappers
 * ========================================================= */

smls_socket_t smls_socket(int domain, int type, int protocol)
{
    return socket(domain, type, protocol);
}

int smls_bind(smls_socket_t sock, const smls_sockaddr_t* addr, smls_socklen_t addrlen)
{
    return bind(sock, (const struct sockaddr*)addr, addrlen);
}

int smls_listen(smls_socket_t sock, int backlog)
{
    return listen(sock, backlog);
}

smls_socket_t smls_accept(smls_socket_t sock, smls_sockaddr_t* addr, smls_socklen_t* addrlen)
{
    return accept(sock, (struct sockaddr*)addr, addrlen);
}

int smls_connect(smls_socket_t sock, const smls_sockaddr_t* addr, smls_socklen_t addrlen)
{
    return connect(sock, (const struct sockaddr*)addr, addrlen);
}

ssize_t smls_send(smls_socket_t sock, const void* buf, size_t len, int flags)
{
    return (ssize_t)send(sock, (const char*)buf, (int)len, flags);
}

ssize_t smls_recv(smls_socket_t sock, void* buf, size_t len, int flags)
{
    return (ssize_t)recv(sock, (char*)buf, (int)len, flags);
}

ssize_t smls_sendto(smls_socket_t sock, const void* buf, size_t len, int flags,
                    const smls_sockaddr_t* addr, smls_socklen_t addrlen)
{
    return (ssize_t)sendto(sock, (const char*)buf, (int)len, flags, (const struct sockaddr*)addr,
                           addrlen);
}

ssize_t smls_recvfrom(smls_socket_t sock, void* buf, size_t len, int flags, smls_sockaddr_t* addr,
                      smls_socklen_t* addrlen)
{
    return (ssize_t)recvfrom(sock, (char*)buf, (int)len, flags, (struct sockaddr*)addr, addrlen);
}

int smls_close(smls_socket_t sock)
{
    return closesocket(sock);
}

int smls_setsockopt(smls_socket_t sock, int level, int optname, const void* optval,
                    smls_socklen_t optlen)
{
    return setsockopt(sock, level, optname, (const char*)optval, optlen);
}

int smls_getsockopt(smls_socket_t sock, int level, int optname, void* optval,
                    smls_socklen_t* optlen)
{
    return getsockopt(sock, level, optname, (char*)optval, optlen);
}

/* =========================================================
 * WinSock helper APIs
 * ========================================================= */

int smls_set_nonblock(smls_socket_t sock, int enable)
{
    u_long mode = (enable != 0) ? 1UL : 0UL;

    return ioctlsocket(sock, FIONBIO, &mode);
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
    DWORD timeout_ms;

    if (recv_timeout_ms >= 0)
    {
        timeout_ms = (DWORD)recv_timeout_ms;

        if (smls_setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout_ms, sizeof(timeout_ms)) < 0)
        {
            return -1;
        }
    }

    if (send_timeout_ms >= 0)
    {
        timeout_ms = (DWORD)send_timeout_ms;

        if (smls_setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout_ms, sizeof(timeout_ms)) < 0)
        {
            return -1;
        }
    }

    return 0;
}

int smls_socket_errno(void)
{
    return WSAGetLastError();
}

#endif
