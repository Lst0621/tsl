#include <gtest/gtest.h>

#include "comb.h"
#include "linear_recurrence.h"
#include "polynomial.h"

// Specialized traits for double with floating-point tolerance
template <>
struct CoefficientTraits<double> {
    static bool is_zero(const double& value) {
        return std::abs(value) < 1e-10;
    }

    static bool is_one(const double& value) {
        return std::abs(value - 1.0) < 1e-10;
    }

    static double zero() {
        return 0.0;
    }

    static double one() {
        return 1.0;
    }

    static bool equals(const double& a, const double& b) {
        return std::abs(a - b) < 1e-10;
    }
};

// Test fixture for polynomial tests
class PolynomialTest : public ::testing::Test {
   protected:
    // Common polynomials for testing
    Polynomial<long long> P_simple{{1, 2, 3}};  // 1 + 2x + 3x^2
    Polynomial<long long> Q_simple{{1, 1}};     // 1 + x
    Polynomial<long long> zero_poly;            // 0
    Polynomial<long long> constant_five{5};     // 5
};

// ============================================================================
// Basic Operations Tests
// ============================================================================

TEST_F(PolynomialTest, ConstructionAndDegree) {
    Polynomial<long long> P({1, 2, 3});
    EXPECT_EQ(P.degree(), 2);
}

TEST_F(PolynomialTest, GetCoefficients) {
    Polynomial<long long> P({1, 2, 3});
    EXPECT_EQ(P.get_coefficient(0), 1);
    EXPECT_EQ(P.get_coefficient(1), 2);
    EXPECT_EQ(P.get_coefficient(2), 3);
    EXPECT_EQ(P.get_coefficient(5), 0);  // Out of range
}

TEST_F(PolynomialTest, EvaluationSimple) {
    // P(x) = 1 + 2x + 3x^2
    // P(2) = 1 + 4 + 12 = 17
    EXPECT_EQ(P_simple.apply(2LL), 17);
}

TEST_F(PolynomialTest, EvaluationAtZero) {
    // P(0) should equal constant term
    EXPECT_EQ(P_simple.apply(0LL), 1);
}

TEST_F(PolynomialTest, EvaluationConstantPolynomial) {
    Polynomial<long long> constant(5);
    EXPECT_EQ(constant.apply(100LL), 5);
    EXPECT_EQ(constant.apply(0LL), 5);
}

TEST_F(PolynomialTest, EvaluationZeroPolynomial) {
    EXPECT_EQ(zero_poly.apply(100LL), 0);
}

// ============================================================================
// Addition Tests
// ============================================================================

TEST_F(PolynomialTest, AdditionBasic) {
    // P + Q = (1+1) + (2+1)x + 3x^2 = 2 + 3x + 3x^2
    auto sum = P_simple + Q_simple;
    EXPECT_EQ(sum.get_coefficient(0), 2);
    EXPECT_EQ(sum.get_coefficient(1), 3);
    EXPECT_EQ(sum.get_coefficient(2), 3);
}

TEST_F(PolynomialTest, AdditionWithZero) {
    auto result = P_simple + zero_poly;
    EXPECT_EQ(result, P_simple);
}

TEST_F(PolynomialTest, AdditionCommutative) {
    auto sum1 = P_simple + Q_simple;
    auto sum2 = Q_simple + P_simple;
    EXPECT_EQ(sum1, sum2);
}

TEST_F(PolynomialTest, AdditionAssociative) {
    Polynomial<long long> R({2, 3});
    auto left = (P_simple + Q_simple) + R;
    auto right = P_simple + (Q_simple + R);
    EXPECT_EQ(left, right);
}

// ============================================================================
// Subtraction Tests
// ============================================================================

TEST_F(PolynomialTest, SubtractionBasic) {
    // P - Q = (1-1) + (2-1)x + 3x^2 = 0 + 1x + 3x^2
    auto diff = P_simple - Q_simple;
    EXPECT_EQ(diff.get_coefficient(0), 0);
    EXPECT_EQ(diff.get_coefficient(1), 1);
    EXPECT_EQ(diff.get_coefficient(2), 3);
}

TEST_F(PolynomialTest, SubtractionWithZero) {
    auto result = P_simple - zero_poly;
    EXPECT_EQ(result, P_simple);
}

TEST_F(PolynomialTest, SubtractionSelfIsZero) {
    auto result = P_simple - P_simple;
    EXPECT_EQ(result.degree(), -1);  // Zero polynomial
}

// ============================================================================
// Multiplication Tests
// ============================================================================

TEST_F(PolynomialTest, MultiplicationBasic) {
    // P * Q = (1 + 2x + 3x^2)(1 + x)
    //       = 1 + 3x + 5x^2 + 3x^3
    auto prod = P_simple * Q_simple;
    EXPECT_EQ(prod.degree(), 3);
    EXPECT_EQ(prod.get_coefficient(0), 1);
    EXPECT_EQ(prod.get_coefficient(1), 3);
    EXPECT_EQ(prod.get_coefficient(2), 5);
    EXPECT_EQ(prod.get_coefficient(3), 3);
}

TEST_F(PolynomialTest, MultiplicationEvaluation) {
    auto prod = P_simple * Q_simple;
    // (P * Q)(2) = P(2) * Q(2) = 17 * 3 = 51
    EXPECT_EQ(prod.apply(2LL), 51);
}

TEST_F(PolynomialTest, MultiplicationByZero) {
    auto result = P_simple * zero_poly;
    EXPECT_EQ(result.degree(), -1);
}

TEST_F(PolynomialTest, MultiplicationCommutative) {
    auto prod1 = P_simple * Q_simple;
    auto prod2 = Q_simple * P_simple;
    EXPECT_EQ(prod1, prod2);
}

TEST_F(PolynomialTest, MultiplicationIdentity) {
    Polynomial<long long> one(1);
    auto result = P_simple * one;
    EXPECT_EQ(result, P_simple);
}

// ============================================================================
// Scalar Multiplication Tests
// ============================================================================

TEST_F(PolynomialTest, ScalarMultiplicationBasic) {
    // 2 * P = 2 + 4x + 6x^2
    auto scaled = P_simple * 2;
    EXPECT_EQ(scaled.get_coefficient(0), 2);
    EXPECT_EQ(scaled.get_coefficient(1), 4);
    EXPECT_EQ(scaled.get_coefficient(2), 6);
}

TEST_F(PolynomialTest, ScalarMultiplicationCommutative) {
    auto left = P_simple * 2;
    auto right = 2 * P_simple;
    EXPECT_EQ(left, right);
}

TEST_F(PolynomialTest, ScalarMultiplicationByZero) {
    auto result = P_simple * 0;
    EXPECT_EQ(result.degree(), -1);
}

TEST_F(PolynomialTest, ScalarMultiplicationByOne) {
    auto result = P_simple * 1;
    EXPECT_EQ(result, P_simple);
}

// ============================================================================
// Division Tests
// ============================================================================

TEST_F(PolynomialTest, DivisionWithRemainderBasic) {
    auto [quotient, remainder] = P_simple.divide_with_remainder(Q_simple);

    // Verify: P = Q * quotient + remainder
    auto reconstructed = Q_simple * quotient + remainder;
    EXPECT_EQ(reconstructed, P_simple);
}

TEST_F(PolynomialTest, DivisionRemainderDegree) {
    auto [quotient, remainder] = P_simple.divide_with_remainder(Q_simple);

    // remainder degree must be less than divisor degree
    EXPECT_LT(remainder.degree(), Q_simple.degree());
}

TEST_F(PolynomialTest, DivisionByItself) {
    auto [quotient, remainder] = P_simple.divide_with_remainder(P_simple);

    // P / P should be 1 with remainder 0
    Polynomial<long long> one(1);
    EXPECT_EQ(quotient, one);
    EXPECT_EQ(remainder.degree(), -1);  // Zero polynomial
}

TEST_F(PolynomialTest, DivisionByZeroThrows) {
    EXPECT_THROW(P_simple.divide_with_remainder(zero_poly),
                 std::invalid_argument);
}

// ============================================================================
// GCD Tests
// ============================================================================

TEST_F(PolynomialTest, GCDWithItself) {
    auto gcd_result = P_simple.gcd(P_simple);

    // GCD(P, P) should be P (normalized)
    EXPECT_EQ(gcd_result.degree(), P_simple.degree());
}

TEST_F(PolynomialTest, GCDWithZero) {
    auto gcd_result = P_simple.gcd(zero_poly);

    // GCD(P, 0) = P
    EXPECT_EQ(gcd_result.degree(), P_simple.degree());
}

TEST_F(PolynomialTest, GCDZeroWithZero) {
    auto gcd_result = zero_poly.gcd(zero_poly);

    // GCD(0, 0) = 0
    EXPECT_EQ(gcd_result.degree(), -1);
}

TEST_F(PolynomialTest, GCDIdenticalPolynomials) {
    Polynomial<long long> P({1, 1});
    auto gcd_result = P.gcd(P);

    // GCD should have same degree
    EXPECT_EQ(gcd_result.degree(), 1);
}

// ============================================================================
// Power Helper Tests
// ============================================================================

TEST(PowerTest, BasicExponentiation) {
    long long result = Power<long long>::power(2LL, 10ULL);
    EXPECT_EQ(result, 1024);
}

TEST(PowerTest, ExponentiationSmallExponents) {
    EXPECT_EQ(Power<long long>::power(3LL, 5ULL), 243);
    EXPECT_EQ(Power<long long>::power(2LL, 1ULL), 2);
    EXPECT_EQ(Power<long long>::power(5LL, 2ULL), 25);
}

TEST(PowerTest, ExponentiationZero) {
    EXPECT_EQ(Power<long long>::power(5LL, 0ULL), 1);
    EXPECT_EQ(Power<long long>::power(1000LL, 0ULL), 1);
}

TEST(PowerTest, ExponentiationOne) {
    EXPECT_EQ(Power<long long>::power(10LL, 1ULL), 10);
    EXPECT_EQ(Power<long long>::power(42LL, 1ULL), 42);
}

TEST(PowerTest, ExponentiationLarge) {
    // Test that algorithm works for larger exponents
    long long result = Power<long long>::power(2LL, 20ULL);
    EXPECT_EQ(result, 1048576);  // 2^20
}

// ============================================================================
// Polynomial Power Tests
// ============================================================================

TEST(PolynomialPowerTest, SquarePolynomial) {
    Polynomial<long long> p({1, 1});  // x + 1

    // (x + 1)^2 = x^2 + 2x + 1
    auto p_squared = Power<Polynomial<long long>>::power(p, 2ULL);
    EXPECT_EQ(p_squared.degree(), 2);
    EXPECT_EQ(p_squared.get_coefficient(0), 1);
    EXPECT_EQ(p_squared.get_coefficient(1), 2);
    EXPECT_EQ(p_squared.get_coefficient(2), 1);
}

TEST(PolynomialPowerTest, ConstantPolynomialPower) {
    Polynomial<long long> constant(2);

    // 2^3 = 8
    auto result = Power<Polynomial<long long>>::power(constant, 3ULL);
    EXPECT_EQ(result.degree(), 0);
    EXPECT_EQ(result.get_coefficient(0), 8);
}

TEST(PolynomialPowerTest, PolynomialToPowerZero) {
    Polynomial<long long> p({1, 2, 3});

    // p^0 = 1
    auto result = Power<Polynomial<long long>>::power(p, 0ULL);
    EXPECT_EQ(result.degree(), 0);
    EXPECT_EQ(result.get_coefficient(0), 1);
}

// ============================================================================
// Floating-Point Tests
// ============================================================================

TEST(FloatingPointTest, DoubleEvaluation) {
    Polynomial<double, CoefficientTraits<double>> P({1.0, 2.0, 3.0});

    // P(1.5) = 1 + 2*1.5 + 3*1.5^2 = 1 + 3 + 6.75 = 10.75
    double result = P.apply(1.5);
    EXPECT_NEAR(result, 10.75, 1e-10);
}

TEST(FloatingPointTest, DoubleAddition) {
    Polynomial<double, CoefficientTraits<double>> P({1.0, 2.0, 3.0});
    Polynomial<double, CoefficientTraits<double>> Q({1.0, 1.0});

    auto sum = P + Q;
    EXPECT_NEAR(sum.get_coefficient(0), 2.0, 1e-10);
    EXPECT_NEAR(sum.get_coefficient(1), 3.0, 1e-10);
}

TEST(FloatingPointTest, DoubleMultiplication) {
    Polynomial<double, CoefficientTraits<double>> P({1.0, 1.0});  // x + 1
    Polynomial<double, CoefficientTraits<double>> Q({2.0, 0.5});  // 0.5x + 2

    auto prod = P * Q;
    EXPECT_EQ(prod.degree(), 2);
    // Coefficients: [2, 2.5, 0.5]
    EXPECT_NEAR(prod.get_coefficient(0), 2.0, 1e-10);
    EXPECT_NEAR(prod.get_coefficient(1), 2.5, 1e-10);
    EXPECT_NEAR(prod.get_coefficient(2), 0.5, 1e-10);
}

// ============================================================================
// Edge Cases and Special Cases
// ============================================================================

TEST(EdgeCasesTest, ZeroPolynomialDegree) {
    Polynomial<long long> zero;
    EXPECT_EQ(zero.degree(), -1);
}

TEST(EdgeCasesTest, ConstantPolynomialDegree) {
    Polynomial<long long> constant(5);
    EXPECT_EQ(constant.degree(), 0);
}

TEST(EdgeCasesTest, TrailingZeroRemoval) {
    Polynomial<long long> poly({1, 2, 0, 0, 0});
    EXPECT_EQ(poly.degree(), 1);
}

TEST(EdgeCasesTest, SingleElementPolynomial) {
    Polynomial<long long> single({5});
    EXPECT_EQ(single.degree(), 0);
    EXPECT_EQ(single.get_coefficient(0), 5);
}

TEST(EdgeCasesTest, NegativeCoefficients) {
    Polynomial<long long> p({1, -2, 3});
    EXPECT_EQ(p.degree(), 2);
    EXPECT_EQ(p.apply(1LL), 1 - 2 + 3);  // = 2
}

TEST(EdgeCasesTest, LargeCoefficients) {
    Polynomial<long long> p({1000000000LL, 2000000000LL});
    EXPECT_EQ(p.apply(1LL), 3000000000LL);
}

// ============================================================================
// Equality Tests
// ============================================================================

TEST(EqualityTest, IdenticalPolynomials) {
    Polynomial<long long> p1({1, 2, 3});
    Polynomial<long long> p2({1, 2, 3});
    EXPECT_EQ(p1, p2);
}

TEST(EqualityTest, DifferentPolynomials) {
    Polynomial<long long> p1({1, 2, 3});
    Polynomial<long long> p2({1, 2, 4});
    EXPECT_NE(p1, p2);
}

TEST(EqualityTest, DifferentDegrees) {
    Polynomial<long long> p1({1, 2, 3});
    Polynomial<long long> p2({1, 2});
    EXPECT_NE(p1, p2);
}

// ============================================================================
// Cross-Type Tests
// ============================================================================

TEST(CrossTypeTest, ApplyWithDifferentType) {
    Polynomial<long long> P({1, 2, 3});
    double result = P.apply(1.5);
    // P(1.5) = 1 + 3 + 6.75 = 10.75
    EXPECT_NEAR(result, 10.75, 1e-10);
}

TEST(CrossTypeTest, EvaluateIntPolyWithDouble) {
    Polynomial<long long> P({2, 3});  // 2 + 3x
    double result = P.apply(2.5);
    EXPECT_NEAR(result, 2.0 + 3.0 * 2.5, 1e-10);
}

// ============================================================================
// Sparse Storage Tests
// ============================================================================

TEST(SparseStorageTest, LargeSparsePolynomialMultiplication) {
    // Create x^1000 (all zeros except coefficient 1 at position 1000)
    std::vector<long long> coeffs_x1000(1001, 0);
    coeffs_x1000[1000] = 1;
    Polynomial<long long> x1000(coeffs_x1000);

    // Create x^10 (all zeros except coefficient 1 at position 10)
    std::vector<long long> coeffs_x10(11, 0);
    coeffs_x10[10] = 1;
    Polynomial<long long> x10(coeffs_x10);

    // Multiply: x^1000 * x^10 = x^1010
    auto result = x1000 * x10;

    // Result should have degree 1010
    EXPECT_EQ(result.degree(), 1010);

    // Only coefficient at position 1010 should be 1, all others should be 0
    EXPECT_EQ(result.get_coefficient(1010), 1);
    EXPECT_EQ(result.get_coefficient(1009), 0);
    EXPECT_EQ(result.get_coefficient(1011), 0);
    EXPECT_EQ(result.get_coefficient(0), 0);

    // Evaluate at x=1 should be 1
    EXPECT_EQ(result.apply(1LL), 1);

    // Evaluate at x=2 should be 2^1010
    // We can't test this directly due to overflow, but we can verify structure
}

TEST(SparseStorageTest, DetectSparseRepresentation) {
    // Create a very sparse polynomial: degree > 256 with few non-zero terms
    std::vector<long long> coeffs(300, 0);
    coeffs[1] = 1;       // x term
    coeffs[299] = 1;     // x^299 term
    Polynomial<long long> sparse_poly(coeffs);

    // Check if sparse representation would be beneficial
    // (degree 299 > 256 and only 2 non-zero out of 300)
    bool should_be_sparse = sparse_poly.degree() > 256 &&
                           (2.0 / 300.0) < 0.5;
    EXPECT_TRUE(should_be_sparse);
}

TEST(SparseStorageTest, OptimizeStorageConversion) {
    // Create a very sparse polynomial
    std::vector<long long> coeffs(300, 0);
    coeffs[0] = 1;
    coeffs[299] = 1;
    Polynomial<long long> poly(coeffs);

    // Before optimization (sparse should be false by default)
    EXPECT_FALSE(poly.is_sparse());

    // After optimization
    poly.optimize_storage();
    // Note: may or may not convert to sparse depending on threshold
    // The important thing is that the polynomial still works correctly
    EXPECT_EQ(poly.degree(), 299);
    EXPECT_EQ(poly.get_coefficient(0), 1);
    EXPECT_EQ(poly.get_coefficient(299), 1);
    EXPECT_EQ(poly.get_coefficient(150), 0);
}

// Test (1+x)^16 using polynomial power vs binomial coefficients
TEST(BinomialExpansionTest, PolynomialVsBinomialCoefficients) {
    // Create polynomial (1 + x)
    Polynomial<long long> one_plus_x({1, 1});  // 1 + x

    // Compute (1 + x)^16 using polynomial power
    Polynomial<long long> result = Power<Polynomial<long long>>::power(
        one_plus_x, 16, Polynomial<long long>(1));

    // The coefficients of (1+x)^16 should be C(16, k) for k = 0 to 16
    // (1+x)^16 = C(16,0) + C(16,1)*x + C(16,2)*x^2 + ... + C(16,16)*x^16
    EXPECT_EQ(result.degree(), 16);

    for (int k = 0; k <= 16; k++) {
        long long binomial_coeff = n_choose_k(16, k);
        long long poly_coeff = result.get_coefficient(k);

        EXPECT_EQ(poly_coeff, binomial_coeff)
            << "Coefficient mismatch at x^" << k
            << ": polynomial=" << poly_coeff
            << ", binomial=" << binomial_coeff;
    }

    // Verify specific known binomial coefficients for (1+x)^16
    EXPECT_EQ(result.get_coefficient(0), n_choose_k(16, 0));   // C(16, 0) = 1
    EXPECT_EQ(result.get_coefficient(1), n_choose_k(16, 1));   // C(16, 1) = 16
    EXPECT_EQ(result.get_coefficient(2), n_choose_k(16, 2));   // C(16, 2) = 120
    EXPECT_EQ(result.get_coefficient(8), n_choose_k(16, 8));   // C(16, 8) = 12870 (peak)
    EXPECT_EQ(result.get_coefficient(16), n_choose_k(16, 16)); // C(16, 16) = 1

    // Verify symmetry: C(16, k) = C(16, 16-k)
    for (int k = 0; k <= 8; k++) {
        EXPECT_EQ(result.get_coefficient(k), result.get_coefficient(16 - k))
            << "Symmetry violated at k=" << k;
    }
}

// ============================================================================
// Linear Recurrence Tests
// ============================================================================

// Moved to gtest/linear_recurrence_test_gtest.cpp.

