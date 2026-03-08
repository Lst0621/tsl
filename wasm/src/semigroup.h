#pragma once

#include <algorithm>
#include <stdexcept>
#include <vector>

/**
 * Generic semigroup generator template.
 *
 * Generates all elements of a semigroup from a set of generators.
 * Assumes that operator== and operator* are already defined for type T.
 *
 * @tparam T The element type. Must support operator== and operator*.
 * @param generators Vector of initial generator elements.
 * @param limit Maximum number of elements to generate. -1 (default) means no
 * limit.
 * @param is_abelian If true, only compute i*j (skips j*i). Default is false.
 * @return Vector containing all generated semigroup elements.
 */
template <typename T>
std::vector<T> generate_semigroup(const std::vector<T>& generators,
                                  const int limit = -1,
                                  const bool is_abelian = false);

/**
 * Generic power function using binary exponentiation.
 * Implements fast exponentiation: a^(2n) = a^n * a^n
 * Useful for semigroups where multiplication is the operation.
 *
 * @tparam T The element type. Must support operator*.
 * @param base The element to raise to a power
 * @param exp The exponent (must be non-negative)
 * @param identity The identity element for multiplication (e.g., 1 for numbers)
 * @return base raised to power exp
 * @throws std::invalid_argument if exp is negative
 */
template <typename T>
T power(const T& base, unsigned long long exp, const T& identity) {
    if (exp == 0) {
        return identity;
    }

    T result = identity;
    T current = base;

    // Binary exponentiation
    while (exp > 0) {
        if (exp % 2 == 1) {
            result = result * current;
        }
        current = current * current;
        exp /= 2;
    }

    return result;
}

/**
 * Generate all elements of a semigroup from a set of generators.
 * Assumes that operator== and operator* are already defined for type T.
 *
 * @tparam T The element type. Must support operator== and operator*.
 * @param generators Vector of initial generator elements.
 * @param limit Maximum number of elements to generate. -1 (default) means no
 * limit.
 * @param is_abelian If true, only compute i*j (skips j*i). Default is false.
 * @return Vector containing all generated semigroup elements.
 */
template <typename T>
std::vector<T> generate_semigroup(const std::vector<T>& generators,
                                  const int limit, const bool is_abelian) {
    std::vector<T> ret(generators.begin(), generators.end());
    size_t last_length = 0;
    size_t current_length = ret.size();

    while (last_length < current_length) {
        // Multiply existing elements with newly generated elements (forward
        // order)
        for (size_t i = 0; i < current_length; i++) {
            for (size_t j = last_length; j < current_length; j++) {
                T product_ij = ret[i] * ret[j];

                // Check if product already exists
                if (std::find(ret.begin(), ret.end(), product_ij) ==
                    ret.end()) {
                    ret.push_back(product_ij);
                }
            }
        }

        // Multiply newly generated elements with existing elements (reverse
        // order) Skip this if the semigroup is abelian (commutative)
        if (!is_abelian) {
            for (size_t i = 0; i < current_length; i++) {
                for (size_t j = last_length; j < current_length; j++) {
                    T product_ji = ret[j] * ret[i];

                    // Check if product already exists
                    if (std::find(ret.begin(), ret.end(), product_ji) ==
                        ret.end()) {
                        ret.push_back(product_ji);
                    }
                }
            }
        }

        last_length = current_length;
        current_length = ret.size();

        // Check if limit exceeded
        if (limit > 0 && current_length > static_cast<size_t>(limit)) {
            break;
        }
    }

    return ret;
}

/**
 * Check if a set of generators is closed under multiplication.
 *
 * A set is closed if the semigroup generated from it has the same size as the
 * generators. (i.e., no new elements are created by multiplication)
 *
 * @tparam T The element type. Must support operator== and operator*.
 * @param generators Vector of generator elements to check.
 * @param is_abelian If true, only compute i*j (skips j*i). Default is false.
 * @return True if the generators form a closed set, false otherwise.
 */
template <typename T>
bool is_closure(const std::vector<T>& generators,
                const bool is_abelian = false) {
    // Set limit to generators.size() + 1 to avoid generating the whole
    // semigroup We only need to check if any new elements are created
    const int limit = static_cast<int>(generators.size()) + 1;
    std::vector<T> closure = generate_semigroup(generators, limit, is_abelian);

    // If closure size equals generators size, no new elements were created
    // (closed)
    return closure.size() == generators.size();
}
