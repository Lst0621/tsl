#include <gtest/gtest.h>

#include <type_traits>

#include "number/rational.h"

TEST(RationalTest, DefaultCtorIsZeroOverOne) {
    tsl::Rational<int> r;
    EXPECT_EQ(r.m(), 0);
    EXPECT_EQ(r.n(), 1);
}

TEST(RationalTest, OneArgCtorIsOverOne) {
    tsl::Rational<int> r(5);
    EXPECT_EQ(r.m(), 5);
    EXPECT_EQ(r.n(), 1);
}

TEST(RationalTest, NormalizesByGcdAndDenomSign) {
    {
        tsl::Rational<int> r(2, 4);
        EXPECT_EQ(r.m(), 1);
        EXPECT_EQ(r.n(), 2);
    }
    {
        tsl::Rational<int> r(-2, 4);
        EXPECT_EQ(r.m(), -1);
        EXPECT_EQ(r.n(), 2);
    }
    {
        tsl::Rational<int> r(2, -4);
        EXPECT_EQ(r.m(), -1);
        EXPECT_EQ(r.n(), 2);
    }
    {
        tsl::Rational<int> r(0, -7);
        EXPECT_EQ(r.m(), 0);
        EXPECT_EQ(r.n(), 1);
    }
}

TEST(RationalTest, Arithmetic) {
    const tsl::Rational<int> a(1, 2);
    const tsl::Rational<int> b(1, 3);

    {
        auto s = a + b;
        EXPECT_EQ(s.m(), 5);
        EXPECT_EQ(s.n(), 6);
    }
    {
        auto d = a - b;
        EXPECT_EQ(d.m(), 1);
        EXPECT_EQ(d.n(), 6);
    }
    {
        auto p = a * b;
        EXPECT_EQ(p.m(), 1);
        EXPECT_EQ(p.n(), 6);
    }
    {
        auto q = a / b;
        EXPECT_EQ(q.m(), 3);
        EXPECT_EQ(q.n(), 2);
    }
}

TEST(RationalTest, MixedTypeOperatorsUseCommonType) {
    const tsl::Rational<int> a(1, 2);
    const tsl::Rational<long long> b(2, 3);

    using P = decltype(a * b);
    static_assert(std::is_same_v<typename P::int_type,
                                 std::common_type_t<int, long long>>);

    auto p = a * b;
    EXPECT_EQ(p.m(), 1);
    EXPECT_EQ(p.n(), 3);
}

TEST(RationalTest, ImplicitWideningConversionAllowed) {
    tsl::Rational<int> a(1, 2);
    tsl::Rational<long long> b = a;
    EXPECT_EQ(b.m(), 1);
    EXPECT_EQ(b.n(), 2);
}

TEST(RationalTest, ToDoubleWholeNumber) {
    tsl::Rational<int> r(3);
    EXPECT_DOUBLE_EQ(static_cast<double>(r), 3.0);
}

TEST(RationalTest, ToDoubleFraction) {
    tsl::Rational<int> r(1, 4);
    EXPECT_DOUBLE_EQ(static_cast<double>(r), 0.25);
}

TEST(RationalTest, ToDoubleNegative) {
    tsl::Rational<int> r(-3, 2);
    EXPECT_DOUBLE_EQ(static_cast<double>(r), -1.5);
}

TEST(RationalTest, ToStringWholeNumber) {
    tsl::Rational<int> r(5);
    EXPECT_EQ(r.to_string(), "5");
}

TEST(RationalTest, ToStringFraction) {
    tsl::Rational<int> r(1, 3);
    EXPECT_EQ(r.to_string(), "1/3");
}

TEST(RationalTest, ToStringNegative) {
    tsl::Rational<int> r(-2, 5);
    EXPECT_EQ(r.to_string(), "-2/5");
}
