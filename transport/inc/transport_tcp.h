#pragma once
#include "transport_types.h"

typedef struct
{
    const char* ip;
    uint16_t port;
    int server; /* 0=client,1=server */
} transport_tcp_args_t;

transport_status_t transport_tcp_client_init(transport_connection_t* conn, const void* args);

transport_status_t transport_tcp_server_init(transport_connection_t* conn, const void* args);