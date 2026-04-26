#pragma once

#include <cmath>
#include <concepts>
#include <stdexcept>
#include <string>
#include <vector>

#include "algebra/matrix.h"
#include "number/rational.h"

namespace tsl {

/**
 * Concept for scalar types usable as Complex components.
 * Requires the standard field operations, default construction, T(1), and
 * an ordering (needed for to_string sign check).
 */
template <typename T>
concept ComplexScalar =
    std::default_initializable<T> &&
    requires(T a, T b) {
        { a + b } -> std::convertible_to<T>;
        { a - b } -> std::convertible_to<T>;
        { a * b } -> std::convertible_to<T>;
        { a / b } -> std::convertible_to<T>;
        { -a } -> std::convertible_to<T>;
        { a == b } -> std::convertible_to<bool>;
        { a >= b } -> std::convertible_to<bool>;
        T(1);
    };

/**
 * Complex number z = a + bi represented internally as two scalars.
 *
 * The isomorphism to 2x2 rotation-scaling matrices is:
 *
 *   z = a + bi  <-->  M(z) = | a  -b |
 *                             | b   a |
 *
 * Arithmetic operators (+, -, *) delegate through this matrix representation.
 * Division uses z1/z2 = z1 * conj(z2) / |z2|^2, which reuses operator*.
 *
 * @tparam T  Scalar type satisfying ComplexScalar: float, double, long double,
 *            or Rational<Int> (Gaussian rationals).
 */
template <ComplexScalar T>
class Complex {
   public:
    constexpr Complex() : re_(T{}), im_(T{}) {}

    constexpr explicit Complex(T re) : re_(re), im_(T{}) {}

    constexpr Complex(T re, T im) : re_(re), im_(im) {}

    // -------------------------------------------------------------------------
    // Accessors
    // -------------------------------------------------------------------------

    constexpr T real() const {
        return re_;
    }

    constexpr T imag() const {
        return im_;
    }

    // -------------------------------------------------------------------------
    // Matrix conversion
    // -------------------------------------------------------------------------

    /**
     * Embed this complex number into the 2x2 rotation-scaling matrix:
     *   | a  -b |
     *   | b   a |
     */
    Matrix<T> to_matrix() const {
        return Matrix<T>(
            std::vector<std::vector<T>>{{re_, -im_}, {im_, re_}});
    }

    /**
     * Recover a Complex<T> from a 2x2 rotation-scaling matrix.
     * Reads a = M(0,0), b = M(1,0).
     * Throws if matrix is not 2x2.
     */
    static Complex<T> from_matrix(const Matrix<T>& m) {
        if (m.get_rows() != 2 || m.get_cols() != 2) {
            throw std::invalid_argument(
                "Complex::from_matrix: matrix must be 2x2");
        }
        return Complex<T>(m.at(0, 0), m.at(1, 0));
    }

    // -------------------------------------------------------------------------
    // Arithmetic operators (all route through the matrix representation)
    // -------------------------------------------------------------------------

    Complex operator+(const Complex& o) const {
        return from_matrix(to_matrix() + o.to_matrix());
    }

    Complex operator-(const Complex& o) const {
        return from_matrix(to_matrix() - o.to_matrix());
    }

    Complex operator*(const Complex& o) const {
        return from_matrix(to_matrix() * o.to_matrix());
    }

    /**
     * Division: z1/z2 = z1 * conj(z2) / |z2|^2.
     * The multiplication step uses operator*, which goes through the matrix path.
     * Throws on division by zero.
     */
    Complex operator/(const Complex& o) const {
        const T denom = o.abs_sq();
        if (denom == T{}) {
            throw std::invalid_argument("Complex: division by zero");
        }
        const Complex num = (*this) * o.conjugate();
        return Complex(num.re_ / denom, num.im_ / denom);
    }

    Complex operator-() const {
        return Complex(-re_, -im_);
    }

    Complex& operator+=(const Complex& o) {
        *this = *this + o;
        return *this;
    }

    Complex& operator-=(const Complex& o) {
        *this = *this - o;
        return *this;
    }

    Complex& operator*=(const Complex& o) {
        *this = *this * o;
        return *this;
    }

    Complex& operator/=(const Complex& o) {
        *this = *this / o;
        return *this;
    }

    // -------------------------------------------------------------------------
    // Comparison
    // -------------------------------------------------------------------------

    bool operator==(const Complex& o) const {
        return re_ == o.re_ && im_ == o.im_;
    }

    bool operator!=(const Complex& o) const {
        return !(*this == o);
    }

    // -------------------------------------------------------------------------
    // Complex-specific operations
    // -------------------------------------------------------------------------

    /**
     * Complex conjugate via matrix transpose:
     *   M(z)^T = | a   b |  -->  a - bi
     *             | -b  a |
     */
    Complex conjugate() const {
        return from_matrix(to_matrix().transpose());
    }

    /**
     * Modulus |z| = sqrt(real(z * conj(z))).
     * For floating-point T returns T; for other scalars (e.g. Rational)
     * returns double via explicit cast.
     */
    auto abs() const {
        const T sq = ((*this) * conjugate()).real();
        if constexpr (std::floating_point<T>) {
            return std::sqrt(sq);
        } else {
            return std::sqrt(static_cast<double>(sq));
        }
    }

    // -------------------------------------------------------------------------
    // String representation
    // -------------------------------------------------------------------------

    std::string to_string() const {
        std::string s = scalar_to_string(re_);
        if (im_ >= T{}) {
            s += "+" + scalar_to_string(im_) + "i";
        } else {
            s += scalar_to_string(im_) + "i";
        }
        return s;
    }

   private:
    T re_;
    T im_;

    /** Squared modulus, used internally by operator/. */
    T abs_sq() const {
        return (*this * conjugate()).real();
    }

    static std::string scalar_to_string(const T& v) {
        if constexpr (std::floating_point<T>) {
            return std::to_string(v);
        } else {
            return v.to_string();
        }
    }
};

// Convenience aliases
using Complexf  = Complex<float>;
using Complexd  = Complex<double>;
using Complexld = Complex<long double>;
// Gaussian rationals: a + bi where a, b ∈ ℚ (represented as Rational<long long>)
using QI = Complex<Rational<long long>>;

}  // namespace tsl
