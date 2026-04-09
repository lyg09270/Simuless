#include <math.h>
#include <stdio.h>

#include "smls_diff_eq.h"
#include "smls_edge.h"
#include "smls_node.h"

#define TEST_CHECK(cond, msg)                                                                      \
    do                                                                                             \
    {                                                                                              \
        if (!(cond))                                                                               \
        {                                                                                          \
            printf("[FAIL] %s\n", msg);                                                            \
            return -1;                                                                             \
        }                                                                                          \
    } while (0)

#define FLOAT_EQ(a, b) (fabsf((a) - (b)) < 1e-4f)

/* =========================================================
 * Common helper
 * ========================================================= */
static int setup_node(smls_node_t* node, smls_edge_t* in_edge, smls_edge_t* out_edge, void* param,
                      void* state, float dt)
{
    TEST_CHECK(smls_node_create(node, 1, SMLS_OP_DIFF_EQ, param, state, dt) == 0,
               "node create failed");

    TEST_CHECK(smls_node_bind_input(node, in_edge, 0) == 0, "bind input failed");

    TEST_CHECK(smls_node_bind_output(node, out_edge, 0) == 0, "bind output failed");

    return 0;
}

/* =========================================================
 * Gain test
 * ========================================================= */
static int test_gain(void)
{
    float num[1]        = {2.0f};
    float den[1]        = {1.0f};
    float input_hist[1] = {0};

    smls_diff_eq_param_t param = {.num = num, .den = den, .num_order = 1, .den_order = 1};

    smls_diff_eq_state_t state = {.input_hist = input_hist, .output_hist = NULL};

    float in_val  = 3.0f;
    float out_val = 0.0f;

    smls_edge_t in_edge, out_edge;
    smls_edge_init(&in_edge);
    smls_edge_init(&out_edge);

    smls_edge_signal_bind(&in_edge, &in_val, SMLS_DTYPE_FLOAT32);
    smls_edge_signal_bind(&out_edge, &out_val, SMLS_DTYPE_FLOAT32);

    in_edge.shape[SMLS_EDGE_DIM_TIME]  = 1;
    out_edge.shape[SMLS_EDGE_DIM_TIME] = 1;

    smls_node_t node;

    TEST_CHECK(setup_node(&node, &in_edge, &out_edge, &param, &state, 0.0f) == 0, "setup failed");

    TEST_CHECK(smls_node_step(&node) == 0, "step failed");

    printf("[DEBUG][GAIN] in=%.4f out=%.4f\n", in_val, out_val);

    TEST_CHECK(FLOAT_EQ(out_val, 6.0f), "gain failed");

    printf("[PASS] test_gain\n");
    return 0;
}

/* =========================================================
 * Integrator test
 * ========================================================= */
static int test_integrator(void)
{
    float dt = 0.1f;

    float num[1] = {dt};
    float den[2] = {1.0f, -1.0f};

    float input_hist[1]  = {0};
    float output_hist[1] = {0};

    smls_diff_eq_param_t param = {.num = num, .den = den, .num_order = 1, .den_order = 2};

    smls_diff_eq_state_t state = {.input_hist = input_hist, .output_hist = output_hist};

    float in_val  = 1.0f;
    float out_val = 0.0f;

    smls_edge_t in_edge, out_edge;
    smls_edge_init(&in_edge);
    smls_edge_init(&out_edge);

    smls_edge_signal_bind(&in_edge, &in_val, SMLS_DTYPE_FLOAT32);
    smls_edge_signal_bind(&out_edge, &out_val, SMLS_DTYPE_FLOAT32);

    in_edge.shape[SMLS_EDGE_DIM_TIME]  = 1;
    out_edge.shape[SMLS_EDGE_DIM_TIME] = 1;

    smls_node_t node;

    TEST_CHECK(setup_node(&node, &in_edge, &out_edge, &param, &state, dt) == 0, "setup failed");

    for (int i = 0; i < 10; i++)
    {
        TEST_CHECK(smls_node_step(&node) == 0, "step failed");

        printf("[DEBUG][INT] step=%d out=%.4f\n", i + 1, out_val);
    }

    printf("[RESULT][INT] final=%.6f err=%.6e\n", out_val, fabsf(out_val - 1.0f));

    TEST_CHECK(FLOAT_EQ(out_val, 1.0f), "integrator failed");

    printf("[PASS] test_integrator\n");
    return 0;
}

/* =========================================================
 * First-order LPF test
 * y = 0.2x + 0.8y_prev
 * ========================================================= */
static int test_lpf(void)
{
    float alpha = 0.2f;

    float num[1] = {alpha};
    float den[2] = {1.0f, -(1.0f - alpha)};

    float input_hist[1]  = {0};
    float output_hist[1] = {0};

    smls_diff_eq_param_t param = {.num = num, .den = den, .num_order = 1, .den_order = 2};

    smls_diff_eq_state_t state = {.input_hist = input_hist, .output_hist = output_hist};

    float in_val  = 1.0f;
    float out_val = 0.0f;

    smls_edge_t in_edge, out_edge;
    smls_edge_init(&in_edge);
    smls_edge_init(&out_edge);

    smls_edge_signal_bind(&in_edge, &in_val, SMLS_DTYPE_FLOAT32);
    smls_edge_signal_bind(&out_edge, &out_val, SMLS_DTYPE_FLOAT32);

    in_edge.shape[SMLS_EDGE_DIM_TIME]  = 1;
    out_edge.shape[SMLS_EDGE_DIM_TIME] = 1;

    smls_node_t node;

    TEST_CHECK(setup_node(&node, &in_edge, &out_edge, &param, &state, 0.1f) == 0, "setup failed");

    for (int i = 0; i < 10; i++)
    {
        TEST_CHECK(smls_node_step(&node) == 0, "step failed");

        printf("[DEBUG][LPF] step=%d out=%.6f\n", i + 1, out_val);
    }

    printf("[PASS] test_lpf\n");
    return 0;
}

/* =========================================================
 * Main
 * ========================================================= */
int main(void)
{
    TEST_CHECK(test_gain() == 0, "test_gain");
    TEST_CHECK(test_integrator() == 0, "test_integrator");
    TEST_CHECK(test_lpf() == 0, "test_lpf");

    printf("[PASS] all tests passed\n");
    return 0;
}
