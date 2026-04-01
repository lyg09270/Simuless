#include "smls_socket_port.h"

#include <limits.h>
#include <stddef.h>
#include <string.h>

#include <winsock2.h>
#include <ws2tcpip.h>

/* =========================================================
 * Internal helpers
 * ========================================================= */

static int smls_socket_is_valid(smls_socket_t sock)
{
    return sock != SMLS_SOCKET_INVALID;
}

static int smls_socket_set_nonblock(smls_socket_t sock)
{
    u_long mode = 1UL;

    if (ioctlsocket((SOCKET)sock, FIONBIO, &mode) != NO_ERROR)
    {
        return SMLS_SOCKET_ERR_IO;
    }

    return SMLS_SOCKET_OK;
}

static int smls_socket_would_block_error(int err)
{
    return (err == WSAEWOULDBLOCK) || (err == WSAEINPROGRESS) || (err == WSAEALREADY);
}

/* =========================================================
 * Public API
 * ========================================================= */

int smls_socket_create(smls_socket_t* sock)
{
    if (sock == NULL)
    {
        return SMLS_SOCKET_ERR_INVALID_ARG;
    }

    const SOCKET fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd == INVALID_SOCKET)
    {
        return SMLS_SOCKET_ERR_IO;
    }

    *sock = (smls_socket_t)fd;
    return smls_socket_set_nonblock(*sock);
}

int smls_socket_bind(smls_socket_t sock, const char* ip, uint16_t port)
{
    if (!smls_socket_is_valid(sock) || ip == NULL)
    {
        return SMLS_SOCKET_ERR_INVALID_ARG;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);

    if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1)
    {
        return SMLS_SOCKET_ERR_INVALID_ARG;
    }

    if (bind((SOCKET)sock, (const struct sockaddr*)&addr, (int)sizeof(addr)) == SOCKET_ERROR)
    {
        return SMLS_SOCKET_ERR_IO;
    }

    return SMLS_SOCKET_OK;
}

int smls_socket_listen(smls_socket_t sock, int backlog)
{
    if (!smls_socket_is_valid(sock))
    {
        return SMLS_SOCKET_ERR_INVALID_ARG;
    }

    if (listen((SOCKET)sock, backlog) == SOCKET_ERROR)
    {
        return SMLS_SOCKET_ERR_IO;
    }

    return SMLS_SOCKET_OK;
}

int smls_socket_accept(smls_socket_t listen_sock, smls_socket_t* client_sock)
{
    if (!smls_socket_is_valid(listen_sock) || client_sock == NULL)
    {
        return SMLS_SOCKET_ERR_INVALID_ARG;
    }

    const SOCKET fd = accept((SOCKET)listen_sock, NULL, NULL);
    if (fd == INVALID_SOCKET)
    {
        const int err = WSAGetLastError();
        if (smls_socket_would_block_error(err))
        {
            return SMLS_SOCKET_ERR_WOULD_BLOCK;
        }

        return SMLS_SOCKET_ERR_IO;
    }

    *client_sock = (smls_socket_t)fd;
    return smls_socket_set_nonblock(*client_sock);
}

int smls_socket_connect(smls_socket_t sock, const char* ip, uint16_t port)
{
    if (!smls_socket_is_valid(sock) || ip == NULL)
    {
        return SMLS_SOCKET_ERR_INVALID_ARG;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);

    if (inet_pton(AF_INET, ip, &addr.sin_addr) != 1)
    {
        return SMLS_SOCKET_ERR_INVALID_ARG;
    }

    if (connect((SOCKET)sock, (const struct sockaddr*)&addr, (int)sizeof(addr)) == SOCKET_ERROR)
    {
        const int err = WSAGetLastError();
        if (smls_socket_would_block_error(err))
        {
            return SMLS_SOCKET_ERR_WOULD_BLOCK;
        }

        return SMLS_SOCKET_ERR_IO;
    }

    return SMLS_SOCKET_OK;
}

int smls_socket_send(smls_socket_t sock, const void* data, uint32_t len)
{
    if (!smls_socket_is_valid(sock) || (data == NULL && len > 0u) || len > (uint32_t)INT_MAX)
    {
        return SMLS_SOCKET_ERR_INVALID_ARG;
    }

    const int ret = send((SOCKET)sock, (const char*)data, (int)len, 0);
    if (ret == SOCKET_ERROR)
    {
        const int err = WSAGetLastError();
        if (smls_socket_would_block_error(err))
        {
            return SMLS_SOCKET_ERR_WOULD_BLOCK;
        }

        return SMLS_SOCKET_ERR_IO;
    }

    return ret;
}

int smls_socket_recv(smls_socket_t sock, void* buf, uint32_t len)
{
    if (!smls_socket_is_valid(sock) || (buf == NULL && len > 0u) || len > (uint32_t)INT_MAX)
    {
        return SMLS_SOCKET_ERR_INVALID_ARG;
    }

    const int ret = recv((SOCKET)sock, (char*)buf, (int)len, 0);
    if (ret == SOCKET_ERROR)
    {
        const int err = WSAGetLastError();
        if (smls_socket_would_block_error(err))
        {
            return SMLS_SOCKET_ERR_WOULD_BLOCK;
        }

        return SMLS_SOCKET_ERR_IO;
    }

    if (ret == 0)
    {
        return SMLS_SOCKET_ERR_CLOSED;
    }

    return ret;
}

int smls_socket_poll(smls_socket_t sock, uint32_t timeout_ms)
{
    if (!smls_socket_is_valid(sock))
    {
        return SMLS_SOCKET_ERR_INVALID_ARG;
    }

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET((SOCKET)sock, &readfds);

    struct timeval tv;
    tv.tv_sec  = (long)(timeout_ms / 1000u);
    tv.tv_usec = (long)((timeout_ms % 1000u) * 1000u);

    const int ret = select(0, &readfds, NULL, NULL, &tv);
    if (ret == SOCKET_ERROR)
    {
        return SMLS_SOCKET_ERR_IO;
    }

    if (ret == 0)
    {
        return 0;
    }

    return FD_ISSET((SOCKET)sock, &readfds) ? 1 : 0;
}

int smls_socket_close(smls_socket_t sock)
{
    if (!smls_socket_is_valid(sock))
    {
        return SMLS_SOCKET_ERR_INVALID_ARG;
    }

    if (closesocket((SOCKET)sock) == SOCKET_ERROR)
    {
        return SMLS_SOCKET_ERR_IO;
    }

    return SMLS_SOCKET_OK;
}