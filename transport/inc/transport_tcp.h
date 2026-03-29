#pragma once
#include "transport_tcp_port.h"
#include "transport_types.h"

typedef struct
{
    const char* ip;
    uint16_t port;
    int server; /* 0=client,1=server */
} transport_tcp_args_t;

typedef struct
{
    tcp_socket_t sock;
} tcp_channel_ctx_t;

typedef struct
{
    tcp_socket_t listen_sock; /* server: listen socket, client: connect socket */
    int is_server;
} tcp_conn_ctx_t;

transport_status_t transport_tcp_client_init(transport_connection_t* conn, const void* args);

transport_status_t transport_tcp_server_init(transport_connection_t* conn, const void* args);