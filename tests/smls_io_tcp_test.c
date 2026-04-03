#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "osal_net.h"
#include "osal_time.h"
#include "smls_io.h"
#include "smls_io_tcp.h"

#define TEST_TIMEOUT_MS 3000u
#define TEST_ACCEPT_TIMEOUT_MS 1000u
#define TEST_POLL_STEP_MS 20u
#define TEST_BURST_COUNT 32

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

static int wait_until_ready(smls_io_t* io, uint32_t timeout_ms)
{
    const uint64_t start = osal_time_us();

    while ((osal_time_us() - start) < ((uint64_t)timeout_ms * 1000ull))
    {
        const int ret = smls_io_tcp_poll(io, TEST_POLL_STEP_MS);

        if (ret > 0)
        {
            return ret;
        }

        if (ret < 0)
        {
            return ret;
        }
    }

    return 0;
}

/* ========================================================= */

static int open_pair(smls_io_t* server_io, smls_io_t* client_io, smls_io_tcp_ctx_t* server_ctx,
                     smls_io_tcp_ctx_t* client_ctx, int nonblock, uint16_t port)
{
    char server_uri[64];
    char client_uri[64];

    if (server_io == NULL || client_io == NULL || server_ctx == NULL || client_ctx == NULL)
    {
        return -1;
    }

    memset(server_io, 0, sizeof(*server_io));
    memset(client_io, 0, sizeof(*client_io));
    memset(server_ctx, 0, sizeof(*server_ctx));
    memset(client_ctx, 0, sizeof(*client_ctx));

    server_io->priv = server_ctx;
    client_io->priv = client_ctx;

    snprintf(server_uri, sizeof(server_uri), "tcp://0.0.0.0:%u", port);

    snprintf(client_uri, sizeof(client_uri), "tcp://127.0.0.1:%u", port);

    const smls_io_desc_t server_desc = {.uri = server_uri, .nonblock = nonblock};

    const smls_io_desc_t client_desc = {.uri = client_uri, .nonblock = nonblock};

    TEST_CHECK(smls_io_tcp_open(server_io, &server_desc) == 0, "server open failed");

    TEST_CHECK(smls_io_tcp_open(client_io, &client_desc) == 0, "client open failed");

    TEST_CHECK(wait_until_ready(server_io, TEST_ACCEPT_TIMEOUT_MS) > 0, "server accept failed");

    return 0;
}

/* ========================================================= */

static void close_pair(smls_io_t* server_io, smls_io_t* client_io)
{
    if (client_io != NULL)
    {
        (void)smls_io_tcp_close(client_io);
    }

    if (server_io != NULL)
    {
        (void)smls_io_tcp_close(server_io);
    }
}

/* =========================================================
 * blocking sync tx/rx
 * ========================================================= */

static int test_tcp_sync_loopback(void)
{
    uint8_t tx_buf[]   = "sync_test_payload";
    uint8_t rx_buf[64] = {0};

    smls_io_t server_io = {0};
    smls_io_t client_io = {0};

    smls_io_tcp_ctx_t server_ctx = {0};
    smls_io_tcp_ctx_t client_ctx = {0};

    TEST_CHECK(open_pair(&server_io, &client_io, &server_ctx, &client_ctx, 0, 38000) == 0,
               "open pair failed");

    smls_packet_t tx_pkt = {.key = 0, .timestamp_us = 0, .data = tx_buf, .len = sizeof(tx_buf)};

    smls_packet_t rx_pkt = {.key = 0, .timestamp_us = 0, .data = rx_buf, .len = sizeof(rx_buf)};

    TEST_CHECK(smls_io_tcp_push(&client_io, &tx_pkt) > 0, "sync push failed");

    TEST_CHECK(wait_until_ready(&server_io, TEST_TIMEOUT_MS) > 0, "sync poll failed");

    TEST_CHECK(smls_io_tcp_pop(&server_io, &rx_pkt) > 0, "sync pop failed");

    TEST_CHECK(memcmp(tx_buf, rx_buf, sizeof(tx_buf)) == 0, "sync payload mismatch");

    close_pair(&server_io, &client_io);

    printf("[PASS] sync loopback\n");

    return 0;
}

/* =========================================================
 * async nonblock tx/rx
 * ========================================================= */

static int test_tcp_async_loopback(void)
{
    uint8_t tx_buf[]   = "async_test_payload";
    uint8_t rx_buf[64] = {0};

    smls_io_t server_io = {0};
    smls_io_t client_io = {0};

    smls_io_tcp_ctx_t server_ctx = {0};
    smls_io_tcp_ctx_t client_ctx = {0};

    TEST_CHECK(open_pair(&server_io, &client_io, &server_ctx, &client_ctx, 1, 38001) == 0,
               "open pair failed");

    smls_packet_t tx_pkt = {.key = 0, .timestamp_us = 0, .data = tx_buf, .len = sizeof(tx_buf)};

    smls_packet_t rx_pkt = {.key = 0, .timestamp_us = 0, .data = rx_buf, .len = sizeof(rx_buf)};

    TEST_CHECK(smls_io_tcp_push(&client_io, &tx_pkt) > 0, "async push failed");

    TEST_CHECK(wait_until_ready(&server_io, TEST_TIMEOUT_MS) > 0, "async poll timeout");

    TEST_CHECK(smls_io_tcp_pop(&server_io, &rx_pkt) > 0, "async pop failed");

    TEST_CHECK(memcmp(tx_buf, rx_buf, sizeof(tx_buf)) == 0, "async payload mismatch");

    close_pair(&server_io, &client_io);

    printf("[PASS] async loopback\n");

    return 0;
}

/* =========================================================
 * full duplex
 * ========================================================= */

static int test_tcp_full_duplex(void)
{
    uint8_t c2s[] = "client_to_server";
    uint8_t s2c[] = "server_to_client";

    uint8_t rx_server[64] = {0};
    uint8_t rx_client[64] = {0};

    smls_io_t server_io = {0};
    smls_io_t client_io = {0};

    smls_io_tcp_ctx_t server_ctx = {0};
    smls_io_tcp_ctx_t client_ctx = {0};

    TEST_CHECK(open_pair(&server_io, &client_io, &server_ctx, &client_ctx, 0, 38002) == 0,
               "open pair failed");

    smls_packet_t c2s_tx = {.key = 0, .timestamp_us = 0, .data = c2s, .len = sizeof(c2s)};

    smls_packet_t s2c_tx = {.key = 0, .timestamp_us = 0, .data = s2c, .len = sizeof(s2c)};

    smls_packet_t server_rx = {
        .key = 0, .timestamp_us = 0, .data = rx_server, .len = sizeof(rx_server)};

    smls_packet_t client_rx = {
        .key = 0, .timestamp_us = 0, .data = rx_client, .len = sizeof(rx_client)};

    TEST_CHECK(smls_io_tcp_push(&client_io, &c2s_tx) > 0, "client tx failed");

    TEST_CHECK(smls_io_tcp_push(&server_io, &s2c_tx) > 0, "server tx failed");

    TEST_CHECK(wait_until_ready(&server_io, TEST_TIMEOUT_MS) > 0, "server rx poll failed");

    TEST_CHECK(wait_until_ready(&client_io, TEST_TIMEOUT_MS) > 0, "client rx poll failed");

    TEST_CHECK(smls_io_tcp_pop(&server_io, &server_rx) > 0, "server rx failed");

    TEST_CHECK(smls_io_tcp_pop(&client_io, &client_rx) > 0, "client rx failed");

    TEST_CHECK(memcmp(c2s, rx_server, sizeof(c2s)) == 0, "c2s mismatch");

    TEST_CHECK(memcmp(s2c, rx_client, sizeof(s2c)) == 0, "s2c mismatch");

    close_pair(&server_io, &client_io);

    printf("[PASS] full duplex\n");

    return 0;
}

/* =========================================================
 * burst stress
 * ========================================================= */

static int test_tcp_burst(void)
{
    smls_io_t server_io = {0};
    smls_io_t client_io = {0};

    smls_io_tcp_ctx_t server_ctx = {0};
    smls_io_tcp_ctx_t client_ctx = {0};

    TEST_CHECK(open_pair(&server_io, &client_io, &server_ctx, &client_ctx, 1, 38003) == 0,
               "open pair failed");

    for (int i = 0; i < TEST_BURST_COUNT; i++)
    {
        uint32_t tx = (uint32_t)i;
        uint32_t rx = 0;

        smls_packet_t tx_pkt = {.key = 0, .timestamp_us = 0, .data = &tx, .len = sizeof(tx)};

        smls_packet_t rx_pkt = {.key = 0, .timestamp_us = 0, .data = &rx, .len = sizeof(rx)};

        TEST_CHECK(smls_io_tcp_push(&client_io, &tx_pkt) > 0, "burst push failed");

        TEST_CHECK(wait_until_ready(&server_io, TEST_TIMEOUT_MS) > 0, "burst poll failed");

        TEST_CHECK(smls_io_tcp_pop(&server_io, &rx_pkt) > 0, "burst pop failed");

        TEST_CHECK(tx == rx, "burst data mismatch");
    }

    close_pair(&server_io, &client_io);

    printf("[PASS] burst stress\n");

    return 0;
}

/* =========================================================
 * parse url
 * ========================================================= */

static int test_parse_url(void)
{
    smls_io_t io          = {0};
    smls_io_tcp_ctx_t ctx = {0};

    io.priv = &ctx;

    const smls_io_desc_t desc = {.uri = "tcp://0.0.0.0:39000", .nonblock = 0};

    TEST_CHECK(smls_io_tcp_open(&io, &desc) == 0, "parse/open failed");

    TEST_CHECK(smls_io_tcp_close(&io) == 0, "close failed");

    printf("[PASS] parse url\n");

    return 0;
}

/* ========================================================= */

int main(void)
{
    TEST_CHECK(osal_net_init() == 0, "osal net init failed");

    TEST_CHECK(test_tcp_sync_loopback() == 0, "sync test failed");

    TEST_CHECK(test_tcp_async_loopback() == 0, "async test failed");

    TEST_CHECK(test_tcp_full_duplex() == 0, "duplex test failed");

    TEST_CHECK(test_tcp_burst() == 0, "burst test failed");

    TEST_CHECK(test_parse_url() == 0, "parse url failed");

    (void)osal_net_deinit();

    printf("[PASS] all tcp tests passed\n");

    return 0;
}
