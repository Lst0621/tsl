#include "linear_recurrence.h"

#include "semigroup.h"

LinearRecurrence::LinearRecurrence(const std::vector<Coeff>& coeffs,
                                   size_t recursive_threshold)
    : coefficients_(coeffs),
      transition_matrix_(1, 1, Coeff(0)),
      recursive_threshold_(recursive_threshold) {
    if (coefficients_.empty()) {
        throw std::invalid_argument("Recurrence coefficients cannot be empty");
    }
    if (recursive_threshold_ == 0) {
        throw std::invalid_argument("Recursive threshold must be positive");
    }
    transition_matrix_ = build_transition_matrix();
}

size_t LinearRecurrence::order() const {
    return coefficients_.size();
}

const std::vector<LinearRecurrence::Coeff>& LinearRecurrence::coefficients()
    const {
    return coefficients_;
}

const Matrix<LinearRecurrence::Coeff>& LinearRecurrence::transition_matrix()
    const {
    return transition_matrix_;
}

Polynomial<LinearRecurrence::Coeff> LinearRecurrence::characteristic_polynomial()
    const {
    // p(x) = x^k - c1*x^(k-1) - ... - ck for recurrence
    // f(n) = c1*f(n-1) + ... + ck*f(n-k)
    const size_t k = order();
    std::vector<Coeff> coeffs(k + 1, Coeff(0));
    coeffs[k] = Coeff(1);

    for (size_t i = 0; i < k; ++i) {
        coeffs[k - 1 - i] = -coefficients_[i];
    }

    return Polynomial<Coeff>(coeffs);
}

LinearRecurrence::Coeff LinearRecurrence::evaluate_recursive(
    const std::vector<Coeff>& initial_values, size_t n) const {
    validate_initial_values(initial_values);

    if (n < order()) {
        return initial_values[n];
    }

    std::vector<Coeff> values = initial_values;
    values.reserve(n + 1);

    for (size_t idx = order(); idx <= n; ++idx) {
        Coeff next(0);
        for (size_t j = 0; j < order(); ++j) {
            next += coefficients_[j] * values[idx - 1 - j];
        }
        values.push_back(next);
    }

    return values[n];
}

LinearRecurrence::Coeff LinearRecurrence::evaluate_matrix(
    const std::vector<Coeff>& initial_values, size_t n) const {
    validate_initial_values(initial_values);

    if (n < order()) {
        return initial_values[n];
    }

    const size_t k = order();

    std::vector<std::vector<Coeff>> identity_data(
        k, std::vector<Coeff>(k, Coeff(0)));
    for (size_t i = 0; i < k; ++i) {
        identity_data[i][i] = Coeff(1);
    }
    const Matrix<Coeff> identity(identity_data);

    const Matrix<Coeff> transition_pow =
        monoid_power(transition_matrix_,
                     static_cast<unsigned long long>(n - (k - 1)), identity);

    std::vector<std::vector<Coeff>> state_data(k, std::vector<Coeff>(1));
    for (size_t i = 0; i < k; ++i) {
        state_data[i][0] = initial_values[k - 1 - i];
    }
    const Matrix<Coeff> state(state_data);

    const Matrix<Coeff> next_state = transition_pow * state;
    return next_state(0, 0);
}

LinearRecurrence::Coeff LinearRecurrence::evaluate(
    const std::vector<Coeff>& initial_values, size_t n) const {
    if (n < recursive_threshold_) {
        return evaluate_recursive(initial_values, n);
    }
    return evaluate_matrix(initial_values, n);
}

Matrix<LinearRecurrence::Coeff> LinearRecurrence::build_transition_matrix() const {
    const size_t k = order();
    std::vector<std::vector<Coeff>> data(k, std::vector<Coeff>(k, Coeff(0)));

    for (size_t j = 0; j < k; ++j) {
        data[0][j] = coefficients_[j];
    }
    for (size_t i = 1; i < k; ++i) {
        data[i][i - 1] = Coeff(1);
    }

    return Matrix<Coeff>(data);
}

void LinearRecurrence::validate_initial_values(
    const std::vector<Coeff>& initial_values) const {
    if (initial_values.size() != order()) {
        throw std::invalid_argument(
            "Initial values size must match recurrence order");
    }
}
