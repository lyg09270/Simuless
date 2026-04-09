#pragma once

#include "smls_node.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define SMLS_GRAPH_MAX_NODES 64
#define SMLS_GRAPH_MAX_EDGES 128

    /**
     * @brief Signal edge
     */
    typedef struct
    {
        /**
         * @brief Current signal value
         */
        float value;

        /**
         * @brief Timestamp
         */
        uint64_t timestamp_us;

        /**
         * @brief Sequence id
         */
        uint32_t sequence;

    } smls_edge_t;

    /**
     * @brief Node graph IR
     */
    typedef struct
    {
        smls_node_t nodes[SMLS_GRAPH_MAX_NODES];
        uint16_t node_count;

        smls_edge_t edges[SMLS_GRAPH_MAX_EDGES];
        uint16_t edge_count;

    } smls_graph_t;

    /**
     * @brief Execute one graph step
     */
    static inline void smls_graph_step(smls_graph_t* graph)
    {
        if (graph == NULL)
        {
            return;
        }

        for (uint16_t i = 0; i < graph->node_count; i++)
        {
            smls_node_t* node = &graph->nodes[i];

            float inputs[SMLS_NODE_MAX_INPUTS] = {0};

            for (uint8_t j = 0; j < node->input_count; j++)
            {
                inputs[j] = graph->edges[node->input_edges[j]].value;
            }

            float y = node->step(node, inputs);

            for (uint8_t j = 0; j < node->output_count; j++)
            {
                graph->edges[node->output_edges[j]].value = y;
            }
        }
    }

#ifdef __cplusplus
}
#endif
