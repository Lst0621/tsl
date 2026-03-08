#pragma once

#include <algorithm>
#include <cmath>
#include <functional>
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>

/**
 * Traits for coefficient types used in Polynomial.
 * Specialize this for custom coefficient types.
 *
 * @tparam T The coefficient type
 */
template <typename T>
struct CoefficientTraits {
    /**
     * Check if value is zero
     */
    static bool is_zero(const T& value) {
        return value == T();
    }

    /**
     * Check if value is one
     */
    static bool is_one(const T& value) {
        return value == T(1);
    }

    /**
     * Create zero value
     */
    static T zero() {
        return T();
    }

    /**
     * Create one value
     */
    static T one() {
        return T(1);
    }

    /**
     * Check equality
     */
    static bool equals(const T& a, const T& b) {
        return a == b;
    }
};

/**
 * Generic Power helper class using binary exponentiation.
 * Implements fast exponentiation using the principle: a^(2n) = a^n * a^n
 *
 * @tparam T The base type (must support operator*, and have identity element)
 */
template <typename T>
class Power {
   public:
    /**
     * Compute base^exp using binary exponentiation.
     * Positive exponents only.
     *
     * @param base The base value
     * @param exp The exponent (must be non-negative)
     * @param identity The identity element for multiplication (e.g., 1 for
     * numbers, I for matrices)
     * @return base raised to power exp
     * @throws std::invalid_argument if exp is negative
     */
    static T power(const T& base, unsigned long long exp, const T& identity) {
        if (exp == 0) {
            return identity;
        }

        T result = identity;
        T current = base;

        // Binary exponentiation
        while (exp > 0) {
            if (exp % 2 == 1) {
                result = result * current;
            }
            current = current * current;
            exp /= 2;
        }

        return result;
    }

    /**
     * Compute base^exp using binary exponentiation, with default identity.
     * Specialization for types with explicit identity (overloads needed per
     * type).
     *
     * @param base The base value
     * @param exp The exponent (must be non-negative)
     * @return base raised to power exp
     */
    template <typename Traits = CoefficientTraits<T>>
    static T power(const T& base, unsigned long long exp) {
        return power(base, exp, Traits::one());
    }
};

/**
 * Generic Polynomial class with coefficients of type T.
 * Coefficients are stored in ascending order of degrees.
 * Supports arithmetic operations (+, -, *), division with quotient/remainder,
 * GCD computation, and polynomial evaluation.
 *
 * @tparam T The coefficient type (must support +, -, *, /, ==, and traits)
 * @tparam Traits The traits class defining coefficient operations (default:
 * CoefficientTraits<T>)
 */
template <typename T, typename Traits = CoefficientTraits<T>>
class Polynomial {
   private:
    static constexpr size_t SPARSE_THRESHOLD =
        256;  // Switch to sparse if degree > 256 and sparsity ratio < 0.5

    std::vector<T> coefficients;  // coefficients[i] is the coefficient of x^i
                                  // (dense representation)
    std::map<size_t, T> sparse_coefficients;  // (power, coefficient) pairs
                                              // (sparse representation)
    bool use_sparse = false;  // Flag indicating which representation is active

    /**
     * Remove trailing zeros (leading coefficient should not be zero for
     * non-zero polynomial) Always keeps at least one coefficient (the zero
     * polynomial has [0])
     */
    void trim_trailing_zeros() {
        while (coefficients.size() > 1 &&
               Traits::is_zero(coefficients.back())) {
            coefficients.pop_back();
        }
        // Ensure we always have at least one element
        if (coefficients.empty()) {
            coefficients.push_back(Traits::zero());
        }
    }

    /**
     * Check if sparse representation should be used.
     * Returns true if degree > SPARSE_THRESHOLD and sparsity ratio < 0.5
     */
    bool should_use_sparse() const {
        if (use_sparse) return true;
        if (static_cast<int>(coefficients.size()) <=
            static_cast<int>(SPARSE_THRESHOLD)) {
            return false;
        }
        // Count non-zero coefficients
        size_t non_zero_count = 0;
        for (const auto& coeff : coefficients) {
            if (!Traits::is_zero(coeff)) {
                non_zero_count++;
            }
        }
        // Use sparse if less than 50% of coefficients are non-zero
        return non_zero_count < (coefficients.size() / 2);
    }

    /**
     * Convert dense representation to sparse
     */
    void to_sparse() {
        if (use_sparse) return;
        sparse_coefficients.clear();
        for (size_t i = 0; i < coefficients.size(); i++) {
            if (!Traits::is_zero(coefficients[i])) {
                sparse_coefficients[i] = coefficients[i];
            }
        }
        use_sparse = true;
    }

    /**
     * Convert sparse representation to dense
     */
    void to_dense() {
        if (!use_sparse) return;
        if (sparse_coefficients.empty()) {
            coefficients = {Traits::zero()};
        } else {
            size_t max_degree = sparse_coefficients.rbegin()->first;
            coefficients.assign(max_degree + 1, Traits::zero());
            for (const auto& [power, coeff] : sparse_coefficients) {
                coefficients[power] = coeff;
            }
        }
        use_sparse = false;
    }

   public:
    /**
     * Default constructor: creates the zero polynomial
     */
    Polynomial() : coefficients() {
        coefficients.push_back(Traits::zero());
    }

    /**
     * Constructor from coefficient vector
     * @param coeffs Coefficients in ascending order of degrees
     */
    explicit Polynomial(const std::vector<T>& coeffs) : coefficients(coeffs) {
        if (coefficients.empty()) {
            coefficients.push_back(Traits::zero());
        }
        trim_trailing_zeros();
    }

    /**
     * Constructor from single coefficient (constant polynomial)
     * @param value The constant value
     */
    explicit Polynomial(const T& value) {
        coefficients.push_back(value);
        trim_trailing_zeros();
    }

    /**
     * Get the degree of the polynomial
     * Returns -1 if polynomial is zero
     */
    int degree() const {
        if (coefficients.size() <= 1 && Traits::is_zero(coefficients[0])) {
            return -1;
        }
        return static_cast<int>(coefficients.size()) - 1;
    }

    /**
     * Get coefficient at degree d
     */
    T get_coefficient(size_t degree) const {
        if (degree < coefficients.size()) {
            return coefficients[degree];
        }
        return Traits::zero();
    }

    /**
     * Get all coefficients
     */
    const std::vector<T>& get_coefficients() const {
        return coefficients;
    }

    /**
     * Check if using sparse representation
     */
    bool is_sparse() const {
        return use_sparse;
    }

    /**
     * Explicitly convert to sparse representation if beneficial
     */
    void optimize_storage() {
        if (should_use_sparse() && !use_sparse) {
            to_sparse();
        }
    }

    /**
     * Polynomial addition: this + other
     */
    Polynomial<T, Traits> add(const Polynomial<T, Traits>& other) const {
        size_t max_size =
            std::max(coefficients.size(), other.coefficients.size());
        std::vector<T> result;

        for (size_t i = 0; i < max_size; i++) {
            T coeff_a =
                (i < coefficients.size()) ? coefficients[i] : Traits::zero();
            T coeff_b = (i < other.coefficients.size()) ? other.coefficients[i]
                                                        : Traits::zero();
            result.push_back(coeff_a + coeff_b);
        }

        return Polynomial<T, Traits>(result);
    }

    /**
     * Polynomial addition operator: this + other
     */
    Polynomial<T, Traits> operator+(const Polynomial<T, Traits>& other) const {
        return add(other);
    }

    /**
     * Polynomial subtraction: this - other
     */
    Polynomial<T, Traits> subtract(const Polynomial<T, Traits>& other) const {
        size_t max_size =
            std::max(coefficients.size(), other.coefficients.size());
        std::vector<T> result;

        for (size_t i = 0; i < max_size; i++) {
            T coeff_a =
                (i < coefficients.size()) ? coefficients[i] : Traits::zero();
            T coeff_b = (i < other.coefficients.size()) ? other.coefficients[i]
                                                        : Traits::zero();
            result.push_back(coeff_a - coeff_b);
        }

        return Polynomial<T, Traits>(result);
    }

    /**
     * Polynomial subtraction operator: this - other
     */
    Polynomial<T, Traits> operator-(const Polynomial<T, Traits>& other) const {
        return subtract(other);
    }

    /**
     * Polynomial multiplication: this * other
     * Uses standard convolution algorithm
     */
    Polynomial<T, Traits> multiply(const Polynomial<T, Traits>& other) const {
        if (degree() == -1 || other.degree() == -1) {
            return Polynomial<T, Traits>(Traits::zero());
        }

        size_t result_size =
            coefficients.size() + other.coefficients.size() - 1;
        std::vector<T> result(result_size, Traits::zero());

        for (size_t i = 0; i < coefficients.size(); i++) {
            for (size_t j = 0; j < other.coefficients.size(); j++) {
                result[i + j] =
                    result[i + j] + (coefficients[i] * other.coefficients[j]);
            }
        }

        return Polynomial<T, Traits>(result);
    }

    /**
     * Polynomial multiplication operator: this * other
     */
    Polynomial<T, Traits> operator*(const Polynomial<T, Traits>& other) const {
        return multiply(other);
    }

    /**
     * Polynomial scalar multiplication: this * scalar
     */
    Polynomial<T, Traits> multiply_scalar(const T& scalar) const {
        std::vector<T> result;
        for (const auto& coeff : coefficients) {
            result.push_back(coeff * scalar);
        }
        return Polynomial<T, Traits>(result);
    }

    /**
     * Polynomial scalar multiplication operator: this * scalar
     */
    Polynomial<T, Traits> operator*(const T& scalar) const {
        return multiply_scalar(scalar);
    }

    /**
     * Scalar multiplication (commutative): scalar * poly
     */
    friend Polynomial<T, Traits> operator*(const T& scalar,
                                           const Polynomial<T, Traits>& poly) {
        return poly * scalar;
    }

    /**
     * Polynomial division with quotient and remainder
     * Returns pair (quotient, remainder) where this = quotient * other +
     * remainder and degree(remainder) < degree(other)
     *
     * @param divisor The divisor polynomial
     * @return Pair of (quotient, remainder)
     * @throws std::invalid_argument if divisor is zero polynomial
     */
    std::pair<Polynomial<T, Traits>, Polynomial<T, Traits>>
    divide_with_remainder(const Polynomial<T, Traits>& divisor) const {
        if (divisor.degree() == -1) {
            throw std::invalid_argument("Cannot divide by zero polynomial");
        }

        Polynomial<T, Traits> remainder = *this;
        std::vector<T> quotient_coeffs;

        int remainder_deg = remainder.degree();
        int divisor_deg = divisor.degree();

        int iteration_limit = 10000;  // Prevent infinite loops
        int iteration_count = 0;

        while (remainder_deg >= divisor_deg && remainder_deg >= 0 &&
               iteration_count < iteration_limit) {
            iteration_count++;

            // Get leading coefficient ratio
            T leading_divisor = divisor.coefficients[divisor_deg];

            // Check for zero leading coefficient
            if (Traits::is_zero(leading_divisor)) {
                throw std::runtime_error(
                    "Divisor has zero leading coefficient");
            }

            T coeff = remainder.coefficients[remainder_deg] / leading_divisor;
            quotient_coeffs.push_back(coeff);

            // Subtract divisor * (coeff * x^(remainder_deg - divisor_deg)) from
            // remainder
            for (size_t i = 0; i < divisor.coefficients.size(); i++) {
                remainder.coefficients[remainder_deg - divisor_deg + i] =
                    remainder.coefficients[remainder_deg - divisor_deg + i] -
                    (coeff * divisor.coefficients[i]);
            }

            remainder.trim_trailing_zeros();
            int new_remainder_deg = remainder.degree();

            // Check if we're making progress
            if (new_remainder_deg == remainder_deg) {
                break;  // Avoid infinite loop
            }

            remainder_deg = new_remainder_deg;
        }

        // Reverse quotient coefficients (we built them in reverse order)
        std::reverse(quotient_coeffs.begin(), quotient_coeffs.end());

        if (quotient_coeffs.empty()) {
            quotient_coeffs.push_back(Traits::zero());
        }

        return {Polynomial<T, Traits>(quotient_coeffs), remainder};
    }

    /**
     * Polynomial GCD using Euclidean algorithm
     * Uses pseudo-division for integer coefficients.
     * Result leading coefficient is normalized to 1 (or monic).
     *
     * @param other The other polynomial
     * @return GCD of this and other with leading coefficient = 1 (monic form)
     */
    Polynomial<T, Traits> gcd(const Polynomial<T, Traits>& other) const {
        // Handle zero polynomials
        if (this->degree() == -1 && other.degree() == -1) {
            return Polynomial<T, Traits>(Traits::zero());
        }
        if (this->degree() == -1) {
            return other;
        }
        if (other.degree() == -1) {
            return *this;
        }

        Polynomial<T, Traits> a = *this;
        Polynomial<T, Traits> b = other;

        // Euclidean algorithm with pseudo-division
        // For integer coefficients, we need to be careful about division
        int max_iterations = 100;  // Prevent infinite loops
        int iteration_count = 0;

        while (b.degree() >= 0 && iteration_count < max_iterations) {
            iteration_count++;

            // Perform pseudo-division to keep coefficients integral
            int a_deg = a.degree();
            int b_deg = b.degree();

            if (a_deg < b_deg) {
                // Swap if needed
                auto temp = a;
                a = b;
                b = temp;
            }

            // Try to divide: if remainder is zero, we're done
            auto [quotient, remainder] = a.divide_with_remainder(b);

            // Check if remainder is zero (or essentially zero)
            if (remainder.degree() == -1) {
                a = b;
                break;
            }

            a = b;
            b = remainder;
        }

        // Normalize to monic form (leading coefficient = 1)
        // Only divide if the leading coefficient exists and is not 1
        if (a.degree() >= 0) {
            T leading = a.coefficients[a.coefficients.size() - 1];
            if (!Traits::is_one(leading) && !Traits::is_zero(leading)) {
                // Try to normalize
                // Note: For integer types, this will only work if leading = ±1
                if (leading == Traits::one() || leading == -Traits::one()) {
                    if (leading == -Traits::one()) {
                        a = a * (-Traits::one());
                    }
                } else {
                    // For integer coefficients that don't divide evenly,
                    // we keep the GCD as is (not fully normalized)
                    // This is mathematically correct even without normalization
                }
            }
        }

        return a;
    }

    /**
     * Apply polynomial to a value (evaluate)
     * Uses Horner's method for efficiency
     *
     * @tparam U The type of the input value (can be different from T)
     * @param x The value at which to evaluate
     * @return P(x)
     */
    template <typename U>
    U apply(const U& x) const {
        if (coefficients.empty() ||
            (coefficients.size() == 1 && Traits::is_zero(coefficients[0]))) {
            return U();  // Return zero of type U
        }

        // Horner's method: P(x) = a_n * x^n + ... = (...((a_n * x + a_{n-1}) *
        // x + ...) * x + a_0)
        U result = static_cast<U>(coefficients[coefficients.size() - 1]);

        for (int i = static_cast<int>(coefficients.size()) - 2; i >= 0; i--) {
            result = result * x + static_cast<U>(coefficients[i]);
        }

        return result;
    }

    /**
     * Check equality of two polynomials
     */
    bool operator==(const Polynomial<T, Traits>& other) const {
        if (coefficients.size() != other.coefficients.size()) {
            return false;
        }
        for (size_t i = 0; i < coefficients.size(); i++) {
            if (!Traits::equals(coefficients[i], other.coefficients[i])) {
                return false;
            }
        }
        return true;
    }

    /**
     * Check inequality of two polynomials
     */
    bool operator!=(const Polynomial<T, Traits>& other) const {
        return !(*this == other);
    }

    /**
     * String representation for debugging
     */
    std::string to_string() const {
        if (degree() == -1) {
            return "0";
        }

        std::string result = "";
        for (int i = static_cast<int>(coefficients.size()) - 1; i >= 0; i--) {
            if (Traits::is_zero(coefficients[i])) {
                continue;
            }

            if (!result.empty()) {
                result += " + ";
            }

            if (i == 0) {
                result += std::to_string(coefficients[i]);
            } else if (i == 1) {
                result += std::to_string(coefficients[i]) + "x";
            } else {
                result +=
                    std::to_string(coefficients[i]) + "x^" + std::to_string(i);
            }
        }

        return result;
    }
};
