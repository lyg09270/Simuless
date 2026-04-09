#include <math.h>
#include <stdio.h>

#include "smls_edge.h"
#include "smls_gain.h"
#include "smls_node.h"

#define TEST_ASSERT(cond, msg)                                                                     \
    do                                                                                             \
    {                                                                                              \
        if (!(cond))                                                                               \
        {                                                                                          \
            printf("[FAIL] %s\n", msg);                                                            \
            return -1;                                                                             \
        }                                                                                          \
    } while (0)

static int test_gain_scalar(void)
{
    float input  = 3.0f;
    float output = 0.0f;

    smls_edge_t input_edge;
    smls_edge_init(&input_edge);
    smls_edge_signal_bind(&input_edge, &input, SMLS_DTYPE_FLOAT32);
    input_edge.shape[SMLS_EDGE_DIM_TIME] = 1;

    smls_edge_t output_edge;
    smls_edge_init(&output_edge);
    smls_edge_signal_bind(&output_edge, &output, SMLS_DTYPE_FLOAT32);
    output_edge.shape[SMLS_EDGE_DIM_TIME] = 1;

    smls_gain_param_t gain_param = {.k = 2.0f};

    smls_node_t gain_node;
    smls_node_create(&gain_node, 0, SMLS_OP_GAIN, &gain_param, NULL, 0.1f);

    smls_node_bind_input(&gain_node, &input_edge, 0);

    smls_node_bind_output(&gain_node, &output_edge, 0);

    int ret = smls_node_step(&gain_node);

    TEST_ASSERT(ret == 0, "gain node step failed");

    TEST_ASSERT(fabsf(output - 6.0f) < 1e-6f, "gain output mismatch");

    printf("[PASS] test_gain_scalar\n");
    return 0;
}

int main(void)
{
    if (test_gain_scalar() != 0)
    {
        return -1;
    }

    printf("[PASS] all gain tests passed\n");
    return 0;
}
