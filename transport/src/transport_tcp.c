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

static void tcp_channel_ctx_reset(tcp_channel_ctx_t* ctx)
{
    tcp_socket_t sock = ctx->sock;

    memset(ctx, 0, sizeof(*ctx));
    ctx->sock  = sock;
    ctx->state = TCP_CH_FREE;
}

static uint32_t tcp_rx_write_capacity(const tcp_channel_ctx_t* ctx)
{
    return TRANSPORT_TCP_RX_BUF_SIZE - ctx->rx_count;
}

static uint32_t tcp_rx_read_available(const tcp_channel_ctx_t* ctx)
{
    return ctx->rx_count;
}

static uint32_t tcp_rx_push_bytes(tcp_channel_ctx_t* ctx, const uint8_t* data, uint32_t len)
{
    uint32_t free_cap = tcp_rx_write_capacity(ctx);
    uint32_t write_n  = (len < free_cap) ? len : free_cap;
    uint32_t first_n;
    uint32_t second_n;

    if (write_n == 0u)
    {
        return 0u;
    }

    first_n = TRANSPORT_TCP_RX_BUF_SIZE - ctx->rx_tail;
    if (first_n > write_n)
    {
        first_n = write_n;
    }

    memcpy(&ctx->rx_buf[ctx->rx_tail], data, first_n);
    second_n = write_n - first_n;
    if (second_n > 0u)
    {
        memcpy(&ctx->rx_buf[0], data + first_n, second_n);
    }

    ctx->rx_tail = (ctx->rx_tail + write_n) % TRANSPORT_TCP_RX_BUF_SIZE;
    ctx->rx_count += write_n;

    return write_n;
}

static uint32_t tcp_rx_pop_bytes(tcp_channel_ctx_t* ctx, uint8_t* out, uint32_t len)
{
    uint32_t avail_n = tcp_rx_read_available(ctx);
    uint32_t read_n  = (len < avail_n) ? len : avail_n;
    uint32_t first_n;
    uint32_t second_n;

    if (read_n == 0u)
    {
        return 0u;
    }

    first_n = TRANSPORT_TCP_RX_BUF_SIZE - ctx->rx_head;
    if (first_n > read_n)
    {
        first_n = read_n;
    }

    memcpy(out, &ctx->rx_buf[ctx->rx_head], first_n);
    second_n = read_n - first_n;
    if (second_n > 0u)
    {
        memcpy(out + first_n, &ctx->rx_buf[0], second_n);
    }

    ctx->rx_head = (ctx->rx_head + read_n) % TRANSPORT_TCP_RX_BUF_SIZE;
    ctx->rx_count -= read_n;

    return read_n;
}

static int tcp_client_check_connected(transport_connection_t* conn)
{
    tcp_conn_ctx_t* c = conn_ctx(conn);
    transport_channel_t* ch;
    tcp_channel_ctx_t* x;
    int so_error      = 0;
    socklen_t opt_len = sizeof(so_error);

    if (c->is_server)
    {
        return 1;
    }

    if (c->state == TCP_CONN_CONNECTED)
    {
        return 1;
    }

    if (c->state == TCP_CONN_ERROR || c->state == TCP_CONN_CLOSED)
    {
        return TRANSPORT_STATUS_ERROR;
    }

    if (c->state != TCP_CONN_CONNECTING)
    {
        return 0;
    }

    ch = &conn->channels[0];
    x  = ch_ctx(ch);

    if (getsockopt(x->sock, SOL_SOCKET, SO_ERROR, (char*)&so_error, &opt_len) != 0)
    {
        c->state = TCP_CONN_ERROR;
        x->state = TCP_CH_CLOSED;
        return TRANSPORT_STATUS_ERROR;
    }

    if (so_error == 0)
    {
        c->state = TCP_CONN_CONNECTED;
        x->state = TCP_CH_ACTIVE;
        return 1;
    }

    if (so_error == TCP_ERR_IN_PROGRESS || so_error == TCP_ERR_WOULD_BLOCK)
    {
        return 0;
    }

    c->state = TCP_CONN_ERROR;
    x->state = TCP_CH_CLOSED;
    return TRANSPORT_STATUS_ERROR;
}

static int tcp_pump_channel(transport_channel_t* ch)
{
    tcp_channel_ctx_t* ctx = ch_ctx(ch);
    int total              = 0;

    if (ctx->state != TCP_CH_ACTIVE)
    {
        return 0;
    }

    while (tcp_rx_write_capacity(ctx) > 0u)
    {
        uint8_t tmp[256];
        uint32_t free_cap = tcp_rx_write_capacity(ctx);
        uint32_t want;
        uint32_t pushed;
        int r;

        want = (free_cap < sizeof(tmp)) ? free_cap : (uint32_t)sizeof(tmp);

        r = recv(ctx->sock, (char*)tmp, (int)want, 0);
        if (r > 0)
        {
            pushed = tcp_rx_push_bytes(ctx, tmp, (uint32_t)r);
            total += (int)pushed;

            if (pushed < (uint32_t)r)
            {
                break;
            }

            continue;
        }

        if (r == 0)
        {
            ctx->state = TCP_CH_CLOSED;
            break;
        }

        if (tcp_would_block())
        {
            break;
        }

        ctx->state = TCP_CH_CLOSED;
        return TRANSPORT_STATUS_ERROR;
    }

    return total;
}

/* =========================================================
 * Channel ops
 * ========================================================= */

static int tcp_send(transport_channel_t* ch, const void* data, uint32_t len)
{
    tcp_channel_ctx_t* ctx = ch_ctx(ch);
    tcp_conn_ctx_t* c      = conn_ctx(ch->parent);
    int connected;

    if (!c->is_server)
    {
        connected = tcp_client_check_connected(ch->parent);
        if (connected < 0)
        {
            return connected;
        }

        if (connected == 0)
        {
            return 0;
        }
    }

    if (ctx->state != TCP_CH_ACTIVE)
    {
        return TRANSPORT_STATUS_CLOSED;
    }

    int r = send(ctx->sock, data, (int)len, 0);
    if (r >= 0)
    {
        return r;
    }
    if (tcp_would_block())
    {
        return 0;
    }
    ctx->state = TCP_CH_CLOSED;
    return TRANSPORT_STATUS_ERROR;
}

static int tcp_recv(transport_channel_t* ch, void* buf, uint32_t len)
{
    tcp_channel_ctx_t* ctx = ch_ctx(ch);
    tcp_conn_ctx_t* c      = conn_ctx(ch->parent);
    int connected;
    uint32_t pop_n;

    if (!c->is_server)
    {
        connected = tcp_client_check_connected(ch->parent);
        if (connected < 0)
        {
            return connected;
        }

        if (connected == 0)
        {
            return 0;
        }
    }

    if (tcp_rx_read_available(ctx) == 0u)
    {
        int pumped = tcp_pump_channel(ch);
        if (pumped < 0)
        {
            return pumped;
        }
    }

    pop_n = tcp_rx_pop_bytes(ctx, (uint8_t*)buf, len);
    if (pop_n > 0u)
    {
        return (int)pop_n;
    }

    if (ctx->state == TCP_CH_CLOSED)
    {
        return TRANSPORT_STATUS_CLOSED;
    }

    return 0;
}

static transport_status_t tcp_channel_close(transport_channel_t* ch)
{
    tcp_channel_ctx_t* ctx = ch_ctx(ch);

    if (ctx->sock)
    {
        tcp_close(ctx->sock);
        ctx->sock = 0;
    }

    ctx->rx_head  = 0u;
    ctx->rx_tail  = 0u;
    ctx->rx_count = 0u;
    ctx->state    = TCP_CH_CLOSED;
    ch->in_use    = false;
    return TRANSPORT_STATUS_OK;
}

/* =========================================================
 * Connection ops
 * ========================================================= */

static transport_status_t tcp_open(transport_connection_t* conn, const void* args)
{
    const transport_tcp_args_t* cfg = args;
    tcp_conn_ctx_t* ctx             = conn_ctx(conn);

    if (cfg == NULL)
    {
        return TRANSPORT_STATUS_INVALID_ARG;
    }

    if (conn->max_channels == 0u)
    {
        return TRANSPORT_STATUS_INVALID_ARG;
    }

    memset(ctx, 0, sizeof(*ctx));
    ctx->is_server = cfg->server;
    ctx->state     = TCP_CONN_IDLE;

    tcp_socket_t s = socket(AF_INET, SOCK_STREAM, 0);
    if ((int)s < 0)
        return TRANSPORT_STATUS_ERROR;

    tcp_set_nonblock(s);

    tcp_addr_t addr;

    if (tcp_addr_make(&addr, cfg->ip, cfg->port) != 0)
    {
        tcp_close(s);
        return TRANSPORT_STATUS_INVALID_ARG;
    }

    if (cfg->server)
    {
        if (bind(s, (struct sockaddr*)&addr, addr.len) < 0)
        {
            tcp_close(s);
            return TRANSPORT_STATUS_ERROR;
        }

        if (listen(s, conn->max_channels) < 0)
        {
            tcp_close(s);
            return TRANSPORT_STATUS_ERROR;
        }

        ctx->listen_sock = s;
        ctx->state       = TCP_CONN_LISTENING;

        for (uint16_t i = 0; i < conn->max_channels; i++)
        {
            tcp_channel_ctx_t* cctx = ch_ctx(&conn->channels[i]);
            if (cctx == NULL)
            {
                tcp_close(s);
                return TRANSPORT_STATUS_INVALID_ARG;
            }

            tcp_channel_ctx_reset(cctx);
            cctx->sock  = 0;
            cctx->state = TCP_CH_FREE;
        }
    }
    else
    {
        int r = connect(s, (struct sockaddr*)&addr, addr.len);
        if (r < 0 && !tcp_in_progress())
        {
            tcp_close(s);
            return TRANSPORT_STATUS_ERROR;
        }

        /* client: 占用 channel 0 */
        transport_channel_t* ch = &conn->channels[0];
        tcp_channel_ctx_t* cctx = ch_ctx(ch);

        tcp_channel_ctx_reset(cctx);
        cctx->sock  = s;
        cctx->state = (r == 0) ? TCP_CH_ACTIVE : TCP_CH_FREE;
        ch->in_use  = true;

        ctx->listen_sock = 0;
        ctx->state       = (r == 0) ? TCP_CONN_CONNECTED : TCP_CONN_CONNECTING;

        if (r != 0)
        {
            (void)tcp_client_check_connected(conn);
        }
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

    ctx->state = TCP_CONN_CLOSED;
    return TRANSPORT_STATUS_OK;
}

static int tcp_poll(transport_connection_t* conn, uint32_t timeout_ms)
{
    (void)timeout_ms;

    tcp_conn_ctx_t* ctx = conn_ctx(conn);

    if (!ctx->is_server)
    {
        int connected = tcp_client_check_connected(conn);
        if (connected < 0)
        {
            ctx->state = TCP_CONN_ERROR;
            return connected;
        }

        if (conn->channels[0].in_use)
        {
            int r = tcp_pump_channel(&conn->channels[0]);
            if (r < 0)
            {
                return r;
            }
        }

        return 0;
    }

    ctx->state = TCP_CONN_LISTENING;

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
                tcp_channel_ctx_reset(cctx);
                cctx->sock  = s;
                cctx->state = TCP_CH_ACTIVE;
                ch->in_use  = true;
                s           = 0;
                break;
            }
        }

        if (s)
        {
            tcp_close(s);
        }
    }

    for (uint16_t i = 0; i < conn->max_channels; i++)
    {
        transport_channel_t* ch = &conn->channels[i];
        int r;

        if (!ch->in_use)
        {
            continue;
        }

        r = tcp_pump_channel(ch);
        if (r < 0)
        {
            return r;
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