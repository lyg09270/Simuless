/**
 * @file parser_encoder_test.c
 * @brief Industrial-style test suite for frame encoder/parser
 *
 * This test suite validates three important properties:
 *
 * 1. Fragmentation robustness:
 *    - Valid frames are split into random chunk sizes.
 *    - No frame corruption is introduced.
 *    - Expected result: zero loss, zero reordering, zero fake frame.
 *
 * 2. Resynchronization robustness:
 *    - Random noise bytes are inserted before and after valid frames.
 *    - Valid frames themselves remain intact.
 *    - Expected result: all valid frames are parsed correctly.
 *
 * 3. Corruption robustness:
 *    - Valid frames are intentionally corrupted by random bit/byte changes.
 *    - Expected result: corrupted frames may be dropped,
 *      but no fake frame or out-of-order frame is allowed.
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "frame.h"
#include "frame_encoder.h"
#include "frame_parser.h"

#define TEST_FRAME_COUNT 2000
#define MAX_PAYLOAD_SIZE 128
#define MAX_FRAME_SIZE (sizeof(frame_header_t) + MAX_PAYLOAD_SIZE + 1)
#define MAX_STREAM_SIZE 65536
#define MAX_NOISE_BYTES 32
#define MAX_FRAGMENT_CHUNK 16

typedef struct
{
    int parsed_frames;
    int out_of_order;
    int duplicate_or_backward;
    int fake_type;
    int invalid_length;
    int first_missing_seq;
    int last_seq;
} parse_stats_t;

/* =========================================================
 * Utilities
 * ========================================================= */

static void fill_random(uint8_t* buf, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++)
    {
        buf[i] = (uint8_t)(rand() & 0xFF);
    }
}

static uint16_t random_payload_len(void)
{
    return (uint16_t)((rand() % MAX_PAYLOAD_SIZE) + 1);
}

static uint16_t random_noise_len(void)
{
    return (uint16_t)(rand() % (MAX_NOISE_BYTES + 1));
}

static void append_noise(uint8_t* stream, uint32_t* offset)
{
    uint16_t n = random_noise_len();

    for (uint16_t i = 0; i < n; i++)
    {
        uint8_t b;

        do
        {
            b = (uint8_t)(rand() & 0xFF);
        } while (b == FRAME_MAGIC);

        stream[*offset] = b;
        (*offset)++;
    }
}

/* =========================================================
 * Frame generation
 * ========================================================= */

static int build_valid_frame(uint8_t* out_buf, uint16_t* out_len, uint32_t seq)
{
    uint8_t payload[MAX_PAYLOAD_SIZE];
    uint16_t payload_len = random_payload_len();

    fill_random(payload, payload_len);

    return frame_encode(1,                                        /* type */
                        payload, payload_len, seq, (uint64_t)seq, /* timestamp */
                        out_buf, out_len);
}

static void corrupt_frame(uint8_t* frame_buf, uint16_t frame_len)
{
    if (frame_len <= sizeof(frame_header_t) + 1)
    {
        return;
    }

    /*
     * Do not corrupt magic byte here.
     * Corrupting internal bytes is more useful for checksum validation.
     */
    uint16_t pos = (uint16_t)(1 + (rand() % (frame_len - 1)));
    uint8_t val  = (uint8_t)((rand() % 255) + 1);

    frame_buf[pos] ^= val;
}

/* =========================================================
 * Parser feeding and validation
 * ========================================================= */

static void parse_stats_init(parse_stats_t* s)
{
    memset(s, 0, sizeof(*s));
    s->first_missing_seq = -1;
    s->last_seq          = -1;
}

static int validate_frame(const frame_t* f, parse_stats_t* stats)
{
    if (f->header.type != 1)
    {
        stats->fake_type++;
        printf("[ERROR] Fake frame type detected: %u\n", f->header.type);
        return -1;
    }

    if (f->header.payload_len == 0 || f->header.payload_len > FRAME_MAX_PAYLOAD)
    {
        stats->invalid_length++;
        printf("[ERROR] Invalid payload length detected: %u\n", f->header.payload_len);
        return -1;
    }

    if ((int)f->header.sequence < stats->last_seq)
    {
        stats->duplicate_or_backward++;
        printf("[ERROR] Backward sequence detected: last=%d current=%u\n", stats->last_seq,
               f->header.sequence);
        return -1;
    }

    if ((int)f->header.sequence > stats->last_seq + 1)
    {
        stats->out_of_order++;
        if (stats->first_missing_seq < 0)
        {
            stats->first_missing_seq = stats->last_seq + 1;
        }

        /*
         * This is not always fatal in corruption tests,
         * but it is fatal in strict fragmentation/resynchronization tests.
         */
    }

    stats->last_seq = (int)f->header.sequence;
    stats->parsed_frames++;

    return 0;
}

static int feed_stream_random_chunks(frame_parser_t* parser, const uint8_t* stream, uint32_t len,
                                     parse_stats_t* stats)
{
    uint32_t i = 0;

    while (i < len)
    {
        uint16_t chunk = (uint16_t)((rand() % MAX_FRAGMENT_CHUNK) + 1);

        if (i + chunk > len)
        {
            chunk = (uint16_t)(len - i);
        }

        (void)frame_parser_input(parser, &stream[i], chunk);

        for (;;)
        {
            frame_t frame;

            if (!frame_parser_get(parser, &frame))
            {
                break;
            }

            // printf("[FRAME] seq=%u len=%u\n",
            //        frame.header.sequence,
            //        frame.header.payload_len);

            if (validate_frame(&frame, stats) != 0)
            {
                return -1;
            }
        }

        i += chunk;
    }

    return 0;
}

/* =========================================================
 * Test case 1: fragmentation only
 * ========================================================= */

static int test_fragmentation_only(void)
{
    uint8_t stream[MAX_STREAM_SIZE];
    uint32_t offset     = 0;
    int expected_frames = 0;

    frame_parser_t parser;
    parse_stats_t stats;

    frame_parser_init(&parser);
    parse_stats_init(&stats);

    for (uint32_t seq = 0; seq < TEST_FRAME_COUNT; seq++)
    {
        uint16_t frame_len = 0;

        if (build_valid_frame(&stream[offset], &frame_len, seq) != 0)
        {
            printf("[ERROR] frame_encode failed in fragmentation test\n");
            return -1;
        }

        offset += frame_len;
        expected_frames++;

        if (offset > MAX_STREAM_SIZE - MAX_FRAME_SIZE)
        {
            break;
        }
    }

    printf("\n=== Test 1: Fragmentation only ===\n");
    printf("Stream bytes     : %u\n", offset);
    printf("Expected frames  : %d\n", expected_frames);

    if (feed_stream_random_chunks(&parser, stream, offset, &stats) != 0)
    {
        return -1;
    }

    printf("Parsed frames    : %d\n", stats.parsed_frames);
    printf("Out-of-order gaps: %d\n", stats.out_of_order);
    printf("Fake types       : %d\n", stats.fake_type);
    printf("Invalid lengths  : %d\n", stats.invalid_length);

    if (stats.parsed_frames != expected_frames || stats.out_of_order != 0 || stats.fake_type != 0 ||
        stats.invalid_length != 0)
    {
        printf("[FAIL] Fragmentation test failed\n");
        return -1;
    }

    printf("[PASS] Fragmentation test passed\n");
    return 0;
}

/* =========================================================
 * Test case 2: noise + resynchronization
 * ========================================================= */

static int test_noise_resync(void)
{
    uint8_t stream[MAX_STREAM_SIZE];
    uint32_t offset     = 0;
    int expected_frames = 0;

    frame_parser_t parser;
    parse_stats_t stats;

    frame_parser_init(&parser);
    parse_stats_init(&stats);

    for (uint32_t seq = 0; seq < TEST_FRAME_COUNT; seq++)
    {
        uint8_t frame_buf[MAX_FRAME_SIZE];
        uint16_t frame_len = 0;

        append_noise(stream, &offset);

        if (build_valid_frame(frame_buf, &frame_len, seq) != 0)
        {
            printf("[ERROR] frame_encode failed in noise test\n");
            return -1;
        }

        memcpy(&stream[offset], frame_buf, frame_len);
        offset += frame_len;
        expected_frames++;

        append_noise(stream, &offset);

        if (offset > MAX_STREAM_SIZE - (MAX_FRAME_SIZE + MAX_NOISE_BYTES * 2))
        {
            break;
        }
    }

    printf("\n=== Test 2: Noise and resynchronization ===\n");
    printf("Stream bytes     : %u\n", offset);
    printf("Expected frames  : %d\n", expected_frames);

    if (feed_stream_random_chunks(&parser, stream, offset, &stats) != 0)
    {
        return -1;
    }

    printf("Parsed frames    : %d\n", stats.parsed_frames);
    printf("Out-of-order gaps: %d\n", stats.out_of_order);
    printf("Fake types       : %d\n", stats.fake_type);
    printf("Invalid lengths  : %d\n", stats.invalid_length);

    if (stats.parsed_frames != expected_frames || stats.out_of_order != 0 || stats.fake_type != 0 ||
        stats.invalid_length != 0)
    {
        printf("[FAIL] Noise/resync test failed\n");
        return -1;
    }

    printf("[PASS] Noise/resync test passed\n");
    return 0;
}

/* =========================================================
 * Test case 3: corruption robustness
 * ========================================================= */

static int test_corruption_robustness(void)
{
    uint8_t stream[MAX_STREAM_SIZE];
    uint32_t offset = 0;

    frame_parser_t parser;
    parse_stats_t stats;

    int valid_frames_built      = 0;
    int intentionally_corrupted = 0;

    frame_parser_init(&parser);
    parse_stats_init(&stats);

    for (uint32_t seq = 0; seq < TEST_FRAME_COUNT; seq++)
    {
        uint8_t frame_buf[MAX_FRAME_SIZE];
        uint16_t frame_len = 0;

        append_noise(stream, &offset);

        if (build_valid_frame(frame_buf, &frame_len, seq) != 0)
        {
            printf("[ERROR] frame_encode failed in corruption test\n");
            return -1;
        }

        /*
         * Corrupt about 20% of frames intentionally.
         * Corrupted frames are expected to be dropped by parser.
         */
        if ((rand() % 100) < 20)
        {
            corrupt_frame(frame_buf, frame_len);
            intentionally_corrupted++;
        }

        memcpy(&stream[offset], frame_buf, frame_len);
        offset += frame_len;
        valid_frames_built++;

        append_noise(stream, &offset);

        if (offset > MAX_STREAM_SIZE - (MAX_FRAME_SIZE + MAX_NOISE_BYTES * 2))
        {
            break;
        }
    }

    printf("\n=== Test 3: Corruption robustness ===\n");
    printf("Stream bytes           : %u\n", offset);
    printf("Frames built           : %d\n", valid_frames_built);
    printf("Frames corrupted       : %d\n", intentionally_corrupted);

    if (feed_stream_random_chunks(&parser, stream, offset, &stats) != 0)
    {
        return -1;
    }

    printf("Parsed frames          : %d\n", stats.parsed_frames);
    printf("Out-of-order gaps      : %d\n", stats.out_of_order);
    printf("Fake types             : %d\n", stats.fake_type);
    printf("Invalid lengths        : %d\n", stats.invalid_length);
    printf("First missing seq      : %d\n", stats.first_missing_seq);

    /*
     * Industrial expectation for corruption test:
     * - no fake frame
     * - no invalid frame shape
     * - missing frames are allowed
     */
    if (stats.fake_type != 0 || stats.invalid_length != 0 || stats.duplicate_or_backward != 0)
    {
        printf("[FAIL] Corruption robustness test failed\n");
        return -1;
    }

    printf("[PASS] Corruption robustness test passed\n");
    return 0;
}

/* =========================================================
 * Main
 * ========================================================= */

int main(void)
{
    srand((unsigned int)time(NULL));

    if (test_fragmentation_only() != 0)
    {
        return -1;
    }

    if (test_noise_resync() != 0)
    {
        return -1;
    }

    if (test_corruption_robustness() != 0)
    {
        return -1;
    }

    printf("\n[SUCCESS] All industrial-style parser tests passed.\n");
    return 0;
}
