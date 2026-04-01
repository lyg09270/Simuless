#include "smls_socket_port.h"

#include <errno.h>
#include <string.h>

#include <fcntl.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <poll.h>
#include <sys/socket.h>

/* =========================================================
 * Internal helpers
 * ========================================================= */

static int smls_socket_is_valid(smls_socket_t sock)
{
    return sock != SMLS_SOCKET_INVALID;
}

static int smls_socket_set_nonblock(smls_socket_t sock)
{
    const int fd = (int)sock;

    const int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
    {
        return SMLS_SOCKET_ERR_IO;
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        return SMLS_SOCKET_ERR_IO;
    }

    return SMLS_SOCKET_OK;
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

    const int fd = socket(AF_INET, SOCK_STREAM, 0);
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

    if (bind((int)sock, (const struct sockaddr*)&addr, sizeof(addr)) < 0)
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

    if (listen((int)sock, backlog) < 0)
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

    const int fd = accept((int)listen_sock, NULL, NULL);

    if (fd < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
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

    const int ret = connect((int)sock, (const struct sockaddr*)&addr, sizeof(addr));

    if (ret < 0)
    {
        if (errno == EINPROGRESS)
        {
            return SMLS_SOCKET_ERR_WOULD_BLOCK;
        }

        return SMLS_SOCKET_ERR_IO;
    }

    return SMLS_SOCKET_OK;
}

int smls_socket_send(smls_socket_t sock, const void* data, uint32_t len)
{
    if (!smls_socket_is_valid(sock) || (data == NULL && len > 0u))
    {
        return SMLS_SOCKET_ERR_INVALID_ARG;
    }

    const int ret = send((int)sock, data, len, 0);

    if (ret < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
        {
            return SMLS_SOCKET_ERR_WOULD_BLOCK;
        }

        return SMLS_SOCKET_ERR_IO;
    }

    return ret;
}

int smls_socket_recv(smls_socket_t sock, void* buf, uint32_t len)
{
    if (!smls_socket_is_valid(sock) || (buf == NULL && len > 0u))
    {
        return SMLS_SOCKET_ERR_INVALID_ARG;
    }

    const int ret = recv((int)sock, buf, len, 0);

    if (ret < 0)
    {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
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

    struct pollfd pfd;
    pfd.fd      = (int)sock;
    pfd.events  = POLLIN;
    pfd.revents = 0;

    const int ret = poll(&pfd, 1, (int)timeout_ms);

    if (ret < 0)
    {
        return SMLS_SOCKET_ERR_IO;
    }

    if (ret == 0)
    {
        return 0;
    }

    if (pfd.revents & POLLIN)
    {
        return 1;
    }

    return 0;
}

int smls_socket_close(smls_socket_t sock)
{
    if (!smls_socket_is_valid(sock))
    {
        return SMLS_SOCKET_ERR_INVALID_ARG;
    }

    if (close((int)sock) < 0)
    {
        return SMLS_SOCKET_ERR_IO;
    }

    return SMLS_SOCKET_OK;
}
