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
 * @brief Check whether IO object is minimally valid.
 *
 * @param io IO object
 * @return 1 if valid, otherwise 0
 */
static inline int smls_io_is_valid(const smls_io_t* io)
{
    return (io != NULL) && (io->ops != NULL) && (io->priv != NULL);
}

/**
 * @brief Check whether ops callback exists.
 *
 * @param fn Function pointer
 * @return 1 if supported, otherwise 0
 */
static inline int smls_io_is_supported(const void* fn)
{
    return fn != NULL;
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
        return SMLS_IO_ERR_INVALID_ARG;
    }

    if (io->priv == NULL)
    {
        return SMLS_IO_ERR_INVALID_ARG;
    }

    if (!smls_io_is_supported((const void*)io->ops->open))
    {
        return SMLS_IO_ERR_NOT_SUPPORTED;
    }

    /**
     * reset semantic kind before backend open
     */
    io->kind = SMLS_IO_NONE;

    return io->ops->open(io, desc);
}

/* ========================================================= */

int smls_io_destroy(smls_io_t* io)
{
    if (!smls_io_is_valid(io))
    {
        return SMLS_IO_ERR_INVALID_ARG;
    }

    if (!smls_io_is_supported((const void*)io->ops->close))
    {
        return SMLS_IO_ERR_NOT_SUPPORTED;
    }

    const int ret = io->ops->close(io);

    /**
     * reset semantic state after destroy
     */
    if (ret == SMLS_IO_OK)
    {
        io->kind = SMLS_IO_NONE;
    }

    return ret;
}

/* ========================================================= */

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

    if (pkt->data == NULL)
    {
        return SMLS_IO_ERR_INVALID_ARG;
    }

    if (pkt->len == 0u)
    {
        return SMLS_IO_ERR_INVALID_ARG;
    }

    if (!smls_io_is_supported((const void*)io->ops->push))
    {
        return SMLS_IO_ERR_NOT_SUPPORTED;
    }

    return io->ops->push(io, pkt);
}

/* ========================================================= */

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

    if (pkt->data == NULL)
    {
        return SMLS_IO_ERR_INVALID_ARG;
    }

    if (pkt->len == 0u)
    {
        return SMLS_IO_ERR_INVALID_ARG;
    }

    if (!smls_io_is_supported((const void*)io->ops->pop))
    {
        return SMLS_IO_ERR_NOT_SUPPORTED;
    }

    return io->ops->pop(io, pkt);
}

/* ========================================================= */

int smls_io_poll(smls_io_t* io, uint32_t timeout_ms)
{
    if (!smls_io_is_valid(io))
    {
        return SMLS_IO_ERR_INVALID_ARG;
    }

    if (!smls_io_is_supported((const void*)io->ops->poll))
    {
        return SMLS_IO_ERR_NOT_SUPPORTED;
    }

    return io->ops->poll(io, timeout_ms);
}
