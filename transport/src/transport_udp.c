#include "transport_udp.h"
#include <string.h>

/* =========================================================
 * Context definitions
 * ========================================================= */

_Static_assert(sizeof(udp_conn_ctx_t) <= TRANSPORT_IMPL_SIZE, "Increase TRANSPORT_IMPL_SIZE");

/* ========================================================= */

static udp_conn_ctx_t* conn_ctx(transport_connection_t* conn)
{
    return (udp_conn_ctx_t*)conn->impl;
}

static udp_channel_ctx_t* ch_ctx(transport_channel_t* ch)
{
    return (udp_channel_ctx_t*)ch->impl;
}

static void udp_channel_ctx_reset(udp_channel_ctx_t* ctx)
{
    udp_socket_t sock = ctx->sock;

    memset(ctx, 0, sizeof(*ctx));
    ctx->sock = sock;
}

static int udp_queue_push(udp_channel_ctx_t* ctx, const void* data, uint32_t len)
{
    uint16_t slot;

    if (ctx->rx_count >= TRANSPORT_UDP_RX_QUEUE_DEPTH)
    {
        return TRANSPORT_STATUS_BUSY;
    }

    if (len > TRANSPORT_UDP_RX_MAX_PACKET)
    {
        len = TRANSPORT_UDP_RX_MAX_PACKET;
    }

    slot = ctx->rx_tail;

    if (len > 0u)
    {
        memcpy(ctx->rx_buf[slot], data, len);
    }

    ctx->rx_len[slot] = (uint16_t)len;

    ctx->rx_tail = (uint16_t)((ctx->rx_tail + 1u) % TRANSPORT_UDP_RX_QUEUE_DEPTH);
    ctx->rx_count++;

    return (int)len;
}

static int udp_queue_pop(udp_channel_ctx_t* ctx, void* buf, uint32_t len)
{
    uint16_t slot;
    uint16_t pkt_len;
    uint16_t out_len;

    if (ctx->rx_count == 0u)
    {
        return 0;
    }

    slot    = ctx->rx_head;
    pkt_len = ctx->rx_len[slot];
    out_len = (len < pkt_len) ? (uint16_t)len : pkt_len;

    if ((out_len > 0u) && (buf != NULL))
    {
        memcpy(buf, ctx->rx_buf[slot], out_len);
    }

    ctx->rx_head = (uint16_t)((ctx->rx_head + 1u) % TRANSPORT_UDP_RX_QUEUE_DEPTH);
    ctx->rx_count--;

    return (int)out_len;
}

/* =========================================================
 * Peer-channel mapping
 * ========================================================= */

static transport_channel_t* udp_find_channel_by_peer(transport_connection_t* conn,
                                                     const udp_addr_t* peer)
{
    for (uint16_t i = 0; i < conn->max_channels; i++)
    {
        transport_channel_t* ch = &conn->channels[i];
        udp_channel_ctx_t* cctx;

        if (!ch->in_use)
        {
            continue;
        }

        cctx = ch_ctx(ch);
        if (cctx == NULL || !cctx->has_peer)
        {
            continue;
        }

        if (udp_addr_equal(&cctx->peer_addr, peer))
        {
            return ch;
        }
    }

    return NULL;
}

static transport_channel_t* udp_alloc_channel_for_peer(transport_connection_t* conn,
                                                       const udp_addr_t* peer)
{
    udp_conn_ctx_t* ctx = conn_ctx(conn);

    for (uint16_t i = 0; i < conn->max_channels; i++)
    {
        transport_channel_t* ch = &conn->channels[i];
        udp_channel_ctx_t* cctx;

        if (ch->in_use)
        {
            continue;
        }

        cctx = ch_ctx(ch);
        if (cctx == NULL)
        {
            continue;
        }

        udp_channel_ctx_reset(cctx);
        cctx->sock      = ctx->sock;
        cctx->peer_addr = *peer;
        cctx->has_peer  = 1;

        ch->in_use = true;
        return ch;
    }

    return NULL;
}

/* =========================================================
 * RX pump
 * ========================================================= */

static int udp_pump_connection(transport_connection_t* conn)
{
    udp_conn_ctx_t* ctx = conn_ctx(conn);
    int packets         = 0;

    while (1)
    {
        uint8_t buf[TRANSPORT_UDP_RX_MAX_PACKET];
        udp_addr_t peer;
        transport_channel_t* ch;
        udp_channel_ctx_t* cctx;
        int r;

        peer.len = sizeof(peer.storage);

        r = recvfrom(ctx->sock, (char*)buf, (int)sizeof(buf), 0, (struct sockaddr*)&peer.storage,
                     &peer.len);
        if (r < 0)
        {
            if (udp_would_block())
            {
                break;
            }

            return TRANSPORT_STATUS_ERROR;
        }

        ch = udp_find_channel_by_peer(conn, &peer);
        if (ch == NULL)
        {
            if (!ctx->is_server)
            {
                continue;
            }

            ch = udp_alloc_channel_for_peer(conn, &peer);
            if (ch == NULL)
            {
                continue;
            }
        }

        cctx = ch_ctx(ch);
        if (cctx == NULL)
        {
            continue;
        }

        if (udp_queue_push(cctx, buf, (uint32_t)r) < 0)
        {
            continue;
        }

        packets++;
    }

    return packets;
}

/* =========================================================
 * Channel ops
 * ========================================================= */

static int udp_send(transport_channel_t* ch, const void* data, uint32_t len)
{
    udp_channel_ctx_t* ctx = ch_ctx(ch);
    int r;

    if (!ctx->has_peer)
    {
        return TRANSPORT_STATUS_INVALID_ARG;
    }

    r = sendto(ctx->sock, (const char*)data, (int)len, 0, (struct sockaddr*)&ctx->peer_addr.storage,
               ctx->peer_addr.len);
    if (r >= 0)
    {
        return r;
    }
    if (udp_would_block())
    {
        return 0;
    }
    return TRANSPORT_STATUS_ERROR;
}

static int udp_recv(transport_channel_t* ch, void* buf, uint32_t len)
{
    udp_channel_ctx_t* ctx = ch_ctx(ch);
    int r;

    if (ctx->rx_count == 0u)
    {
        r = udp_pump_connection(ch->parent);
        if (r < 0)
        {
            return r;
        }
    }

    return udp_queue_pop(ctx, buf, len);
}

static transport_status_t udp_channel_close(transport_channel_t* ch)
{
    udp_conn_ctx_t* conn   = conn_ctx(ch->parent);
    udp_channel_ctx_t* ctx = ch_ctx(ch);

    if (!conn->is_server && ctx->sock)
    {
        udp_close(ctx->sock);
        ctx->sock  = 0;
        conn->sock = 0;
    }

    udp_channel_ctx_reset(ctx);
    ch->in_use = false;
    return TRANSPORT_STATUS_OK;
}

/* =========================================================
 * Connection ops
 * ========================================================= */

static transport_status_t udp_open(transport_connection_t* conn, const void* args)
{
    const transport_udp_args_t* cfg = args;
    udp_conn_ctx_t* ctx             = conn_ctx(conn);
    udp_socket_t s;
    udp_addr_t addr;

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

    s = socket(AF_INET, SOCK_DGRAM, 0);
    if ((int)s < 0)
    {
        return TRANSPORT_STATUS_ERROR;
    }

    udp_set_nonblock(s);

    if (udp_addr_make(&addr, cfg->ip, cfg->port) != 0)
    {
        udp_close(s);
        return TRANSPORT_STATUS_INVALID_ARG;
    }

    if (cfg->server)
    {
        if (bind(s, (struct sockaddr*)&addr.storage, addr.len) < 0)
        {
            udp_close(s);
            return TRANSPORT_STATUS_ERROR;
        }

        ctx->sock = s;

        for (uint16_t i = 0; i < conn->max_channels; i++)
        {
            transport_channel_t* ch = &conn->channels[i];
            udp_channel_ctx_t* cctx = ch_ctx(ch);

            if (cctx == NULL)
            {
                udp_close(s);
                return TRANSPORT_STATUS_INVALID_ARG;
            }

            udp_channel_ctx_reset(cctx);
            cctx->sock = s;
            ch->in_use = false;
        }
    }
    else
    {
        transport_channel_t* ch = &conn->channels[0];
        udp_channel_ctx_t* cctx = ch_ctx(ch);

        if (cctx == NULL)
        {
            udp_close(s);
            return TRANSPORT_STATUS_INVALID_ARG;
        }

        ctx->sock = s;

        udp_channel_ctx_reset(cctx);
        cctx->sock      = s;
        cctx->peer_addr = addr;
        cctx->has_peer  = 1;
        ch->in_use      = true;
    }

    return TRANSPORT_STATUS_OK;
}

static transport_status_t udp_close_conn(transport_connection_t* conn)
{
    udp_conn_ctx_t* ctx = conn_ctx(conn);

    for (uint16_t i = 0; i < conn->max_channels; i++)
    {
        transport_channel_t* ch = &conn->channels[i];
        if (ch->in_use)
        {
            udp_channel_close(ch);
        }
    }

    if (ctx->sock)
    {
        udp_close(ctx->sock);
        ctx->sock = 0;
    }

    return TRANSPORT_STATUS_OK;
}

static int udp_poll(transport_connection_t* conn, uint32_t timeout_ms)
{
    int r;

    (void)timeout_ms;

    r = udp_pump_connection(conn);
    if (r < 0)
    {
        return r;
    }

    return 0;
}

/* =========================================================
 * Ops tables
 * ========================================================= */

static const transport_channel_ops_t ch_ops = {
    .send  = udp_send,
    .recv  = udp_recv,
    .close = udp_channel_close,
};

static const transport_connection_ops_t conn_ops = {
    .open  = udp_open,
    .close = udp_close_conn,
    .poll  = udp_poll,
};

/* =========================================================
 * Public init
 * ========================================================= */

transport_status_t transport_udp_init(transport_connection_t* conn, const void* args)
{
    (void)args;
    conn->conn_ops = &conn_ops;
    conn->ch_ops   = &ch_ops;
    conn->caps     = TRANSPORT_CAP_DATAGRAM | TRANSPORT_CAP_ASYNC;
    return TRANSPORT_STATUS_OK;
}