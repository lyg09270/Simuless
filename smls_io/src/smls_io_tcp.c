#include "smls_io_tcp.h"
#include "smls_socket_port.h"

#include <string.h>

#define SMLS_TCP_PREFIX "tcp://"
#define SMLS_TCP_PREFIX_LEN 6

/* ========================================================= */

static smls_io_tcp_ctx_t* smls_tcp_ctx(smls_io_t* io)
{
    if (io == NULL)
    {
        return NULL;
    }

    return (smls_io_tcp_ctx_t*)io->priv;
}

/* ========================================================= */

static void smls_tcp_ctx_reset(smls_io_tcp_ctx_t* ctx)
{
    if (ctx == NULL)
    {
        return;
    }

    ctx->sock        = SMLS_SOCKET_INVALID;
    ctx->listen_sock = SMLS_SOCKET_INVALID;

    ctx->host[0] = '\0';
    ctx->port    = 0;

    ctx->is_server = 0;
    ctx->nonblock  = 0;
}

/* =========================================================
 * URI parser
 * ========================================================= */

int smls_io_tcp_parse_url(smls_io_t* io, const char* url)
{
    if (io == NULL || url == NULL)
    {
        return SMLS_IO_ERR_INVALID_ARG;
    }

    smls_io_tcp_ctx_t* ctx = smls_tcp_ctx(io);

    if (ctx == NULL)
    {
        return SMLS_IO_ERR_INVALID_STATE;
    }

    if (strncmp(url, SMLS_TCP_PREFIX, SMLS_TCP_PREFIX_LEN) != 0)
    {
        return SMLS_IO_ERR_INVALID_ARG;
    }

    const char* addr = url + SMLS_TCP_PREFIX_LEN;

    const char* colon = strrchr(addr, ':');

    if (colon == NULL)
    {
        return SMLS_IO_ERR_INVALID_ARG;
    }

    const size_t host_len = (size_t)(colon - addr);

    if (host_len == 0 || host_len >= sizeof(ctx->host))
    {
        return SMLS_IO_ERR_INVALID_ARG;
    }

    memcpy(ctx->host, addr, host_len);

    ctx->host[host_len] = '\0';

    const int port = atoi(colon + 1);

    if (port <= 0 || port > 65535)
    {
        return SMLS_IO_ERR_INVALID_ARG;
    }

    ctx->port = (uint16_t)port;

    /**
     * 0.0.0.0 = server
     */
    ctx->is_server = (strcmp(ctx->host, "0.0.0.0") == 0);

    return SMLS_IO_OK;
}

/* =========================================================
 * open
 * ========================================================= */

int smls_io_tcp_open(smls_io_t* io, const smls_io_desc_t* desc)
{
    if (io == NULL || desc == NULL || desc->uri == NULL)
    {
        return SMLS_IO_ERR_INVALID_ARG;
    }

    smls_io_tcp_ctx_t* ctx = smls_tcp_ctx(io);

    if (ctx == NULL)
    {
        return SMLS_IO_ERR_INVALID_STATE;
    }

    smls_tcp_ctx_reset(ctx);

    ctx->nonblock = (uint8_t)(desc->nonblock ? 1u : 0u);

    io->kind = SMLS_IO_STREAM;

    const int ret = smls_io_tcp_parse_url(io, desc->uri);

    if (ret < 0)
    {
        goto fail;
    }

    smls_sockaddr_t addr;

    if (smls_socket_make_ipv4_addr(&addr, ctx->host, ctx->port) < 0)
    {
        goto fail;
    }

    if (ctx->is_server)
    {
        ctx->listen_sock = smls_socket(AF_INET, SOCK_STREAM, 0);

        if (ctx->listen_sock == SMLS_SOCKET_INVALID)
        {
            goto fail;
        }

        if (smls_bind(ctx->listen_sock, &addr, sizeof(addr)) < 0)
        {
            goto fail;
        }

        if (smls_listen(ctx->listen_sock, 8) < 0)
        {
            goto fail;
        }

        if (ctx->nonblock)
        {
            if (smls_set_nonblock(ctx->listen_sock, 1) < 0)
            {
                goto fail;
            }
        }
    }
    else
    {
        ctx->sock = smls_socket(AF_INET, SOCK_STREAM, 0);

        if (ctx->sock == SMLS_SOCKET_INVALID)
        {
            goto fail;
        }

        if (ctx->nonblock)
        {
            if (smls_set_nonblock(ctx->sock, 1) < 0)
            {
                goto fail;
            }
        }

        const int conn_ret = smls_connect(ctx->sock, &addr, sizeof(addr));

        if (conn_ret < 0)
        {
            /**
             * blocking mode:
             * any connect error is fatal
             */
            if (!ctx->nonblock)
            {
                goto fail;
            }

            /**
             * nonblocking mode:
             * connection may still be in progress
             *
             * Windows:
             *   WSAEWOULDBLOCK
             * POSIX:
             *   EINPROGRESS
             *
             * treat as success and let poll()
             * finish the connection handshake
             */
        }
    }

    return SMLS_IO_OK;

fail:
    if (ctx->sock != SMLS_SOCKET_INVALID)
    {
        (void)smls_close(ctx->sock);
    }

    if (ctx->listen_sock != SMLS_SOCKET_INVALID)
    {
        (void)smls_close(ctx->listen_sock);
    }

    smls_tcp_ctx_reset(ctx);

    io->kind = SMLS_IO_NONE;

    return SMLS_IO_ERR_IO;
}

/* ========================================================= */

int smls_io_tcp_close(smls_io_t* io)
{
    smls_io_tcp_ctx_t* ctx = smls_tcp_ctx(io);

    if (ctx == NULL)
    {
        return SMLS_IO_ERR_INVALID_STATE;
    }

    if (ctx->sock != SMLS_SOCKET_INVALID)
    {
        (void)smls_close(ctx->sock);
    }

    if (ctx->listen_sock != SMLS_SOCKET_INVALID)
    {
        (void)smls_close(ctx->listen_sock);
    }

    smls_tcp_ctx_reset(ctx);

    io->kind = SMLS_IO_NONE;

    return SMLS_IO_OK;
}

/* ========================================================= */

int smls_io_tcp_push(smls_io_t* io, const smls_packet_t* pkt)
{
    if (io == NULL || pkt == NULL || pkt->data == NULL)
    {
        return SMLS_IO_ERR_INVALID_ARG;
    }

    smls_io_tcp_ctx_t* ctx = smls_tcp_ctx(io);

    if (ctx == NULL)
    {
        return SMLS_IO_ERR_INVALID_STATE;
    }

    if (ctx->sock == SMLS_SOCKET_INVALID)
    {
        return SMLS_IO_ERR_CLOSED;
    }

    return (int)smls_send(ctx->sock, pkt->data, pkt->len, 0);
}

/* ========================================================= */

int smls_io_tcp_pop(smls_io_t* io, smls_packet_t* pkt)
{
    if (io == NULL || pkt == NULL || pkt->data == NULL)
    {
        return SMLS_IO_ERR_INVALID_ARG;
    }

    smls_io_tcp_ctx_t* ctx = smls_tcp_ctx(io);

    if (ctx == NULL)
    {
        return SMLS_IO_ERR_INVALID_STATE;
    }

    if (ctx->sock == SMLS_SOCKET_INVALID)
    {
        return SMLS_IO_ERR_CLOSED;
    }

    return (int)smls_recv(ctx->sock, pkt->data, pkt->len, 0);
}

/* =========================================================
 * poll
 * ========================================================= */

int smls_io_tcp_poll(smls_io_t* io, uint32_t timeout_ms)
{
    smls_io_tcp_ctx_t* ctx = smls_tcp_ctx(io);

    if (ctx == NULL)
    {
        return SMLS_IO_ERR_INVALID_STATE;
    }

    smls_socket_t target;

    if (ctx->is_server)
    {
        target = (ctx->sock != SMLS_SOCKET_INVALID) ? ctx->sock : ctx->listen_sock;
    }
    else
    {
        target = ctx->sock;
    }

    if (target == SMLS_SOCKET_INVALID)
    {
        return SMLS_IO_ERR_CLOSED;
    }

    const int ret = smls_socket_poll(target, timeout_ms);

    if (ret <= 0)
    {
        return ret;
    }

    /**
     * first accept
     */
    if (ctx->is_server && target == ctx->listen_sock)
    {
        const smls_socket_t client = smls_accept(ctx->listen_sock, NULL, NULL);

        if (client == SMLS_SOCKET_INVALID)
        {
            return SMLS_IO_ERR_IO;
        }

        if (ctx->nonblock)
        {
            if (smls_set_nonblock(client, 1) < 0)
            {
                (void)smls_close(client);

                return SMLS_IO_ERR_IO;
            }
        }

        if (ctx->sock != SMLS_SOCKET_INVALID)
        {
            (void)smls_close(ctx->sock);
        }

        ctx->sock = client;

        return 2;
    }

    return 1;
}

/* ========================================================= */

const smls_io_ops_t g_smls_io_tcp_ops = {.open  = smls_io_tcp_open,
                                         .close = smls_io_tcp_close,
                                         .push  = smls_io_tcp_push,
                                         .pop   = smls_io_tcp_pop,
                                         .poll  = smls_io_tcp_poll};
