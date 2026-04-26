#pragma once

#include <stdexcept>
#include <utility>
#include <vector>

#include "algebra/linear_recurrence.h"
#include "algebra/polynomial.h"

template <typename T>
struct RationalFunction {
    Polynomial<T> num;
    Polynomial<T> den;

    RationalFunction() : num(), den(T(1)) {}

    RationalFunction(const Polynomial<T>& numerator,
                     const Polynomial<T>& denominator)
        : num(numerator), den(denominator) {
        normalize();
    }

    void normalize() {
        // Require den(0) != 0 so the formal power series expansion exists.
        const T d0 = den.get_coefficient(0);
        if (d0 == T()) {
            throw std::invalid_argument(
                "RationalFunction: denominator constant term must be non-zero");
        }
        if (d0 == T(1)) {
            return;
        }
        // Scale so den(0) == 1.
        const T inv = T(1) / d0;
        num = num * inv;
        den = den * inv;
    }
};

template <typename T>
RationalFunction<T> operator*(const RationalFunction<T>& a,
                               const RationalFunction<T>& b) {
    return RationalFunction<T>(a.num * b.num, a.den * b.den);
}

template <typename T>
RationalFunction<T> to_rational_function(
    const LinearRecurrenceT<T>& lr, const std::vector<T>& init) {
    const size_t k = lr.order();
    if (init.size() != k) {
        throw std::invalid_argument(
            "to_rational_function: init size must match recurrence order");
    }

    // Q(x) = 1 - c1 x - c2 x^2 - ... - ck x^k
    std::vector<T> q_coeffs(k + 1, T(0));
    q_coeffs[0] = T(1);
    for (size_t i = 0; i < k; ++i) {
        q_coeffs[i + 1] = -lr.coefficients()[i];
    }
    const Polynomial<T> Q(q_coeffs);

    // P_n = f(n) - sum_{i=1..min(k,n)} c_i f(n-i)  for n = 0..k-1
    std::vector<T> p_coeffs(k, T(0));
    for (size_t n = 0; n < k; ++n) {
        T pn = init[n];
        const size_t max_i = (n < k) ? n : k;
        for (size_t i = 1; i <= max_i; ++i) {
            pn -= lr.coefficients()[i - 1] * init[n - i];
        }
        p_coeffs[n] = pn;
    }
    const Polynomial<T> P(p_coeffs);

    return RationalFunction<T>(P, Q);
}

template <typename T>
std::pair<LinearRecurrenceT<T>, std::vector<T>>
to_linear_recurrence(const RationalFunction<T>& F) {
    RationalFunction<T> G = F;
    G.normalize();

    const Polynomial<T>& Q = G.den;
    const int degQ = Q.degree();
    if (degQ <= 0) {
        throw std::invalid_argument(
            "to_linear_recurrence: denominator degree must be >= 1");
    }
    const size_t k = static_cast<size_t>(degQ);

    if (Q.get_coefficient(0) != T(1)) {
        throw std::logic_error(
            "to_linear_recurrence: expected normalized Q(0)=1");
    }

    // Extract recurrence coefficients: Q(x)=1 - c1 x - ... - ck x^k => c_i = -q_i
    std::vector<T> coeffs(k, T(0));
    for (size_t i = 0; i < k; ++i) {
        coeffs[i] = -Q.get_coefficient(i + 1);
    }
    LinearRecurrenceT<T> lr(coeffs);

    // Compute init terms f(0..k-1) from Q(x)F(x) = P(x).
    std::vector<T> init(k, T(0));
    for (size_t n = 0; n < k; ++n) {
        T pn = G.num.get_coefficient(n);
        T sum(0);
        const size_t max_i = (n < k) ? n : k;
        for (size_t i = 1; i <= max_i; ++i) {
            sum += Q.get_coefficient(i) * init[n - i];
        }
        init[n] = pn - sum;
    }

    return {lr, init};
}

template <typename T>
std::pair<LinearRecurrenceT<T>, std::vector<T>> convolve(
    const LinearRecurrenceT<T>& a, const std::vector<T>& init_a,
    const LinearRecurrenceT<T>& b, const std::vector<T>& init_b) {
    const RationalFunction<T> fa = to_rational_function(a, init_a);
    const RationalFunction<T> fb = to_rational_function(b, init_b);
    const RationalFunction<T> fg = fa * fb;

    auto [lr_g, init_g] = to_linear_recurrence(fg);
    const size_t k = lr_g.order();

    if (init_g.size() != k) {
        throw std::logic_error(
            "convolve: unexpected init size from to_linear_recurrence");
    }

    return {lr_g, init_g};
}
