#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "osal_net.h"
#include "osal_time.h"
#include "transport_tcp.h"

#ifndef TEST_CLIENT_COUNT
#define TEST_CLIENT_COUNT 3u
#endif

#ifndef TEST_TCP_PORT
#define TEST_TCP_PORT 39091u
#endif

#define TEST_TIMEOUT_MS 4000u
#define TEST_POLL_INTERVAL_MS 5u
#define TEST_IO_PAYLOAD_LEN 32u
#define TEST_ASYNC_BURST_COUNT 24u

typedef struct
{
    transport_connection_t server;
    transport_connection_t clients[TEST_CLIENT_COUNT];

    tcp_channel_ctx_t server_ch_ctx[TRANSPORT_MAX_CHANNELS];
    tcp_channel_ctx_t client_ch_ctx[TEST_CLIENT_COUNT][1];

    int server_ch_of_client[TEST_CLIENT_COUNT];
} tcp_multi_test_ctx_t;

static uint64_t now_ms(void)
{
    return osal_time_ms();
}

static void sleep_ms(uint32_t ms)
{
    osal_sleep_ms(ms);
}

static int platform_socket_startup(void)
{
    return osal_net_init();
}

static void platform_socket_cleanup(void)
{
    osal_net_deinit();
}

static uint16_t count_active_channels(const transport_connection_t* conn)
{
    uint16_t n = 0;

    for (uint16_t i = 0; i < conn->max_channels; ++i)
    {
        if (conn->channels[i].in_use)
        {
            ++n;
        }
    }

    return n;
}

static int send_all(transport_channel_t* ch, const uint8_t* data, uint32_t len, uint32_t timeout_ms)
{
    uint32_t sent     = 0;
    uint64_t deadline = now_ms() + timeout_ms;

    while (sent < len && now_ms() < deadline)
    {
        int r = transport_channel_send(ch, data + sent, len - sent);
        if (r > 0)
        {
            sent += (uint32_t)r;
            continue;
        }
        if (r == 0)
        {
            sleep_ms(TEST_POLL_INTERVAL_MS);
            continue;
        }
        return -1;
    }

    return (sent == len) ? 0 : -1;
}

static int recv_exact(transport_channel_t* ch, uint8_t* buf, uint32_t len, uint32_t timeout_ms)
{
    uint32_t got      = 0;
    uint64_t deadline = now_ms() + timeout_ms;

    while (got < len && now_ms() < deadline)
    {
        int r = transport_channel_recv(ch, buf + got, len - got);
        if (r > 0)
        {
            got += (uint32_t)r;
            continue;
        }
        if (r == 0)
        {
            sleep_ms(TEST_POLL_INTERVAL_MS);
            continue;
        }
        return -1;
    }

    return (got == len) ? 0 : -1;
}

static int wait_server_accept_all(transport_connection_t* server)
{
    uint64_t deadline = now_ms() + TEST_TIMEOUT_MS;

    while (now_ms() < deadline)
    {
        (void)transport_connection_poll(server, TEST_POLL_INTERVAL_MS);

        if (count_active_channels(server) == TEST_CLIENT_COUNT)
        {
            return 0;
        }

        sleep_ms(TEST_POLL_INTERVAL_MS);
    }

    return -1;
}

static int test_multi_client_accept_and_map(tcp_multi_test_ctx_t* t)
{
    printf("\n=== test_multi_client_accept ===\n");

    /* 关键异步语义：server 不调用 poll 前，不会把 pending 连接转成 active channel */
    if (count_active_channels(&t->server) != 0u)
    {
        printf("[FAIL] server active channels should be 0 before poll\n");
        return -1;
    }

    if (wait_server_accept_all(&t->server) != 0)
    {
        printf("[FAIL] server accept timeout, active=%u expected=%u\n",
               (unsigned)count_active_channels(&t->server), (unsigned)TEST_CLIENT_COUNT);
        return -1;
    }

    for (uint32_t i = 0; i < TEST_CLIENT_COUNT; ++i)
    {
        t->server_ch_of_client[i] = -1;
    }

    for (uint32_t i = 0; i < TEST_CLIENT_COUNT; ++i)
    {
        uint8_t id              = (uint8_t)i;
        transport_channel_t* ch = &t->clients[i].channels[0];
        if (send_all(ch, &id, 1u, TEST_TIMEOUT_MS) != 0)
        {
            printf("[FAIL] client %u handshake send failed\n", (unsigned)i);
            return -1;
        }
    }

    {
        uint32_t mapped   = 0;
        uint64_t deadline = now_ms() + TEST_TIMEOUT_MS;

        while (mapped < TEST_CLIENT_COUNT && now_ms() < deadline)
        {
            for (uint16_t sidx = 0; sidx < t->server.max_channels; ++sidx)
            {
                transport_channel_t* sch = &t->server.channels[sidx];
                uint8_t id               = 0;
                int r;

                if (!sch->in_use)
                {
                    continue;
                }

                r = transport_channel_recv(sch, &id, 1u);
                if (r == 1)
                {
                    if (id >= TEST_CLIENT_COUNT)
                    {
                        printf("[FAIL] invalid handshake id=%u\n", (unsigned)id);
                        return -1;
                    }

                    if (t->server_ch_of_client[id] == -1)
                    {
                        t->server_ch_of_client[id] = (int)sidx;
                        ++mapped;
                    }
                }
                else if (r < 0)
                {
                    printf("[FAIL] server handshake recv error on ch=%u\n", (unsigned)sidx);
                    return -1;
                }
            }

            sleep_ms(TEST_POLL_INTERVAL_MS);
        }

        if (mapped != TEST_CLIENT_COUNT)
        {
            printf("[FAIL] server channel mapping timeout, mapped=%u expected=%u\n",
                   (unsigned)mapped, (unsigned)TEST_CLIENT_COUNT);
            return -1;
        }
    }

    printf("[PASS] accepted %u clients and completed channel mapping\n",
           (unsigned)TEST_CLIENT_COUNT);
    return 0;
}

static int test_async_idle_would_block(tcp_multi_test_ctx_t* t)
{
    printf("\n=== test_async_idle_would_block ===\n");

    for (uint32_t cid = 0; cid < TEST_CLIENT_COUNT; ++cid)
    {
        int sidx = t->server_ch_of_client[cid];
        uint8_t b;
        int r;

        if (sidx < 0)
        {
            printf("[FAIL] no mapped server channel for client %u\n", (unsigned)cid);
            return -1;
        }

        r = transport_channel_recv(&t->server.channels[sidx], &b, 1u);
        if (r != 0)
        {
            printf("[FAIL] server idle recv expected 0, got %d (client=%u, ch=%d)\n", r,
                   (unsigned)cid, sidx);
            return -1;
        }

        r = transport_channel_recv(&t->clients[cid].channels[0], &b, 1u);
        if (r != 0)
        {
            printf("[FAIL] client idle recv expected 0, got %d (client=%u)\n", r, (unsigned)cid);
            return -1;
        }
    }

    printf("[PASS] idle non-blocking recv returned 0 on both server/client sides\n");
    return 0;
}

static int test_async_interleaved_burst_io(tcp_multi_test_ctx_t* t)
{
    uint8_t expected_c2s[TEST_CLIENT_COUNT][TEST_ASYNC_BURST_COUNT];
    uint8_t expected_s2c[TEST_CLIENT_COUNT][TEST_ASYNC_BURST_COUNT];
    uint32_t got_c2s[TEST_CLIENT_COUNT] = {0};
    uint32_t got_s2c[TEST_CLIENT_COUNT] = {0};

    printf("\n=== test_async_interleaved_burst_io ===\n");

    for (uint32_t cid = 0; cid < TEST_CLIENT_COUNT; ++cid)
    {
        for (uint32_t k = 0; k < TEST_ASYNC_BURST_COUNT; ++k)
        {
            expected_c2s[cid][k] = (uint8_t)(0x10u + cid * 3u + k);
            expected_s2c[cid][k] = (uint8_t)(0x80u + cid * 5u + k);
        }
    }

    /* 客户端交错发送，服务端异步拉取 */
    for (uint32_t k = 0; k < TEST_ASYNC_BURST_COUNT; ++k)
    {
        for (uint32_t cid = 0; cid < TEST_CLIENT_COUNT; ++cid)
        {
            if (send_all(&t->clients[cid].channels[0], &expected_c2s[cid][k], 1u,
                         TEST_TIMEOUT_MS) != 0)
            {
                printf("[FAIL] async c2s send failed (client=%u, idx=%u)\n", (unsigned)cid,
                       (unsigned)k);
                return -1;
            }
        }
    }

    {
        uint32_t total_target = TEST_CLIENT_COUNT * TEST_ASYNC_BURST_COUNT;
        uint32_t total_got    = 0;
        uint64_t deadline     = now_ms() + TEST_TIMEOUT_MS;

        while ((total_got < total_target) && (now_ms() < deadline))
        {
            int progressed = 0;

            for (uint32_t cid = 0; cid < TEST_CLIENT_COUNT; ++cid)
            {
                int sidx = t->server_ch_of_client[cid];
                if (sidx < 0)
                {
                    printf("[FAIL] no mapped server channel for client %u\n", (unsigned)cid);
                    return -1;
                }

                while (got_c2s[cid] < TEST_ASYNC_BURST_COUNT)
                {
                    uint8_t b;
                    int r = transport_channel_recv(&t->server.channels[sidx], &b, 1u);

                    if (r == 1)
                    {
                        if (b != expected_c2s[cid][got_c2s[cid]])
                        {
                            printf("[FAIL] async c2s order/content mismatch (client=%u, exp=%u, "
                                   "got=%u)\n",
                                   (unsigned)cid, (unsigned)expected_c2s[cid][got_c2s[cid]],
                                   (unsigned)b);
                            return -1;
                        }
                        ++got_c2s[cid];
                        ++total_got;
                        progressed = 1;
                    }
                    else if (r == 0)
                    {
                        break;
                    }
                    else
                    {
                        printf("[FAIL] async c2s recv error (client=%u, ch=%d, r=%d)\n",
                               (unsigned)cid, sidx, r);
                        return -1;
                    }
                }
            }

            if (!progressed)
            {
                sleep_ms(TEST_POLL_INTERVAL_MS);
            }
        }

        if (total_got != total_target)
        {
            printf("[FAIL] async c2s timeout: got=%u target=%u\n", (unsigned)total_got,
                   (unsigned)total_target);
            return -1;
        }
    }

    /* 服务端交错发送，客户端异步拉取 */
    for (uint32_t k = 0; k < TEST_ASYNC_BURST_COUNT; ++k)
    {
        for (uint32_t cid = 0; cid < TEST_CLIENT_COUNT; ++cid)
        {
            int sidx = t->server_ch_of_client[cid];
            if (sidx < 0)
            {
                printf("[FAIL] no mapped server channel for client %u\n", (unsigned)cid);
                return -1;
            }

            if (send_all(&t->server.channels[sidx], &expected_s2c[cid][k], 1u, TEST_TIMEOUT_MS) !=
                0)
            {
                printf("[FAIL] async s2c send failed (client=%u, idx=%u)\n", (unsigned)cid,
                       (unsigned)k);
                return -1;
            }
        }
    }

    {
        uint32_t total_target = TEST_CLIENT_COUNT * TEST_ASYNC_BURST_COUNT;
        uint32_t total_got    = 0;
        uint64_t deadline     = now_ms() + TEST_TIMEOUT_MS;

        while ((total_got < total_target) && (now_ms() < deadline))
        {
            int progressed = 0;

            for (uint32_t cid = 0; cid < TEST_CLIENT_COUNT; ++cid)
            {
                while (got_s2c[cid] < TEST_ASYNC_BURST_COUNT)
                {
                    uint8_t b;
                    int r = transport_channel_recv(&t->clients[cid].channels[0], &b, 1u);

                    if (r == 1)
                    {
                        if (b != expected_s2c[cid][got_s2c[cid]])
                        {
                            printf("[FAIL] async s2c order/content mismatch (client=%u, exp=%u, "
                                   "got=%u)\n",
                                   (unsigned)cid, (unsigned)expected_s2c[cid][got_s2c[cid]],
                                   (unsigned)b);
                            return -1;
                        }
                        ++got_s2c[cid];
                        ++total_got;
                        progressed = 1;
                    }
                    else if (r == 0)
                    {
                        break;
                    }
                    else
                    {
                        printf("[FAIL] async s2c recv error (client=%u, r=%d)\n", (unsigned)cid, r);
                        return -1;
                    }
                }
            }

            if (!progressed)
            {
                sleep_ms(TEST_POLL_INTERVAL_MS);
            }
        }

        if (total_got != total_target)
        {
            printf("[FAIL] async s2c timeout: got=%u target=%u\n", (unsigned)total_got,
                   (unsigned)total_target);
            return -1;
        }
    }

    printf("[PASS] interleaved async burst io verified (%u clients x %u bytes each direction)\n",
           (unsigned)TEST_CLIENT_COUNT, (unsigned)TEST_ASYNC_BURST_COUNT);
    return 0;
}

static int test_client_to_server_io(tcp_multi_test_ctx_t* t)
{
    uint8_t tx[TEST_CLIENT_COUNT][TEST_IO_PAYLOAD_LEN];
    uint8_t rx[TEST_IO_PAYLOAD_LEN];

    printf("\n=== test_client_to_server_io ===\n");

    for (uint32_t i = 0; i < TEST_CLIENT_COUNT; ++i)
    {
        for (uint32_t j = 0; j < TEST_IO_PAYLOAD_LEN; ++j)
        {
            tx[i][j] = (uint8_t)(0xA0u + i + j);
        }

        if (send_all(&t->clients[i].channels[0], tx[i], TEST_IO_PAYLOAD_LEN, TEST_TIMEOUT_MS) != 0)
        {
            printf("[FAIL] client %u send failed\n", (unsigned)i);
            return -1;
        }
    }

    for (uint32_t cid = 0; cid < TEST_CLIENT_COUNT; ++cid)
    {
        int sidx = t->server_ch_of_client[cid];
        if (sidx < 0)
        {
            printf("[FAIL] no mapped server channel for client %u\n", (unsigned)cid);
            return -1;
        }

        if (recv_exact(&t->server.channels[sidx], rx, TEST_IO_PAYLOAD_LEN, TEST_TIMEOUT_MS) != 0)
        {
            printf("[FAIL] server recv failed for client %u (server_ch=%d)\n", (unsigned)cid, sidx);
            return -1;
        }

        if (memcmp(rx, tx[cid], TEST_IO_PAYLOAD_LEN) != 0)
        {
            printf("[FAIL] payload mismatch in c2s for client %u\n", (unsigned)cid);
            return -1;
        }
    }

    printf("[PASS] client->server io verified for %u clients\n", (unsigned)TEST_CLIENT_COUNT);
    return 0;
}

static int test_server_to_client_io(tcp_multi_test_ctx_t* t)
{
    uint8_t tx[TEST_CLIENT_COUNT][TEST_IO_PAYLOAD_LEN];
    uint8_t rx[TEST_IO_PAYLOAD_LEN];

    printf("\n=== test_server_to_client_io ===\n");

    for (uint32_t cid = 0; cid < TEST_CLIENT_COUNT; ++cid)
    {
        int sidx = t->server_ch_of_client[cid];
        if (sidx < 0)
        {
            printf("[FAIL] no mapped server channel for client %u\n", (unsigned)cid);
            return -1;
        }

        for (uint32_t j = 0; j < TEST_IO_PAYLOAD_LEN; ++j)
        {
            tx[cid][j] = (uint8_t)(0x55u + cid + j);
        }

        if (send_all(&t->server.channels[sidx], tx[cid], TEST_IO_PAYLOAD_LEN, TEST_TIMEOUT_MS) != 0)
        {
            printf("[FAIL] server send failed for client %u (server_ch=%d)\n", (unsigned)cid, sidx);
            return -1;
        }
    }

    for (uint32_t cid = 0; cid < TEST_CLIENT_COUNT; ++cid)
    {
        if (recv_exact(&t->clients[cid].channels[0], rx, TEST_IO_PAYLOAD_LEN, TEST_TIMEOUT_MS) != 0)
        {
            printf("[FAIL] client %u recv failed in s2c\n", (unsigned)cid);
            return -1;
        }

        if (memcmp(rx, tx[cid], TEST_IO_PAYLOAD_LEN) != 0)
        {
            printf("[FAIL] payload mismatch in s2c for client %u\n", (unsigned)cid);
            return -1;
        }
    }

    printf("[PASS] server->client io verified for %u clients\n", (unsigned)TEST_CLIENT_COUNT);
    return 0;
}

static int open_test_topology(tcp_multi_test_ctx_t* t)
{
    transport_tcp_args_t s_args;
    transport_tcp_args_t c_args;

    /* =========================
     * server: create + bind + open
     * ========================= */

    if (transport_connection_create(TRANSPORT_TYPE_TCP_SERVER, &t->server, NULL) !=
        TRANSPORT_STATUS_OK)
    {
        printf("[FAIL] transport_connection_create(server) failed\n");
        return -1;
    }

    if (transport_connection_bind_channel_storage(&t->server, (uint16_t)TEST_CLIENT_COUNT,
                                                  t->server_ch_ctx, sizeof(t->server_ch_ctx[0])) !=
        TRANSPORT_STATUS_OK)
    {
        printf("[FAIL] server bind_channel_storage failed\n");
        return -1;
    }

    s_args.ip     = "127.0.0.1";
    s_args.port   = (uint16_t)TEST_TCP_PORT;
    s_args.server = 1;

    if (transport_connection_open(&t->server, &s_args) != TRANSPORT_STATUS_OK)
    {
        printf("[FAIL] server open failed on port %u\n", (unsigned)TEST_TCP_PORT);
        return -1;
    }

    /* =========================
     * clients: create + bind + open
     * ========================= */

    c_args.ip     = "127.0.0.1";
    c_args.port   = (uint16_t)TEST_TCP_PORT;
    c_args.server = 0;

    for (uint32_t i = 0; i < TEST_CLIENT_COUNT; ++i)
    {
        if (transport_connection_create(TRANSPORT_TYPE_TCP_CLIENT, &t->clients[i], NULL) !=
            TRANSPORT_STATUS_OK)
        {
            printf("[FAIL] transport_connection_create(client %u) failed\n", (unsigned)i);
            return -1;
        }

        if (transport_connection_bind_channel_storage(&t->clients[i], 1u, t->client_ch_ctx[i],
                                                      sizeof(t->client_ch_ctx[i][0])) !=
            TRANSPORT_STATUS_OK)
        {
            printf("[FAIL] client %u bind_channel_storage failed\n", (unsigned)i);
            return -1;
        }

        if (transport_connection_open(&t->clients[i], &c_args) != TRANSPORT_STATUS_OK)
        {
            printf("[FAIL] client %u open failed\n", (unsigned)i);
            return -1;
        }
    }

    return 0;
}

static void close_test_topology(tcp_multi_test_ctx_t* t)
{
    for (uint32_t i = 0; i < TEST_CLIENT_COUNT; ++i)
    {
        (void)transport_connection_close(&t->clients[i]);
    }

    (void)transport_connection_close(&t->server);
}

int main(void)
{
    tcp_multi_test_ctx_t t;
    int rc = -1;

    memset(&t, 0, sizeof(t));

    if (platform_socket_startup() != 0)
    {
        printf("[FAIL] platform socket startup failed\n");
        return -1;
    }

    if (open_test_topology(&t) != 0)
    {
        goto cleanup;
    }

    if (test_multi_client_accept_and_map(&t) != 0)
    {
        goto cleanup;
    }

    if (test_client_to_server_io(&t) != 0)
    {
        goto cleanup;
    }

    if (test_async_idle_would_block(&t) != 0)
    {
        goto cleanup;
    }

    if (test_async_interleaved_burst_io(&t) != 0)
    {
        goto cleanup;
    }

    if (test_server_to_client_io(&t) != 0)
    {
        goto cleanup;
    }

    if (test_async_idle_would_block(&t) != 0)
    {
        goto cleanup;
    }

    printf("\n[SUCCESS] transport tcp multi-connection io test passed\n");
    rc = 0;

cleanup:
    close_test_topology(&t);
    platform_socket_cleanup();
    return rc;
}