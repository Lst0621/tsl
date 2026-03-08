#pragma once

#include <stdexcept>
#include <vector>

#include "matrix.h"
#include "polynomial.h"

class LinearRecurrence {
   public:
    explicit LinearRecurrence(const std::vector<long long>& coeffs,
                              size_t recursive_threshold = 20);

    size_t order() const;
    const std::vector<long long>& coefficients() const;
    const Matrix<long long>& transition_matrix() const;

    Polynomial<long long> characteristic_polynomial() const;

    long long evaluate_recursive(const std::vector<long long>& initial_values,
                                 size_t n) const;
    long long evaluate_matrix(const std::vector<long long>& initial_values,
                              size_t n) const;
    long long evaluate(const std::vector<long long>& initial_values,
                       size_t n) const;

   private:
    std::vector<long long> coefficients_;
    Matrix<long long> transition_matrix_;
    size_t recursive_threshold_;

    Matrix<long long> build_transition_matrix() const;
    void validate_initial_values(
        const std::vector<long long>& initial_values) const;
};
