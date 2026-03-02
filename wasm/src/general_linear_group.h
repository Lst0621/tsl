#pragma once

#include <vector>

#include "matrix.h"
#include "modular_number.h"

/**
 * General Linear Group GL(n, m) - All invertible n×n matrices over Z_m
 *
 * An n×n matrix is invertible if and only if its determinant is non-zero (mod
 * m). For prime modulus m, this includes all matrices with non-zero
 * determinant.
 */

/**
 * Get all members of the general linear group GL(n, m)
 *
 * Generates all n×n matrices with elements in Z_m (0 to m-1)
 * and filters to keep only those with non-zero determinant.
 *
 * @param n Size of the matrix (n×n)
 * @param m Modulus for elements (elements will be 0 to m-1)
 * @return Vector of all invertible matrices in GL(n, m)
 *
 * Note: This function can be expensive for large n and m
 * as it generates m^(n²) total matrices and checks determinants.
 */
std::vector<Matrix<ModularNumber>> get_gl_n_zm(size_t n, size_t m);

/**
 * Get the size (number of elements) of the general linear group GL(n, m)
 *
 * Returns the count of all invertible n×n matrices with elements in Z_m.
 * This is more efficient than get_gl_n_zm as it doesn't need to store all
 * matrices.
 *
 * @param n Size of the matrix (n×n)
 * @param m Modulus for elements
 * @return Size of GL(n, m) - number of invertible matrices
 */
size_t get_gl_n_zm_size(size_t n, size_t m);

/**
 * Generate all possible n×n matrices with elements in Z_m
 * This is a helper function used internally by get_gl_n_zm
 *
 * @param n Size of the matrix
 * @param m Modulus for elements
 * @return Vector of all n×n matrices with elements in [0, m)
 */
std::vector<Matrix<ModularNumber>> generate_all_matrices(size_t n, size_t m);
