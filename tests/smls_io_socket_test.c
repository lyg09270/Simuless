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

/* ========================================================= */

static int bind_listen_on_available_port(smls_socket_t* listen_sock, uint16_t* selected_port)
{
    if (!listen_sock || !selected_port)
    {
        return -1;
    }

    for (uint16_t p = TEST_PORT_BEGIN; p < TEST_PORT_END; ++p)
    {
        smls_socket_t sock = smls_socket(AF_INET, SOCK_STREAM, 0);

        if (sock == SMLS_SOCKET_INVALID)
        {
            return -1;
        }

        smls_sockaddr_t addr;

        TEST_CHECK(smls_socket_make_ipv4_addr(&addr, TEST_IP, p) == 0, "make ipv4 addr failed");

        (void)smls_set_options(sock, SMLS_SOCKET_OPT_REUSEADDR);

        if (smls_bind(sock, &addr, sizeof(addr)) == 0 && smls_listen(sock, 1) == 0)
        {
            *listen_sock   = sock;
            *selected_port = p;
            return 0;
        }

        (void)smls_close(sock);
    }

    return -1;
}

/* ========================================================= */

static int wait_accept(smls_socket_t listen_sock, smls_socket_t* accepted_sock, int nonblock)
{
    const uint64_t deadline = osal_time_ms() + TEST_TIMEOUT_MS;

    while (osal_time_ms() < deadline)
    {
        const int ready = smls_socket_poll(listen_sock, 10);

        if (ready > 0)
        {
            const smls_socket_t sock = smls_accept(listen_sock, NULL, NULL);

            if (sock != SMLS_SOCKET_INVALID)
            {
                if (nonblock)
                {
                    TEST_CHECK(smls_set_nonblock(sock, 1) == 0, "server nonblock failed");
                }

                *accepted_sock = sock;
                return 0;
            }
        }

        osal_sleep_ms(1u);
    }

    return -1;
}

/* =========================================================
 * sync
 * ========================================================= */

static int send_all_sync(smls_socket_t sock, const uint8_t* data, uint32_t len)
{
    uint32_t sent = 0;

    while (sent < len)
    {
        const int ret = (int)smls_send(sock, data + sent, len - sent, 0);

        if (ret <= 0)
        {
            return -1;
        }

        sent += (uint32_t)ret;
    }

    return 0;
}

static int recv_all_sync(smls_socket_t sock, uint8_t* out, uint32_t len)
{
    uint32_t received = 0;

    while (received < len)
    {
        const int ret = (int)smls_recv(sock, out + received, len - received, 0);

        if (ret <= 0)
        {
            return -1;
        }

        received += (uint32_t)ret;
    }

    return 0;
}

/* =========================================================
 * async
 * ========================================================= */

static int send_all_nonblock(smls_socket_t sock, const uint8_t* data, uint32_t len)
{
    uint32_t sent = 0;

    const uint64_t deadline = osal_time_ms() + TEST_TIMEOUT_MS;

    while (sent < len && osal_time_ms() < deadline)
    {
        const int ret = (int)smls_send(sock, data + sent, len - sent, 0);

        if (ret > 0)
        {
            sent += (uint32_t)ret;
            continue;
        }

        osal_sleep_ms(1u);
    }

    return (sent == len) ? 0 : -1;
}

static int recv_all_nonblock(smls_socket_t sock, uint8_t* out, uint32_t len)
{
    uint32_t received = 0;

    const uint64_t deadline = osal_time_ms() + TEST_TIMEOUT_MS;

    while (received < len && osal_time_ms() < deadline)
    {
        const int ready = smls_socket_poll(sock, 10);

        if (ready > 0)
        {
            const int ret = (int)smls_recv(sock, out + received, len - received, 0);

            if (ret > 0)
            {
                received += (uint32_t)ret;
                continue;
            }
        }

        osal_sleep_ms(1u);
    }

    return (received == len) ? 0 : -1;
}

/* ========================================================= */

static int run_test(int nonblock)
{
    static const uint8_t payload[] = "smls_socket_loopback_payload";

    smls_socket_t listen_sock = SMLS_SOCKET_INVALID;

    smls_socket_t client_sock = SMLS_SOCKET_INVALID;

    smls_socket_t server_sock = SMLS_SOCKET_INVALID;

    uint16_t port                     = 0;
    uint8_t recv_buf[sizeof(payload)] = {0};

    TEST_CHECK(bind_listen_on_available_port(&listen_sock, &port) == 0, "bind failed");

    client_sock = smls_socket(AF_INET, SOCK_STREAM, 0);

    TEST_CHECK(client_sock != SMLS_SOCKET_INVALID, "client create failed");

    if (nonblock)
    {
        TEST_CHECK(smls_set_nonblock(client_sock, 1) == 0, "client nonblock failed");
    }

    smls_sockaddr_t server_addr;

    TEST_CHECK(smls_socket_make_ipv4_addr(&server_addr, TEST_IP, port) == 0, "make addr failed");

    /* =========================================
     * connect
     * ========================================= */
    {
        const int ret = smls_connect(client_sock, &server_addr, sizeof(server_addr));

        if (!nonblock)
        {
            TEST_CHECK(ret == 0, "sync connect failed");
        }
        else
        {
            /**
             * nonblock connect:
             * 0 = immediate success
             * <0 = in progress / would block
             */
            if (ret != 0)
            {
                const int ready = smls_socket_poll(client_sock, 100);

                TEST_CHECK(ready >= 0, "async connect poll failed");
            }
        }
    }

    /* =========================================
     * accept
     * ========================================= */
    TEST_CHECK(wait_accept(listen_sock, &server_sock, nonblock) == 0, "accept failed");

    /* =========================================
     * tx/rx
     * ========================================= */
    if (nonblock)
    {
        TEST_CHECK(send_all_nonblock(client_sock, payload, sizeof(payload)) == 0,
                   "async send failed");

        TEST_CHECK(recv_all_nonblock(server_sock, recv_buf, sizeof(payload)) == 0,
                   "async recv failed");
    }
    else
    {
        TEST_CHECK(send_all_sync(client_sock, payload, sizeof(payload)) == 0, "sync send failed");

        TEST_CHECK(recv_all_sync(server_sock, recv_buf, sizeof(payload)) == 0, "sync recv failed");
    }

    /* =========================================
     * verify
     * ========================================= */
    TEST_CHECK(memcmp(recv_buf, payload, sizeof(payload)) == 0, "payload mismatch");

    /* =========================================
     * cleanup
     * ========================================= */
    if (server_sock != SMLS_SOCKET_INVALID)
    {
        (void)smls_close(server_sock);
    }

    if (client_sock != SMLS_SOCKET_INVALID)
    {
        (void)smls_close(client_sock);
    }

    if (listen_sock != SMLS_SOCKET_INVALID)
    {
        (void)smls_close(listen_sock);
    }

    return 0;
}

/* ========================================================= */

int main(void)
{
    TEST_CHECK(osal_net_init() == 0, "osal_net_init failed");

    TEST_CHECK(run_test(0) == 0, "sync test failed");

    TEST_CHECK(run_test(1) == 0, "async test failed");

    (void)osal_net_deinit();

    printf("[PASS] sync + async socket test\n");

    return 0;
}
