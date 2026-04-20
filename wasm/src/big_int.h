#pragma once

#include <compare>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "rational.h"

/**
 * Simple arbitrary-precision signed integer.
 *
 * Representation: sign + base 1e9 limbs (little-endian).
 *
 * This is designed for correctness and WASM portability, not maximum speed.
 */
class BigInt {
   public:
    BigInt();
    explicit BigInt(std::int64_t v);

    static BigInt from_string(const std::string& s, int base);  // base: 0/2/10/16
    std::string to_string(int base) const;                      // base: 2/10/16

    bool is_zero() const;
    int signum() const;  // -1, 0, +1

    int cmp(const BigInt& other) const;

    BigInt operator-() const;

    BigInt add(const BigInt& other) const;
    BigInt sub(const BigInt& other) const;
    BigInt mul(const BigInt& other) const;

    // Floor division and modulo (Python semantics): a = q*b + r, with 0 <= r < |b|
    // when b != 0. Works for negative values too.
    BigInt floordiv(const BigInt& other) const;
    BigInt mod(const BigInt& other) const;
    std::pair<BigInt, BigInt> divmod(const BigInt& other) const;

    static BigInt gcd(BigInt a, BigInt b);
    static BigInt powmod(BigInt base, BigInt exp, const BigInt& mod);
    static BigInt invmod(const BigInt& a, const BigInt& mod);

    // Numeric-style operators built atop the named methods above. These let
    // BigInt be plugged into generic numeric templates (e.g. tsl::Rational).
    // Note on operator/ and operator%: Python-style floor division/modulo.
    // For exact divisions (such as the reductions performed by
    // tsl::Rational::normalize) this coincides with true division.
    BigInt operator+(const BigInt& other) const { return add(other); }
    BigInt operator-(const BigInt& other) const { return sub(other); }
    BigInt operator*(const BigInt& other) const { return mul(other); }
    BigInt operator/(const BigInt& other) const { return floordiv(other); }
    BigInt operator%(const BigInt& other) const { return mod(other); }

    BigInt& operator+=(const BigInt& other) {
        *this = add(other);
        return *this;
    }
    BigInt& operator-=(const BigInt& other) {
        *this = sub(other);
        return *this;
    }
    BigInt& operator*=(const BigInt& other) {
        *this = mul(other);
        return *this;
    }
    BigInt& operator/=(const BigInt& other) {
        *this = floordiv(other);
        return *this;
    }
    BigInt& operator%=(const BigInt& other) {
        *this = mod(other);
        return *this;
    }

    bool operator==(const BigInt& other) const { return cmp(other) == 0; }
    std::strong_ordering operator<=>(const BigInt& other) const {
        const int c = cmp(other);
        if (c < 0) {
            return std::strong_ordering::less;
        }
        if (c > 0) {
            return std::strong_ordering::greater;
        }
        return std::strong_ordering::equal;
    }

    // Best-effort conversion to double (precision limited by double's mantissa).
    explicit operator double() const;

   private:
    static constexpr std::uint32_t LIMB_BASE = 1000000000u;  // 1e9

    // sign_ is -1, 0, +1. When sign_==0, limbs_ is empty.
    int sign_;
    std::vector<std::uint32_t> limbs_;

    void normalize();

    static int cmp_abs(const BigInt& a, const BigInt& b);
    static BigInt add_abs(const BigInt& a, const BigInt& b);  // |a|+|b|, non-negative
    static BigInt sub_abs(const BigInt& a, const BigInt& b);  // |a|-|b|, assuming |a|>=|b|, non-negative

    static BigInt mul_abs(const BigInt& a, const BigInt& b);

    // Divide |a| by uint32 divisor; returns (quotient, remainder).
    static std::pair<BigInt, std::uint32_t> divmod_abs_small(const BigInt& a, std::uint32_t d);

    // Divide |a| by |b| (both non-negative, b != 0); returns (q, r) non-negative.
    static std::pair<BigInt, BigInt> divmod_abs(const BigInt& a, const BigInt& b);

    void mul_small(std::uint32_t m);
    void add_small(std::uint32_t a);

    static int char_to_digit(char c);
    static void parse_prefix_base(const std::string& s_in, int base_in, std::string& out_digits, int& out_base, bool& out_neg);

    static BigInt abs_copy(const BigInt& x);
};

namespace tsl {

// Specialization that lets tsl::Rational<BigInt> work without any changes
// to rational.h itself.
template <>
struct rational_int_traits<BigInt> {
    using widen_type = BigInt;

    static BigInt abs(const BigInt& x) {
        return x.signum() < 0 ? -x : x;
    }

    static BigInt gcd(const BigInt& a, const BigInt& b) {
        return BigInt::gcd(a, b);
    }

    static std::string to_string(const BigInt& x) {
        return x.to_string(10);
    }
};

}  // namespace tsl
