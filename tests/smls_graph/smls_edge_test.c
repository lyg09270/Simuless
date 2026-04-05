#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "smls_edge.h"

#define TEST_CHECK(cond, msg)                                                                      \
    do                                                                                             \
    {                                                                                              \
        if (!(cond))                                                                               \
        {                                                                                          \
            printf("[FAIL] %s\n", msg);                                                            \
            return -1;                                                                             \
        }                                                                                          \
    } while (0)

/* ========================================================= */

static int test_edge_basic(void)
{
    smls_edge_spec_t spec = {.dtype = SMLS_EDGE_DTYPE_FLOAT32, .rank = 2, .dims = {2, 3}};

    smls_edge_t edge = {0};

    int ret = smls_edge_init(&spec, &edge);

    TEST_CHECK(ret == SMLS_EDGE_ERR_NONE, "edge init failed");

    TEST_CHECK(edge.bytes == 2 * 3 * sizeof(float), "edge bytes mismatch");

    ret = smls_edge_alloc(&edge);

    TEST_CHECK(ret == SMLS_EDGE_ERR_NONE, "edge alloc failed");

    TEST_CHECK(edge.data != NULL, "edge data null");

    float input[6] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};

    ret = smls_edge_write(&edge, input, sizeof(input), 123456u);

    TEST_CHECK(ret == SMLS_EDGE_ERR_NONE, "edge write failed");

    TEST_CHECK(edge.timestamp_us == 123456u, "timestamp mismatch");

    TEST_CHECK(edge.sequence_id == 1u, "sequence mismatch");

    float* data = (float*)edge.data;

    for (int i = 0; i < 6; i++)
    {
        TEST_CHECK(data[i] == input[i], "write data mismatch");
    }

    smls_edge_clear(&edge);

    for (int i = 0; i < 6; i++)
    {
        TEST_CHECK(data[i] == 0.0f, "clear failed");
    }

    smls_edge_free(&edge);

    TEST_CHECK(edge.data == NULL, "free failed");

    return 0;
}

/* ========================================================= */

static int test_edge_bind(void)
{
    smls_edge_spec_t spec = {.dtype = SMLS_EDGE_DTYPE_INT32, .rank = 1, .dims = {4}};

    smls_edge_t edge = {0};

    int ret = smls_edge_init(&spec, &edge);

    TEST_CHECK(ret == SMLS_EDGE_ERR_NONE, "init failed");

    int32_t ext_buffer[4] = {0};

    ret = smls_edge_bind(&edge, ext_buffer, sizeof(ext_buffer));

    TEST_CHECK(ret == SMLS_EDGE_ERR_NONE, "bind failed");

    TEST_CHECK(edge.data == ext_buffer, "bind pointer mismatch");

    int32_t input[4] = {10, 20, 30, 40};

    ret = smls_edge_write(&edge, input, sizeof(input), 100u);

    TEST_CHECK(ret == SMLS_EDGE_ERR_NONE, "write after bind failed");

    for (int i = 0; i < 4; i++)
    {
        TEST_CHECK(ext_buffer[i] == input[i], "external buffer write mismatch");
    }

    smls_edge_free(&edge);

    TEST_CHECK(edge.data == NULL, "free after bind failed");

    return 0;
}

/* ========================================================= */

int main(void)
{
    printf("Running smls_edge tests...\n");

    TEST_CHECK(test_edge_basic() == 0, "basic test failed");

    TEST_CHECK(test_edge_bind() == 0, "bind test failed");

    printf("[PASS] all smls_edge tests passed\n");

    return 0;
}
