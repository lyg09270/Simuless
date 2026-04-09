#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

    /* =========================================================
     * Basic arithmetic
     * ========================================================= */

#define SMLS_ADD(a, b) ((a) + (b))

#define SMLS_SUB(a, b) ((a) - (b))

#define SMLS_MUL(a, b) ((a) * (b))

#define SMLS_DIV(a, b) ((a) / (b))

#define SMLS_NEG(x) (-(x))

#define SMLS_ABS(x) (((x) >= 0) ? (x) : -(x))

    /* =========================================================
     * Comparison
     * ========================================================= */

#define SMLS_GT(a, b) ((a) > (b))

#define SMLS_LT(a, b) ((a) < (b))

#define SMLS_EQ(a, b) ((a) == (b))

#define SMLS_NEAR(a, b, eps) (SMLS_ABS((a) - (b)) < (eps))

#define SMLS_MIN(a, b) (((a) < (b)) ? (a) : (b))

#define SMLS_MAX(a, b) (((a) > (b)) ? (a) : (b))

    /* =========================================================
     * Control semantics
     * ========================================================= */

#define SMLS_GAIN(x, k) ((x) * (k))

#define SMLS_ERR(ref, fb) ((ref) - (fb))

#define SMLS_SIGN(x) (((x) > 0) ? 1.0f : (((x) < 0) ? -1.0f : 0.0f))

    /* =========================================================
     * Limiter / saturation
     * ========================================================= */

#define SMLS_CLAMP(x, lo, hi) (((x) < (lo)) ? (lo) : (((x) > (hi)) ? (hi) : (x)))

#define SMLS_SAT(x, limit) SMLS_CLAMP((x), -(limit), (limit))

#ifdef __cplusplus
}
#endif
