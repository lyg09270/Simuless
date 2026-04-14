#include <math.h>
#include <windows.h>

#include "gnuplotter.h"
#include "smls_edge.h"
#include "smls_gain.h"
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

    smls_gain_param_t gain_param = {.k = 2.0f};

    smls_node_t gain_node;
    smls_node_create(&gain_node, 0, SMLS_OP_GAIN, &gain_param, NULL, 0.1f);

    smls_node_bind_input(&gain_node, &input_edge, 0);

    smls_node_bind_output(&gain_node, &output_edge, 0);

    gnuplotter_t gp;
    gnuplotter_init(&gp);
    gnuplotter_set_yrange(&gp, -3.0, 3.0);

    for (int t = 0; t < 1000; t++)
    {
        input = sinf(t * 0.1f);

        smls_node_step(&gain_node);

        smls_ringbuf_push(&in_rb, input);
        smls_ringbuf_push(&out_rb, output);

        gnuplotter_plot_ringbuf(&gp, &out_rb);

        Sleep(16);
    }

    gnuplotter_close(&gp);
    return 0;
}
