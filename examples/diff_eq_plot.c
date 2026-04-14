#include <math.h>
#include <windows.h>

#include "gnuplotter.h"
#include "smls_diff_eq.h"
#include "smls_edge.h"
#include "smls_node.h"
#include "smls_ringbuf.h"

#define N 200

int main()
{
    float input  = 0.0f;
    float output = 0.0f;

    float in_storage[N]  = {0};
    float out_storage[N] = {0};

    smls_ringbuf_t in_rb;
    smls_ringbuf_t out_rb;

    smls_ringbuf_init(&in_rb, in_storage, N);
    smls_ringbuf_init(&out_rb, out_storage, N);

    smls_edge_t input_edge;
    smls_edge_init(&input_edge);
    smls_edge_signal_bind(&input_edge, &input, SMLS_DTYPE_FLOAT32);

    smls_edge_t output_edge;
    smls_edge_init(&output_edge);
    smls_edge_signal_bind(&output_edge, &output, SMLS_DTYPE_FLOAT32);

    /*
     * Biquad low-pass:
     *
     * y[k] =
     *   b0*x[k]
     * + b1*x[k-1]
     * + b2*x[k-2]
     * - a1*y[k-1]
     * - a2*y[k-2]
     */
    float num[] = {0.0675f, 0.1349f, 0.0675f};

    float den[] = {1.0f, -1.1430f, 0.4128f};

    smls_diff_eq_param_t diff_param = {.num = num, .den = den, .num_order = 3, .den_order = 3};

    float input_hist[3]  = {0};
    float output_hist[2] = {0};

    smls_diff_eq_state_t diff_state = {.input_hist = input_hist, .output_hist = output_hist};

    smls_node_t diff_node;
    smls_node_create(&diff_node, 0, SMLS_OP_DIFF_EQ, &diff_param, &diff_state, 0.1f);

    smls_node_bind_input(&diff_node, &input_edge, 0);

    smls_node_bind_output(&diff_node, &output_edge, 0);

    gnuplotter_t gp;
    gnuplotter_init(&gp);
    gnuplotter_set_yrange(&gp, -1.5, 1.5);

    const smls_ringbuf_t* rbs[] = {&in_rb, &out_rb};

    const char* labels[] = {"input", "biquad"};

    for (int t = 0; t < 1000; t++)
    {
        input = sinf(t * 0.1f);

        smls_node_step(&diff_node);

        smls_ringbuf_push(&in_rb, input);
        smls_ringbuf_push(&out_rb, output);

        gnuplotter_plot_ringbuf_n(&gp, rbs, labels, 2);

        Sleep(16);
    }

    gnuplotter_close(&gp);

    return 0;
}
