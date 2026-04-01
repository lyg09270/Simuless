#include "frame_encoder.h"
#include <string.h>

/**
 * @brief XOR checksum (same as parser)
 */
static uint8_t frame_checksum(const uint8_t* data, uint16_t len)
{
    uint8_t c = 0;

    for (uint16_t i = 0; i < len; i++)
    {
        c ^= data[i];
    }

    return c;
}

int frame_encode(uint8_t type, const void* payload, uint16_t len, uint32_t seq, uint64_t timestamp,
                 uint8_t* out_buf, uint16_t* out_len)
{
    if (!out_buf || !out_len)
        return -1;

    if (len > FRAME_MAX_PAYLOAD)
        return -2;

    // build header
    frame_header_t* h = (frame_header_t*)out_buf;

    h->magic        = FRAME_MAGIC;
    h->type         = type;
    h->timestamp_us = timestamp;
    h->sequence     = seq;
    h->payload_len  = len;

    // copy payload
    if (payload && len > 0)
    {
        memcpy(out_buf + sizeof(frame_header_t), payload, len);
    }

    uint16_t total = sizeof(frame_header_t) + len + 1;

    // append checksum
    out_buf[total - 1] = frame_checksum(out_buf, total - 1);

    *out_len = total;

    return 0;
}
