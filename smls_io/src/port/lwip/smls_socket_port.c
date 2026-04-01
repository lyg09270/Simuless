#include "smls_socket_port.h"

#include <errno.h>
#include <limits.h>
#include <string.h>

#include <lwip/inet.h>
#include <lwip/ioctl.h>
#include <lwip/sockets.h>

/* =========================================================
 * Internal helpers
 * ========================================================= */

static int smls_socket_is_valid(smls_socket_t sock)
{
    return sock != SMLS_SOCKET_INVALID;
}

static int smls_socket_set_nonblock(smls_socket_t sock)
{
    int on = 1;
    if (lwip_ioctl((int)sock, FIONBIO, &on) < 0)
    {
        return SMLS_SOCKET_ERR_IO;
    }

    return SMLS_SOCKET_OK;
}

static int smls_socket_is_would_block_errno(int err)
{
    return (err == EAGAIN) || (err == EWOULDBLOCK) || (err == EINPROGRESS) || (err == EALREADY);
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

    const int fd = lwip_socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
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

    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0)
    {
        return SMLS_SOCKET_ERR_INVALID_ARG;
    }

    if (lwip_bind((int)sock, (const struct sockaddr*)&addr, sizeof(addr)) < 0)
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

    if (lwip_listen((int)sock, backlog) < 0)
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

    const int fd = lwip_accept((int)listen_sock, NULL, NULL);
    if (fd < 0)
    {
        if (smls_socket_is_would_block_errno(errno))
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

    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0)
    {
        return SMLS_SOCKET_ERR_INVALID_ARG;
    }

    if (lwip_connect((int)sock, (const struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        if (smls_socket_is_would_block_errno(errno))
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

    const int ret = (int)lwip_send((int)sock, data, len, 0);
    if (ret < 0)
    {
        if (smls_socket_is_would_block_errno(errno))
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

    const int ret = (int)lwip_recv((int)sock, buf, len, 0);
    if (ret < 0)
    {
        if (smls_socket_is_would_block_errno(errno))
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

    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET((int)sock, &rfds);

    struct timeval tv;
    tv.tv_sec  = (long)(timeout_ms / 1000u);
    tv.tv_usec = (long)((timeout_ms % 1000u) * 1000u);

    const int ret = lwip_select((int)sock + 1, &rfds, NULL, NULL, &tv);
    if (ret < 0)
    {
        return SMLS_SOCKET_ERR_IO;
    }

    if (ret == 0)
    {
        return 0;
    }

    return FD_ISSET((int)sock, &rfds) ? 1 : 0;
}

int smls_socket_close(smls_socket_t sock)
{
    if (!smls_socket_is_valid(sock))
    {
        return SMLS_SOCKET_ERR_INVALID_ARG;
    }

    if (lwip_close((int)sock) < 0)
    {
        return SMLS_SOCKET_ERR_IO;
    }

    return SMLS_SOCKET_OK;
}
