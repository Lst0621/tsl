#include <gtest/gtest.h>

#include <stdexcept>

#include "number/big_int.h"

TEST(BigIntTest, ParseAndToStringDecimal) {
    BigInt a = BigInt::from_string("0", 0);
    EXPECT_EQ(a.to_string(10), "0");

    BigInt b = BigInt::from_string("123456789012345678901234567890", 10);
    EXPECT_EQ(b.to_string(10), "123456789012345678901234567890");

    BigInt c = BigInt::from_string("-42", 0);
    EXPECT_EQ(c.to_string(10), "-42");
}

TEST(BigIntTest, ParsePrefixHexAndBinAuto) {
    BigInt h = BigInt::from_string("0x10", 0);
    EXPECT_EQ(h.to_string(10), "16");
    EXPECT_EQ(h.to_string(16), "10");

    BigInt b = BigInt::from_string("0b101101", 0);
    EXPECT_EQ(b.to_string(10), "45");
    EXPECT_EQ(b.to_string(2), "101101");
}

TEST(BigIntTest, ArithmeticBasic) {
    BigInt a = BigInt::from_string("999999999999999999999", 10);
    BigInt b = BigInt::from_string("2", 10);
    EXPECT_EQ(a.add(b).to_string(10), "1000000000000000000001");
    EXPECT_EQ(a.sub(b).to_string(10), "999999999999999999997");
    EXPECT_EQ(a.mul(b).to_string(10), "1999999999999999999998");
}

TEST(BigIntTest, FloorDivAndModPythonSemantics) {
    BigInt a = BigInt::from_string("-7", 10);
    BigInt b = BigInt::from_string("3", 10);
    EXPECT_EQ(a.floordiv(b).to_string(10), "-3");
    EXPECT_EQ(a.mod(b).to_string(10), "2");
}

TEST(BigIntTest, GcdPowModInvMod) {
    BigInt a = BigInt::from_string("54", 10);
    BigInt b = BigInt::from_string("24", 10);
    EXPECT_EQ(BigInt::gcd(a, b).to_string(10), "6");

    BigInt pm = BigInt::powmod(BigInt::from_string("2", 10),
                               BigInt::from_string("10", 10),
                               BigInt::from_string("1000", 10));
    EXPECT_EQ(pm.to_string(10), "24");

    BigInt inv = BigInt::invmod(BigInt::from_string("3", 10),
                                BigInt::from_string("11", 10));
    EXPECT_EQ(inv.to_string(10), "4");

    EXPECT_THROW(BigInt::invmod(BigInt::from_string("2", 10),
                                BigInt::from_string("4", 10)),
                 std::invalid_argument);
}

