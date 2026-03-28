#pragma once

typedef enum
{
    TCP_CONN_IDLE = 0,
    TCP_CONN_LISTENING,
    TCP_CONN_CONNECTING,
    TCP_CONN_CONNECTED,
    TCP_CONN_CLOSED,
    TCP_CONN_ERROR
} tcp_conn_state_t;

typedef enum
{
    TCP_CH_FREE = 0,
    TCP_CH_ACTIVE,
    TCP_CH_CLOSED
} tcp_ch_state_t;