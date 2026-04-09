#pragma once

#include <stdio.h>

/**
 * @brief Basic structured log
 *
 * Usage:
 * SMLS_LOG("GAIN", "y=%f", y);
 */
#define SMLS_LOG(module, fmt, ...)                                                                 \
    do                                                                                             \
    {                                                                                              \
        printf("[%s] " fmt "\n", module, ##__VA_ARGS__);                                           \
    } while (0)

/**
 * @brief Info log
 */
#define SMLS_INFO(module, fmt, ...)                                                                \
    do                                                                                             \
    {                                                                                              \
        printf("[INFO][%s] " fmt "\n", module, ##__VA_ARGS__);                                     \
    } while (0)

/**
 * @brief Warning log
 */
#define SMLS_WARN(module, fmt, ...)                                                                \
    do                                                                                             \
    {                                                                                              \
        printf("[WARN][%s] " fmt "\n", module, ##__VA_ARGS__);                                     \
    } while (0)

/**
 * @brief Error log
 */
#define SMLS_ERROR(module, fmt, ...)                                                               \
    do                                                                                             \
    {                                                                                              \
        printf("[ERROR][%s] " fmt "\n", module, ##__VA_ARGS__);                                    \
    } while (0)
