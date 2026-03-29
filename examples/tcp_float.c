#include "osal_time.h"
#include "transport_tcp.h"
#include "transport_types.h"
#include <stdio.h>
#include <string.h>

/* =========================
 * args
 * ========================= */

transport_tcp_args_t server_args = {.ip = "127.0.0.1", .port = 12345, .server = true};

transport_tcp_args_t client_args = {
    .ip     = "127.0.0.1",
    .port   = 12345,
    .server = false,
};

/* =========================
 * connection
 * ========================= */

transport_connection_t server_conn;
transport_connection_t client_conn;

/* =========================
 * channel ctx（用户提供）
 * ========================= */
#define SERVER_MAX_CH 4
#define CLIENT_MAX_CH 1

static tcp_channel_ctx_t server_ctx[SERVER_MAX_CH];
static tcp_channel_ctx_t client_ctx[CLIENT_MAX_CH];

/* =========================
 * main
 * ========================= */

int main(void)
{
    /* =========================
     * 1. create（只绑定 backend）
     * ========================= */
    transport_connection_create(TRANSPORT_TYPE_TCP_SERVER, &server_conn, &server_args);
    transport_connection_create(TRANSPORT_TYPE_TCP_CLIENT, &client_conn, &client_args);

    /* =========================
     * 2. bind static memory（核心）
     * ========================= */
    transport_connection_bind_channel_storage(&server_conn, SERVER_MAX_CH, server_ctx,
                                              sizeof(server_ctx[0]));

    transport_connection_bind_channel_storage(&client_conn, CLIENT_MAX_CH, client_ctx,
                                              sizeof(client_ctx[0]));

    /* =========================
     * 3. open
     * ========================= */
    transport_connection_open(&server_conn, &server_args);
    transport_connection_open(&client_conn, &client_args);

    printf("server + client started\n");

    /* =========================
     * 4. main loop
     * ========================= */
    while (1)
    {
        /* 推进状态机 */
        transport_connection_poll(&server_conn, 0);
        transport_connection_poll(&client_conn, 0);

        /* client send */
        transport_channel_t* client_ch = transport_connection_get_channel(&client_conn, 0);

        if (client_ch && client_ch->in_use)
        {
            const char* msg = "hello from client";
            (void)transport_channel_send(client_ch, msg, (uint32_t)strlen(msg));
        }

        /* server recv */
        transport_channel_t* server_ch = transport_connection_get_channel(&server_conn, 0);

        if (server_ch && server_ch->in_use)
        {
            char buf[128];
            int r = transport_channel_recv(server_ch, buf, sizeof(buf) - 1);

            if (r > 0)
            {
                buf[r] = '\0';
                printf("server recv: %s\n", buf);
            }
        }

        osal_sleep_ms(1000);
    }

    return 0;
}