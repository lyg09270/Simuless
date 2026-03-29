#include "transport_types.h"

#include <stddef.h>
#include <string.h>

/* =========================================================
 * Backend init (implemented by each backend)
 * ========================================================= */

extern transport_status_t transport_tcp_client_init(transport_connection_t* conn, const void* args);
extern transport_status_t transport_tcp_server_init(transport_connection_t* conn, const void* args);
extern transport_status_t transport_udp_init(transport_connection_t* conn, const void* args);
extern transport_status_t transport_uart_init(transport_connection_t* conn, const void* args);

/* =========================================================
 * Internal helpers
 * ========================================================= */

/**
 * @brief Reset connection object to a clean state.
 *
 * @param conn Connection object
 */
static void transport_connection_reset(transport_connection_t* conn)
{
    uint16_t i;

    conn->conn_ops = NULL;
    conn->ch_ops   = NULL;
    memset(conn->impl, 0, sizeof(conn->impl));
    conn->caps         = TRANSPORT_CAP_NONE;
    conn->max_channels = 0u;

    for (i = 0u; i < TRANSPORT_MAX_CHANNELS; ++i)
    {
        conn->channels[i].parent = conn;
        conn->channels[i].impl   = NULL;
        conn->channels[i].id     = i;
        conn->channels[i].in_use = false;
    }
}

/**
 * @brief Get backend init function by transport type.
 *
 * @param type Transport type
 * @return Backend init function or NULL
 */
static transport_status_t (*transport_get_backend(transport_type_t type))(transport_connection_t*,
                                                                          const void*)
{
    switch (type)
    {
    case TRANSPORT_TYPE_TCP_CLIENT:
        return transport_tcp_client_init;

    case TRANSPORT_TYPE_TCP_SERVER:
        return transport_tcp_server_init;

    case TRANSPORT_TYPE_UDP:
        // TODO: add transport_uart_init
        //  return transport_udp_init;
        return NULL;

    case TRANSPORT_TYPE_UART:
        // TODO: add transport_uart_init
        //  return transport_uart_init;
        return NULL;

    default:
        return NULL;
    }
}

/**
 * @brief Validate connection object.
 *
 * @param conn Connection object
 * @return 1 if valid, otherwise 0
 */
static int transport_connection_valid(const transport_connection_t* conn)
{
    return (conn != NULL);
}

/**
 * @brief Validate channel object.
 *
 * @param ch Channel object
 * @return 1 if valid, otherwise 0
 */
static int transport_channel_valid(const transport_channel_t* ch)
{
    if (ch == NULL)
    {
        return 0;
    }

    if (ch->parent == NULL)
    {
        return 0;
    }

    return 1;
}

/* =========================================================
 * Public API
 * ========================================================= */

transport_status_t transport_connection_create(transport_type_t type, transport_connection_t* conn,
                                               const void* args)
{
    transport_status_t (*init_fn)(transport_connection_t*, const void*);

    if (conn == NULL)
    {
        return TRANSPORT_STATUS_INVALID_ARG;
    }

    transport_connection_reset(conn);

    init_fn = transport_get_backend(type);
    if (init_fn == NULL)
    {
        return TRANSPORT_STATUS_NOT_SUPPORTED;
    }

    return init_fn(conn, args);
}

transport_status_t transport_connection_bind_channel_storage(transport_connection_t* conn,
                                                             uint16_t max_channels, void* storage,
                                                             uint32_t stride)
{
    uint16_t i;
    uint8_t* base;

    if (conn == NULL || storage == NULL)
    {
        return TRANSPORT_STATUS_INVALID_ARG;
    }

    if (max_channels == 0u || max_channels > TRANSPORT_MAX_CHANNELS)
    {
        return TRANSPORT_STATUS_INVALID_ARG;
    }

    if (stride == 0u)
    {
        return TRANSPORT_STATUS_INVALID_ARG;
    }

    base               = (uint8_t*)storage;
    conn->max_channels = max_channels;

    for (i = 0u; i < max_channels; ++i)
    {
        conn->channels[i].parent = conn;
        conn->channels[i].impl   = (void*)(base + ((uint32_t)i * stride));
        conn->channels[i].id     = i;
        conn->channels[i].in_use = false;
    }

    return TRANSPORT_STATUS_OK;
}

transport_status_t transport_connection_open(transport_connection_t* conn, const void* args)
{
    if (!transport_connection_valid(conn))
    {
        return TRANSPORT_STATUS_INVALID_ARG;
    }

    if ((conn->conn_ops == NULL) || (conn->conn_ops->open == NULL))
    {
        return TRANSPORT_STATUS_NOT_SUPPORTED;
    }

    return conn->conn_ops->open(conn, args);
}

transport_status_t transport_connection_close(transport_connection_t* conn)
{
    if (!transport_connection_valid(conn))
    {
        return TRANSPORT_STATUS_INVALID_ARG;
    }

    if ((conn->conn_ops == NULL) || (conn->conn_ops->close == NULL))
    {
        return TRANSPORT_STATUS_NOT_SUPPORTED;
    }

    return conn->conn_ops->close(conn);
}

int transport_connection_poll(transport_connection_t* conn, uint32_t timeout_ms)
{
    if (!transport_connection_valid(conn))
    {
        return TRANSPORT_STATUS_INVALID_ARG;
    }

    if ((conn->conn_ops == NULL) || (conn->conn_ops->poll == NULL))
    {
        return TRANSPORT_STATUS_NOT_SUPPORTED;
    }

    return conn->conn_ops->poll(conn, timeout_ms);
}

int transport_channel_send(transport_channel_t* channel, const void* data, uint32_t len)
{
    const transport_channel_ops_t* ops;

    if (!transport_channel_valid(channel))
    {
        return TRANSPORT_STATUS_INVALID_ARG;
    }

    if (!channel->in_use)
    {
        return TRANSPORT_STATUS_CLOSED;
    }

    if ((data == NULL) && (len > 0u))
    {
        return TRANSPORT_STATUS_INVALID_ARG;
    }

    ops = channel->parent->ch_ops;
    if ((ops == NULL) || (ops->send == NULL))
    {
        return TRANSPORT_STATUS_NOT_SUPPORTED;
    }

    return ops->send(channel, data, len);
}

int transport_channel_recv(transport_channel_t* channel, void* buf, uint32_t len)
{
    const transport_channel_ops_t* ops;

    if (!transport_channel_valid(channel))
    {
        return TRANSPORT_STATUS_INVALID_ARG;
    }

    if (!channel->in_use)
    {
        return TRANSPORT_STATUS_CLOSED;
    }

    if ((buf == NULL) && (len > 0u))
    {
        return TRANSPORT_STATUS_INVALID_ARG;
    }

    ops = channel->parent->ch_ops;
    if ((ops == NULL) || (ops->recv == NULL))
    {
        return TRANSPORT_STATUS_NOT_SUPPORTED;
    }

    return ops->recv(channel, buf, len);
}

transport_channel_t* transport_connection_get_channel(transport_connection_t* conn, uint16_t index)
{
    if (!transport_connection_valid(conn))
    {
        return NULL;
    }

    if (index >= conn->max_channels)
    {
        return NULL;
    }

    return &conn->channels[index];
}