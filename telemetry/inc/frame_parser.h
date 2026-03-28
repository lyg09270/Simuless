#pragma once
#include "frame.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        FRAME_PARSE_OK = 0,
        FRAME_PARSE_INCOMPLETE,
        FRAME_PARSE_ERROR
    } frame_parse_result_t;

    /**
     * @brief Frame parser context (state machine)
     */
    typedef struct
    {
        uint8_t buffer[sizeof(frame_t)];
        uint16_t index;

    } frame_parser_t;

    /**
     * @brief Initialize parser
     */
    void frame_parser_init(frame_parser_t* parser);

    /**
     * @brief Input data stream into parser
     *
     * @param p parser instance
     * @param data input bytes
     * @param len input length
     *
     * @return parse result
     */
    frame_parse_result_t frame_parser_input(frame_parser_t* parser, const uint8_t* data,
                                            uint16_t len);

    /**
     * @brief Try to get a complete frame
     *
     * @param p parser instance
     * @param out_frame output frame
     *
     * @return 1 if frame ready, 0 otherwise
     */
    int frame_parser_get(frame_parser_t* parser, frame_t* out_frame);

#ifdef __cplusplus
}
#endif