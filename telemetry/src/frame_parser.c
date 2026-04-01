#include "frame_parser.h"
#include <string.h>

/**
 * @brief XOR checksum (same as encoder)
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

/**
 * @brief Initialize parser
 */
void frame_parser_init(frame_parser_t* parser)
{
    parser->index = 0;
}

/**
 * @brief Feed data into parser (append-only buffer)
 *
 * This function ONLY appends data.
 * No parsing is performed here.
 */
frame_parse_result_t frame_parser_input(frame_parser_t* parser, const uint8_t* data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++)
    {
        if (parser->index >= sizeof(parser->buffer))
        {
            /**
             * Buffer overflow protection:
             * Shift buffer left by 1 byte to make space.
             * This guarantees forward progress.
             */
            memmove(parser->buffer, &parser->buffer[1], parser->index - 1);
            parser->index--;
        }

        parser->buffer[parser->index++] = data[i];
    }

    return FRAME_PARSE_INCOMPLETE;
}

/**
 * @brief Try to extract one complete frame from buffer
 *
 * This function implements a robust streaming parser:
 *
 * - Always makes forward progress (never stuck)
 * - Never skips valid frames
 * - Automatically resynchronizes from noise
 *
 * @return 1 if a frame is extracted, 0 otherwise
 */
int frame_parser_get(frame_parser_t* parser, frame_t* out_frame)
{
    while (1)
    {
        /**
         * Step 1: Ensure header is available
         */
        if (parser->index < sizeof(frame_header_t))
            return 0;

        /**
         * Step 2: Align to FRAME_MAGIC
         *
         * If the first byte is not magic, discard one byte.
         * This is a safe sliding-window strategy.
         */
        if (parser->buffer[0] != FRAME_MAGIC)
        {
            memmove(parser->buffer, &parser->buffer[1], parser->index - 1);
            parser->index--;
            continue;
        }

        frame_header_t* h = (frame_header_t*)parser->buffer;

        /**
         * Step 3: Validate payload length
         *
         * Invalid length indicates a false header (noise).
         */
        if (h->payload_len == 0 || h->payload_len > FRAME_MAX_PAYLOAD)
        {
            memmove(parser->buffer, &parser->buffer[1], parser->index - 1);
            parser->index--;
            continue;
        }

        uint16_t total = sizeof(frame_header_t) + h->payload_len + 1;

        /**
         * Step 4: Wait for full frame data
         */
        if (parser->index < total)
        {
            return 0;
        }

        /**
         * Step 5: Verify checksum
         *
         * If checksum fails, treat as false sync and discard 1 byte.
         */
        uint8_t expected = frame_checksum(parser->buffer, total - 1);
        uint8_t received = parser->buffer[total - 1];

        if (expected != received)
        {
            memmove(parser->buffer, &parser->buffer[1], parser->index - 1);
            parser->index--;
            continue;
        }

        /**
         * Step 6: Valid frame found
         */
        memcpy(out_frame, parser->buffer, total);

        /**
         * Step 7: Remove consumed bytes
         */
        memmove(parser->buffer, &parser->buffer[total], parser->index - total);
        parser->index -= total;

        return 1;
    }
}
