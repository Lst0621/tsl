#include <gtest/gtest.h>

#include <stdexcept>

#include "big_int.h"
#include "rational.h"

using BigRational = tsl::Rational<BigInt>;

TEST(BigRationalTest, NormalizesAndToString) {
    BigRational r(BigInt::from_string("2", 10), BigInt::from_string("4", 10));
    EXPECT_EQ(r.to_string(), "1/2");

    BigRational n(BigInt::from_string("-2", 10), BigInt::from_string("4", 10));
    EXPECT_EQ(n.to_string(), "-1/2");

    BigRational d(BigInt::from_string("2", 10), BigInt::from_string("-4", 10));
    EXPECT_EQ(d.to_string(), "-1/2");

    BigRational z(BigInt::from_string("0", 10), BigInt::from_string("-7", 10));
    EXPECT_EQ(z.to_string(), "0");
}

TEST(BigRationalTest, Arithmetic) {
    const BigRational a(BigInt::from_string("1", 10), BigInt::from_string("2", 10));
    const BigRational b(BigInt::from_string("1", 10), BigInt::from_string("3", 10));

    EXPECT_EQ((a + b).to_string(), "5/6");
    EXPECT_EQ((a - b).to_string(), "1/6");
    EXPECT_EQ((a * b).to_string(), "1/6");
    EXPECT_EQ((a / b).to_string(), "3/2");
}

TEST(BigRationalTest, RejectsZeroDenominator) {
    EXPECT_THROW(BigRational(BigInt::from_string("1", 10), BigInt::from_string("0", 10)),
                 std::invalid_argument);
}

TEST(BigRationalTest, LargeReduction) {
    // (10^30)/(10^12) => 10^18
    const BigInt m = BigInt::from_string("1000000000000000000000000000000", 10);
    const BigInt n = BigInt::from_string("1000000000000", 10);
    BigRational r(m, n);
    EXPECT_EQ(r.to_string(), "1000000000000000000");
}

TEST(BigRationalTest, Comparison) {
    const BigRational half(BigInt::from_string("1", 10), BigInt::from_string("2", 10));
    const BigRational third(BigInt::from_string("1", 10), BigInt::from_string("3", 10));
    EXPECT_TRUE(third < half);
    EXPECT_TRUE(half > third);
    EXPECT_FALSE(half == third);
    EXPECT_TRUE(half == BigRational(BigInt::from_string("2", 10), BigInt::from_string("4", 10)));
}

TEST(BigRationalTest, CompoundAssignment) {
    BigRational r(BigInt::from_string("1", 10), BigInt::from_string("2", 10));
    r += BigRational(BigInt::from_string("1", 10), BigInt::from_string("3", 10));
    EXPECT_EQ(r.to_string(), "5/6");
    r *= BigRational(BigInt::from_string("6", 10), BigInt::from_string("5", 10));
    EXPECT_EQ(r.to_string(), "1");
}
