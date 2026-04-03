#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "osal_net.h"
#include "osal_time.h"
#include "smls_io.h"
#include "smls_io_tcp.h"

#define EXAMPLE_TIMEOUT_MS 3000u
#define EXAMPLE_POLL_STEP_MS 20u

static int wait_ready(smls_io_t* io, uint32_t timeout_ms)
{
    const uint64_t start = osal_time_us();

    while ((osal_time_us() - start) < ((uint64_t)timeout_ms * 1000ull))
    {
        const int ret = smls_io_poll(io, EXAMPLE_POLL_STEP_MS);

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

int main(void)
{
    if (osal_net_init() != 0)
    {
        printf("network init failed\n");
        return -1;
    }

    /* =====================================
     * create server/client objects
     * ===================================== */

    smls_io_t server_io = {0};
    smls_io_t client_io = {0};

    smls_io_tcp_ctx_t server_ctx = {0};
    smls_io_tcp_ctx_t client_ctx = {0};

    server_io.ops = &g_smls_io_tcp_ops;
    client_io.ops = &g_smls_io_tcp_ops;

    server_io.priv = &server_ctx;
    client_io.priv = &client_ctx;

    /* =====================================
     * descriptors
     * ===================================== */

    const smls_io_desc_t server_desc = {.uri = "tcp://0.0.0.0:38000", .nonblock = 0};

    const smls_io_desc_t client_desc = {.uri = "tcp://127.0.0.1:38000", .nonblock = 0};

    /* =====================================
     * create
     * ===================================== */

    if (smls_io_create(&server_io, &server_desc) != 0)
    {
        printf("server create failed\n");
        return -1;
    }

    if (smls_io_create(&client_io, &client_desc) != 0)
    {
        printf("client create failed\n");
        return -1;
    }

    if (wait_ready(&server_io, EXAMPLE_TIMEOUT_MS) <= 0)
    {
        printf("server accept timeout\n");
        return -1;
    }

    printf("tcp loopback connected\n");

    /* =====================================
     * send data
     * ===================================== */

    uint8_t tx_buf[]   = "hello simuless";
    uint8_t rx_buf[64] = {0};

    smls_packet_t tx_pkt = {
        .key = 0, .timestamp_us = osal_time_us(), .data = tx_buf, .len = sizeof(tx_buf)};

    smls_packet_t rx_pkt = {.key = 0, .timestamp_us = 0, .data = rx_buf, .len = sizeof(rx_buf)};

    if (smls_io_push(&client_io, &tx_pkt) <= 0)
    {
        printf("send failed\n");
        return -1;
    }

    if (wait_ready(&server_io, EXAMPLE_TIMEOUT_MS) <= 0)
    {
        printf("receive timeout\n");
        return -1;
    }

    if (smls_io_pop(&server_io, &rx_pkt) <= 0)
    {
        printf("receive failed\n");
        return -1;
    }

    printf("server received: %s\n", rx_buf);

    /* =====================================
     * cleanup
     * ===================================== */

    (void)smls_io_destroy(&client_io);
    (void)smls_io_destroy(&server_io);

    (void)osal_net_deinit();

    printf("loopback example done\n");

    return 0;
}
