#pragma once

#include <compare>
#include <cstdint>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

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

// Primary trait template: the customization point for Rational<Int>.
//
// Specialize this for any user-defined integer-like type (e.g. a big-int) to
// opt into tsl::Rational<T>. A specialization must provide:
//   - using widen_type = ...;        // type used for cross-multiplication in
//                                    //   operator<=>; may equal T itself.
//   - static T abs(const T&);        // absolute value, returned in T.
//   - static T gcd(const T&, const T&); // gcd, returned as a non-negative T.
//   - static std::string to_string(const T&);
//
// The primary template is intentionally left undefined so that any reference
// to `rational_int_traits<T>` with an unsupported T yields a clear error.
template <class T>
struct rational_int_traits;

// Default: built-in integral types (but not bool). Preserves the previous
// std::make_unsigned / std::gcd / std::to_string behavior exactly.
template <class T>
    requires(std::is_integral_v<T> && !std::is_same_v<T, bool>)
struct rational_int_traits<T> {
    using widen_type = std::common_type_t<T, std::intmax_t>;

    static constexpr T abs(T x) {
        using U = std::make_unsigned_t<T>;
        U u = detail::unsigned_abs(x);
        return static_cast<T>(u);
    }

    static constexpr T gcd(T a, T b) {
        using U = std::make_unsigned_t<T>;
        U g = std::gcd(detail::unsigned_abs(a), detail::unsigned_abs(b));
        return static_cast<T>(g);
    }

    static std::string to_string(const T& x) { return std::to_string(x); }
};

// Concept satisfied by any T with a usable rational_int_traits<T>.
template <class T>
concept RationalInt = requires {
    typename rational_int_traits<T>::widen_type;
    { rational_int_traits<T>::abs(std::declval<T>()) } -> std::convertible_to<T>;
    { rational_int_traits<T>::gcd(std::declval<T>(), std::declval<T>()) } -> std::convertible_to<T>;
    { rational_int_traits<T>::to_string(std::declval<const T&>()) } -> std::convertible_to<std::string>;
};

template <RationalInt Int>
class Rational {
   public:
    using int_type = Int;

    constexpr Rational() : m_(Int{}), n_(Int(1)) {}

    constexpr Rational(Int m) : m_(std::move(m)), n_(Int(1)) { normalize(); }

    constexpr Rational(Int m, Int n) : m_(std::move(m)), n_(std::move(n)) { normalize(); }

    template <detail::Integral Other>
    constexpr Rational(Other m)
        requires(std::is_convertible_v<Other, Int>)
        : m_(static_cast<Int>(m)), n_(Int(1)) {
        normalize();
    }

    template <detail::Integral OtherInt>
    constexpr Rational(const Rational<OtherInt>& other)
        requires(detail::Integral<Int> &&
                 detail::is_non_narrowing_integral_conversion_v<OtherInt, Int>)
        : m_(static_cast<Int>(other.m())), n_(static_cast<Int>(other.n())) {
        normalize();
    }

    template <detail::Integral OtherInt>
    explicit constexpr Rational(const Rational<OtherInt>& other)
        requires(detail::Integral<Int> &&
                 std::is_convertible_v<OtherInt, Int> &&
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
        using Tr = rational_int_traits<Int>;
        if (n_ == Int(1)) {
            return Tr::to_string(m_);
        }
        return Tr::to_string(m_) + "/" + Tr::to_string(n_);
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
        if (other.m_ == Int{}) {
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
        // Note: may overflow for built-in `Int`; for user-defined arbitrary
        // precision types, the trait's widen_type should equal Int itself.
        using Widen = typename rational_int_traits<Int>::widen_type;
        auto lhs = static_cast<Widen>(a.m_) * static_cast<Widen>(b.n_);
        auto rhs = static_cast<Widen>(b.m_) * static_cast<Widen>(a.n_);
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
        using Tr = rational_int_traits<Int>;
        if (n_ == Int{}) {
            throw std::invalid_argument("Rational: zero denominator");
        }
        if (m_ == Int{}) {
            n_ = Int(1);
            return;
        }
        if (n_ < Int{}) {
            m_ = -m_;
            n_ = -n_;
        }
        Int g = Tr::gcd(Tr::abs(m_), Tr::abs(n_));
        if (!(g == Int{}) && !(g == Int(1))) {
            m_ = m_ / g;
            n_ = n_ / g;
        }
    }
};

template <RationalInt A, RationalInt B>
using RationalCommon = Rational<std::common_type_t<A, B>>;

template <RationalInt A, RationalInt B>
constexpr RationalCommon<A, B> operator+(const Rational<A>& x,
                                        const Rational<B>& y) {
    using C = std::common_type_t<A, B>;
    return Rational<C>(static_cast<C>(x.m()) * static_cast<C>(y.n()) +
                           static_cast<C>(y.m()) * static_cast<C>(x.n()),
                       static_cast<C>(x.n()) * static_cast<C>(y.n()));
}

template <RationalInt A, RationalInt B>
constexpr RationalCommon<A, B> operator-(const Rational<A>& x,
                                        const Rational<B>& y) {
    using C = std::common_type_t<A, B>;
    return Rational<C>(static_cast<C>(x.m()) * static_cast<C>(y.n()) -
                           static_cast<C>(y.m()) * static_cast<C>(x.n()),
                       static_cast<C>(x.n()) * static_cast<C>(y.n()));
}

template <RationalInt A, RationalInt B>
constexpr RationalCommon<A, B> operator*(const Rational<A>& x,
                                        const Rational<B>& y) {
    using C = std::common_type_t<A, B>;
    return Rational<C>(static_cast<C>(x.m()) * static_cast<C>(y.m()),
                       static_cast<C>(x.n()) * static_cast<C>(y.n()));
}

template <RationalInt A, RationalInt B>
constexpr RationalCommon<A, B> operator/(const Rational<A>& x,
                                        const Rational<B>& y) {
    using C = std::common_type_t<A, B>;
    if (y.m() == C{}) {
        throw std::invalid_argument("Rational: division by zero");
    }
    return Rational<C>(static_cast<C>(x.m()) * static_cast<C>(y.n()),
                       static_cast<C>(x.n()) * static_cast<C>(y.m()));
}

}  // namespace tsl
