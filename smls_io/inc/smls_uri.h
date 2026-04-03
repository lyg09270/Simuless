#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Supported URI schemes.
     */
    typedef enum
    {
        SMLS_URI_SCHEME_UNKNOWN = 0,
        SMLS_URI_SCHEME_TCP,
        SMLS_URI_SCHEME_UDP,
        SMLS_URI_SCHEME_UART,
        SMLS_URI_SCHEME_SHM,
    } smls_uri_scheme_t;

    /**
     * @brief Parse URI scheme header only.
     *
     * Example:
     * tcp://192.168.1.10:9000
     *
     * Output:
     * SMLS_URI_SCHEME_TCP
     *
     * @param uri Input URI string
     * @return Parsed scheme enum
     */
    smls_uri_scheme_t smls_uri_parse_scheme(const char* uri);

#ifdef __cplusplus
}
#endif
