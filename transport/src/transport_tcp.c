#include "transport_tcp.h"

#include <errno.h>
#include <string.h>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
_Static_assert(sizeof(transport_tcp_connection_t) <= TRANSPORT_IMPL_SIZE,
               "transport_tcp_connection_t exceeds TRANSPORT_IMPL_SIZE");
#endif

/* =========================================================
 * Internal helpers
 * ========================================================= */

/**
 * @brief Set socket to non-blocking mode.
 *
 * @param fd Socket file descriptor
 * @return transport_status_t
 */
static transport_status_t transport_tcp_set_nonblocking(int fd)
{
    int flags;

    flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
    {
        return TRANSPORT_STATUS_ERROR;
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        return TRANSPORT_STATUS_ERROR;
    }

    return TRANSPORT_STATUS_OK;
}

/**
 * @brief Close socket if valid.
 *
 * @param fd Socket file descriptor
 */
static void transport_tcp_close_fd(int fd)
{
    if (fd >= 0)
    {
        close(fd);
    }
}

/**
 * @brief Get TCP connection context from inline storage.
 *
 * @param conn Connection object
 * @return TCP connection context pointer or NULL
 */
static transport_tcp_connection_t* transport_tcp_get_ctx(transport_connection_t* conn)
{
    if (conn == NULL)
    {
        return NULL;
    }

    return (transport_tcp_connection_t*)conn->impl;
}

/**
 * @brief Initialize channel context array.
 *
 * @param tcp TCP connection context
 */
static void transport_tcp_init_channel_ctx(transport_tcp_connection_t* tcp)
{
    uint16_t i;

    for (i = 0u; i < TRANSPORT_MAX_CHANNELS; ++i)
    {
        tcp->channel_ctx[i].fd = -1;
    }
}

/**
 * @brief Build sockaddr_in from configuration.
 *
 * @param addr Output address
 * @param ip IP string
 * @param port Port number
 * @param allow_any Non-zero to allow NULL IP as INADDR_ANY
 *
 * @return transport_status_t
 */
static transport_status_t transport_tcp_make_addr(struct sockaddr_in* addr, const char* ip,
                                                  uint16_t port, int allow_any)
{
    if ((addr == NULL) || (port == 0u))
    {
        return TRANSPORT_STATUS_INVALID_ARG;
    }

    memset(addr, 0, sizeof(*addr));
    addr->sin_family = AF_INET;
    addr->sin_port   = htons(port);

    if (ip == NULL)
    {
        if (!allow_any)
        {
            return TRANSPORT_STATUS_INVALID_ARG;
        }

        addr->sin_addr.s_addr = htonl(INADDR_ANY);
        return TRANSPORT_STATUS_OK;
    }

    if (inet_pton(AF_INET, ip, &addr->sin_addr) != 1)
    {
        return TRANSPORT_STATUS_INVALID_ARG;
    }

    return TRANSPORT_STATUS_OK;
}

/**
 * @brief Find a free channel slot.
 *
 * @param conn Connection object
 * @return Free channel pointer or NULL
 */
static transport_channel_t* transport_tcp_find_free_channel(transport_connection_t* conn)
{
    uint16_t i;

    if (conn == NULL)
    {
        return NULL;
    }

    for (i = 0u; i < conn->max_channels; ++i)
    {
        if (!conn->channels[i].in_use)
        {
            return &conn->channels[i];
        }
    }

    return NULL;
}

/**
 * @brief Deactivate a channel and close its socket.
 *
 * @param conn Connection object
 * @param ch Channel object
 */
static void transport_tcp_deactivate_channel(transport_connection_t* conn, transport_channel_t* ch)
{
    transport_tcp_connection_t* tcp;
    transport_tcp_channel_t* ctx;

    if ((conn == NULL) || (ch == NULL) || (ch->impl == NULL))
    {
        return;
    }

    tcp = transport_tcp_get_ctx(conn);
    if (tcp == NULL)
    {
        return;
    }

    ctx = (transport_tcp_channel_t*)ch->impl;

    if (ctx->fd >= 0)
    {
        transport_tcp_close_fd(ctx->fd);
        ctx->fd = -1;
    }

    ch->impl   = NULL;
    ch->in_use = false;

    if ((tcp->role == TRANSPORT_TCP_ROLE_CLIENT) && (ch->id == 0u))
    {
        tcp->conn_fd = -1;
    }
}

/**
 * @brief Activate one channel with a connected socket.
 *
 * @param conn Connection object
 * @param index Channel index
 * @param fd Connected socket file descriptor
 *
 * @return transport_status_t
 */
static transport_status_t transport_tcp_activate_channel(transport_connection_t* conn,
                                                         uint16_t index, int fd)
{
    transport_tcp_connection_t* tcp;
    transport_channel_t* ch;
    transport_tcp_channel_t* ctx;

    if ((conn == NULL) || (fd < 0))
    {
        return TRANSPORT_STATUS_INVALID_ARG;
    }

    if (index >= conn->max_channels)
    {
        return TRANSPORT_STATUS_INVALID_ARG;
    }

    tcp = transport_tcp_get_ctx(conn);
    if (tcp == NULL)
    {
        return TRANSPORT_STATUS_INVALID_ARG;
    }

    ch  = &conn->channels[index];
    ctx = &tcp->channel_ctx[index];

    ctx->fd    = fd;
    ch->impl   = ctx;
    ch->in_use = true;

    if ((tcp->role == TRANSPORT_TCP_ROLE_CLIENT) && (index == 0u))
    {
        tcp->conn_fd = fd;
    }

    return TRANSPORT_STATUS_OK;
}

/**
 * @brief Check whether a socket is dead.
 *
 * @param fd Socket file descriptor
 * @return 1 if closed or errored, otherwise 0
 */
static int transport_tcp_socket_dead(int fd)
{
    char c;
    int ret;

    ret = (int)recv(fd, &c, 1, MSG_PEEK | MSG_DONTWAIT);
    if (ret > 0)
    {
        return 0;
    }

    if (ret == 0)
    {
        return 1;
    }

    if ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR))
    {
        return 0;
    }

    return 1;
}

/**
 * @brief Accept all pending incoming connections.
 *
 * @param conn Connection object
 * @return Number of accepted channels, or negative error
 */
static int transport_tcp_accept_pending(transport_connection_t* conn)
{
    transport_tcp_connection_t* tcp;
    int count;
    int fd;
    struct sockaddr_in addr;
    socklen_t addr_len;
    transport_channel_t* free_ch;

    tcp = transport_tcp_get_ctx(conn);
    if ((tcp == NULL) || (tcp->listen_fd < 0))
    {
        return TRANSPORT_STATUS_INVALID_ARG;
    }

    count = 0;

    for (;;)
    {
        addr_len = (socklen_t)sizeof(addr);
        fd       = accept(tcp->listen_fd, (struct sockaddr*)&addr, &addr_len);
        if (fd < 0)
        {
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
            {
                break;
            }

            if (errno == EINTR)
            {
                continue;
            }

            return TRANSPORT_STATUS_ERROR;
        }

        if (transport_tcp_set_nonblocking(fd) != TRANSPORT_STATUS_OK)
        {
            transport_tcp_close_fd(fd);
            continue;
        }

        free_ch = transport_tcp_find_free_channel(conn);
        if (free_ch == NULL)
        {
            transport_tcp_close_fd(fd);
            continue;
        }

        if (transport_tcp_activate_channel(conn, free_ch->id, fd) != TRANSPORT_STATUS_OK)
        {
            transport_tcp_close_fd(fd);
            continue;
        }

        ++count;
    }

    return count;
}

/* =========================================================
 * Channel ops
 * ========================================================= */

/**
 * @brief Send data through a TCP channel.
 *
 * @param ch Channel object
 * @param data Data buffer
 * @param len Data length
 *
 * @return int
 */
static int transport_tcp_channel_send_impl(transport_channel_t* ch, const void* data, uint32_t len)
{
    transport_tcp_channel_t* ctx;
    ssize_t ret;

    if ((ch == NULL) || (ch->impl == NULL))
    {
        return TRANSPORT_STATUS_INVALID_ARG;
    }

    if ((data == NULL) && (len > 0u))
    {
        return TRANSPORT_STATUS_INVALID_ARG;
    }

    ctx = (transport_tcp_channel_t*)ch->impl;
    if (ctx->fd < 0)
    {
        return TRANSPORT_STATUS_CLOSED;
    }

    ret = send(ctx->fd, data, (size_t)len, 0);
    if (ret > 0)
    {
        return (int)ret;
    }

    if (ret == 0)
    {
        return TRANSPORT_STATUS_WOULD_BLOCK;
    }

    if ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR))
    {
        return TRANSPORT_STATUS_WOULD_BLOCK;
    }

    return TRANSPORT_STATUS_ERROR;
}

/**
 * @brief Receive data from a TCP channel.
 *
 * @param ch Channel object
 * @param buf Output buffer
 * @param len Buffer size
 *
 * @return int
 */
static int transport_tcp_channel_recv_impl(transport_channel_t* ch, void* buf, uint32_t len)
{
    transport_connection_t* conn;
    transport_tcp_channel_t* ctx;
    ssize_t ret;

    if ((ch == NULL) || (ch->impl == NULL))
    {
        return TRANSPORT_STATUS_INVALID_ARG;
    }

    if ((buf == NULL) && (len > 0u))
    {
        return TRANSPORT_STATUS_INVALID_ARG;
    }

    conn = ch->parent;
    ctx  = (transport_tcp_channel_t*)ch->impl;

    if (ctx->fd < 0)
    {
        return TRANSPORT_STATUS_CLOSED;
    }

    ret = recv(ctx->fd, buf, (size_t)len, 0);
    if (ret > 0)
    {
        return (int)ret;
    }

    if (ret == 0)
    {
        transport_tcp_deactivate_channel(conn, ch);
        return TRANSPORT_STATUS_CLOSED;
    }

    if ((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINTR))
    {
        return TRANSPORT_STATUS_WOULD_BLOCK;
    }

    transport_tcp_deactivate_channel(conn, ch);
    return TRANSPORT_STATUS_ERROR;
}

/**
 * @brief TCP channel operations table.
 */
static const transport_channel_ops_t g_transport_tcp_channel_ops = {
    .send = transport_tcp_channel_send_impl, .recv = transport_tcp_channel_recv_impl};

/* =========================================================
 * Connection ops
 * ========================================================= */

/**
 * @brief Open TCP connection or listener.
 *
 * @param conn Connection object
 * @param args Pointer to transport_tcp_args_t
 *
 * @return transport_status_t
 */
static transport_status_t transport_tcp_open_impl(transport_connection_t* conn, const void* args)
{
    transport_tcp_connection_t* tcp;
    const transport_tcp_args_t* cfg;
    struct sockaddr_in addr;
    int fd;
    int one;
    int ret;

    if ((conn == NULL) || (args == NULL))
    {
        return TRANSPORT_STATUS_INVALID_ARG;
    }

    tcp = transport_tcp_get_ctx(conn);
    cfg = (const transport_tcp_args_t*)args;

    if (tcp == NULL)
    {
        return TRANSPORT_STATUS_INVALID_ARG;
    }

    if (tcp->role == TRANSPORT_TCP_ROLE_CLIENT)
    {
        if (transport_tcp_make_addr(&addr, cfg->ip, cfg->port, 0) != TRANSPORT_STATUS_OK)
        {
            return TRANSPORT_STATUS_INVALID_ARG;
        }

        fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0)
        {
            return TRANSPORT_STATUS_ERROR;
        }

        if (transport_tcp_set_nonblocking(fd) != TRANSPORT_STATUS_OK)
        {
            transport_tcp_close_fd(fd);
            return TRANSPORT_STATUS_ERROR;
        }

        ret = connect(fd, (struct sockaddr*)&addr, sizeof(addr));
        if ((ret < 0) && (errno != EINPROGRESS))
        {
            transport_tcp_close_fd(fd);
            return TRANSPORT_STATUS_ERROR;
        }

        if (transport_tcp_activate_channel(conn, 0u, fd) != TRANSPORT_STATUS_OK)
        {
            transport_tcp_close_fd(fd);
            return TRANSPORT_STATUS_ERROR;
        }

        return TRANSPORT_STATUS_OK;
    }

    if (tcp->role == TRANSPORT_TCP_ROLE_SERVER)
    {
        if (transport_tcp_make_addr(&addr, cfg->ip, cfg->port, 1) != TRANSPORT_STATUS_OK)
        {
            return TRANSPORT_STATUS_INVALID_ARG;
        }

        fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0)
        {
            return TRANSPORT_STATUS_ERROR;
        }

        one = 1;
        (void)setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, (socklen_t)sizeof(one));

        if (transport_tcp_set_nonblocking(fd) != TRANSPORT_STATUS_OK)
        {
            transport_tcp_close_fd(fd);
            return TRANSPORT_STATUS_ERROR;
        }

        if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        {
            transport_tcp_close_fd(fd);
            return TRANSPORT_STATUS_ERROR;
        }

        if (listen(fd, (int)conn->max_channels) < 0)
        {
            transport_tcp_close_fd(fd);
            return TRANSPORT_STATUS_ERROR;
        }

        tcp->listen_fd = fd;
        return TRANSPORT_STATUS_OK;
    }

    return TRANSPORT_STATUS_NOT_SUPPORTED;
}

/**
 * @brief Close TCP backend.
 *
 * @param conn Connection object
 * @return transport_status_t
 */
static transport_status_t transport_tcp_close_impl(transport_connection_t* conn)
{
    transport_tcp_connection_t* tcp;
    uint16_t i;

    if (conn == NULL)
    {
        return TRANSPORT_STATUS_INVALID_ARG;
    }

    tcp = transport_tcp_get_ctx(conn);
    if (tcp == NULL)
    {
        return TRANSPORT_STATUS_INVALID_ARG;
    }

    for (i = 0u; i < conn->max_channels; ++i)
    {
        if (conn->channels[i].in_use)
        {
            transport_tcp_deactivate_channel(conn, &conn->channels[i]);
        }
    }

    if (tcp->listen_fd >= 0)
    {
        transport_tcp_close_fd(tcp->listen_fd);
        tcp->listen_fd = -1;
    }

    if (tcp->conn_fd >= 0)
    {
        transport_tcp_close_fd(tcp->conn_fd);
        tcp->conn_fd = -1;
    }

    conn->conn_ops = NULL;
    conn->ch_ops   = NULL;
    memset(conn->impl, 0, sizeof(conn->impl));
    conn->caps         = TRANSPORT_CAP_NONE;
    conn->max_channels = 0u;

    return TRANSPORT_STATUS_OK;
}

/**
 * @brief Poll TCP backend.
 *
 * @param conn Connection object
 * @param timeout_ms Timeout in milliseconds
 *
 * @return
 *   >=0 : number of accepted/closed/changed channels
 *   <0  : error
 */
static int transport_tcp_poll_impl(transport_connection_t* conn, uint32_t timeout_ms)
{
    transport_tcp_connection_t* tcp;
    fd_set rfds;
    fd_set wfds;
    fd_set efds;
    struct timeval tv;
    int maxfd;
    int ret;
    int changes;
    uint16_t i;

    if (conn == NULL)
    {
        return TRANSPORT_STATUS_INVALID_ARG;
    }

    tcp = transport_tcp_get_ctx(conn);
    if (tcp == NULL)
    {
        return TRANSPORT_STATUS_INVALID_ARG;
    }

    FD_ZERO(&rfds);
    FD_ZERO(&wfds);
    FD_ZERO(&efds);

    maxfd   = -1;
    changes = 0;

    if (tcp->listen_fd >= 0)
    {
        FD_SET(tcp->listen_fd, &rfds);
        FD_SET(tcp->listen_fd, &efds);

        if (tcp->listen_fd > maxfd)
        {
            maxfd = tcp->listen_fd;
        }
    }

    for (i = 0u; i < conn->max_channels; ++i)
    {
        transport_tcp_channel_t* ch_ctx;

        if (!conn->channels[i].in_use || (conn->channels[i].impl == NULL))
        {
            continue;
        }

        ch_ctx = (transport_tcp_channel_t*)conn->channels[i].impl;
        if (ch_ctx->fd < 0)
        {
            continue;
        }

        FD_SET(ch_ctx->fd, &rfds);
        FD_SET(ch_ctx->fd, &efds);

        if ((tcp->role == TRANSPORT_TCP_ROLE_CLIENT) && (i == 0u))
        {
            FD_SET(ch_ctx->fd, &wfds);
        }

        if (ch_ctx->fd > maxfd)
        {
            maxfd = ch_ctx->fd;
        }
    }

    if (maxfd < 0)
    {
        return 0;
    }

    tv.tv_sec  = (long)(timeout_ms / 1000u);
    tv.tv_usec = (long)((timeout_ms % 1000u) * 1000u);

    ret = select(maxfd + 1, &rfds, &wfds, &efds, &tv);
    if (ret < 0)
    {
        if (errno == EINTR)
        {
            return 0;
        }

        return TRANSPORT_STATUS_ERROR;
    }

    if (ret == 0)
    {
        return 0;
    }

    if ((tcp->listen_fd >= 0) &&
        (FD_ISSET(tcp->listen_fd, &rfds) || FD_ISSET(tcp->listen_fd, &efds)))
    {
        ret = transport_tcp_accept_pending(conn);
        if (ret < 0)
        {
            return ret;
        }

        changes += ret;
    }

    for (i = 0u; i < conn->max_channels; ++i)
    {
        transport_channel_t* ch;
        transport_tcp_channel_t* ch_ctx;

        ch = &conn->channels[i];
        if (!ch->in_use || (ch->impl == NULL))
        {
            continue;
        }

        ch_ctx = (transport_tcp_channel_t*)ch->impl;
        if (ch_ctx->fd < 0)
        {
            continue;
        }

        if (FD_ISSET(ch_ctx->fd, &efds))
        {
            transport_tcp_deactivate_channel(conn, ch);
            ++changes;
            continue;
        }

        if (FD_ISSET(ch_ctx->fd, &wfds))
        {
            int so_error;
            socklen_t len;

            so_error = 0;
            len      = (socklen_t)sizeof(so_error);

            if ((getsockopt(ch_ctx->fd, SOL_SOCKET, SO_ERROR, &so_error, &len) < 0) ||
                (so_error != 0))
            {
                transport_tcp_deactivate_channel(conn, ch);
                ++changes;
                continue;
            }
        }

        if (FD_ISSET(ch_ctx->fd, &rfds))
        {
            if (transport_tcp_socket_dead(ch_ctx->fd))
            {
                transport_tcp_deactivate_channel(conn, ch);
                ++changes;
                continue;
            }
        }
    }

    return changes;
}

/**
 * @brief TCP connection operations table.
 */
static const transport_connection_ops_t g_transport_tcp_connection_ops = {
    .open  = transport_tcp_open_impl,
    .close = transport_tcp_close_impl,
    .poll  = transport_tcp_poll_impl};

/* =========================================================
 * Backend init
 * ========================================================= */

transport_status_t transport_tcp_client_init(transport_connection_t* conn, const void* args)
{
    transport_tcp_connection_t* tcp;

    (void)args;

    if (conn == NULL)
    {
        return TRANSPORT_STATUS_INVALID_ARG;
    }

    tcp = transport_tcp_get_ctx(conn);
    memset(tcp, 0, sizeof(*tcp));

    tcp->listen_fd = -1;
    tcp->conn_fd   = -1;
    tcp->role      = TRANSPORT_TCP_ROLE_CLIENT;
    transport_tcp_init_channel_ctx(tcp);

    conn->conn_ops     = &g_transport_tcp_connection_ops;
    conn->ch_ops       = &g_transport_tcp_channel_ops;
    conn->caps         = TRANSPORT_CAP_ASYNC | TRANSPORT_CAP_STREAM;
    conn->max_channels = 1u;

    return TRANSPORT_STATUS_OK;
}

transport_status_t transport_tcp_server_init(transport_connection_t* conn, const void* args)
{
    transport_tcp_connection_t* tcp;

    (void)args;

    if (conn == NULL)
    {
        return TRANSPORT_STATUS_INVALID_ARG;
    }

    tcp = transport_tcp_get_ctx(conn);
    memset(tcp, 0, sizeof(*tcp));

    tcp->listen_fd = -1;
    tcp->conn_fd   = -1;
    tcp->role      = TRANSPORT_TCP_ROLE_SERVER;
    transport_tcp_init_channel_ctx(tcp);

    conn->conn_ops     = &g_transport_tcp_connection_ops;
    conn->ch_ops       = &g_transport_tcp_channel_ops;
    conn->caps         = TRANSPORT_CAP_ASYNC | TRANSPORT_CAP_STREAM;
    conn->max_channels = TRANSPORT_MAX_CHANNELS;

    return TRANSPORT_STATUS_OK;
}