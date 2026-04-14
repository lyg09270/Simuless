#include <math.h>
#include <windows.h>

#include "gnuplotter.h"
#include "smls_edge.h"
#include "smls_integrator.h"
#include "smls_node.h"
#include "smls_ringbuf.h"

#define N 200
#define DT 0.01f

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

    // input edge
    smls_edge_t input_edge;
    smls_edge_init(&input_edge);
    smls_edge_signal_bind(&input_edge, &input, SMLS_DTYPE_FLOAT32);

    // ensure scalar shape metadata
    input_edge.rank     = 1;
    input_edge.shape[0] = 1;

    // output edge
    smls_edge_t output_edge;
    smls_edge_init(&output_edge);
    smls_edge_signal_bind(&output_edge, &output, SMLS_DTYPE_FLOAT32);

    output_edge.rank     = 1;
    output_edge.shape[0] = 1;

    // integrator params
    smls_integrator_param_t int_param = {.x0 = 1.0f};

    // integrator internal memory
    float integrator_mem[1] = {1.0f};

    // integrator state struct
    smls_integrator_state_t int_state = {.state = integrator_mem};

    // node
    smls_node_t int_node;
    smls_node_create(&int_node, 0, SMLS_OP_INTEGRATOR, &int_param, &int_state, DT);

    smls_node_bind_input(&int_node, &input_edge, 0);
    smls_node_bind_output(&int_node, &output_edge, 0);

    // plotter
    gnuplotter_t gp;
    gnuplotter_init(&gp);

    // smaller range for better visibility
    gnuplotter_set_yrange(&gp, -2.0, 2.0);

    const smls_ringbuf_t* rbs[] = {&in_rb, &out_rb};

    const char* labels[] = {"input", "integrator"};

    for (int t = 0; t < 1000; t++)
    {
        float time = t * DT;

        input = sinf(time * 5.0f);

        smls_node_step(&int_node);

        smls_ringbuf_push(&in_rb, input);
        smls_ringbuf_push(&out_rb, output);

        gnuplotter_plot_ringbuf_n(&gp, rbs, labels, 2);

        Sleep(16);
    }

    gnuplotter_close(&gp);

    return 0;
}
