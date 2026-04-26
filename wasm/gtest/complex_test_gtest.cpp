#include <gtest/gtest.h>

#include <cmath>
#include <stdexcept>

#include "number/complex.h"

using CD = tsl::Complex<double>;
using CF = tsl::Complex<float>;
using CLD = tsl::Complex<long double>;

// -----------------------------------------------------------------------------
// Construction & accessors
// -----------------------------------------------------------------------------

TEST(ComplexTest, DefaultCtorIsZero) {
    CD z;
    EXPECT_EQ(z.real(), 0.0);
    EXPECT_EQ(z.imag(), 0.0);
}

TEST(ComplexTest, OneArgCtorHasZeroImag) {
    CD z(3.0);
    EXPECT_EQ(z.real(), 3.0);
    EXPECT_EQ(z.imag(), 0.0);
}

TEST(ComplexTest, TwoArgCtor) {
    CD z(3.0, 4.0);
    EXPECT_EQ(z.real(), 3.0);
    EXPECT_EQ(z.imag(), 4.0);
}

// -----------------------------------------------------------------------------
// Matrix conversion round-trip
// -----------------------------------------------------------------------------

TEST(ComplexTest, ToMatrixLayout) {
    // z = a + bi  -->  [[a, -b], [b, a]]
    CD z(3.0, 4.0);
    auto m = z.to_matrix();
    EXPECT_EQ(m.at(0, 0),  3.0);
    EXPECT_EQ(m.at(0, 1), -4.0);
    EXPECT_EQ(m.at(1, 0),  4.0);
    EXPECT_EQ(m.at(1, 1),  3.0);
}

TEST(ComplexTest, FromMatrixRoundTrip) {
    CD z(3.0, 4.0);
    CD z2 = CD::from_matrix(z.to_matrix());
    EXPECT_DOUBLE_EQ(z2.real(), z.real());
    EXPECT_DOUBLE_EQ(z2.imag(), z.imag());
}

TEST(ComplexTest, FromMatrixBadSizeThrows) {
    Matrix<double> bad(3, 3);
    EXPECT_THROW(CD::from_matrix(bad), std::invalid_argument);
}

// -----------------------------------------------------------------------------
// Arithmetic (all routes through the 2x2 matrix representation)
// -----------------------------------------------------------------------------

TEST(ComplexTest, Addition) {
    CD a(1.0, 2.0);
    CD b(3.0, 4.0);
    CD c = a + b;
    EXPECT_DOUBLE_EQ(c.real(), 4.0);
    EXPECT_DOUBLE_EQ(c.imag(), 6.0);
}

TEST(ComplexTest, Subtraction) {
    CD a(5.0, 3.0);
    CD b(2.0, 1.0);
    CD c = a - b;
    EXPECT_DOUBLE_EQ(c.real(), 3.0);
    EXPECT_DOUBLE_EQ(c.imag(), 2.0);
}

TEST(ComplexTest, Multiplication) {
    // (1 + 2i)(3 + 4i) = 3 + 4i + 6i - 8 = -5 + 10i
    CD a(1.0, 2.0);
    CD b(3.0, 4.0);
    CD c = a * b;
    EXPECT_DOUBLE_EQ(c.real(), -5.0);
    EXPECT_DOUBLE_EQ(c.imag(), 10.0);
}

TEST(ComplexTest, Division) {
    // (1 + 2i) / (1 + i)
    //   = (1 + 2i)(1 - i) / 2
    //   = (1 - i + 2i + 2) / 2
    //   = (3 + i) / 2  =  1.5 + 0.5i
    CD a(1.0, 2.0);
    CD b(1.0, 1.0);
    CD c = a / b;
    EXPECT_DOUBLE_EQ(c.real(), 1.5);
    EXPECT_DOUBLE_EQ(c.imag(), 0.5);
}

TEST(ComplexTest, DivisionByZeroThrows) {
    CD a(1.0, 2.0);
    CD zero;
    EXPECT_THROW(a / zero, std::invalid_argument);
}

TEST(ComplexTest, Negation) {
    CD z(1.0, -2.0);
    CD neg = -z;
    EXPECT_DOUBLE_EQ(neg.real(), -1.0);
    EXPECT_DOUBLE_EQ(neg.imag(),  2.0);
}

TEST(ComplexTest, CompoundAssignment) {
    CD a(1.0, 2.0);
    CD b(3.0, 4.0);
    a += b;
    EXPECT_DOUBLE_EQ(a.real(), 4.0);
    EXPECT_DOUBLE_EQ(a.imag(), 6.0);
}

// -----------------------------------------------------------------------------
// Complex-specific operations
// -----------------------------------------------------------------------------

TEST(ComplexTest, Conjugate) {
    CD z(3.0, -4.0);
    CD conj = z.conjugate();
    EXPECT_DOUBLE_EQ(conj.real(),  3.0);
    EXPECT_DOUBLE_EQ(conj.imag(),  4.0);
}

TEST(ComplexTest, Abs) {
    CD z(3.0, 4.0);
    EXPECT_DOUBLE_EQ(z.abs(), 5.0);
}

TEST(ComplexTest, PureImaginaryAbs) {
    CD z(0.0, 1.0);
    EXPECT_DOUBLE_EQ(z.abs(), 1.0);
}

// -----------------------------------------------------------------------------
// Identity: multiply by conjugate / abs_sq recovers self
// -----------------------------------------------------------------------------

TEST(ComplexTest, DivBySelfIsOne) {
    CD z(3.0, 4.0);
    CD one = z / z;
    EXPECT_DOUBLE_EQ(one.real(), 1.0);
    EXPECT_NEAR(one.imag(), 0.0, 1e-14);
}

// -----------------------------------------------------------------------------
// Other instantiation types compile and give correct results
// -----------------------------------------------------------------------------

TEST(ComplexTest, FloatInstantiation) {
    CF z(3.0f, 4.0f);
    EXPECT_FLOAT_EQ(z.abs(), 5.0f);
}

TEST(ComplexTest, LongDoubleInstantiation) {
    CLD z(3.0L, 4.0L);
    EXPECT_DOUBLE_EQ(static_cast<double>(z.abs()), 5.0);
}

TEST(ComplexTest, FloatMultiply) {
    CF a(1.0f, 2.0f);
    CF b(3.0f, 4.0f);
    CF c = a * b;
    EXPECT_FLOAT_EQ(c.real(), -5.0f);
    EXPECT_FLOAT_EQ(c.imag(), 10.0f);
}
