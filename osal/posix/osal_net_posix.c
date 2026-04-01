#include "osal_net.h"

#ifndef _WIN32

int osal_net_init(void)
{
    /* POSIX: no initialization required */
    return 0;
}

void osal_net_deinit(void)
{
    /* POSIX: no cleanup required */
}

#endif
