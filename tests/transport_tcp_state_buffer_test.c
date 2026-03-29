#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "osal_net.h"
#include "osal_time.h"
#include "transport_tcp.h"

#ifndef TEST_TCP_PORT
#define TEST_TCP_PORT 12347u
#endif

#define TEST_TIMEOUT_MS 4000u
#define TEST_POLL_INTERVAL_MS 5u
#define TEST_STREAM_PAYLOAD_LEN 3072u
#define TEST_RECV_CHUNK_LEN 64u

typedef struct
{
    transport_connection_t server;
    transport_connection_t client;

    tcp_channel_ctx_t server_ch_ctx[TRANSPORT_MAX_CHANNELS];
    tcp_channel_ctx_t client_ch_ctx[1];

    int mapped_server_ch;
} tcp_state_buffer_test_ctx_t;

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

static tcp_conn_ctx_t* test_conn_ctx(transport_connection_t* conn)
{
    return (tcp_conn_ctx_t*)conn->impl;
}

static tcp_channel_ctx_t* test_ch_ctx(transport_channel_t* ch)
{
    return (tcp_channel_ctx_t*)ch->impl;
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

static int recv_exact_chunked(transport_connection_t* server, transport_channel_t* ch, uint8_t* out,
                              uint32_t len, uint32_t timeout_ms)
{
    uint32_t got      = 0;
    uint64_t deadline = now_ms() + timeout_ms;

    while (got < len && now_ms() < deadline)
    {
        uint32_t want = len - got;
        int r;

        if (want > TEST_RECV_CHUNK_LEN)
        {
            want = TEST_RECV_CHUNK_LEN;
        }

        (void)transport_connection_poll(server, TEST_POLL_INTERVAL_MS);

        r = transport_channel_recv(ch, out + got, want);
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

static int wait_server_accept_one(tcp_state_buffer_test_ctx_t* t)
{
    uint64_t deadline = now_ms() + TEST_TIMEOUT_MS;

    while (now_ms() < deadline)
    {
        (void)transport_connection_poll(&t->server, TEST_POLL_INTERVAL_MS);

        for (uint16_t i = 0; i < t->server.max_channels; ++i)
        {
            if (t->server.channels[i].in_use)
            {
                t->mapped_server_ch = (int)i;
                return 0;
            }
        }

        sleep_ms(TEST_POLL_INTERVAL_MS);
    }

    return -1;
}

static int wait_client_connected(tcp_state_buffer_test_ctx_t* t)
{
    tcp_conn_ctx_t* cctx = test_conn_ctx(&t->client);
    uint64_t deadline    = now_ms() + TEST_TIMEOUT_MS;

    while (now_ms() < deadline)
    {
        (void)transport_connection_poll(&t->client, TEST_POLL_INTERVAL_MS);

        if (cctx->state == TCP_CONN_CONNECTED)
        {
            return 0;
        }

        sleep_ms(TEST_POLL_INTERVAL_MS);
    }

    return -1;
}

static int test_tcp_state_machine(tcp_state_buffer_test_ctx_t* t)
{
    tcp_conn_ctx_t* sctx = test_conn_ctx(&t->server);
    tcp_conn_ctx_t* cctx = test_conn_ctx(&t->client);

    printf("\n=== test_tcp_state_machine ===\n");

    if (sctx->state != TCP_CONN_LISTENING)
    {
        printf("[FAIL] server state expected LISTENING, got=%d\n", (int)sctx->state);
        return -1;
    }

    if (cctx->state != TCP_CONN_CONNECTING && cctx->state != TCP_CONN_CONNECTED)
    {
        printf("[FAIL] client state expected CONNECTING/CONNECTED, got=%d\n", (int)cctx->state);
        return -1;
    }

    if (wait_client_connected(t) != 0)
    {
        printf("[FAIL] client did not reach CONNECTED\n");
        return -1;
    }

    if (wait_server_accept_one(t) != 0)
    {
        printf("[FAIL] server did not accept client\n");
        return -1;
    }

    {
        transport_channel_t* sch = &t->server.channels[t->mapped_server_ch];
        tcp_channel_ctx_t* scctx = test_ch_ctx(sch);
        tcp_channel_ctx_t* ccctx = test_ch_ctx(&t->client.channels[0]);

        if (scctx->state != TCP_CH_ACTIVE || ccctx->state != TCP_CH_ACTIVE)
        {
            printf("[FAIL] channel state expected ACTIVE, server=%d client=%d\n", (int)scctx->state,
                   (int)ccctx->state);
            return -1;
        }
    }

    printf("[PASS] tcp connection/channel state transitions are valid\n");
    return 0;
}

static int test_tcp_rx_buffer_behavior(tcp_state_buffer_test_ctx_t* t)
{
    uint8_t tx[TEST_STREAM_PAYLOAD_LEN];
    uint8_t rx[TEST_STREAM_PAYLOAD_LEN];
    transport_channel_t* cch;
    transport_channel_t* sch;

    printf("\n=== test_tcp_rx_buffer_behavior ===\n");

    cch = &t->client.channels[0];
    sch = &t->server.channels[t->mapped_server_ch];

    for (uint32_t i = 0; i < TEST_STREAM_PAYLOAD_LEN; ++i)
    {
        tx[i] = (uint8_t)(i & 0xFFu);
    }

    if (send_all(cch, tx, TEST_STREAM_PAYLOAD_LEN, TEST_TIMEOUT_MS) != 0)
    {
        printf("[FAIL] send large stream failed\n");
        return -1;
    }

    if (recv_exact_chunked(&t->server, sch, rx, TEST_STREAM_PAYLOAD_LEN, TEST_TIMEOUT_MS) != 0)
    {
        printf("[FAIL] recv chunked stream failed\n");
        return -1;
    }

    if (memcmp(tx, rx, TEST_STREAM_PAYLOAD_LEN) != 0)
    {
        printf("[FAIL] stream payload mismatch\n");
        return -1;
    }

    {
        int r;
        uint8_t b;

        (void)transport_connection_poll(&t->server, TEST_POLL_INTERVAL_MS);
        r = transport_channel_recv(sch, &b, 1u);
        if (r != 0)
        {
            printf("[FAIL] idle recv expected 0, got=%d\n", r);
            return -1;
        }
    }

    printf("[PASS] tcp rx buffer behavior validated with chunked reads\n");
    return 0;
}

static int open_test_topology(tcp_state_buffer_test_ctx_t* t)
{
    transport_tcp_args_t s_args;
    transport_tcp_args_t c_args;

    if (transport_connection_create(TRANSPORT_TYPE_TCP_SERVER, &t->server, NULL) !=
        TRANSPORT_STATUS_OK)
    {
        printf("[FAIL] create server failed\n");
        return -1;
    }

    if (transport_connection_bind_channel_storage(
            &t->server, 2u, t->server_ch_ctx, sizeof(t->server_ch_ctx[0])) != TRANSPORT_STATUS_OK)
    {
        printf("[FAIL] bind server channel storage failed\n");
        return -1;
    }

    s_args.ip     = "127.0.0.1";
    s_args.port   = (uint16_t)TEST_TCP_PORT;
    s_args.server = 1;

    if (transport_connection_open(&t->server, &s_args) != TRANSPORT_STATUS_OK)
    {
        printf("[FAIL] open server failed\n");
        return -1;
    }

    if (transport_connection_create(TRANSPORT_TYPE_TCP_CLIENT, &t->client, NULL) !=
        TRANSPORT_STATUS_OK)
    {
        printf("[FAIL] create client failed\n");
        return -1;
    }

    if (transport_connection_bind_channel_storage(
            &t->client, 1u, t->client_ch_ctx, sizeof(t->client_ch_ctx[0])) != TRANSPORT_STATUS_OK)
    {
        printf("[FAIL] bind client channel storage failed\n");
        return -1;
    }

    c_args.ip     = "127.0.0.1";
    c_args.port   = (uint16_t)TEST_TCP_PORT;
    c_args.server = 0;

    if (transport_connection_open(&t->client, &c_args) != TRANSPORT_STATUS_OK)
    {
        printf("[FAIL] open client failed\n");
        return -1;
    }

    return 0;
}

static void close_test_topology(tcp_state_buffer_test_ctx_t* t)
{
    (void)transport_connection_close(&t->client);
    (void)transport_connection_close(&t->server);
}

int main(void)
{
    tcp_state_buffer_test_ctx_t t;
    int rc = -1;

    memset(&t, 0, sizeof(t));
    t.mapped_server_ch = -1;

    if (platform_socket_startup() != 0)
    {
        printf("[FAIL] platform socket startup failed\n");
        return -1;
    }

    if (open_test_topology(&t) != 0)
    {
        goto cleanup;
    }

    if (test_tcp_state_machine(&t) != 0)
    {
        goto cleanup;
    }

    if (test_tcp_rx_buffer_behavior(&t) != 0)
    {
        goto cleanup;
    }

    {
        tcp_conn_ctx_t* sctx = test_conn_ctx(&t.server);
        tcp_conn_ctx_t* cctx = test_conn_ctx(&t.client);

        close_test_topology(&t);

        if (sctx->state != TCP_CONN_CLOSED || cctx->state != TCP_CONN_CLOSED)
        {
            printf("[FAIL] close state expected CLOSED, server=%d client=%d\n", (int)sctx->state,
                   (int)cctx->state);
            platform_socket_cleanup();
            return -1;
        }
    }

    printf("\n[SUCCESS] transport tcp state+buffer test passed\n");
    platform_socket_cleanup();
    return 0;

cleanup:
    close_test_topology(&t);
    platform_socket_cleanup();
    return rc;
}