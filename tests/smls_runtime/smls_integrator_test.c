#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include "smls_integrator.h"
#include "smls_node.h"
#include "smls_slot.h"

#define TEST_CHECK(cond, msg)                                                                      \
    do                                                                                             \
    {                                                                                              \
        if (!(cond))                                                                               \
        {                                                                                          \
            printf("[FAIL] %s\n", (msg));                                                          \
            return -1;                                                                             \
        }                                                                                          \
    } while (0)

#define TEST_FLOAT_EQ(a, b, eps, msg)                                                              \
    do                                                                                             \
    {                                                                                              \
        if (fabsf((a) - (b)) > (eps))                                                              \
        {                                                                                          \
            printf("[FAIL] %s\n", (msg));                                                          \
            printf("  expect: %.6f\n", (b));                                                       \
            printf("  actual: %.6f\n", (a));                                                       \
            return -1;                                                                             \
        }                                                                                          \
    } while (0)

/* ======================================== */

static int test_integrator_basic_step(void)
{
    printf("[TEST] integrator basic step\n");

    static float input_mem  = 1.0f;
    static float output_mem = 0.0f;

    uint16_t dims[1] = {1};

    smls_slot_t input_slot  = {0};
    smls_slot_t output_slot = {0};

    smls_slot_bind(&input_slot, &input_mem, 1, dims);

    smls_slot_bind(&output_slot, &output_mem, 1, dims);

    smls_integrator_t integ = {0};

    integ.base.ops = &smls_integrator_ops;

    smls_node_set_rate(&integ.base, 0.01f, 1);

    integ.input  = &input_slot;
    integ.output = &output_slot;
    integ.gain   = 1.0f;

    TEST_CHECK(smls_node_init(&integ.base) == 0, "node init failed");

    smls_node_step(&integ.base);

    TEST_FLOAT_EQ(output_mem, 0.01f, 1e-6f, "single step output mismatch");

    return 0;
}

/* ======================================== */

static int test_integrator_multi_step(void)
{
    printf("[TEST] integrator multi step\n");

    static float input_mem  = 1.0f;
    static float output_mem = 0.0f;

    uint16_t dims[1] = {1};

    smls_slot_t input_slot  = {0};
    smls_slot_t output_slot = {0};

    smls_slot_bind(&input_slot, &input_mem, 1, dims);

    smls_slot_bind(&output_slot, &output_mem, 1, dims);

    smls_integrator_t integ = {0};

    integ.base.ops = &smls_integrator_ops;

    smls_node_set_rate(&integ.base, 0.01f, 1);

    integ.input  = &input_slot;
    integ.output = &output_slot;
    integ.gain   = 1.0f;

    TEST_CHECK(smls_node_init(&integ.base) == 0, "node init failed");

    for (int i = 0; i < 100; i++)
    {
        smls_node_step(&integ.base);
    }

    TEST_FLOAT_EQ(output_mem, 1.0f, 1e-5f, "100 step integration mismatch");

    return 0;
}

/* ======================================== */

static int test_integrator_vector_step(void)
{
    printf("[TEST] integrator vector step\n");

    static float input_mem[3] = {1.0f, 2.0f, 3.0f};

    static float output_mem[3] = {0.0f, 0.0f, 0.0f};

    uint16_t dims[1] = {3};

    smls_slot_t input_slot  = {0};
    smls_slot_t output_slot = {0};

    smls_slot_bind(&input_slot, input_mem, 1, dims);

    smls_slot_bind(&output_slot, output_mem, 1, dims);

    smls_integrator_t integ = {0};

    integ.base.ops = &smls_integrator_ops;

    smls_node_set_rate(&integ.base, 0.1f, 1);

    integ.input  = &input_slot;
    integ.output = &output_slot;
    integ.gain   = 1.0f;

    TEST_CHECK(smls_node_init(&integ.base) == 0, "vector node init failed");

    smls_node_step(&integ.base);

    TEST_FLOAT_EQ(output_mem[0], 0.1f, 1e-6f, "vector dim0 mismatch");
    TEST_FLOAT_EQ(output_mem[1], 0.2f, 1e-6f, "vector dim1 mismatch");
    TEST_FLOAT_EQ(output_mem[2], 0.3f, 1e-6f, "vector dim2 mismatch");

    return 0;
}

/* ======================================== */

static int test_integrator_matrix_step(void)
{
    printf("[TEST] integrator matrix step\n");

    static float input_mem[4] = {1.0f, 2.0f, 3.0f, 4.0f};

    static float output_mem[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    uint16_t dims[2] = {2, 2};

    smls_slot_t input_slot  = {0};
    smls_slot_t output_slot = {0};

    smls_slot_bind(&input_slot, input_mem, 2, dims);

    smls_slot_bind(&output_slot, output_mem, 2, dims);

    smls_integrator_t integ = {0};

    integ.base.ops = &smls_integrator_ops;

    smls_node_set_rate(&integ.base, 0.1f, 1);

    integ.input  = &input_slot;
    integ.output = &output_slot;
    integ.gain   = 1.0f;

    TEST_CHECK(smls_node_init(&integ.base) == 0, "matrix node init failed");

    smls_node_step(&integ.base);

    TEST_FLOAT_EQ(output_mem[0], 0.1f, 1e-6f, "matrix [0,0] mismatch");
    TEST_FLOAT_EQ(output_mem[1], 0.2f, 1e-6f, "matrix [0,1] mismatch");
    TEST_FLOAT_EQ(output_mem[2], 0.3f, 1e-6f, "matrix [1,0] mismatch");
    TEST_FLOAT_EQ(output_mem[3], 0.4f, 1e-6f, "matrix [1,1] mismatch");

    return 0;
}

/* ======================================== */

static int test_integrator_reset(void)
{
    printf("[TEST] integrator reset\n");

    static float input_mem  = 1.0f;
    static float output_mem = 0.0f;

    uint16_t dims[1] = {1};

    smls_slot_t input_slot  = {0};
    smls_slot_t output_slot = {0};

    smls_slot_bind(&input_slot, &input_mem, 1, dims);

    smls_slot_bind(&output_slot, &output_mem, 1, dims);

    smls_integrator_t integ = {0};

    integ.base.ops = &smls_integrator_ops;

    smls_node_set_rate(&integ.base, 0.01f, 1);

    integ.input  = &input_slot;
    integ.output = &output_slot;
    integ.gain   = 1.0f;

    TEST_CHECK(smls_node_init(&integ.base) == 0, "node init failed");

    for (int i = 0; i < 10; i++)
    {
        smls_node_step(&integ.base);
    }

    smls_node_reset(&integ.base);

    TEST_FLOAT_EQ(output_mem, 0.0f, 1e-6f, "reset output mismatch");

    return 0;
}

/* ======================================== */

static int test_integrator_set_param(void)
{
    printf("[TEST] integrator set param\n");

    static float input_mem  = 2.0f;
    static float output_mem = 0.0f;

    uint16_t dims[1] = {1};

    smls_slot_t input_slot  = {0};
    smls_slot_t output_slot = {0};

    smls_slot_bind(&input_slot, &input_mem, 1, dims);

    smls_slot_bind(&output_slot, &output_mem, 1, dims);

    smls_integrator_t integ = {0};

    integ.base.ops = &smls_integrator_ops;

    smls_node_set_rate(&integ.base, 0.1f, 1);

    integ.input  = &input_slot;
    integ.output = &output_slot;

    smls_integrator_param_t param = {.gain = 0.5f, .init_value = 1.0f};

    TEST_CHECK(smls_node_set_param(&integ.base, &param) == 0, "set param failed");

    smls_node_step(&integ.base);

    TEST_FLOAT_EQ(output_mem, 1.1f, 1e-6f, "set param integration mismatch");

    return 0;
}

/* ======================================== */

int main(void)
{
    TEST_CHECK(test_integrator_basic_step() == 0, "basic step failed");
    TEST_CHECK(test_integrator_multi_step() == 0, "multi step failed");
    TEST_CHECK(test_integrator_vector_step() == 0, "vector step failed");
    TEST_CHECK(test_integrator_matrix_step() == 0, "matrix step failed");
    TEST_CHECK(test_integrator_reset() == 0, "reset failed");
    TEST_CHECK(test_integrator_set_param() == 0, "set param failed");

    printf("[PASS] all integrator tests passed\n");

    return 0;
}
