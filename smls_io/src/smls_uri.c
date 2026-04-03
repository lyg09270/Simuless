/**
 * @file smls_uri.c
 * @brief URI scheme parser for SimuLess.
 */

#include "smls_uri.h"

#include <stddef.h>
#include <string.h>

/**
 * @brief Match URI prefix.
 *
 * @param uri Input URI string
 * @param prefix Prefix string
 * @return 1 if matched, 0 otherwise
 */
static int smls_uri_match_prefix(const char* uri, const char* prefix)
{
    if (uri == NULL || prefix == NULL)
    {
        return 0;
    }

    return strncmp(uri, prefix, strlen(prefix)) == 0;
}

smls_uri_scheme_t smls_uri_parse_scheme(const char* uri)
{
    if (uri == NULL)
    {
        return SMLS_URI_SCHEME_UNKNOWN;
    }

    if (smls_uri_match_prefix(uri, "tcp://"))
    {
        return SMLS_URI_SCHEME_TCP;
    }

    if (smls_uri_match_prefix(uri, "udp://"))
    {
        return SMLS_URI_SCHEME_UDP;
    }

    if (smls_uri_match_prefix(uri, "uart://"))
    {
        return SMLS_URI_SCHEME_UART;
    }

    if (smls_uri_match_prefix(uri, "shm://"))
    {
        return SMLS_URI_SCHEME_SHM;
    }

    return SMLS_URI_SCHEME_UNKNOWN;
}
