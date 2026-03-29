#include "transport_tcp.h"
#include <string.h>

/* =========================================================
 * Context definitions
 * ========================================================= */

_Static_assert(sizeof(tcp_conn_ctx_t) <= TRANSPORT_IMPL_SIZE, "Increase TRANSPORT_IMPL_SIZE");

/* ========================================================= */

static tcp_conn_ctx_t* conn_ctx(transport_connection_t* conn)
{
    return (tcp_conn_ctx_t*)conn->impl;
}

static tcp_channel_ctx_t* ch_ctx(transport_channel_t* ch)
{
    return (tcp_channel_ctx_t*)ch->impl;
}

/* =========================================================
 * Channel ops
 * ========================================================= */

static int tcp_send(transport_channel_t* ch, const void* data, uint32_t len)
{
    tcp_channel_ctx_t* ctx = ch_ctx(ch);

    int r = send(ctx->sock, data, len, 0);
    if (r >= 0)
        return r;
    if (tcp_would_block())
        return 0;
    return TRANSPORT_STATUS_ERROR;
}

static int tcp_recv(transport_channel_t* ch, void* buf, uint32_t len)
{
    tcp_channel_ctx_t* ctx = ch_ctx(ch);

    int r = recv(ctx->sock, buf, len, 0);
    if (r > 0)
        return r;
    if (r == 0)
        return TRANSPORT_STATUS_CLOSED;
    if (tcp_would_block())
        return 0;
    return TRANSPORT_STATUS_ERROR;
}

static transport_status_t tcp_channel_close(transport_channel_t* ch)
{
    tcp_channel_ctx_t* ctx = ch_ctx(ch);

    if (ctx->sock)
    {
        tcp_close(ctx->sock);
        ctx->sock = 0;
    }

    ch->in_use = false;
    return TRANSPORT_STATUS_OK;
}

/* =========================================================
 * Connection ops
 * ========================================================= */

static transport_status_t tcp_open(transport_connection_t* conn, const void* args)
{
    const transport_tcp_args_t* cfg = args;
    tcp_conn_ctx_t* ctx             = conn_ctx(conn);

    memset(ctx, 0, sizeof(*ctx));
    ctx->is_server = cfg->server;

    tcp_socket_t s = socket(AF_INET, SOCK_STREAM, 0);
    if ((int)s < 0)
        return TRANSPORT_STATUS_ERROR;

    tcp_set_nonblock(s);

    tcp_addr_t addr;

    if (tcp_addr_make(&addr, cfg->ip, cfg->port) != 0)
    {
        return TRANSPORT_STATUS_INVALID_ARG;
    }

    if (cfg->server)
    {
        if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0)
            return TRANSPORT_STATUS_ERROR;

        if (listen(s, conn->max_channels) < 0)
            return TRANSPORT_STATUS_ERROR;

        ctx->listen_sock = s;
    }
    else
    {
        int r = connect(s, (struct sockaddr*)&addr, sizeof(addr));
        if (r < 0 && !tcp_in_progress())
            return TRANSPORT_STATUS_ERROR;

        /* client: 占用 channel 0 */
        transport_channel_t* ch = &conn->channels[0];
        tcp_channel_ctx_t* cctx = ch_ctx(ch);

        cctx->sock = s;
        ch->in_use = true;

        ctx->listen_sock = 0;
    }

    return TRANSPORT_STATUS_OK;
}

static transport_status_t tcp_close_conn(transport_connection_t* conn)
{
    tcp_conn_ctx_t* ctx = conn_ctx(conn);

    if (ctx->listen_sock)
    {
        tcp_close(ctx->listen_sock);
        ctx->listen_sock = 0;
    }

    for (uint16_t i = 0; i < conn->max_channels; i++)
    {
        transport_channel_t* ch = &conn->channels[i];
        if (ch->in_use)
            tcp_channel_close(ch);
    }

    return TRANSPORT_STATUS_OK;
}

static int tcp_poll(transport_connection_t* conn, uint32_t timeout_ms)
{
    (void)timeout_ms;

    tcp_conn_ctx_t* ctx = conn_ctx(conn);

    if (!ctx->is_server)
        return 0;

    while (1)
    {
        tcp_socket_t s = accept(ctx->listen_sock, NULL, NULL);
        if ((int)s < 0)
            break;

        tcp_set_nonblock(s);

        for (uint16_t i = 0; i < conn->max_channels; i++)
        {
            transport_channel_t* ch = &conn->channels[i];
            if (!ch->in_use)
            {
                tcp_channel_ctx_t* cctx = ch_ctx(ch);
                cctx->sock              = s;
                ch->in_use              = true;
                break;
            }
        }
    }

    return 0;
}

/* =========================================================
 * Ops tables
 * ========================================================= */

static const transport_channel_ops_t ch_ops = {
    .send  = tcp_send,
    .recv  = tcp_recv,
    .close = tcp_channel_close,
};

static const transport_connection_ops_t conn_ops = {
    .open  = tcp_open,
    .close = tcp_close_conn,
    .poll  = tcp_poll,
};

/* =========================================================
 * Public init
 * ========================================================= */

transport_status_t transport_tcp_client_init(transport_connection_t* conn, const void* args)
{
    (void)args;
    conn->conn_ops = &conn_ops;
    conn->ch_ops   = &ch_ops;
    conn->caps     = TRANSPORT_CAP_STREAM | TRANSPORT_CAP_ASYNC;
    return TRANSPORT_STATUS_OK;
}

transport_status_t transport_tcp_server_init(transport_connection_t* conn, const void* args)
{
    (void)args;
    conn->conn_ops = &conn_ops;
    conn->ch_ops   = &ch_ops;
    conn->caps     = TRANSPORT_CAP_STREAM | TRANSPORT_CAP_ASYNC;
    return TRANSPORT_STATUS_OK;
}