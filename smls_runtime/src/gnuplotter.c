#include "gnuplotter.h"

#ifdef _WIN32
#define GP_POPEN _popen
#define GP_PCLOSE _pclose
#else
#define GP_POPEN popen
#define GP_PCLOSE pclose
#endif

bool gnuplotter_init(gnuplotter_t* gp)
{
    gp->pipe = GP_POPEN("gnuplot -persistent", "w");
    if (!gp->pipe)
    {
        return false;
    }

    gp->initialized = true;
    gp->ymin        = -1.5;
    gp->ymax        = 1.5;
    gp->width       = 800;
    gp->height      = 600;

    fprintf(gp->pipe, "set term wxt size %d,%d\n", gp->width, gp->height);

    fprintf(gp->pipe, "set grid\n");
    fprintf(gp->pipe, "set yrange [%f:%f]\n", gp->ymin, gp->ymax);

    fflush(gp->pipe);

    return true;
}

void gnuplotter_close(gnuplotter_t* gp)
{
    if (!gp || !gp->initialized)
    {
        return;
    }

    GP_PCLOSE(gp->pipe);
    gp->initialized = false;
}

void gnuplotter_set_yrange(gnuplotter_t* gp, double ymin, double ymax)
{
    if (!gp || !gp->initialized)
    {
        return;
    }

    gp->ymin = ymin;
    gp->ymax = ymax;

    fprintf(gp->pipe, "set yrange [%f:%f]\n", ymin, ymax);

    fflush(gp->pipe);
}

void gnuplotter_plot_array(gnuplotter_t* gp, const float* data, size_t size)
{
    if (!gp || !gp->initialized)
    {
        return;
    }

    fprintf(gp->pipe, "set xrange [0:%zu]\n", size);

    fprintf(gp->pipe, "plot '-' with lines notitle\n");

    for (size_t i = 0; i < size; i++)
    {
        fprintf(gp->pipe, "%zu %f\n", i, data[i]);
    }

    fprintf(gp->pipe, "e\n");
    fflush(gp->pipe);
}

void gnuplotter_plot_ringbuf(gnuplotter_t* gp, const smls_ringbuf_t* rb)
{
    if (!gp || !gp->initialized || !rb)
    {
        return;
    }

    fprintf(gp->pipe, "set xrange [0:%zu]\n", rb->size);

    fprintf(gp->pipe, "plot '-' with lines notitle\n");

    for (size_t i = 0; i < rb->size; i++)
    {
        size_t idx = (rb->tail + i) % rb->capacity;

        fprintf(gp->pipe, "%zu %f\n", i, rb->data[idx]);
    }

    fprintf(gp->pipe, "e\n");
    fflush(gp->pipe);
}

void gnuplotter_plot_ringbuf_n(gnuplotter_t* gp, const smls_ringbuf_t* const* rbs,
                               const char* const* labels, uint32_t count)
{
    if ((gp == NULL) || (gp->pipe == NULL) || (rbs == NULL) || (count == 0))
    {
        return;
    }

    FILE* pipe = gp->pipe;

    /* build plot command */
    fprintf(pipe, "plot ");

    for (uint32_t ch = 0; ch < count; ch++)
    {
        const char* label = (labels != NULL) ? labels[ch] : "signal";

        fprintf(pipe, "'-' with lines title '%s'", label);

        if (ch != count - 1)
        {
            fprintf(pipe, ", ");
        }
    }

    fprintf(pipe, "\n");

    /* write all channels */
    for (uint32_t ch = 0; ch < count; ch++)
    {
        const smls_ringbuf_t* rb = rbs[ch];

        if (rb == NULL)
        {
            fprintf(pipe, "e\n");
            continue;
        }

        for (uint32_t i = 0; i < rb->size; i++)
        {
            uint32_t idx = (rb->head + i) % rb->size;

            fprintf(pipe, "%u %f\n", i, rb->data[idx]);
        }

        fprintf(pipe, "e\n");
    }

    fflush(pipe);
}
