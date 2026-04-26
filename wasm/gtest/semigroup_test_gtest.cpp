#include <gtest/gtest.h>

#include "algebra/semigroup.h"
#include "number/modular_number.h"
#include "string/lang_string.h"

#include <algorithm>
#include <string>
#include <vector>

TEST(SemigroupTest, SemigroupPower) {
    ModularNumber a(2, 7);
    ModularNumber a3 = semigroup_power(a, 3);
    EXPECT_EQ(a3.get_value(), 1);  // 2^3 mod 7 = 8 mod 7 = 1
}

TEST(SemigroupTest, IsAssociativeModN) {
    std::vector<ModularNumber> elements;
    for (int i = 0; i < 5; i++) {
        elements.push_back(ModularNumber(i, 5));
    }
    EXPECT_TRUE(is_associative(elements));
}

TEST(SemigroupTest, GetIdempotentPower) {
    ModularNumber zero(0, 5);
    auto opt = get_idempotent_power(zero, 20);
    ASSERT_TRUE(opt.has_value());
    EXPECT_EQ(opt->first, 1u);
    EXPECT_EQ(opt->second.get_value(), 0);
}

TEST(SemigroupTest, GetAllIdempotentElements) {
    std::vector<ModularNumber> elements = {
        ModularNumber(0, 3), ModularNumber(1, 3), ModularNumber(2, 3)};
    auto idem = get_all_idempotent_elements(elements);
    EXPECT_EQ(idem.size(), 2u);  // 0 and 1 are idempotents under multiplication mod 3
    std::vector<long long> vals = {idem[0].get_value(), idem[1].get_value()};
    std::sort(vals.begin(), vals.end());
    EXPECT_EQ(vals[0], 0);
    EXPECT_EQ(vals[1], 1);
}

TEST(SemigroupTest, IsAbelian) {
    std::vector<ModularNumber> elements = {
        ModularNumber(0, 3), ModularNumber(1, 3), ModularNumber(2, 3)};
    EXPECT_TRUE(is_abelian(elements));
}

TEST(SemigroupTest, IsMonoidAndIsGroup) {
    std::vector<ModularNumber> elements = {
        ModularNumber(0, 3), ModularNumber(1, 3), ModularNumber(2, 3)};
    auto identity_opt = is_monoid(elements);
    ASSERT_TRUE(identity_opt.has_value());
    EXPECT_EQ(identity_opt->get_value(), 1);  // identity under multiplication
    EXPECT_FALSE(is_group(elements).has_value());  // 0 has no inverse
}

struct SuffixKString {
    std::string s;
    size_t k;
    SuffixKString(std::string s_, size_t k_) : s(std::move(s_)), k(k_) {
        if (s.size() > k) {
            s = s.substr(s.size() - k);
        }
    }
    SuffixKString operator*(const SuffixKString& other) const {
        return SuffixKString(s + other.s, k);
    }
    bool operator==(const SuffixKString& other) const {
        return s == other.s;
    }
};

TEST(SemigroupTest, DefiniteK) {
    std::vector<std::string> alphabet = lang::get_alphabet_from_strings({"abcd"});
    size_t k = 3;
    std::vector<SuffixKString> generators;
    for (const std::string& c : alphabet) {
        generators.push_back(SuffixKString(c, k));
    }
    std::vector<SuffixKString> strs = generate_semigroup(generators);
    EXPECT_EQ(get_highest_idempotent_power(strs), k);
    EXPECT_EQ(get_definite_k(strs), static_cast<int>(k));
    EXPECT_FALSE(is_abelian(strs));
    EXPECT_FALSE(is_monoid(strs).has_value());
    EXPECT_FALSE(is_group(strs).has_value());
}

TEST(SemigroupTest, GroupPowerModularNumberPositive) {
    const long long p = 7;
    ModularNumber one(1, p);
    ModularNumber a(2, p);
    auto inverse_op = [](const ModularNumber& x) {
        return ModularNumber(1, x.get_modulus()) / x;
    };
    ModularNumber a2 = group_power(a, 2, one, inverse_op);
    EXPECT_EQ(a2.get_value(), 4);  // 2^2 mod 7
    ModularNumber a0 = group_power(a, 0, one, inverse_op);
    EXPECT_TRUE(a0 == one);
}

TEST(SemigroupTest, GroupPowerModularNumberNegative) {
    const long long p = 7;
    ModularNumber one(1, p);
    ModularNumber a(2, p);
    auto inverse_op = [](const ModularNumber& x) {
        return ModularNumber(1, x.get_modulus()) / x;
    };
    ModularNumber a_neg1 = group_power(a, -1, one, inverse_op);
    EXPECT_EQ(a_neg1.get_value(), 4);  // 2^(-1) mod 7 = 4 (since 2*4=8 equiv 1)
    ModularNumber a_neg2 = group_power(a, -2, one, inverse_op);
    EXPECT_EQ(a_neg2.get_value(), 2);  // 2^(-2) = 4^2 = 16 mod 7 = 2
    ModularNumber a2_times_a_neg2 = a_neg2 * group_power(a, 2, one, inverse_op);
    EXPECT_TRUE(a2_times_a_neg2 == one);
}
