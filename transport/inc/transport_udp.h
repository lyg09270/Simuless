#pragma once
#include "transport_types.h"
#include "transport_udp_port.h"

#ifndef TRANSPORT_UDP_RX_QUEUE_DEPTH
#define TRANSPORT_UDP_RX_QUEUE_DEPTH 32u
#endif

#ifndef TRANSPORT_UDP_RX_MAX_PACKET
#define TRANSPORT_UDP_RX_MAX_PACKET 128u
#endif

typedef struct
{
    const char* ip;
    uint16_t port;
    int server; /* 0=client,1=server */
} transport_udp_args_t;

typedef struct
{
    udp_socket_t sock;

    udp_addr_t peer_addr;
    int has_peer;

    uint16_t rx_head;
    uint16_t rx_tail;
    uint16_t rx_count;

    uint16_t rx_len[TRANSPORT_UDP_RX_QUEUE_DEPTH];
    uint8_t rx_buf[TRANSPORT_UDP_RX_QUEUE_DEPTH][TRANSPORT_UDP_RX_MAX_PACKET];
} udp_channel_ctx_t;

typedef struct
{
    udp_socket_t sock;
    int is_server;
} udp_conn_ctx_t;

transport_status_t transport_udp_init(transport_connection_t* conn, const void* args);