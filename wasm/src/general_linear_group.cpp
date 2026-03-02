#include "general_linear_group.h"

#include <iostream>

/**
 * Helper function to generate all possible values for n² matrix positions
 * Uses a recursive approach to fill the matrix with all combinations
 */
static void fill_matrices_recursive(
    std::vector<Matrix<ModularNumber>>& result,
    std::vector<std::vector<ModularNumber>>& current_data,
    size_t n,
    size_t m,
    size_t position) {

    if (position == n * n) {
        // Create matrix from filled data and check determinant
        Matrix<ModularNumber> mat(current_data);

        // Check if determinant is non-zero
        try {
            ModularNumber det = mat.determinant();
            if (det.get_value() != 0) {
                result.push_back(mat);
            }
        } catch (...) {
            // Skip matrices that cause issues in determinant calculation
        }
        return;
    }

    // Calculate which row and column this position corresponds to
    size_t row = position / n;
    size_t col = position % n;

    // Try all values 0 to m-1 at this position
    for (long long val = 0; val < static_cast<long long>(m); val++) {
        current_data[row][col] = ModularNumber(val, m);
        fill_matrices_recursive(result, current_data, n, m, position + 1);
    }
}

/**
 * Generate all possible n×n matrices with elements in Z_m
 */
std::vector<Matrix<ModularNumber>> generate_all_matrices(size_t n, size_t m) {
    if (n == 0 || m == 0) {
        throw std::invalid_argument("Matrix size and modulus must be positive");
    }

    std::vector<Matrix<ModularNumber>> result;

    // Create default value: ModularNumber(0, m)
    ModularNumber default_value(0, m);

    // Initialize a matrix with default values
    std::vector<std::vector<ModularNumber>> current_data(
        n, std::vector<ModularNumber>(n, default_value));

    // Generate all combinations recursively
    fill_matrices_recursive(result, current_data, n, m, 0);

    return result;
}

/**
 * Get all members of the general linear group GL(n, m)
 * Filters matrices with non-zero determinant
 */
std::vector<Matrix<ModularNumber>> get_gl_n_zm(size_t n, size_t m) {
    if (n == 0 || m == 0) {
        throw std::invalid_argument("Matrix size and modulus must be positive");
    }

    // This calls generate_all_matrices which already filters by non-zero determinant
    return generate_all_matrices(n, m);
}

/**
 * Get the size of the general linear group GL(n, m)
 */
size_t get_gl_n_zm_size(size_t n, size_t m) {
    if (n == 0 || m == 0) {
        throw std::invalid_argument("Matrix size and modulus must be positive");
    }

    // Generate all matrices and return their count
    std::vector<Matrix<ModularNumber>> gl_matrices = generate_all_matrices(n, m);
    std::cout<< "Generated " << gl_matrices.size() << " invertible matrices in GL(" << n
             << ", " << m << ")\n";
    return gl_matrices.size();
}


