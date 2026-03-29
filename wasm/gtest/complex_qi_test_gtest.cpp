#include <gtest/gtest.h>

#include "complex.h"
#include "polynomial.h"
#include "rational_function.h"
#include "sequence_ops.h"

using R  = tsl::Rational<long long>;
using QI = tsl::QI;  // Complex<Rational<long long>>

// Helper: Rational<long long> from integer
static R ri(long long v) { return R(v); }

// The imaginary unit i = 0 + 1*i
static const QI I(R(0), R(1));

// -----------------------------------------------------------------------------
// QI basic arithmetic
// -----------------------------------------------------------------------------

TEST(QITest, DefaultCtorIsZero) {
    QI z;
    EXPECT_EQ(z.real(), ri(0));
    EXPECT_EQ(z.imag(), ri(0));
}

TEST(QITest, Construction) {
    QI z(R(3), R(4));
    EXPECT_EQ(z.real(), ri(3));
    EXPECT_EQ(z.imag(), ri(4));
}

TEST(QITest, ISquaredIsMinusOne) {
    // i * i = -1 + 0i
    QI result = I * I;
    EXPECT_EQ(result.real(), ri(-1));
    EXPECT_EQ(result.imag(), ri(0));
}

TEST(QITest, Addition) {
    QI a(R(1), R(2));
    QI b(R(3), R(4));
    QI c = a + b;
    EXPECT_EQ(c.real(), ri(4));
    EXPECT_EQ(c.imag(), ri(6));
}

TEST(QITest, Multiplication) {
    // (1 + 2i)(3 + 4i) = -5 + 10i
    QI a(R(1), R(2));
    QI b(R(3), R(4));
    QI c = a * b;
    EXPECT_EQ(c.real(), ri(-5));
    EXPECT_EQ(c.imag(), ri(10));
}

TEST(QITest, Division) {
    // (1 + 2i) / (1 + i) = 3/2 + 1/2 * i
    QI a(R(1), R(2));
    QI b(R(1), R(1));
    QI c = a / b;
    EXPECT_EQ(c.real(), R(3, 2));
    EXPECT_EQ(c.imag(), R(1, 2));
}

TEST(QITest, Conjugate) {
    QI z(R(3), R(-4));
    QI conj = z.conjugate();
    EXPECT_EQ(conj.real(), ri(3));
    EXPECT_EQ(conj.imag(), ri(4));
}

TEST(QITest, AbsReturnsDouble) {
    // |3 + 4i| = 5
    QI z(R(3), R(4));
    EXPECT_DOUBLE_EQ(z.abs(), 5.0);
}

TEST(QITest, AbsOneHalf) {
    // |1/2 + 0i| = 0.5
    QI z(R(1, 2), R(0));
    EXPECT_DOUBLE_EQ(z.abs(), 0.5);
}

TEST(QITest, ToStringFraction) {
    QI z(R(1, 2), R(3, 4));
    EXPECT_EQ(z.to_string(), "1/2+3/4i");
}

// -----------------------------------------------------------------------------
// Polynomial<QI>: verify x^2 + 1 = (x - i)(x + i)
// -----------------------------------------------------------------------------

TEST(PolyQITest, FactorizationXSquaredPlusOne) {
    // x^2 + 1  <=>  coefficients [1, 0, 1]
    Polynomial<QI> p({QI(ri(1)), QI(ri(0)), QI(ri(1))});

    // (x - i)  <=>  coefficients [-i, 1]
    Polynomial<QI> f1({-I, QI(ri(1))});

    // (x + i)  <=>  coefficients [i, 1]
    Polynomial<QI> f2({I, QI(ri(1))});

    Polynomial<QI> product = f1 * f2;

    EXPECT_EQ(product, p)
        << "x^2+1 should equal (x-i)*(x+i) over QI";
}

TEST(PolyQITest, CharacteristicPolynomialsMatchFactored) {
    // Order-2 recurrence  f(n) = -f(n-2)  has char poly  x^2 + 1
    LinearRecurrenceT<QI> lr2({QI(ri(0)), QI(ri(-1))});
    Polynomial<QI> p2 = lr2.characteristic_polynomial();

    // Order-1 recurrence  f(n) = i*f(n-1)  has char poly  x - i
    LinearRecurrenceT<QI> lr_pos_i({I});
    Polynomial<QI> p_pos_i = lr_pos_i.characteristic_polynomial();

    // Order-1 recurrence  f(n) = -i*f(n-1)  has char poly  x + i
    LinearRecurrenceT<QI> lr_neg_i({-I});
    Polynomial<QI> p_neg_i = lr_neg_i.characteristic_polynomial();

    EXPECT_EQ(p2, p_pos_i * p_neg_i)
        << "char poly of f(n)=-f(n-2) should factor as (x-i)(x+i)";
}

// -----------------------------------------------------------------------------
// LinearRecurrenceT<QI>: factored recurrence terms match the order-2 recurrence
//
// The sequence  f(n) = i^n + (-i)^n  satisfies f(n) = -f(n-2)
// (initial values f(0)=2, f(1)=0 for this particular combination).
// We verify that the convolve path recovers consistent terms.
// -----------------------------------------------------------------------------

TEST(LinearRecurQITest, IToThePowerN) {
    // Recurrence f(n) = i*f(n-1),  f(0) = 1  =>  f(n) = i^n
    LinearRecurrenceT<QI> lr({I});
    std::vector<QI> init = {QI(ri(1))};

    EXPECT_EQ(lr.evaluate(init, 0), QI(ri(1)));      // i^0 = 1
    EXPECT_EQ(lr.evaluate(init, 1), I);              // i^1 = i
    EXPECT_EQ(lr.evaluate(init, 2), QI(ri(-1)));     // i^2 = -1
    EXPECT_EQ(lr.evaluate(init, 3), -I);             // i^3 = -i
    EXPECT_EQ(lr.evaluate(init, 4), QI(ri(1)));      // i^4 = 1  (period 4)
}

TEST(LinearRecurQITest, NegIToThePowerN) {
    // Recurrence f(n) = -i*f(n-1),  f(0) = 1  =>  f(n) = (-i)^n
    LinearRecurrenceT<QI> lr({-I});
    std::vector<QI> init = {QI(ri(1))};

    EXPECT_EQ(lr.evaluate(init, 0), QI(ri(1)));      // (-i)^0 = 1
    EXPECT_EQ(lr.evaluate(init, 1), -I);             // (-i)^1 = -i
    EXPECT_EQ(lr.evaluate(init, 2), QI(ri(-1)));     // (-i)^2 = -1
    EXPECT_EQ(lr.evaluate(init, 3), I);              // (-i)^3 = i
    EXPECT_EQ(lr.evaluate(init, 4), QI(ri(1)));      // period 4
}

TEST(LinearRecurQITest, Order2RecurrenceMinusFNMinus2) {
    // f(n) = -f(n-2),  f(0)=1, f(1)=i  =>  f(n) = i^n
    // char poly x^2+1, coefficients: c1=0 (for f(n-1)), c2=-1 (for f(n-2))
    LinearRecurrenceT<QI> lr({QI(ri(0)), QI(ri(-1))});
    std::vector<QI> init = {QI(ri(1)), I};

    EXPECT_EQ(lr.evaluate(init, 0), QI(ri(1)));
    EXPECT_EQ(lr.evaluate(init, 1), I);
    EXPECT_EQ(lr.evaluate(init, 2), QI(ri(-1)));  // = -f(0) = -1
    EXPECT_EQ(lr.evaluate(init, 3), -I);          // = -f(1) = -i
    EXPECT_EQ(lr.evaluate(init, 4), QI(ri(1)));   // = -f(2) = 1
}

TEST(LinearRecurQITest, ConvolveFactoredRecurrencesMatchOrder2) {
    // convolve computes the Cauchy product of generating functions:
    //   F_a(x) = 1/(1 - ix)   (generates a(n) = i^n)
    //   F_b(x) = 1/(1 + ix)   (generates b(n) = (-i)^n)
    //   F_g(x) = F_a * F_b = 1/(1 + x^2)
    //
    // This is exactly the generating function for the order-2 recurrence
    // with characteristic polynomial x^2 + 1.
    //
    // Explicit Cauchy convolution:
    //   g(n) = sum_{k=0}^{n} i^k * (-i)^(n-k)
    //        = (-i)^n * sum_{k=0}^{n} (-1)^k
    //   => g(n) = 1, 0, -1, 0, 1, 0, -1, 0, ...

    LinearRecurrenceT<QI> lr_a({I});
    LinearRecurrenceT<QI> lr_b({-I});
    std::vector<QI> init_a = {QI(ri(1))};
    std::vector<QI> init_b = {QI(ri(1))};

    auto [lr_g, init_g] = convolve(lr_a, init_a, lr_b, init_b);

    // The resulting recurrence must have characteristic polynomial x^2 + 1,
    // confirming the factorization (x - i)(x + i) = x^2 + 1.
    Polynomial<QI> expected_char_poly({QI(ri(1)), QI(ri(0)), QI(ri(1))});
    EXPECT_EQ(lr_g.characteristic_polynomial(), expected_char_poly)
        << "convolved recurrence should have char poly x^2+1";

    // Cross-check 1: convolve terms must match the order-2 recurrence
    // f(n) = -f(n-2) with f(0)=1, f(1)=0 (same generating function 1/(1+x^2)).
    LinearRecurrenceT<QI> lr_order2({QI(ri(0)), QI(ri(-1))});
    std::vector<QI> init_order2 = {QI(ri(1)), QI(ri(0))};

    for (size_t n = 0; n <= 10; ++n) {
        EXPECT_EQ(lr_g.evaluate(init_g, n),
                  lr_order2.evaluate(init_order2, n))
            << "convolve term differs from order-2 recurrence at n=" << n;
    }

    // Cross-check 2: use sequence_convolve (inner product + reverse) on the
    // pre-computed term vectors and compare each position.
    constexpr size_t N = 11;
    std::vector<QI> seq_a(N), seq_b(N);
    for (size_t k = 0; k < N; ++k) {
        seq_a[k] = lr_a.evaluate(init_a, k);
        seq_b[k] = lr_b.evaluate(init_b, k);
    }

    std::vector<QI> direct = sequence_convolve(seq_a, seq_b);

    for (size_t n = 0; n <= 10; ++n) {
        EXPECT_EQ(lr_g.evaluate(init_g, n), direct[n])
            << "convolve term differs from sequence_convolve at n=" << n;
    }
}
