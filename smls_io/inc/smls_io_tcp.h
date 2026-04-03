#pragma once

#include "smls_io.h"
#include "smls_socket_port.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct smls_io_tcp_ctx
    {
        smls_socket_t sock;
        smls_socket_t listen_sock;

        char host[64];
        uint16_t port;

        uint8_t is_server;
        uint8_t nonblock;

    } smls_io_tcp_ctx_t;

    /**
     * @brief Parse TCP URI into configuration.
     *
     * Example:
     * tcp://127.0.0.1:9000
     * tcp://0.0.0.0:9000
     *
     * @param url Input URI
     * @param IO descriptor
     * @return 0 on success, negative on error
     */
    int smls_io_tcp_open(smls_io_t* io, const smls_io_desc_t* desc);

    /**
     * @brief Close TCP backend.
     */
    int smls_io_tcp_close(smls_io_t* io);

    /**
     * @brief Push stream data over TCP.
     */
    int smls_io_tcp_push(smls_io_t* io, const smls_packet_t* pkt);

    /**
     * @brief Receive stream data from TCP.
     */
    int smls_io_tcp_pop(smls_io_t* io, smls_packet_t* pkt);

    /**
     * @brief Poll TCP socket.
     */
    int smls_io_tcp_poll(smls_io_t* io, uint32_t timeout_ms);

    /**
     * @brief TCP backend ops table.
     */
    extern const smls_io_ops_t g_smls_io_tcp_ops;

#ifdef __cplusplus
}
#endif
