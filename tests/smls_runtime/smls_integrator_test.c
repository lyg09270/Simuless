#include <math.h>
#include <stdio.h>

#include "smls_edge.h"
#include "smls_integrator.h"
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

static int test_integrator_scalar(void)
{
    float input        = 1.0f;
    float output       = 0.0f;
    float state_buf[1] = {0.0f};

    smls_integrator_param_t param = {.x0 = 0.0f};

    smls_integrator_state_t state = {.state = state_buf, .dim = 1};

    smls_edge_t in_edge;
    smls_edge_init(&in_edge);
    smls_edge_signal_bind(&in_edge, &input, SMLS_DTYPE_FLOAT32);
    in_edge.rank     = 1;
    in_edge.shape[0] = 1;

    smls_edge_t out_edge;
    smls_edge_init(&out_edge);
    smls_edge_signal_bind(&out_edge, &output, SMLS_DTYPE_FLOAT32);
    out_edge.rank     = 1;
    out_edge.shape[0] = 1;

    smls_node_t node;
    smls_node_create(&node, 0, SMLS_OP_INTEGRATOR, &param, &state, 0.1f);

    smls_node_bind_input(&node, &in_edge, 0);
    smls_node_bind_output(&node, &out_edge, 0);

    smls_node_init(&node);

    int ret = smls_node_step(&node);

    TEST_ASSERT(ret == 0, "scalar integrator step failed");

    TEST_ASSERT(fabsf(output - 0.1f) < 1e-6f, "scalar output mismatch");

    printf("[PASS] test_integrator_scalar\n");
    return 0;
}

static int test_integrator_vector(void)
{
    float input[3]     = {1.0f, 2.0f, 3.0f};
    float output[3]    = {0.0f};
    float state_buf[3] = {0.0f};

    smls_integrator_param_t param = {.x0 = 0.0f};

    smls_integrator_state_t state = {.state = state_buf, .dim = 3};

    smls_edge_t in_edge;
    smls_edge_init(&in_edge);
    smls_edge_signal_bind(&in_edge, input, SMLS_DTYPE_FLOAT32);
    in_edge.rank     = 1;
    in_edge.shape[0] = 3;

    smls_edge_t out_edge;
    smls_edge_init(&out_edge);
    smls_edge_signal_bind(&out_edge, output, SMLS_DTYPE_FLOAT32);
    out_edge.rank     = 1;
    out_edge.shape[0] = 3;

    smls_node_t node;
    smls_node_create(&node, 0, SMLS_OP_INTEGRATOR, &param, &state, 0.1f);

    smls_node_bind_input(&node, &in_edge, 0);
    smls_node_bind_output(&node, &out_edge, 0);

    smls_node_init(&node);

    int ret = smls_node_step(&node);

    TEST_ASSERT(ret == 0, "vector integrator step failed");

    TEST_ASSERT(fabsf(output[0] - 0.1f) < 1e-6f, "vector output[0] mismatch");

    TEST_ASSERT(fabsf(output[1] - 0.2f) < 1e-6f, "vector output[1] mismatch");

    TEST_ASSERT(fabsf(output[2] - 0.3f) < 1e-6f, "vector output[2] mismatch");

    printf("[PASS] test_integrator_vector\n");
    return 0;
}

static int test_integrator_matrix_should_fail(void)
{
    float input[4]     = {1, 2, 3, 4};
    float output[4]    = {0};
    float state_buf[4] = {0};

    smls_integrator_param_t param = {.x0 = 0.0f};

    smls_integrator_state_t state = {.state = state_buf, .dim = 4};

    smls_edge_t in_edge;
    smls_edge_init(&in_edge);
    smls_edge_signal_bind(&in_edge, input, SMLS_DTYPE_FLOAT32);

    /* matrix: rank = 2 */
    in_edge.rank     = 2;
    in_edge.shape[0] = 2;
    in_edge.shape[1] = 2;

    smls_edge_t out_edge;
    smls_edge_init(&out_edge);
    smls_edge_signal_bind(&out_edge, output, SMLS_DTYPE_FLOAT32);

    out_edge.rank     = 2;
    out_edge.shape[0] = 2;
    out_edge.shape[1] = 2;

    smls_node_t node;
    smls_node_create(&node, 0, SMLS_OP_INTEGRATOR, &param, &state, 0.1f);

    smls_node_bind_input(&node, &in_edge, 0);
    smls_node_bind_output(&node, &out_edge, 0);

    int ret = smls_node_step(&node);

    TEST_ASSERT(ret < 0, "matrix input should fail");

    printf("[PASS] test_integrator_matrix_should_fail\n");
    return 0;
}

int main(void)
{
    TEST_ASSERT(test_integrator_scalar() == 0, "scalar test failed");

    TEST_ASSERT(test_integrator_vector() == 0, "vector test failed");

    TEST_ASSERT(test_integrator_matrix_should_fail() == 0, "matrix fail test failed");

    printf("[PASS] all integrator tests passed\n");
    return 0;
}
