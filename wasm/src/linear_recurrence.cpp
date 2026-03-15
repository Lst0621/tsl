#include "linear_recurrence.h"

#include "semigroup.h"

LinearRecurrence::LinearRecurrence(const std::vector<long long>& coeffs,
                                   size_t recursive_threshold)
    : coefficients_(coeffs),
      transition_matrix_(1, 1, 0LL),
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

const std::vector<long long>& LinearRecurrence::coefficients() const {
    return coefficients_;
}

const Matrix<long long>& LinearRecurrence::transition_matrix() const {
    return transition_matrix_;
}

Polynomial<long long> LinearRecurrence::characteristic_polynomial() const {
    // p(x) = x^k - c1*x^(k-1) - ... - ck for recurrence
    // f(n) = c1*f(n-1) + ... + ck*f(n-k)
    const size_t k = order();
    std::vector<long long> coeffs(k + 1, 0LL);
    coeffs[k] = 1LL;

    for (size_t i = 0; i < k; ++i) {
        coeffs[k - 1 - i] = -coefficients_[i];
    }

    return Polynomial<long long>(coeffs);
}

long long LinearRecurrence::evaluate_recursive(
    const std::vector<long long>& initial_values, size_t n) const {
    validate_initial_values(initial_values);

    if (n < order()) {
        return initial_values[n];
    }

    std::vector<long long> values = initial_values;
    values.reserve(n + 1);

    for (size_t idx = order(); idx <= n; ++idx) {
        long long next = 0;
        for (size_t j = 0; j < order(); ++j) {
            next += coefficients_[j] * values[idx - 1 - j];
        }
        values.push_back(next);
    }

    return values[n];
}

long long LinearRecurrence::evaluate_matrix(
    const std::vector<long long>& initial_values, size_t n) const {
    validate_initial_values(initial_values);

    if (n < order()) {
        return initial_values[n];
    }

    const size_t k = order();

    std::vector<std::vector<long long>> identity_data(
        k, std::vector<long long>(k, 0LL));
    for (size_t i = 0; i < k; ++i) {
        identity_data[i][i] = 1LL;
    }
    const Matrix<long long> identity(identity_data);

    const Matrix<long long> transition_pow =
        monoid_power(transition_matrix_,
                     static_cast<unsigned long long>(n - (k - 1)), identity);

    std::vector<std::vector<long long>> state_data(k,
                                                   std::vector<long long>(1));
    for (size_t i = 0; i < k; ++i) {
        state_data[i][0] = initial_values[k - 1 - i];
    }
    const Matrix<long long> state(state_data);

    const Matrix<long long> next_state = transition_pow * state;
    return next_state(0, 0);
}

long long LinearRecurrence::evaluate(
    const std::vector<long long>& initial_values, size_t n) const {
    if (n < recursive_threshold_) {
        return evaluate_recursive(initial_values, n);
    }
    return evaluate_matrix(initial_values, n);
}

Matrix<long long> LinearRecurrence::build_transition_matrix() const {
    const size_t k = order();
    std::vector<std::vector<long long>> data(k, std::vector<long long>(k, 0LL));

    for (size_t j = 0; j < k; ++j) {
        data[0][j] = coefficients_[j];
    }
    for (size_t i = 1; i < k; ++i) {
        data[i][i - 1] = 1LL;
    }

    return Matrix<long long>(data);
}

void LinearRecurrence::validate_initial_values(
    const std::vector<long long>& initial_values) const {
    if (initial_values.size() != order()) {
        throw std::invalid_argument(
            "Initial values size must match recurrence order");
    }
}
