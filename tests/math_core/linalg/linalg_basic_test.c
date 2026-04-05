#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "smls_linalg.h"

#define TEST_ASSERT(cond)                                                                          \
    do                                                                                             \
    {                                                                                              \
        if (!(cond))                                                                               \
        {                                                                                          \
            printf("[FAIL] %s:%d\n", __FILE__, __LINE__);                                          \
            return -1;                                                                             \
        }                                                                                          \
    } while (0)

#define FLOAT_EQ(a, b) (fabsf((a) - (b)) < 1e-6f)

#define TEST_ASSERT_FLOAT_WITHIN(delta, expected, actual)                                          \
    do                                                                                             \
    {                                                                                              \
        float _exp  = (float)(expected);                                                           \
        float _act  = (float)(actual);                                                             \
        float _diff = fabsf(_exp - _act);                                                          \
                                                                                                   \
        if (_diff > (delta))                                                                       \
        {                                                                                          \
            printf("[FAIL] Float assert failed\n");                                                \
            printf("  expected: %.6f\n", _exp);                                                    \
            printf("  actual  : %.6f\n", _act);                                                    \
            printf("  diff    : %.6f > %.6f\n", _diff, (float)(delta));                            \
            return -1;                                                                             \
        }                                                                                          \
    } while (0)

static int test_mat_add(void)
{
    float A[4] = {1, 2, 3, 4};

    float B[4] = {5, 6, 7, 8};

    float C[4] = {0};

    float expected[4] = {6, 8, 10, 12};

    smls_mat_add(C, A, B, 2, 2);

    for (int i = 0; i < 4; i++)
    {
        TEST_ASSERT(FLOAT_EQ(C[i], expected[i]));
    }

    return 0;
}

static int test_mat_inv(void)
{
    float A[4] = {4, 7, 2, 6};

    float A_inv[4] = {0};

    float expected[4] = {0.6f, -0.7f, -0.2f, 0.4f};

    int ret = smls_mat_inv(A_inv, A, 2);
    TEST_ASSERT(ret == 0);

    for (int i = 0; i < 4; i++)
    {
        TEST_ASSERT(FLOAT_EQ(A_inv[i], expected[i]));
    }

    return 0;
}

static int test_mat_transpose(void)
{
    float A[6] = {1, 2, 3, 4, 5, 6};

    float At[6] = {0};

    float expected[6] = {1, 4, 2, 5, 3, 6};

    smls_mat_transpose(At, A, 2, 3);

    for (int i = 0; i < 6; i++)
    {
        TEST_ASSERT(FLOAT_EQ(At[i], expected[i]));
    }

    return 0;
}

static int test_mat_mul(void)
{
    float A[6] = {1, 2, 3, 4, 5, 6};

    float B[6] = {7, 8, 9, 10, 11, 12};

    float C[4] = {0};

    float expected[4] = {58, 64, 139, 154};

    int ret = smls_mat_mul(C, A, B, 2, 3, 3, 2);
    TEST_ASSERT(ret == 0);

    for (int i = 0; i < 4; i++)
    {
        TEST_ASSERT(FLOAT_EQ(C[i], expected[i]));
    }

    return 0;
}

static int test_mat_qr(void)
{
    float A[6] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};

    float Q[9]   = {0}; /* 3x3 */
    float R[6]   = {0}; /* 3x2 */
    float QR[6]  = {0}; /* 3x2 */
    float QT[9]  = {0}; /* 3x3 */
    float QTQ[9] = {0}; /* 3x3 */

    int ret = smls_mat_qr(A, Q, R, 3, 2, false);
    TEST_ASSERT(ret == 0);

    ret = smls_mat_mul(QR, Q, R, 3, 3, 3, 2);
    TEST_ASSERT(ret == 0);

    for (int i = 0; i < 6; i++)
    {
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, A[i], QR[i]);
    }

    smls_mat_transpose(QT, Q, 3, 3);

    ret = smls_mat_mul(QTQ, QT, Q, 3, 3, 3, 3);
    TEST_ASSERT(ret == 0);

    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 1.0f, QTQ[0]);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, QTQ[1]);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, QTQ[2]);

    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, QTQ[3]);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 1.0f, QTQ[4]);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, QTQ[5]);

    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, QTQ[6]);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, QTQ[7]);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 1.0f, QTQ[8]);

    return 0;
}

static int test_mat_lup(void)
{
    const uint16_t row = 3;

    float A[9] = {0.0f, 2.0f, 1.0f, 1.0f, 1.0f, 0.0f, 2.0f, 1.0f, 1.0f};

    float LU[9]  = {0};
    uint8_t P[3] = {0};

    int ret = smls_mat_lup(A, LU, P, row);
    TEST_ASSERT(ret == 0);

    /* Verify permutation indices are valid */
    for (uint16_t i = 0; i < row; i++)
    {
        TEST_ASSERT(P[i] < row);
    }

    return 0;
}

static int test_mat_det(void)
{
    float A[9] = {4, 2, 1, 0, 3, -1, 0, 0, 2};

    float det_A = smls_mat_det(A, 3);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 24.0f, det_A);

    return 0;
}

static int test_mat_chol(void)
{
    const uint16_t row = 2;

    float A[4] = {4.0f, 2.0f, 2.0f, 3.0f};

    float L[4]   = {0};
    float LT[4]  = {0};
    float LLT[4] = {0};

    /* Compute Cholesky decomposition */
    smls_mat_chol(A, L, row);

    /* Verify lower triangular structure */
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, L[1]);

    /* Verify A ≈ L * L^T */
    smls_mat_transpose(LT, L, row, row);

    int ret = smls_mat_mul(LLT, L, LT, row, row, row, row);
    TEST_ASSERT(ret == 0);

    for (uint16_t i = 0; i < row * row; i++)
    {
        TEST_ASSERT_FLOAT_WITHIN(1e-5f, A[i], LLT[i]);
    }

    return 0;
}

static int test_mat_chol_update(void)
{
    const uint16_t row = 2;

    float A[4] = {4.0f, 2.0f, 2.0f, 3.0f};

    float x[2] = {1.0f, 2.0f};

    float L[4]   = {0};
    float LT[4]  = {0};
    float LLT[4] = {0};

    float expected[4] = {5.0f, 4.0f, 4.0f, 7.0f};

    /* Initial Cholesky */
    smls_mat_chol(A, L, row);

    /* Rank-1 update */
    smls_mat_chol_update(L, x, row, true);

    /* Reconstruct updated matrix */
    smls_mat_transpose(LT, L, row, row);

    smls_mat_mul(LLT, L, LT, row, row, row, row);

    for (uint16_t i = 0; i < row * row; i++)
    {
        TEST_ASSERT_FLOAT_WITHIN(1e-5f, expected[i], LLT[i]);
    }

    return 0;
}

static int test_mat_pinv(void)
{
    const uint16_t row    = 3;
    const uint16_t column = 2;

    /*
     * A =
     * [1 2]
     * [3 4]
     * [5 6]
     */
    float A[6] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};

    /*
     * Expected pseudo inverse:
     * [-1.333333  -0.333333   0.666667]
     * [ 1.083333   0.333333  -0.416667]
     */
    float expected[6] = {-1.333333f, -0.333333f, 0.666667f, 1.083333f, 0.333333f, -0.416667f};

    float Ai[6] = {0};

    /*
     * Verify A⁺
     * size = column x row = 2 x 3
     */
    smls_mat_pinv(Ai, A, row, column);

    for (uint16_t i = 0; i < 6; i++)
    {
        TEST_ASSERT_FLOAT_WITHIN(1e-4f, expected[i], Ai[i]);
    }

    /*
     * Moore-Penrose property:
     * A * A⁺ * A ≈ A
     */
    float tmp1[9] = {0}; /* 3x3 */
    float tmp2[6] = {0}; /* 3x2 */

    int ret = smls_mat_mul(tmp1, A, Ai, row, column, column, row);

    TEST_ASSERT(ret == 0);

    ret = smls_mat_mul(tmp2, tmp1, A, row, row, row, column);

    TEST_ASSERT(ret == 0);

    for (uint16_t i = 0; i < 6; i++)
    {
        TEST_ASSERT_FLOAT_WITHIN(1e-4f, A[i], tmp2[i]);
    }

    return 0;
}

static int test_mat_hankel(void)
{
    /*
     * V = [1 2 3 4 5]
     *
     * Expected H (5x5):
     * [1 2 3 4 5
     *  2 3 4 5 0
     *  3 4 5 0 0
     *  4 5 0 0 0
     *  5 0 0 0 0]
     */
    float V[5] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f};

    float H[25] = {0};

    float expected[25] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 2.0f, 3.0f, 4.0f, 5.0f,
                          0.0f, 3.0f, 4.0f, 5.0f, 0.0f, 0.0f, 4.0f, 5.0f, 0.0f,
                          0.0f, 0.0f, 5.0f, 0.0f, 0.0f, 0.0f, 0.0f};

    int ret = smls_mat_hankel(V, H, 1, /* row_v */
                              5,       /* column_v */
                              5,       /* row_h */
                              5,       /* column_h */
                              0);

    TEST_ASSERT(ret == 0);

    for (int i = 0; i < 25; i++)
    {
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, expected[i], H[i]);
    }

    return 0;
}

static int test_mat_balance(void)
{
    const uint16_t row = 3;

    /*
     * Deliberately badly scaled matrix
     */
    float A[9] = {1.0f, 1000.0f, 0.0f, 0.001f, 2.0f, 1000.0f, 0.0f, 0.001f, 3.0f};

    float A_before[9];
    memcpy(A_before, A, sizeof(A));

    smls_mat_balance(A, row);

    /*
     * Verify matrix changed
     */
    bool changed = false;

    for (uint16_t i = 0; i < 9; i++)
    {
        if (fabsf(A[i] - A_before[i]) > 1e-6f)
        {
            changed = true;
            break;
        }
    }

    TEST_ASSERT(changed);

    return 0;
}

static int test_mat_eig(void)
{
    const uint16_t row = 3;

    /*
     * Upper triangular matrix
     * Eigenvalues = diagonal
     */
    float A[9] = {1.0f, 2.0f, 3.0f, 0.0f, 4.0f, 5.0f, 0.0f, 0.0f, 6.0f};

    float wr[3] = {0};
    float wi[3] = {0};

    smls_mat_eig(A, wr, wi, row);

    /*
     * Sort order may vary
     */
    bool found1 = false;
    bool found4 = false;
    bool found6 = false;

    for (uint16_t i = 0; i < row; i++)
    {
        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, wi[i]);

        if (fabsf(wr[i] - 1.0f) < 1e-5f)
            found1 = true;

        if (fabsf(wr[i] - 4.0f) < 1e-5f)
            found4 = true;

        if (fabsf(wr[i] - 6.0f) < 1e-5f)
            found6 = true;
    }

    TEST_ASSERT(found1);
    TEST_ASSERT(found4);
    TEST_ASSERT(found6);

    return 0;
}

static int test_mat_eig_sym(void)
{
    const uint16_t row = 2;

    float A[4] = {2.0f, 1.0f, 1.0f, 2.0f};

    float ev[4] = {0};
    float d[2]  = {0};

    smls_mat_eig_sym(A, ev, d, row);

    bool found1 = false;
    bool found3 = false;

    for (uint16_t i = 0; i < row; i++)
    {
        if (fabsf(d[i] - 1.0f) < 1e-5f)
            found1 = true;

        if (fabsf(d[i] - 3.0f) < 1e-5f)
            found3 = true;
    }

    TEST_ASSERT(found1);
    TEST_ASSERT(found3);

    /*
     * Check orthogonality:
     * V^T V = I
     */
    float ev_t[4] = {0};
    float eye[4]  = {0};

    smls_mat_transpose(ev_t, ev, row, row);

    int ret = smls_mat_mul(eye, ev_t, ev, row, row, row, row);

    TEST_ASSERT(ret == 0);

    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 1.0f, eye[0]);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 0.0f, eye[1]);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 0.0f, eye[2]);
    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 1.0f, eye[3]);

    return 0;
}

static int test_mat_sum_row(void)
{
    float A[6] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};

    float Ar[6] = {0};

    smls_mat_sum(Ar, A, 2, 3, 1);

    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 5.0f, Ar[0]);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 7.0f, Ar[1]);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 9.0f, Ar[2]);

    return 0;
}

static int test_mat_sum_column(void)
{
    float A[6] = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};

    float Ar[6] = {0};

    smls_mat_sum(Ar, A, 2, 3, 2);

    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 6.0f, Ar[0]);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 15.0f, Ar[3]);

    return 0;
}

static int test_mat_norm_l1(void)
{
    float A[4] = {1.0f, 2.0f, 3.0f, 4.0f};

    float n = smls_mat_norm(A, 2, 2, 1);

    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 6.0f, n);

    return 0;
}

static int test_mat_norm_l2(void)
{
    float A[4] = {1.0f, 2.0f, 3.0f, 4.0f};

    float n = smls_mat_norm(A, 2, 2, 2);

    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 5.4649857f, n);

    return 0;
}

static int test_mat_expm(void)
{
    /*
     * --------------------------------
     * Case 1: Zero matrix
     * exp(0) = I
     * --------------------------------
     */
    {
        float A[4] = {0.0f, 0.0f, 0.0f, 0.0f};

        float E[4] = {0};

        float expected[4] = {1.0f, 0.0f, 0.0f, 1.0f};

        smls_mat_expm(A, E, 2);

        for (int i = 0; i < 4; i++)
        {
            TEST_ASSERT_FLOAT_WITHIN(1e-6f, expected[i], E[i]);
        }
    }

    /*
     * --------------------------------
     * Case 2: Diagonal matrix
     * exp(diag(1,2))
     * --------------------------------
     */
    {
        float A[4] = {1.0f, 0.0f, 0.0f, 2.0f};

        float E[4] = {0};

        smls_mat_expm(A, E, 2);

        TEST_ASSERT_FLOAT_WITHIN(1e-5f, expf(1.0f), E[0]);

        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, E[1]);

        TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.0f, E[2]);

        TEST_ASSERT_FLOAT_WITHIN(1e-5f, expf(2.0f), E[3]);
    }

    /*
     * --------------------------------
     * Case 3: Nilpotent matrix
     * exp(A) = I + A
     * --------------------------------
     */
    {
        float A[4] = {0.0f, 1.0f, 0.0f, 0.0f};

        float E[4] = {0};

        float expected[4] = {1.0f, 1.0f, 0.0f, 1.0f};

        smls_mat_expm(A, E, 2);

        for (int i = 0; i < 4; i++)
        {
            TEST_ASSERT_FLOAT_WITHIN(1e-6f, expected[i], E[i]);
        }
    }

    return 0;
}

static int test_linsolve_qr(void)
{
    const uint16_t row    = 3;
    const uint16_t column = 3;

    float A[9] = {1.0f, 1.0f, 1.0f, 2.0f, 3.0f, 1.0f, 1.0f, -1.0f, 2.0f};

    float b[3] = {6.0f, 11.0f, 5.0f};

    float x[3]  = {0};
    float Ax[3] = {0};

    /* Solve Ax = b */
    smls_linsolve_qr(A, x, b, row, column);

    /* Verify A * x ≈ b */
    int ret = smls_mat_mul(Ax, A, x, row, column, column, 1);
    TEST_ASSERT(ret == 0);

    for (uint16_t i = 0; i < row; i++)
    {
        TEST_ASSERT_FLOAT_WITHIN(1e-5f, b[i], Ax[i]);
    }

    return 0;
}

static int test_linsolve_lower_triangular(void)
{
    float A[9] = {1.0f, 0.0f, 0.0f, 2.0f, 1.0f, 0.0f, 3.0f, 4.0f, 1.0f};

    float b[3] = {1.0f, 4.0f, 14.0f};

    float x[3]  = {0};
    float Ax[3] = {0};

    smls_linsolve_lower_triangular(A, x, b, 3);

    int ret = smls_mat_mul(Ax, A, x, 3, 3, 3, 1);
    TEST_ASSERT(ret == 0);

    for (int i = 0; i < 3; i++)
    {
        TEST_ASSERT_FLOAT_WITHIN(1e-5f, b[i], Ax[i]);
    }

    return 0;
}

static int test_linsolve_lup(void)
{
    const uint16_t row = 3;

    float A[9] = {1.0f, 1.0f, 1.0f, 2.0f, 3.0f, 1.0f, 1.0f, -1.0f, 2.0f};

    float b[3] = {6.0f, 11.0f, 5.0f};

    float x[3]  = {0};
    float Ax[3] = {0};

    int ret = smls_linsolve_lup(A, x, b, row);
    TEST_ASSERT(ret == 0);

    /* Check expected solution */
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 1.0f, x[0]);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 2.0f, x[1]);
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 3.0f, x[2]);

    /* Verify A * x ≈ b */
    ret = smls_mat_mul(Ax, A, x, row, row, row, 1);
    TEST_ASSERT(ret == 0);

    for (uint16_t i = 0; i < row; i++)
    {
        TEST_ASSERT_FLOAT_WITHIN(1e-5f, b[i], Ax[i]);
    }

    return 0;
}

static int test_linsolve_chol(void)
{
    const uint16_t row = 3;

    /*
     * SPD matrix:
     * [ 4  1  1 ]
     * [ 1  3  0 ]
     * [ 1  0  2 ]
     */
    float A[9] = {4.0f, 1.0f, 1.0f, 1.0f, 3.0f, 0.0f, 1.0f, 0.0f, 2.0f};

    /*
     * Expected solution:
     * x = [1, 2, 3]
     */
    float expected_x[3] = {1.0f, 2.0f, 3.0f};

    /*
     * b = A * x
     */
    float b[3] = {9.0f, 7.0f, 7.0f};

    float x[3]  = {0};
    float Ax[3] = {0};

    /* Solve Ax = b */
    smls_linsolve_chol(A, x, b, row);

    /* Verify x */
    for (uint16_t i = 0; i < row; i++)
    {
        TEST_ASSERT_FLOAT_WITHIN(1e-5f, expected_x[i], x[i]);
    }

    /* Verify A*x ≈ b */
    int ret = smls_mat_mul(Ax, A, x, row, row, row, 1);

    TEST_ASSERT(ret == 0);

    for (uint16_t i = 0; i < row; i++)
    {
        TEST_ASSERT_FLOAT_WITHIN(1e-5f, b[i], Ax[i]);
    }

    return 0;
}

static int test_linsolve_gauss(void)
{
    float A[4] = {2.0f, 1.0f, 1.0f, 3.0f};

    float b[2] = {4.0f, 7.0f};

    float x[2] = {0};

    smls_linsolve_gauss(A, x, b, 2, 2, 0.0f);

    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 1.0f, x[0]);

    TEST_ASSERT_FLOAT_WITHIN(1e-5f, 2.0f, x[1]);

    return 0;
}

static void nonlinear_test_func(float dx[], float b[], float x[])
{
    dx[0] = x[0] * x[0] - b[0];
}

static int test_nonlinsolve(void)
{
    float b[1] = {4.0f};

    float x[1] = {1.0f};

    smls_nonlinsolve(nonlinear_test_func, b, x, 1, 0.01f, 10.0f, -10.0f, false);

    /*
     * Accept either +2 or -2
     */
    TEST_ASSERT(fabsf(fabsf(x[0]) - 2.0f) < 1e-2f);

    return 0;
}

int main(void)
{
    if (test_mat_add() != 0)
        return -1;

    printf("[PASS] test_mat_add\n");

    if (test_mat_inv() != 0)
        return -1;

    printf("[PASS] test_mat_inv\n");

    if (test_mat_transpose() != 0)
        return -1;

    printf("[PASS] test_mat_transpose\n");

    if (test_mat_mul() != 0)
        return -1;

    printf("[PASS] test_mat_mul\n");

    if (test_mat_qr() != 0)
        return -1;

    printf("[PASS] test_mat_qr\n");

    if (test_mat_lup() != 0)
        return -1;

    printf("[PASS] test_mat_lup\n");

    if (test_mat_det() != 0)
        return -1;

    printf("[PASS] test_mat_det\n");

    if (test_mat_chol() != 0)
        return -1;

    printf("[PASS] test_mat_chol\n");

    if (test_mat_chol_update() != 0)
        return -1;

    printf("[PASS] test_mat_chol_update\n");

    if (test_mat_pinv() != 0)
        return -1;

    printf("[PASS] test_mat_pinv\n");

    if (test_mat_hankel() != 0)
        return -1;

    printf("[PASS] test_mat_hankel\n");

    if (test_mat_balance() != 0)
        return -1;

    printf("[PASS] test_mat_balance\n");

    if (test_mat_eig() != 0)
        return -1;

    printf("[PASS] test_mat_eig\n");

    if (test_mat_eig_sym() != 0)
        return -1;

    printf("[PASS] test_mat_eig_sym\n");

    if (test_mat_sum_row() != 0)
        return -1;

    printf("[PASS] test_mat_sum_row\n");

    if (test_mat_sum_column() != 0)
        return -1;

    printf("[PASS] test_mat_sum_column\n");

    if (test_mat_norm_l1() != 0)
        return -1;

    printf("[PASS] test_mat_norm_l1\n");

    if (test_mat_norm_l2() != 0)
        return -1;

    printf("[PASS] test_mat_norm_l2\n");

    if (test_mat_expm() != 0)
        return -1;

    printf("[PASS] test_mat_expm\n");

    if (test_linsolve_qr() != 0)
        return -1;

    printf("[PASS] test_linsolve_qr\n");

    if (test_linsolve_lower_triangular() != 0)
        return -1;

    printf("[PASS] test_linsolve_lower_triangular\n");

    if (test_linsolve_lup() != 0)
        return -1;

    printf("[PASS] test_linsolve_lup\n");

    if (test_linsolve_chol() != 0)
        return -1;

    printf("[PASS] test_linsolve_chol\n");

    if (test_linsolve_gauss() != 0)
        return -1;

    printf("[PASS] test_linsolve_gauss\n");

    if (test_nonlinsolve() != 0)
        return -1;

    printf("[PASS] test_nonlinsolve\n");

    return 0;
}
