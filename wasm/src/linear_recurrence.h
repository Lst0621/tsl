#pragma once

#include <stdexcept>
#include <vector>

#include "matrix.h"
#include "polynomial.h"
#include "rational.h"
#include "semigroup.h"

/**
 * Generic linear recurrence over a coefficient field T.
 *
 * Represents f(n) = c1*f(n-1) + c2*f(n-2) + ... + ck*f(n-k).
 * Supports recursive evaluation (O(k*n)) and matrix-exponentiation evaluation
 * (O(k^3 * log n)).
 *
 * @tparam T  Coefficient type. Must support +, -, *, /, ==, T(0), T(1).
 *            Examples: tsl::Rational<long long>, tsl::QI.
 */
template <typename T>
class LinearRecurrenceT {
   public:
    using Coeff = T;

    explicit LinearRecurrenceT(const std::vector<T>& coeffs,
                               size_t recursive_threshold = 20)
        : coefficients_(coeffs),
          transition_matrix_(1, 1, T(0)),
          recursive_threshold_(recursive_threshold) {
        if (coefficients_.empty()) {
            throw std::invalid_argument(
                "Recurrence coefficients cannot be empty");
        }
        if (recursive_threshold_ == 0) {
            throw std::invalid_argument(
                "Recursive threshold must be positive");
        }
        transition_matrix_ = build_transition_matrix();
    }

    size_t order() const {
        return coefficients_.size();
    }

    const std::vector<T>& coefficients() const {
        return coefficients_;
    }

    const Matrix<T>& transition_matrix() const {
        return transition_matrix_;
    }

    Polynomial<T> characteristic_polynomial() const {
        // p(x) = x^k - c1*x^(k-1) - ... - ck
        const size_t k = order();
        std::vector<T> coeffs(k + 1, T(0));
        coeffs[k] = T(1);
        for (size_t i = 0; i < k; ++i) {
            coeffs[k - 1 - i] = -coefficients_[i];
        }
        return Polynomial<T>(coeffs);
    }

    T evaluate_recursive(const std::vector<T>& initial_values,
                         size_t n) const {
        validate_initial_values(initial_values);

        if (n < order()) {
            return initial_values[n];
        }

        std::vector<T> values = initial_values;
        values.reserve(n + 1);

        for (size_t idx = order(); idx <= n; ++idx) {
            T next(0);
            for (size_t j = 0; j < order(); ++j) {
                next += coefficients_[j] * values[idx - 1 - j];
            }
            values.push_back(next);
        }

        return values[n];
    }

    T evaluate_matrix(const std::vector<T>& initial_values, size_t n) const {
        validate_initial_values(initial_values);

        if (n < order()) {
            return initial_values[n];
        }

        const size_t k = order();

        std::vector<std::vector<T>> identity_data(
            k, std::vector<T>(k, T(0)));
        for (size_t i = 0; i < k; ++i) {
            identity_data[i][i] = T(1);
        }
        const Matrix<T> identity(identity_data);

        const Matrix<T> transition_pow = monoid_power(
            transition_matrix_,
            static_cast<unsigned long long>(n - (k - 1)),
            identity);

        std::vector<std::vector<T>> state_data(k, std::vector<T>(1));
        for (size_t i = 0; i < k; ++i) {
            state_data[i][0] = initial_values[k - 1 - i];
        }
        const Matrix<T> state(state_data);

        const Matrix<T> next_state = transition_pow * state;
        return next_state(0, 0);
    }

    T evaluate(const std::vector<T>& initial_values, size_t n) const {
        if (n < recursive_threshold_) {
            return evaluate_recursive(initial_values, n);
        }
        return evaluate_matrix(initial_values, n);
    }

   private:
    std::vector<T> coefficients_;
    Matrix<T> transition_matrix_;
    size_t recursive_threshold_;

    Matrix<T> build_transition_matrix() const {
        const size_t k = order();
        std::vector<std::vector<T>> data(k, std::vector<T>(k, T(0)));

        for (size_t j = 0; j < k; ++j) {
            data[0][j] = coefficients_[j];
        }
        for (size_t i = 1; i < k; ++i) {
            data[i][i - 1] = T(1);
        }

        return Matrix<T>(data);
    }

    void validate_initial_values(const std::vector<T>& initial_values) const {
        if (initial_values.size() != order()) {
            throw std::invalid_argument(
                "Initial values size must match recurrence order");
        }
    }
};

// Backward-compatible alias: the original LinearRecurrence over rational coefficients.
using LinearRecurrence = LinearRecurrenceT<tsl::Rational<long long>>;
