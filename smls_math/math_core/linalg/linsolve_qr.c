// SPDX-License-Identifier: MIT
/**
 * Copyright 2019 Daniel Mårtensson <daniel.martensson100@outlook.com>
 * Copyright 2022 Martin Schröder <info@swedishembedded.com>
 * Consulting: https://swedishembedded.com/consulting
 * Simulation: https://swedishembedded.com/simulation
 * Training: https://swedishembedded.com/training
 */

#include "smls_linalg.h"

void smls_linsolve_qr(const float* const A, float* x, const float* const b, uint16_t row,
                      uint16_t column)
{
    // QR-decomposition
    float Q[row * row];
    float R[row * column];
    float QTb[row];

    smls_mat_qr(A, Q, R, row, column, false);
    smls_mat_transpose(Q, Q, row, row);        // Do transpose Q -> Q^T
    smls_mat_mul(QTb, Q, b, row, row, row, 1); // Q^Tb = Q^T*b
    smls_linsolve_upper_triangular(R, x, QTb, column);
}
