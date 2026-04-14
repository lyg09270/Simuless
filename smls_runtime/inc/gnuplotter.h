#pragma once

#include "smls_ringbuf.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        FILE* pipe;
        bool initialized;
        double ymin;
        double ymax;
        int width;
        int height;
    } gnuplotter_t;

    /**
     * @brief Initialize gnuplot window
     *
     * @return true if success
     */
    bool gnuplotter_init(gnuplotter_t* gp);

    /**
     * @brief Close plotter
     */
    void gnuplotter_close(gnuplotter_t* gp);

    /**
     * @brief Set y range
     */
    void gnuplotter_set_yrange(gnuplotter_t* gp, double ymin, double ymax);

    /**
     * @brief Plot one float array
     */
    void gnuplotter_plot_array(gnuplotter_t* gp, const float* data, size_t size);

    /**
     * @brief Plot ring buffer as scope
     */
    void gnuplotter_plot_ringbuf(gnuplotter_t* gp, const smls_ringbuf_t* rb);

    /**
     * @brief Plot multiple ring buffers
     *
     * Plot N channels in one scope window.
     *
     * @param gp plotter
     * @param rbs array of ring buffer pointers
     * @param labels array of channel names
     * @param count number of channels
     */
    void gnuplotter_plot_ringbuf_n(gnuplotter_t* gp, const smls_ringbuf_t* const* rbs,
                                   const char* const* labels, uint32_t count);

#ifdef __cplusplus
}
#endif
