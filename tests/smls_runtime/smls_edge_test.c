#include "smls_edge.h"
#include <stdio.h>

int main()
{
    smls_edge_t edge;

    int ret = smls_edge_init(&edge);

    if (ret != 0)
    {
        printf("Edge initialization failed: %d\n", ret);
        return -1;
    }

    float signal_value[5]          = {3.14f, 2.71f, 1.41f, 1.73f, 0.57f};
    edge.shape[SMLS_EDGE_DIM_TIME] = 5; // vector of length 5

    ret = smls_edge_signal_bind(&edge, signal_value, SMLS_DTYPE_FLOAT32);

    for (int i = 0; i < edge.shape[SMLS_EDGE_DIM_TIME]; i++)
    {
        printf("Signal value[%d]: %.2f\n", i, signal_value[i]);
    }

    if (ret != 0)
    {
        printf("Edge signal bind failed: %d\n", ret);
        return -1;
    }

    printf("Edge initialized and signal bound successfully.\n");
    printf("Signal value: %.2f\n", *(float*)edge.signal);
    printf("Signal type: %s\n", smls_dtype_get_name(edge.type));

    return 0;
}
