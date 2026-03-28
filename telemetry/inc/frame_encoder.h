#pragma once
#include "frame.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Encode a frame into buffer
     *
     * @param type        Message type
     * @param payload     Pointer to payload data
     * @param len         Payload length
     * @param seq         Sequence number
     * @param timestamp   Timestamp (us)
     * @param out_buf     Output buffer
     * @param out_len     Output length
     *
     * @return 0 on success, negative on error
     */
    int frame_encode(uint8_t type, const void* payload, uint16_t len, uint32_t seq,
                     uint64_t timestamp, uint8_t* out_buf, uint16_t* out_len);

#ifdef __cplusplus
}
#endif