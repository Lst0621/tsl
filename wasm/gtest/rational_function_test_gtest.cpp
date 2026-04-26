#include <gtest/gtest.h>

#include <vector>

#include "algebra/polynomial.h"
#include "algebra/rational_function.h"
#include "algebra/sequence_ops.h"

using R = LinearRecurrence::Coeff;

static std::vector<R> eval_terms(const LinearRecurrence& lr,
                                 const std::vector<R>& init, size_t n_terms) {
    std::vector<R> out;
    out.reserve(n_terms);
    for (size_t n = 0; n < n_terms; ++n) {
        out.push_back(lr.evaluate(init, n));
    }
    return out;
}

TEST(RationalFunctionTest, FibonacciToRationalFunctionHasExpectedDenAndNum) {
    LinearRecurrence fib({1, 1});
    std::vector<R> init = {0, 1};

    const auto F = to_rational_function(fib, init);

    // Q(x) = 1 - x - x^2
    EXPECT_EQ(F.den.degree(), 2);
    EXPECT_EQ(F.den.get_coefficient(0), R(1));
    EXPECT_EQ(F.den.get_coefficient(1), R(-1));
    EXPECT_EQ(F.den.get_coefficient(2), R(-1));

    // P(x) = x
    EXPECT_EQ(F.num.degree(), 1);
    EXPECT_EQ(F.num.get_coefficient(0), R(0));
    EXPECT_EQ(F.num.get_coefficient(1), R(1));
}

TEST(RationalFunctionTest, RoundTripToLinearRecurrenceReturnsSameInitAndCoeff) {
    LinearRecurrence lr({R(1, 2), R(1, 3)});
    std::vector<R> init = {R(1), R(2)};

    const auto F = to_rational_function(lr, init);
    const auto [lr2, init2] = to_linear_recurrence(F);

    ASSERT_EQ(lr2.order(), lr.order());
    ASSERT_EQ(init2.size(), init.size());

    for (size_t i = 0; i < lr.order(); ++i) {
        EXPECT_EQ(lr2.coefficients()[i], lr.coefficients()[i]);
        EXPECT_EQ(init2[i], init[i]);
    }
}

TEST(RationalFunctionTest, ConvolutionMatchesDirectSequenceConvolution) {
    LinearRecurrence fib({1, 1});
    std::vector<R> init = {0, 1};

    const auto [lr_g, init_g] = convolve(fib, init, fib, init);

    const size_t n_terms = 12;
    const std::vector<R> f = eval_terms(fib, init, n_terms);
    const std::vector<R> g = eval_terms(lr_g, init_g, n_terms);

    const std::vector<R> expected = sequence_convolve(f, f);

    for (size_t n = 0; n < n_terms; ++n) {
        EXPECT_EQ(g[n], expected[n]) << "Mismatch at n=" << n;
    }
}

TEST(RationalFunctionTest, ConvolutionFibFibFirst200TermsPolynomialVsDirectVsRecurrence) {
    LinearRecurrence fib({1, 1});
    std::vector<R> init = {0, 1};

    const size_t n_terms = 200;
    const std::vector<R> f = eval_terms(fib, init, n_terms);

    // Method 1: polynomial multiply of truncated series coefficients.
    Polynomial<R> F_poly(f);
    Polynomial<R> G_poly = F_poly * F_poly;
    std::vector<R> coeffs_poly = G_poly.get_coefficients();
    coeffs_poly.resize(n_terms, R(0));

    // Method 2: sequence_convolve (inner product + reverse).
    std::vector<R> coeffs_direct = sequence_convolve(f, f);
    coeffs_direct.resize(n_terms, R(0));

    // Method 3: generating-function product -> linear recurrence -> evaluate.
    const auto [lr_g, init_g] = convolve(fib, init, fib, init);
    const std::vector<R> coeffs_lr = eval_terms(lr_g, init_g, n_terms);

    for (size_t n = 0; n < n_terms; ++n) {
        EXPECT_EQ(coeffs_poly[n], coeffs_direct[n]) << "Poly vs direct mismatch at n=" << n;
        EXPECT_EQ(coeffs_lr[n], coeffs_direct[n]) << "LR vs direct mismatch at n=" << n;
    }
}

