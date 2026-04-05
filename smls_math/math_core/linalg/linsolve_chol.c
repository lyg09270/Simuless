// SPDX-License-Identifier: MIT
/**
 * Copyright 2019 Daniel Mårtensson <daniel.martensson100@outlook.com>
 * Copyright 2022 Martin Schröder <info@swedishembedded.com>
 * Consulting: https://swedishembedded.com/consulting
 * Simulation: https://swedishembedded.com/simulation
 * Training: https://swedishembedded.com/training
 */

#include "smls_linalg.h"

void smls_linsolve_chol(const float* const A, float* x, const float* const b, uint16_t row)
{
    float L[row * row];
    float y[row];

    smls_mat_chol(A, L, row);
    smls_linsolve_lower_triangular(L, y, b, row);
    smls_mat_transpose(L, L, row, row);
    smls_linsolve_upper_triangular(L, x, y, row);
}
