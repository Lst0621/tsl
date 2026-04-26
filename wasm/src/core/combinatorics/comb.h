#pragma once
#include <numeric>
#include <unordered_map>
#include <vector>

#include "util/helper.h"
int number_of_sequences(const std::vector<int>& arr,
                        const std::vector<int>& sums);

std::unordered_map<std::vector<int>, int, vector_hash<int>>
number_of_sequences_all(const std::vector<int>& arr,
                        const std::vector<int>& sums);

/**
 * Compute n choose k (binomial coefficient).
 * Optimized to avoid overflow by:
 * 1. Using symmetry: n choose k = n choose (n-k), pick the smaller one
 * 2. Alternating multiplication and division with GCD to keep numbers small
 *
 * Can be evaluated at compile time (constexpr).
 *
 * Note: Invalid inputs (n < 0, k < 0, k > n) have undefined behavior
 * in constexpr context. Ensure valid inputs when using at compile time.
 *
 * @param n Total number of items
 * @param k Number of items to choose
 * @return The binomial coefficient C(n, k)
 */
constexpr long long n_choose_k(int n, int k) {
    // Use symmetry: n choose k = n choose (n-k), pick the smaller one
    if (k > n - k) {
        k = n - k;
    }

    if (k == 0) {
        return 1;
    }

    long long result = 1;

    // Compute result = n * (n-1) * ... * (n-k+1) / (k * (k-1) * ... * 1)
    // Optimize by applying GCD twice before multiplication to avoid overflow
    for (int i = 1; i <= k; i++) {
        long long numerator = n - i + 1;
        long long denominator = i;

        // First GCD: reduce numerator and denominator
        long long g1 = std::gcd(numerator, denominator);
        numerator /= g1;
        denominator /= g1;

        // Second GCD: reduce result and denominator
        long long g2 = std::gcd(result, denominator);
        result /= g2;
        denominator /= g2;

        // Now multiply and divide with reduced values
        result *= numerator;
        if (denominator != 1) {
            result /= denominator;
        }
    }

    return result;
}
