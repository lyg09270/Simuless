#pragma once

/**
 * @file transport_tcp.h
 * @brief TCP transport backend (client + server)
 *
 * This module implements a TCP backend for the generic transport layer.
 *
 * Features:
 * - TCP client
 * - TCP server
 * - Non-blocking operation
 * - Poll-driven state progression
 * - Multi-channel accepted clients for server mode
 *
 * Notes:
 * - Client mode uses channel 0 as the active TCP stream
 * - Server mode uses channels[] for accepted client sockets
 * - All sockets are configured as non-blocking
 */

#include "transport_types.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief TCP role.
     */
    typedef enum
    {
        TRANSPORT_TCP_ROLE_CLIENT = 0, /**< TCP client */
        TRANSPORT_TCP_ROLE_SERVER = 1  /**< TCP server */
    } transport_tcp_role_t;

    /**
     * @brief TCP backend arguments.
     */
    typedef struct
    {
        transport_tcp_role_t role; /**< Client or server role */

        /**
         * @brief IP address string.
         *
         * Semantics:
         * - Client: remote server IP, must not be NULL
         * - Server: bind IP, NULL means 0.0.0.0
         */
        const char* ip;

        /**
         * @brief TCP port.
         *
         * Semantics:
         * - Client: remote server port
         * - Server: local listen port
         */
        uint16_t port;
    } transport_tcp_args_t;

    /**
     * @brief TCP channel context.
     *
     * One channel corresponds to one connected TCP socket.
     */
    typedef struct
    {
        int fd; /**< Connected socket file descriptor */
    } transport_tcp_channel_t;

    /**
     * @brief TCP connection context.
     *
     * This object is stored inline in transport_connection_t::impl.
     */
    typedef struct
    {
        int listen_fd; /**< Listen socket in server mode, otherwise -1 */
        int conn_fd;   /**< Client socket in client mode, otherwise -1 */

        transport_tcp_role_t role; /**< TCP role */

        transport_tcp_channel_t channel_ctx[TRANSPORT_MAX_CHANNELS]; /**< Per-channel storage */
    } transport_tcp_connection_t;

    /**
     * @brief Initialize TCP client backend.
     *
     * @param conn Connection object
     * @param args Pointer to transport_tcp_args_t
     *
     * @return transport_status_t
     */
    transport_status_t transport_tcp_client_init(transport_connection_t* conn, const void* args);

    /**
     * @brief Initialize TCP server backend.
     *
     * @param conn Connection object
     * @param args Pointer to transport_tcp_args_t
     *
     * @return transport_status_t
     */
    transport_status_t transport_tcp_server_init(transport_connection_t* conn, const void* args);

#ifdef __cplusplus
}
#endif