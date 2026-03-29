#pragma once
#include "transport_tcp_port.h"
#include "transport_tcp_sm.h"
#include "transport_types.h"

#ifndef TRANSPORT_TCP_RX_BUF_SIZE
#define TRANSPORT_TCP_RX_BUF_SIZE 2048u
#endif

typedef struct
{
    const char* ip;
    uint16_t port;
    int server; /* 0=client,1=server */
} transport_tcp_args_t;

typedef struct
{
    tcp_socket_t sock;

    tcp_ch_state_t state;

    uint32_t rx_head;
    uint32_t rx_tail;
    uint32_t rx_count;

    uint8_t rx_buf[TRANSPORT_TCP_RX_BUF_SIZE];
} tcp_channel_ctx_t;

typedef struct
{
    tcp_socket_t listen_sock; /* server: listen socket, client: connect socket */
    int is_server;

    tcp_conn_state_t state;
} tcp_conn_ctx_t;

transport_status_t transport_tcp_client_init(transport_connection_t* conn, const void* args);

transport_status_t transport_tcp_server_init(transport_connection_t* conn, const void* args);