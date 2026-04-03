#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "osal_net.h"
#include "osal_time.h"

#include "smls_socket_port.h"

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <unistd.h>
#endif

#define TEST_IP "127.0.0.1"
#define TEST_PORT_BEGIN 36000u
#define TEST_PORT_END 36100u
#define TEST_TIMEOUT_MS 3000u

#define TEST_CHECK(cond, msg)                                                                      \
    do                                                                                             \
    {                                                                                              \
        if (!(cond))                                                                               \
        {                                                                                          \
            printf("[FAIL] %s\n", (msg));                                                          \
            return -1;                                                                             \
        }                                                                                          \
    } while (0)

static int fill_ipv4_addr(struct sockaddr_in* addr, uint16_t port)
{
    if (addr == NULL)
    {
        return -1;
    }

    memset(addr, 0, sizeof(*addr));

    addr->sin_family = AF_INET;
    addr->sin_port   = htons(port);

#if defined(_WIN32)
    return (InetPtonA(AF_INET, TEST_IP, &addr->sin_addr) == 1) ? 0 : -1;
#else
    return (inet_pton(AF_INET, TEST_IP, &addr->sin_addr) == 1) ? 0 : -1;
#endif
}

static int bind_listen_on_available_port(smls_socket_t* listen_sock, uint16_t* selected_port)
{
    if (listen_sock == NULL || selected_port == NULL)
    {
        return -1;
    }

    for (uint16_t p = TEST_PORT_BEGIN; p < TEST_PORT_END; ++p)
    {
        smls_socket_t sock = smls_socket(AF_INET, SOCK_STREAM, 0);

        if (sock == SMLS_SOCKET_INVALID)
        {
            return -1;
        }

        struct sockaddr_in addr;

        TEST_CHECK(fill_ipv4_addr(&addr, p) == 0, "fill_ipv4_addr failed");

        (void)smls_set_options(sock, SMLS_SOCKET_OPT_REUSEADDR);

        if (smls_bind(sock, (const smls_sockaddr_t*)&addr, sizeof(addr)) == 0 &&
            smls_listen(sock, 1) == 0)
        {
            *listen_sock   = sock;
            *selected_port = p;
            return 0;
        }

        (void)smls_close(sock);
    }

    return -1;
}

static int wait_accept(smls_socket_t listen_sock, smls_socket_t* accepted_sock)
{
    const uint64_t deadline = osal_time_ms() + TEST_TIMEOUT_MS;

    while (osal_time_ms() < deadline)
    {
        struct sockaddr_in peer_addr;
        smls_socklen_t peer_len = sizeof(peer_addr);

        const smls_socket_t sock =
            smls_accept(listen_sock, (smls_sockaddr_t*)&peer_addr, &peer_len);

        if (sock != SMLS_SOCKET_INVALID)
        {
            *accepted_sock = sock;
            return 0;
        }

        osal_sleep_ms(10u);
    }

    return -1;
}

static int send_all_nonblock(smls_socket_t sock, const uint8_t* data, uint32_t len)
{
    uint32_t sent = 0u;

    const uint64_t deadline = osal_time_ms() + TEST_TIMEOUT_MS;

    while (sent < len && osal_time_ms() < deadline)
    {
        const int ret = (int)smls_send(sock, data + sent, len - sent, 0);

        if (ret > 0)
        {
            sent += (uint32_t)ret;
            continue;
        }

        osal_sleep_ms(10u);
    }

    return (sent == len) ? 0 : -1;
}

static int recv_all_nonblock(smls_socket_t sock, uint8_t* out, uint32_t len)
{
    uint32_t received = 0u;

    const uint64_t deadline = osal_time_ms() + TEST_TIMEOUT_MS;

    while (received < len && osal_time_ms() < deadline)
    {
        const int ret = (int)smls_recv(sock, out + received, len - received, 0);

        if (ret > 0)
        {
            received += (uint32_t)ret;
            continue;
        }

        osal_sleep_ms(10u);
    }

    return (received == len) ? 0 : -1;
}

static int test_loopback_send_recv(void)
{
    static const uint8_t k_payload[] = "smls_socket_loopback_payload";

    smls_socket_t listen_sock = SMLS_SOCKET_INVALID;
    smls_socket_t client_sock = SMLS_SOCKET_INVALID;
    smls_socket_t server_sock = SMLS_SOCKET_INVALID;

    uint16_t port = 0u;

    uint8_t recv_buf[sizeof(k_payload)] = {0};

    TEST_CHECK(bind_listen_on_available_port(&listen_sock, &port) == 0, "failed to bind/listen");

    client_sock = smls_socket(AF_INET, SOCK_STREAM, 0);

    TEST_CHECK(client_sock != SMLS_SOCKET_INVALID, "client socket create failed");

    struct sockaddr_in server_addr;

    TEST_CHECK(fill_ipv4_addr(&server_addr, port) == 0, "fill_ipv4_addr failed");

    TEST_CHECK(
        smls_connect(client_sock, (const smls_sockaddr_t*)&server_addr, sizeof(server_addr)) == 0,
        "connect failed");

    TEST_CHECK(wait_accept(listen_sock, &server_sock) == 0, "accept failed");

    TEST_CHECK(send_all_nonblock(client_sock, k_payload, sizeof(k_payload)) == 0, "send failed");

    TEST_CHECK(recv_all_nonblock(server_sock, recv_buf, sizeof(k_payload)) == 0, "recv failed");

    TEST_CHECK(memcmp(recv_buf, k_payload, sizeof(k_payload)) == 0, "payload mismatch");

    (void)smls_close(server_sock);
    (void)smls_close(client_sock);
    (void)smls_close(listen_sock);

    return 0;
}

int main(void)
{
    TEST_CHECK(osal_net_init() == 0, "osal_net_init failed");

    if (test_loopback_send_recv() != 0)
    {
        osal_net_deinit();
        return -1;
    }

    osal_net_deinit();

    printf("[PASS] smls_io_socket_test\n");

    return 0;
}
