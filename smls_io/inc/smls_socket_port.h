#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef _WIN32
#include <winsock2.h>
typedef SOCKET smls_socket_t;
typedef int smls_socklen_t;
typedef struct sockaddr smls_sockaddr_t;
typedef int ssize_t;

#elif defined(SMLS_USE_LWIP)
#include <lwip/sockets.h>
typedef int smls_socket_t;
typedef socklen_t smls_socklen_t;
typedef struct sockaddr smls_sockaddr_t;

#else
#include <sys/socket.h>
#include <sys/types.h>
typedef int smls_socket_t;
typedef socklen_t smls_socklen_t;
typedef struct sockaddr smls_sockaddr_t;
#endif

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief Invalid socket handle.
 */
#define SMLS_SOCKET_INVALID ((smls_socket_t)(-1))

    /**
     * @brief Common cross-platform socket option flags.
     *
     * These flags support bitwise combination.
     */
    typedef enum
    {
        SMLS_SOCKET_OPT_NONE = 0u,

        /**
         * @brief SO_REUSEADDR
         */
        SMLS_SOCKET_OPT_REUSEADDR = 1u << 0,

        /**
         * @brief SO_KEEPALIVE
         */
        SMLS_SOCKET_OPT_KEEPALIVE = 1u << 1,

        /**
         * @brief SO_BROADCAST
         */
        SMLS_SOCKET_OPT_BROADCAST = 1u << 2,

        /**
         * @brief TCP_NODELAY
         */
        SMLS_SOCKET_OPT_TCP_NODELAY = 1u << 3,

        /**
         * @brief Receive timeout in milliseconds.
         *
         * This flag should be used with timeout API only.
         */
        SMLS_SOCKET_OPT_RCVTIMEO_MS = 1u << 4,

        /**
         * @brief Send timeout in milliseconds.
         *
         * This flag should be used with timeout API only.
         */
        SMLS_SOCKET_OPT_SNDTIMEO_MS = 1u << 5

    } smls_socket_option_t;

    /**
     * @brief Cross-platform socket().
     *
     * Same semantics as native socket().
     */
    smls_socket_t smls_socket(int domain, int type, int protocol);

    /**
     * @brief Cross-platform bind().
     *
     * Same semantics as native bind().
     */
    int smls_bind(smls_socket_t sock, const smls_sockaddr_t* addr, smls_socklen_t addrlen);

    /**
     * @brief Cross-platform listen().
     *
     * Same semantics as native listen().
     */
    int smls_listen(smls_socket_t sock, int backlog);

    /**
     * @brief Cross-platform accept().
     *
     * Same semantics as native accept().
     */
    smls_socket_t smls_accept(smls_socket_t sock, smls_sockaddr_t* addr, smls_socklen_t* addrlen);

    /**
     * @brief Cross-platform connect().
     *
     * Same semantics as native connect().
     */
    int smls_connect(smls_socket_t sock, const smls_sockaddr_t* addr, smls_socklen_t addrlen);

    /**
     * @brief Cross-platform send().
     *
     * Same semantics as native send().
     */
    ssize_t smls_send(smls_socket_t sock, const void* buf, size_t len, int flags);

    /**
     * @brief Cross-platform recv().
     *
     * Same semantics as native recv().
     */
    ssize_t smls_recv(smls_socket_t sock, void* buf, size_t len, int flags);

    /**
     * @brief Cross-platform sendto().
     *
     * Same semantics as native sendto().
     */
    ssize_t smls_sendto(smls_socket_t sock, const void* buf, size_t len, int flags,
                        const smls_sockaddr_t* addr, smls_socklen_t addrlen);

    /**
     * @brief Cross-platform recvfrom().
     *
     * Same semantics as native recvfrom().
     */
    ssize_t smls_recvfrom(smls_socket_t sock, void* buf, size_t len, int flags,
                          smls_sockaddr_t* addr, smls_socklen_t* addrlen);

    /**
     * @brief Cross-platform closesocket()/close().
     *
     * Same semantics as native close on POSIX and closesocket on Windows.
     */
    int smls_close(smls_socket_t sock);

    /**
     * @brief Cross-platform setsockopt().
     *
     * Same semantics as native setsockopt().
     */
    int smls_setsockopt(smls_socket_t sock, int level, int optname, const void* optval,
                        smls_socklen_t optlen);

    /**
     * @brief Cross-platform getsockopt().
     *
     * Same semantics as native getsockopt().
     */
    int smls_getsockopt(smls_socket_t sock, int level, int optname, void* optval,
                        smls_socklen_t* optlen);

    /**
     * @brief Set socket non-blocking mode.
     *
     * Helper API, not part of the native socket mirror set.
     */
    int smls_set_nonblock(smls_socket_t sock, int enable);

    /**
     * @brief Apply common socket options using bitwise flags.
     *
     * Example:
     * @code
     * smls_set_options(sock,
     *     SMLS_SOCKET_OPT_REUSEADDR |
     *     SMLS_SOCKET_OPT_KEEPALIVE |
     *     SMLS_SOCKET_OPT_TCP_NODELAY);
     * @endcode
     *
     * Timeout flags only indicate timeout usage and should be
     * configured through @ref smls_set_timeout.
     *
     * @param sock Socket handle
     * @param options Bitwise combination of
     *        @ref smls_socket_option_t
     * @return 0 on success, -1 on failure
     */
    int smls_set_options(smls_socket_t sock, uint32_t options);

    /**
     * @brief Set socket timeout in milliseconds.
     *
     * A negative timeout means "do not modify".
     *
     * @param sock Socket handle
     * @param recv_timeout_ms Receive timeout
     * @param send_timeout_ms Send timeout
     * @return 0 on success, -1 on failure
     */
    int smls_set_timeout(smls_socket_t sock, int recv_timeout_ms, int send_timeout_ms);

    /**
     * @brief Build IPv4 socket address from ip and port.
     *
     * @param addr Output address buffer
     * @param ip IPv4 string, e.g. "127.0.0.1"
     * @param port TCP/UDP port
     * @return 0 on success, -1 on failure
     */
    int smls_socket_make_ipv4_addr(smls_sockaddr_t* addr, const char* ip, uint16_t port);

    /**
     * @brief Wait for socket read readiness.
     *
     * @param sock Socket handle
     * @param timeout_ms Timeout in ms
     * @return >0 ready, 0 timeout, <0 error
     */
    int smls_socket_poll(smls_socket_t sock, uint32_t timeout_ms);

    /**
     * @brief Get last socket error.
     *
     * Helper API, not part of the native socket mirror set.
     */
    int smls_socket_errno(void);

#ifdef __cplusplus
}
#endif
