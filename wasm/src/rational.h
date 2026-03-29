#pragma once

#include <compare>
#include <cstdint>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace tsl {

namespace detail {

template <typename T>
concept Integral = std::is_integral_v<T> && !std::is_same_v<T, bool>;

template <Integral T>
using make_unsigned_t = std::make_unsigned_t<T>;

template <Integral T>
constexpr make_unsigned_t<T> unsigned_abs(T x) {
    using U = make_unsigned_t<T>;
    if constexpr (std::is_signed_v<T>) {
        if (x < 0) {
            // Avoid UB on INT_MIN by working in unsigned domain.
            return U(0) - static_cast<U>(x);
        } else {
            return static_cast<U>(x);
        }
    } else {
        return static_cast<U>(x);
    }
}

template <Integral From, Integral To>
constexpr bool is_non_narrowing_integral_conversion_v =
    std::is_convertible_v<From, To> &&
    // Signedness change can be narrowing even if digits match.
    ((std::is_signed_v<From> == std::is_signed_v<To>) ||
     // Allow signed -> unsigned only when From is non-negative representable
     // at compile-time (we can't know values), so treat it as potentially narrowing.
     false) &&
    (std::numeric_limits<To>::digits >= std::numeric_limits<From>::digits);

}  // namespace detail

template <detail::Integral Int>
class Rational {
   public:
    using int_type = Int;

    constexpr Rational() : m_(0), n_(1) {}

    constexpr Rational(Int m) : m_(m), n_(1) { normalize(); }

    constexpr Rational(Int m, Int n) : m_(m), n_(n) { normalize(); }

    template <detail::Integral Other>
    constexpr Rational(Other m)
        requires(std::is_convertible_v<Other, Int>)
        : m_(static_cast<Int>(m)), n_(1) {
        normalize();
    }

    template <detail::Integral OtherInt>
    constexpr Rational(const Rational<OtherInt>& other)
        requires(detail::is_non_narrowing_integral_conversion_v<OtherInt, Int>)
        : m_(static_cast<Int>(other.m())), n_(static_cast<Int>(other.n())) {
        normalize();
    }

    template <detail::Integral OtherInt>
    explicit constexpr Rational(const Rational<OtherInt>& other)
        requires(std::is_convertible_v<OtherInt, Int> &&
                 !detail::is_non_narrowing_integral_conversion_v<OtherInt, Int>)
        : m_(static_cast<Int>(other.m())), n_(static_cast<Int>(other.n())) {
        normalize();
    }

    constexpr Int m() const { return m_; }
    constexpr Int n() const { return n_; }

    constexpr explicit operator double() const {
        return static_cast<double>(m_) / static_cast<double>(n_);
    }

    std::string to_string() const {
        if (n_ == 1) {
            return std::to_string(m_);
        }
        return std::to_string(m_) + "/" + std::to_string(n_);
    }

    constexpr Rational operator+() const { return *this; }
    constexpr Rational operator-() const { return Rational(-m_, n_); }

    constexpr Rational& operator+=(const Rational& other) {
        // a/b + c/d = (ad + cb) / bd
        m_ = m_ * other.n_ + other.m_ * n_;
        n_ = n_ * other.n_;
        normalize();
        return *this;
    }

    constexpr Rational& operator-=(const Rational& other) {
        m_ = m_ * other.n_ - other.m_ * n_;
        n_ = n_ * other.n_;
        normalize();
        return *this;
    }

    constexpr Rational& operator*=(const Rational& other) {
        m_ = m_ * other.m_;
        n_ = n_ * other.n_;
        normalize();
        return *this;
    }

    constexpr Rational& operator/=(const Rational& other) {
        if (other.m_ == 0) {
            throw std::invalid_argument("Rational: division by zero");
        }
        m_ = m_ * other.n_;
        n_ = n_ * other.m_;
        normalize();
        return *this;
    }

    friend constexpr bool operator==(const Rational& a, const Rational& b) {
        return a.m_ == b.m_ && a.n_ == b.n_;
    }

    friend constexpr std::strong_ordering operator<=>(const Rational& a,
                                                      const Rational& b) {
        // Compare a/b and c/d by cross-multiplication.
        // Note: may overflow for large values; this is a simple exact-in-type compare.
        auto lhs = static_cast<std::common_type_t<Int, std::intmax_t>>(a.m_) *
                   static_cast<std::common_type_t<Int, std::intmax_t>>(b.n_);
        auto rhs = static_cast<std::common_type_t<Int, std::intmax_t>>(b.m_) *
                   static_cast<std::common_type_t<Int, std::intmax_t>>(a.n_);
        if (lhs < rhs) {
            return std::strong_ordering::less;
        } else if (lhs > rhs) {
            return std::strong_ordering::greater;
        } else {
            return std::strong_ordering::equal;
        }
    }

   private:
    Int m_;
    Int n_;

    constexpr void normalize() {
        if (n_ == 0) {
            throw std::invalid_argument("Rational: zero denominator");
        }
        if (m_ == 0) {
            n_ = 1;
            return;
        }
        if (n_ < 0) {
            m_ = -m_;
            n_ = -n_;
        }

        using U = detail::make_unsigned_t<Int>;
        U am = detail::unsigned_abs(m_);
        U an = detail::unsigned_abs(n_);
        U g = std::gcd(am, an);
        if (g != 0) {
            U rm = am / g;
            U rn = an / g;
            if constexpr (std::is_signed_v<Int>) {
                if (m_ < 0) {
                    m_ = -static_cast<Int>(rm);
                } else {
                    m_ = static_cast<Int>(rm);
                }
            } else {
                m_ = static_cast<Int>(rm);
            }
            n_ = static_cast<Int>(rn);
        }
    }
};

template <detail::Integral A, detail::Integral B>
using RationalCommon = Rational<std::common_type_t<A, B>>;

template <detail::Integral A, detail::Integral B>
constexpr RationalCommon<A, B> operator+(const Rational<A>& x,
                                        const Rational<B>& y) {
    using C = std::common_type_t<A, B>;
    return Rational<C>(static_cast<C>(x.m()) * static_cast<C>(y.n()) +
                           static_cast<C>(y.m()) * static_cast<C>(x.n()),
                       static_cast<C>(x.n()) * static_cast<C>(y.n()));
}

template <detail::Integral A, detail::Integral B>
constexpr RationalCommon<A, B> operator-(const Rational<A>& x,
                                        const Rational<B>& y) {
    using C = std::common_type_t<A, B>;
    return Rational<C>(static_cast<C>(x.m()) * static_cast<C>(y.n()) -
                           static_cast<C>(y.m()) * static_cast<C>(x.n()),
                       static_cast<C>(x.n()) * static_cast<C>(y.n()));
}

template <detail::Integral A, detail::Integral B>
constexpr RationalCommon<A, B> operator*(const Rational<A>& x,
                                        const Rational<B>& y) {
    using C = std::common_type_t<A, B>;
    return Rational<C>(static_cast<C>(x.m()) * static_cast<C>(y.m()),
                       static_cast<C>(x.n()) * static_cast<C>(y.n()));
}

template <detail::Integral A, detail::Integral B>
constexpr RationalCommon<A, B> operator/(const Rational<A>& x,
                                        const Rational<B>& y) {
    using C = std::common_type_t<A, B>;
    if (y.m() == 0) {
        throw std::invalid_argument("Rational: division by zero");
    }
    return Rational<C>(static_cast<C>(x.m()) * static_cast<C>(y.n()),
                       static_cast<C>(x.n()) * static_cast<C>(y.m()));
}

}  // namespace tsl

