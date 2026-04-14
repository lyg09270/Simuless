#include "smls_ringbuf.h"
#include <string.h>

void smls_ringbuf_init(smls_ringbuf_t* rb, float* storage, size_t capacity)
{
    rb->data     = storage;
    rb->capacity = capacity;
    rb->head     = 0;
    rb->tail     = 0;
    rb->size     = 0;

    memset(storage, 0, sizeof(float) * capacity);
}

void smls_ringbuf_push(smls_ringbuf_t* rb, float value)
{
    rb->data[rb->head] = value;
    rb->head           = (rb->head + 1) % rb->capacity;

    if (rb->size < rb->capacity)
    {
        rb->size++;
    }
    else
    {
        rb->tail = (rb->tail + 1) % rb->capacity;
    }
}

bool smls_ringbuf_pop(smls_ringbuf_t* rb, float* out)
{
    if (rb->size == 0)
    {
        return false;
    }

    *out     = rb->data[rb->tail];
    rb->tail = (rb->tail + 1) % rb->capacity;
    rb->size--;

    return true;
}

bool smls_ringbuf_latest(const smls_ringbuf_t* rb, float* out)
{
    if (rb->size == 0)
    {
        return false;
    }

    size_t idx = (rb->head == 0) ? rb->capacity - 1 : rb->head - 1;

    *out = rb->data[idx];
    return true;
}

bool smls_ringbuf_at(const smls_ringbuf_t* rb, size_t age, float* out)
{
    if (age >= rb->size)
    {
        return false;
    }

    size_t idx = (rb->head + rb->capacity - 1 - age) % rb->capacity;

    *out = rb->data[idx];
    return true;
}

bool smls_ringbuf_empty(const smls_ringbuf_t* rb)
{
    return rb->size == 0;
}

bool smls_ringbuf_full(const smls_ringbuf_t* rb)
{
    return rb->size == rb->capacity;
}

size_t smls_ringbuf_size(const smls_ringbuf_t* rb)
{
    return rb->size;
}
