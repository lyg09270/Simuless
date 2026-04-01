#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Unified socket handle.
     *
     * Compatible with POSIX fd, Windows SOCKET,
     * and embedded socket handles.
     */
    typedef intptr_t smls_socket_t;

/**
 * @brief Invalid socket handle.
 */
#define SMLS_SOCKET_INVALID ((smls_socket_t)(-1))

    /**
     * @brief Socket port status codes.
     */
    typedef enum
    {
        SMLS_SOCKET_OK              = 0,
        SMLS_SOCKET_ERR_INVALID_ARG = -1,
        SMLS_SOCKET_ERR_IO          = -2,
        SMLS_SOCKET_ERR_TIMEOUT     = -3,
        SMLS_SOCKET_ERR_CLOSED      = -4,
        SMLS_SOCKET_ERR_WOULD_BLOCK = -5
    } smls_socket_status_t;

    /**
     * @brief Create TCP stream socket.
     */
    int smls_socket_create(smls_socket_t* sock);

    /**
     * @brief Bind socket to local address.
     */
    int smls_socket_bind(smls_socket_t sock, const char* ip, uint16_t port);

    /**
     * @brief Put socket into listening mode.
     */
    int smls_socket_listen(smls_socket_t sock, int backlog);

    /**
     * @brief Accept incoming connection.
     */
    int smls_socket_accept(smls_socket_t listen_sock, smls_socket_t* client_sock);

    /**
     * @brief Connect to remote endpoint.
     */
    int smls_socket_connect(smls_socket_t sock, const char* ip, uint16_t port);

    /**
     * @brief Send bytes.
     */
    int smls_socket_send(smls_socket_t sock, const void* data, uint32_t len);

    /**
     * @brief Receive bytes.
     */
    int smls_socket_recv(smls_socket_t sock, void* buf, uint32_t len);

    /**
     * @brief Wait for socket readability.
     *
     * @return >0 ready, 0 timeout, negative on error
     */
    int smls_socket_poll(smls_socket_t sock, uint32_t timeout_ms);

    /**
     * @brief Close socket.
     */
    int smls_socket_close(smls_socket_t sock);

#ifdef __cplusplus
}
#endif