/**
 * @file frame.h
 * @brief SimuLess frame protocol definition
 *
 * This file defines the binary frame format used by SimuLess telemetry.
 *
 * The frame is designed to be:
 *  - transport agnostic (UART / TCP / UDP)
 *  - easy to parse (fixed header + variable payload)
 *  - extensible
 *
 * Frame layout:
 *
 *  | magic | type | timestamp_us | sequence | payload_len | payload |
 *  |  1B   | 1B   |    8B        |   4B     |    2B       |  nB     |
 *
 */

#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/** @brief Frame magic byte used for synchronization */
#define FRAME_MAGIC 0xAA

/** @brief Maximum payload size in bytes */
#define FRAME_MAX_PAYLOAD 256

/**
 * @brief Frame header structure
 *
 * This header precedes every payload.
 * It is packed to ensure consistent layout across platforms.
 */
#pragma pack(push, 1)
    typedef struct
    {
        uint8_t magic;         /**< Frame start marker (0xAA) */
        uint8_t type;          /**< Message type identifier */
        uint64_t timestamp_us; /**< Timestamp in microseconds */
        uint32_t sequence;     /**< Packet sequence number */
        uint16_t payload_len;  /**< Length of payload in bytes */

    } frame_header_t;
#pragma pack(pop)

    /**
     * @brief Full frame structure
     *
     * Note:
     * - Payload length is defined by header.payload_len
     * - This struct allocates maximum payload size for convenience
     * - Actual transmitted size should be computed dynamically
     */
    typedef struct
    {
        frame_header_t header;              /**< Frame header */
        uint8_t payload[FRAME_MAX_PAYLOAD]; /**< Payload buffer */
        uint8_t checksum;                   /**< Checksum */

    } frame_t;

    /**
     * @brief Compute total frame size (header + payload)
     *
     * @param payload_len Payload size in bytes
     * @return Total frame size in bytes
     */
    static inline uint16_t frame_size(uint16_t payload_len)
    {
        return sizeof(frame_header_t) + payload_len + 1;
    }

#ifdef __cplusplus
}
#endif
