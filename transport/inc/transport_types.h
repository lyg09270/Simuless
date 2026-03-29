#pragma once

/**
 * @file transport_types.h
 * @brief Minimal generic transport abstraction (connection + channel model)
 *
 * This module defines a lightweight and extensible transport layer for:
 *
 * - TCP / UDP / UART / custom transports
 * - Embedded and desktop environments
 * - Blocking and non-blocking backends
 *
 * Design principles:
 *
 * - Clear separation:
 *     - connection = control plane
 *     - channel    = data plane
 *
 * - No lifecycle leakage:
 *     - channel is owned and managed internally by connection
 *
 * - Capability-driven:
 *     - behavior is defined via caps
 *
 * - No dynamic allocation required
 *
 * - No inline functions (ABI stable, testable)
 *
 * Typical mapping:
 *
 * - UART:
 *     - 1 connection
 *     - 1 channel
 *
 * - TCP client:
 *     - 1 connection
 *     - 1 channel
 *
 * - TCP server:
 *     - 1 connection
 *     - multiple channels (accepted clients)
 */

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* =========================================================
 * Config
 * ========================================================= */

/**
 * @brief Maximum number of channels per connection.
 *
 * This is a compile-time limit. Runtime usage is controlled
 * by transport_connection_t::max_channels.
 */
#ifndef TRANSPORT_MAX_CHANNELS
#define TRANSPORT_MAX_CHANNELS 1u
#endif

/**
 * @brief Inline backend storage size in bytes.
 *
 * This storage is used to hold backend-specific connection context
 * without heap allocation.
 *
 * Every backend must satisfy:
 * @code
 * sizeof(backend_connection_context_t) <= TRANSPORT_IMPL_SIZE
 * @endcode
 */
#ifndef TRANSPORT_IMPL_SIZE
#define TRANSPORT_IMPL_SIZE 128u
#endif

    /* =========================================================
     * Status
     * ========================================================= */

    /**
     * @brief Transport status codes.
     */
    typedef enum
    {
        TRANSPORT_STATUS_OK            = 0,  /**< Success */
        TRANSPORT_STATUS_ERROR         = -1, /**< Generic error */
        TRANSPORT_STATUS_INVALID_ARG   = -2, /**< Invalid argument */
        TRANSPORT_STATUS_TIMEOUT       = -3, /**< Timeout */
        TRANSPORT_STATUS_CLOSED        = -4, /**< Closed */
        TRANSPORT_STATUS_WOULD_BLOCK   = -5, /**< Non-blocking no progress */
        TRANSPORT_STATUS_BUSY          = -6, /**< Resource busy */
        TRANSPORT_STATUS_NOT_SUPPORTED = -7  /**< Operation not supported */
    } transport_status_t;

    /* =========================================================
     * Transport type (factory key)
     * ========================================================= */

    /**
     * @brief Transport backend type.
     *
     * Used by transport_connection_create() to construct backend.
     */
    typedef enum
    {
        TRANSPORT_TYPE_NONE = 0,

        TRANSPORT_TYPE_TCP_CLIENT, /**< TCP client */
        TRANSPORT_TYPE_TCP_SERVER, /**< TCP server */

        TRANSPORT_TYPE_UDP, /**< UDP */

        TRANSPORT_TYPE_UART, /**< UART */

        TRANSPORT_TYPE_CUSTOM /**< User-defined */
    } transport_type_t;

    /* =========================================================
     * Capability flags
     * ========================================================= */

    /**
     * @brief Transport capability flags.
     *
     * Used to describe runtime behavior of a transport connection.
     */
    typedef enum
    {
        TRANSPORT_CAP_NONE     = 0,
        TRANSPORT_CAP_ASYNC    = (1U << 0), /**< Non-blocking, requires polling */
        TRANSPORT_CAP_EVENT    = (1U << 1), /**< Event-driven (future extension) */
        TRANSPORT_CAP_STREAM   = (1U << 2), /**< Stream-based (TCP / UART) */
        TRANSPORT_CAP_DATAGRAM = (1U << 3)  /**< Datagram-based (UDP) */
    } transport_cap_t;

    /* =========================================================
     * Forward declarations
     * ========================================================= */

    typedef struct transport_channel transport_channel_t;
    typedef struct transport_channel_ops transport_channel_ops_t;

    typedef struct transport_connection transport_connection_t;
    typedef struct transport_connection_ops transport_connection_ops_t;

    /* =========================================================
     * Channel (data plane)
     * ========================================================= */

    /**
     * @brief Channel operation table.
     *
     * Shared across all channels within a connection.
     */
    struct transport_channel_ops
    {
        /**
         * @brief Send data.
         *
         * @param ch Channel pointer
         * @param data Data buffer
         * @param len Number of bytes to send
         *
         * @return
         *   >0 : bytes sent
         *   =0 : no progress
         *   <0 : error (transport_status_t)
         */
        int (*send)(transport_channel_t* channel, const void* data, uint32_t len);

        /**
         * @brief Receive data.
         *
         * @param ch Channel pointer
         * @param buf Output buffer
         * @param len Buffer size
         *
         * @return
         *   >0 : bytes received
         *   =0 : no data
         *   <0 : error (transport_status_t)
         */
        int (*recv)(transport_channel_t* channel, void* buf, uint32_t len);

        /**
         * @brief Close channel.
         *
         * Backend must release all resources related to this channel.
         *
         * @param channel Channel pointer
         * @return transport_status_t
         */
        transport_status_t (*close)(transport_channel_t* channel);
    };

    /**
     * @brief Channel object.
     *
     * Owned and managed by transport_connection_t.
     */
    struct transport_channel
    {
        transport_connection_t* parent; /**< Owning connection */
        void* impl;                     /**< Backend-specific channel context */
        uint16_t id;                    /**< Channel index */
        bool in_use;                    /**< Channel active flag */
    };

    /* =========================================================
     * Connection (control plane)
     * ========================================================= */

    /**
     * @brief Connection operation table.
     */
    struct transport_connection_ops
    {
        /**
         * @brief Open / initialize connection.
         *
         * Behavior depends on backend:
         * - TCP client: connect
         * - TCP server: listen
         * - UDP: bind
         * - UART: init
         *
         * @param conn Connection object
         * @param args Backend-specific arguments
         *
         * @return transport_status_t
         */
        transport_status_t (*open)(transport_connection_t* conn, const void* args);

        /**
         * @brief Close connection and all channels.
         *
         * @param conn Connection object
         * @return transport_status_t
         */
        transport_status_t (*close)(transport_connection_t* conn);

        /**
         * @brief Poll connection state.
         *
         * Used for:
         * - Accept new channels (server)
         * - Progress async IO
         * - Handle internal state transitions
         *
         * @param conn Connection object
         * @param timeout_ms Timeout in milliseconds
         *
         * @return
         *   >=0 : backend-defined
         *   <0  : error
         */
        int (*poll)(transport_connection_t* conn, uint32_t timeout_ms);
    };

    /**
     * @brief Transport connection object.
     */
    struct transport_connection
    {
        const transport_connection_ops_t* conn_ops; /**< Connection ops */
        const transport_channel_ops_t* ch_ops;      /**< Channel ops */

        /**
         * @brief Inline backend storage.
         *
         * Backend implementations cast this buffer to their own connection
         * context structure.
         */
        uint8_t impl[TRANSPORT_IMPL_SIZE];

        uint32_t caps;         /**< Capability flags (transport_cap_t) */
        uint16_t max_channels; /**< Runtime channel capacity */

        transport_channel_t channels[TRANSPORT_MAX_CHANNELS]; /**< Channel storage */
    };

    /* =========================================================
     * Public API (no inline)
     * ========================================================= */

    /**
     * @brief Create a transport connection by type.
     *
     * Factory entry point.
     *
     * @param type Transport type
     * @param conn Output connection object
     * @param args Backend-specific arguments
     *
     * @return transport_status_t
     */
    transport_status_t transport_connection_create(transport_type_t type,
                                                   transport_connection_t* conn, const void* args);

    /**
     * @brief Bind externally provided channel storage.
     *
     * The storage is split into @p max_channels blocks, each block having
     * @p stride bytes. Each block is assigned to one channel as its backend
     * implementation context.
     *
     * Must be called after transport_connection_create() and before
     * transport_connection_open().
     *
     * @param conn Connection object
     * @param max_channels Number of channels to bind
     * @param storage Base address of channel context array
     * @param stride Size in bytes of each channel context object
     *
     * @return transport_status_t
     */
    transport_status_t transport_connection_bind_channel_storage(transport_connection_t* conn,
                                                                 uint16_t max_channels,
                                                                 void* storage, uint32_t stride);

    /**
     * @brief Open connection.
     *
     * @param conn Connection object
     * @param args Backend-specific arguments
     *
     * @return transport_status_t
     */
    transport_status_t transport_connection_open(transport_connection_t* conn, const void* args);

    /**
     * @brief Close connection.
     *
     * @param conn Connection object
     *
     * @return transport_status_t
     */
    transport_status_t transport_connection_close(transport_connection_t* conn);

    /**
     * @brief Poll connection.
     *
     * @param conn Connection object
     * @param timeout_ms Timeout in milliseconds
     *
     * @return
     *   >=0 : backend-defined
     *   <0  : error
     */
    int transport_connection_poll(transport_connection_t* conn, uint32_t timeout_ms);

    /**
     * @brief Send data through a channel.
     *
     * @param ch Channel object
     * @param data Data buffer
     * @param len Number of bytes to send
     *
     * @return
     *   >0 : bytes sent
     *   =0 : no progress
     *   <0 : error
     */
    int transport_channel_send(transport_channel_t* channel, const void* data, uint32_t len);

    /**
     * @brief Receive data from a channel.
     *
     * @param ch Channel object
     * @param buf Output buffer
     * @param len Buffer size
     *
     * @return
     *   >0 : bytes received
     *   =0 : no data
     *   <0 : error
     */
    int transport_channel_recv(transport_channel_t* channel, void* buf, uint32_t len);

    /**
     * @brief Close a channel.
     *
     * @param channel Channel object
     * @return transport_status_t
     */
    transport_status_t transport_channel_close(transport_channel_t* channel);
    /**
     * @brief Get channel by index.
     *
     * @param conn Connection object
     * @param index Channel index
     *
     * @return Channel pointer or NULL
     */
    transport_channel_t* transport_connection_get_channel(transport_connection_t* conn,
                                                          uint16_t index);

#ifdef __cplusplus
}
#endif