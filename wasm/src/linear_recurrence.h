#pragma once

#include <stdexcept>
#include <vector>

#include "matrix.h"
#include "polynomial.h"
#include "rational.h"

class LinearRecurrence {
   public:
    using Coeff = tsl::Rational<long long>;

    explicit LinearRecurrence(const std::vector<Coeff>& coeffs,
                              size_t recursive_threshold = 20);

    size_t order() const;
    const std::vector<Coeff>& coefficients() const;
    const Matrix<Coeff>& transition_matrix() const;

    Polynomial<Coeff> characteristic_polynomial() const;

    Coeff evaluate_recursive(const std::vector<Coeff>& initial_values,
                             size_t n) const;
    Coeff evaluate_matrix(const std::vector<Coeff>& initial_values,
                          size_t n) const;
    Coeff evaluate(const std::vector<Coeff>& initial_values, size_t n) const;

   private:
    std::vector<Coeff> coefficients_;
    Matrix<Coeff> transition_matrix_;
    size_t recursive_threshold_;

    Matrix<Coeff> build_transition_matrix() const;
    void validate_initial_values(const std::vector<Coeff>& initial_values) const;
};
