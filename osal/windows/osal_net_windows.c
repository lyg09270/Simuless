#include "osal_net.h"

#ifdef _WIN32
#include <winsock2.h>
#endif

int osal_net_init(void)
{
#ifdef _WIN32
    WSADATA wsa;
    return (WSAStartup(MAKEWORD(2, 2), &wsa) == 0) ? 0 : -1;
#else
    return 0;
#endif
}

void osal_net_deinit(void)
{
#ifdef _WIN32
    WSACleanup();
#endif
}
