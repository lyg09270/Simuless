#include "smls_integrator.h"

static inline smls_integrator_t* smls_integrator_cast(smls_node_t* node)
{
    return (smls_integrator_t*)node;
}

int smls_integrator_init(struct smls_node* node)
{
    if (node == NULL)
    {
        return -1;
    }

    smls_integrator_t* self = smls_integrator_cast(node);

    self->state = 0.0f;

    if (self->output != NULL)
    {
        self->output->data[0] = 0.0f;
    }

    return 0;
}

void smls_integrator_reset(struct smls_node* node)
{
    if (node == NULL)
    {
        return;
    }

    smls_integrator_t* self = smls_integrator_cast(node);

    self->state = 0.0f;

    if (self->output != NULL)
    {
        self->output->data[0] = 0.0f;
    }
}

int smls_integrator_set_param(struct smls_node* node, const void* param)
{
    if ((node == NULL) || (param == NULL))
    {
        return -1;
    }

    smls_integrator_t* self = smls_integrator_cast(node);

    const smls_integrator_param_t* p = (const smls_integrator_param_t*)param;

    self->gain  = p->gain;
    self->state = p->init_value;

    if (self->output != NULL)
    {
        self->output->data[0] = p->init_value;
    }

    return 0;
}

void smls_integrator_step(struct smls_node* node)
{
    if (node == NULL)
    {
        return;
    }

    smls_integrator_t* self = (smls_integrator_t*)node;

    if ((self->input == NULL) || (self->output == NULL))
    {
        return;
    }

    uint32_t size = smls_slot_size(self->input);

    for (uint32_t i = 0; i < size; i++)
    {
        self->output->data[i] += self->gain * self->input->data[i] * self->base.dt;
    }
}

const smls_node_ops_t smls_integrator_ops = {.init      = smls_integrator_init,
                                             .reset     = smls_integrator_reset,
                                             .set_param = smls_integrator_set_param,
                                             .step      = smls_integrator_step};
