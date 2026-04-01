#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "osal_net.h"
#include "osal_time.h"

#include "smls_socket_port.h"

#define TEST_IP "127.0.0.1"
#define TEST_PORT_BEGIN 36000u
#define TEST_PORT_END 36100u
#define TEST_TIMEOUT_MS 3000u

#define TEST_CHECK(cond, msg)                                                                      \
    do                                                                                             \
    {                                                                                              \
        if (!(cond))                                                                               \
        {                                                                                          \
            printf("[FAIL] %s\n", (msg));                                                          \
            return -1;                                                                             \
        }                                                                                          \
    } while (0)

static int bind_listen_on_available_port(smls_socket_t* listen_sock, uint16_t* selected_port)
{
    if (listen_sock == NULL || selected_port == NULL)
    {
        return -1;
    }

    for (uint16_t p = TEST_PORT_BEGIN; p < TEST_PORT_END; ++p)
    {
        smls_socket_t sock = SMLS_SOCKET_INVALID;

        if (smls_socket_create(&sock) != SMLS_SOCKET_OK)
        {
            return -1;
        }

        if (smls_socket_bind(sock, TEST_IP, p) == SMLS_SOCKET_OK &&
            smls_socket_listen(sock, 1) == SMLS_SOCKET_OK)
        {
            *listen_sock   = sock;
            *selected_port = p;
            return 0;
        }

        (void)smls_socket_close(sock);
    }

    return -1;
}

static int wait_accept(smls_socket_t listen_sock, smls_socket_t* accepted_sock)
{
    const uint64_t deadline = osal_time_ms() + TEST_TIMEOUT_MS;

    while (osal_time_ms() < deadline)
    {
        const int poll_ret = smls_socket_poll(listen_sock, 50u);
        if (poll_ret < 0)
        {
            return -1;
        }

        if (poll_ret == 0)
        {
            continue;
        }

        const int accept_ret = smls_socket_accept(listen_sock, accepted_sock);
        if (accept_ret == SMLS_SOCKET_OK)
        {
            return 0;
        }

        if (accept_ret != SMLS_SOCKET_ERR_WOULD_BLOCK)
        {
            return -1;
        }
    }

    return -1;
}

static int send_all_nonblock(smls_socket_t sock, const uint8_t* data, uint32_t len)
{
    uint32_t sent           = 0u;
    const uint64_t deadline = osal_time_ms() + TEST_TIMEOUT_MS;

    while (sent < len && osal_time_ms() < deadline)
    {
        const int ret = smls_socket_send(sock, data + sent, len - sent);
        if (ret > 0)
        {
            sent += (uint32_t)ret;
            continue;
        }

        if (ret == SMLS_SOCKET_ERR_WOULD_BLOCK)
        {
            osal_sleep_ms(10u);
            continue;
        }

        return -1;
    }

    return (sent == len) ? 0 : -1;
}

static int recv_all_nonblock(smls_socket_t sock, uint8_t* out, uint32_t len)
{
    uint32_t received       = 0u;
    const uint64_t deadline = osal_time_ms() + TEST_TIMEOUT_MS;

    while (received < len && osal_time_ms() < deadline)
    {
        const int poll_ret = smls_socket_poll(sock, 50u);
        if (poll_ret < 0)
        {
            return -1;
        }

        if (poll_ret == 0)
        {
            continue;
        }

        const int ret = smls_socket_recv(sock, out + received, len - received);
        if (ret > 0)
        {
            received += (uint32_t)ret;
            continue;
        }

        if (ret == SMLS_SOCKET_ERR_WOULD_BLOCK)
        {
            continue;
        }

        return -1;
    }

    return (received == len) ? 0 : -1;
}

static int test_invalid_args(void)
{
    TEST_CHECK(smls_socket_create(NULL) == SMLS_SOCKET_ERR_INVALID_ARG,
               "smls_socket_create(NULL) should fail with INVALID_ARG");

    TEST_CHECK(smls_socket_bind(SMLS_SOCKET_INVALID, TEST_IP, 1000u) == SMLS_SOCKET_ERR_INVALID_ARG,
               "smls_socket_bind(INVALID, ...) should fail with INVALID_ARG");

    TEST_CHECK(smls_socket_connect(SMLS_SOCKET_INVALID, TEST_IP, 1000u) ==
                   SMLS_SOCKET_ERR_INVALID_ARG,
               "smls_socket_connect(INVALID, ...) should fail with INVALID_ARG");

    TEST_CHECK(smls_socket_close(SMLS_SOCKET_INVALID) == SMLS_SOCKET_ERR_INVALID_ARG,
               "smls_socket_close(INVALID) should fail with INVALID_ARG");

    return 0;
}

static int test_loopback_send_recv(void)
{
    static const uint8_t k_payload[] = "smls_socket_loopback_payload";

    smls_socket_t listen_sock = SMLS_SOCKET_INVALID;
    smls_socket_t client_sock = SMLS_SOCKET_INVALID;
    smls_socket_t server_sock = SMLS_SOCKET_INVALID;

    uint16_t port                       = 0u;
    uint8_t recv_buf[sizeof(k_payload)] = {0};

    TEST_CHECK(bind_listen_on_available_port(&listen_sock, &port) == 0,
               "failed to bind/listen on local port");

    TEST_CHECK(smls_socket_create(&client_sock) == SMLS_SOCKET_OK,
               "failed to create client socket");

    {
        const int conn_ret = smls_socket_connect(client_sock, TEST_IP, port);
        TEST_CHECK(conn_ret == SMLS_SOCKET_OK || conn_ret == SMLS_SOCKET_ERR_WOULD_BLOCK,
                   "connect should return OK or WOULD_BLOCK in non-blocking mode");
    }

    TEST_CHECK(wait_accept(listen_sock, &server_sock) == 0, "accept timeout or failure");

    TEST_CHECK(send_all_nonblock(client_sock, k_payload, (uint32_t)sizeof(k_payload)) == 0,
               "send_all_nonblock failed");

    TEST_CHECK(recv_all_nonblock(server_sock, recv_buf, (uint32_t)sizeof(k_payload)) == 0,
               "recv_all_nonblock failed");

    TEST_CHECK(memcmp(recv_buf, k_payload, sizeof(k_payload)) == 0,
               "payload mismatch after loopback transfer");

    TEST_CHECK(smls_socket_close(server_sock) == SMLS_SOCKET_OK,
               "close accepted server socket failed");
    TEST_CHECK(smls_socket_close(client_sock) == SMLS_SOCKET_OK, "close client socket failed");
    TEST_CHECK(smls_socket_close(listen_sock) == SMLS_SOCKET_OK, "close listen socket failed");

    return 0;
}

int main(void)
{
    TEST_CHECK(osal_net_init() == 0, "osal_net_init failed");

    if (test_invalid_args() != 0)
    {
        osal_net_deinit();
        return -1;
    }

    if (test_loopback_send_recv() != 0)
    {
        osal_net_deinit();
        return -1;
    }

    osal_net_deinit();

    printf("[PASS] smls_io_socket_test\n");
    return 0;
}