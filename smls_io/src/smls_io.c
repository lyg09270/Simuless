/*
 Copyright (c) 2026 Civic_Crab

 Permission is hereby granted, free of charge, to any person obtaining a copy of
 this software and associated documentation files (the "Software"), to deal in
 the Software without restriction, including without limitation the rights to
 use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 the Software, and to permit persons to whom the Software is furnished to do so,
 subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "smls_io.h"

#include <stddef.h>

/* =========================================================
 * Internal helpers
 * ========================================================= */

/**
 * @brief Check whether IO object is valid.
 *
 * @param io IO object
 * @return 1 if valid, otherwise 0
 */
static inline int smls_io_is_valid(const smls_io_t* io)
{
    return (io != NULL) && (io->ops != NULL);
}

/* =========================================================
 * Public API
 * ========================================================= */

int smls_io_create(smls_io_t* io, const smls_io_desc_t* desc)
{
    if (io == NULL || desc == NULL)
    {
        return SMLS_IO_ERR_INVALID_ARG;
    }

    if (io->ops == NULL)
    {
        return SMLS_IO_ERR_INVALID_STATE;
    }

    if (io->ops->open == NULL)
    {
        return SMLS_IO_ERR_NOT_SUPPORTED;
    }

    return io->ops->open(io, desc->uri);
}

int smls_io_destroy(smls_io_t* io)
{
    if (!smls_io_is_valid(io))
    {
        return SMLS_IO_ERR_INVALID_ARG;
    }

    if (io->ops->close == NULL)
    {
        return SMLS_IO_ERR_NOT_SUPPORTED;
    }

    return io->ops->close(io);
}

int smls_io_push(smls_io_t* io, const smls_packet_t* pkt)
{
    if (!smls_io_is_valid(io))
    {
        return SMLS_IO_ERR_INVALID_ARG;
    }

    if (pkt == NULL)
    {
        return SMLS_IO_ERR_INVALID_ARG;
    }

    if (io->ops->push == NULL)
    {
        return SMLS_IO_ERR_NOT_SUPPORTED;
    }

    return io->ops->push(io, pkt);
}

int smls_io_pop(smls_io_t* io, smls_packet_t* pkt)
{
    if (!smls_io_is_valid(io))
    {
        return SMLS_IO_ERR_INVALID_ARG;
    }

    if (pkt == NULL)
    {
        return SMLS_IO_ERR_INVALID_ARG;
    }

    if (io->ops->pop == NULL)
    {
        return SMLS_IO_ERR_NOT_SUPPORTED;
    }

    return io->ops->pop(io, pkt);
}

int smls_io_poll(smls_io_t* io, uint32_t timeout_ms)
{
    if (!smls_io_is_valid(io))
    {
        return SMLS_IO_ERR_INVALID_ARG;
    }

    if (io->ops->poll == NULL)
    {
        return SMLS_IO_ERR_NOT_SUPPORTED;
    }

    return io->ops->poll(io, timeout_ms);
}
