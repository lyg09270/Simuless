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
#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * @brief Simuless IO runtime object.
     */
    typedef struct smls_io smls_io_t;

    /**
     * @brief IO backend operation table.
     */
    typedef struct smls_io_ops smls_io_ops_t;

    /**
     * @brief Unified data packet.
     */
    typedef struct smls_packet smls_packet_t;

    /**
     * @brief IO descriptor used during creation.
     */
    typedef struct smls_io_desc smls_io_desc_t;

    /**
     * @brief IO status codes.
     */
    typedef enum
    {
        SMLS_IO_OK = 0,

        SMLS_IO_ERR_INVALID_ARG   = -1,
        SMLS_IO_ERR_INVALID_STATE = -2,
        SMLS_IO_ERR_NOT_SUPPORTED = -3,
        SMLS_IO_ERR_TIMEOUT       = -4,
        SMLS_IO_ERR_IO            = -5,
        SMLS_IO_ERR_CLOSED        = -6
    } smls_io_status_t;

    /**
     * @brief IO semantic category.
     */
    typedef enum
    {
        /** Continuous byte stream (TCP, UART, pipe) */
        SMLS_IO_STREAM,

        /** Discrete packet / frame (UDP, CAN, RF) */
        SMLS_IO_PACKET,

        /** Publish-subscribe model (DDS, ROS topic, MQTT) */
        SMLS_IO_PUBSUB,

        /** Shared memory / queue / ringbuffer */
        SMLS_IO_SHM
    } smls_io_kind_t;

    /**
     * @brief Unified data packet.
     */
    struct smls_packet
    {
        /** Channel / topic / packet key */
        uint32_t key;

        /** Timestamp in microseconds */
        uint64_t timestamp_us;

        /** Payload pointer */
        void* data;

        /** Payload length in bytes */
        uint32_t len;
    };

    /**
     * @brief Backend operation table.
     */
    struct smls_io_ops
    {
        /**
         * @brief Open backend from URI.
         *
         * @param io IO object
         * @param uri Backend URI
         * @return 0 on success, negative on failure
         */
        int (*open)(smls_io_t* io, const char* uri);

        /**
         * @brief Close backend.
         *
         * @param io IO object
         * @return 0 on success, negative on failure
         */
        int (*close)(smls_io_t* io);

        /**
         * @brief Push packet to backend.
         *
         * @param io IO object
         * @param pkt Packet to send
         * @return Number of bytes or status code
         */
        int (*push)(smls_io_t* io, const smls_packet_t* pkt);

        /**
         * @brief Pop packet from backend.
         *
         * @param io IO object
         * @param pkt Output packet
         * @return Number of bytes or status code
         */
        int (*pop)(smls_io_t* io, smls_packet_t* pkt);

        /**
         * @brief Poll backend for readiness / events.
         *
         * @param io IO object
         * @param timeout_ms Timeout in milliseconds
         * @return >0 if ready, 0 if timeout, negative on error
         */
        int (*poll)(smls_io_t* io, uint32_t timeout_ms);
    };

    /**
     * @brief IO creation descriptor.
     */
    struct smls_io_desc
    {
        /** Backend URI */
        const char* uri;

        /** Backend-specific configuration */
        const void* cfg;
    };

    /**
     * @brief Runtime IO object.
     */
    struct smls_io
    {
        /** Backend operations */
        const smls_io_ops_t* ops;

        /** IO semantic kind */
        smls_io_kind_t kind;

        /** Backend private context */
        void* priv;
    };

    /* =========================================================
     * Public API
     * ========================================================= */

    /**
     * @brief Create and initialize IO object.
     *
     * @param io IO object
     * @param desc IO descriptor
     * @return 0 on success, negative on failure
     */
    int smls_io_create(smls_io_t* io, const smls_io_desc_t* desc);

    /**
     * @brief Destroy IO object.
     *
     * @param io IO object
     * @return 0 on success, negative on failure
     */
    int smls_io_destroy(smls_io_t* io);

    /**
     * @brief Push packet to IO backend.
     *
     * @param io IO object
     * @param pkt Packet to send
     * @return Number of bytes or status code
     */
    int smls_io_push(smls_io_t* io, const smls_packet_t* pkt);

    /**
     * @brief Pop packet from IO backend.
     *
     * @param io IO object
     * @param pkt Output packet
     * @return Number of bytes or status code
     */
    int smls_io_pop(smls_io_t* io, smls_packet_t* pkt);

    /**
     * @brief Poll IO backend.
     *
     * @param io IO object
     * @param timeout_ms Timeout in milliseconds
     * @return >0 if ready, 0 if timeout, negative on error
     */
    int smls_io_poll(smls_io_t* io, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif
