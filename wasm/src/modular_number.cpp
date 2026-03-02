#include "modular_number.h"

#include <stdexcept>

/**
 * Fast modular exponentiation: base^exp mod mod
 */
long long ModularNumber::modular_pow(long long base, long long exp,
                                     long long mod) {
    long long result = 1;
    base = base % mod;
    while (exp > 0) {
        if (exp % 2 == 1) {
            result = (result * base) % mod;
        }
        exp = exp / 2;
        base = (base * base) % mod;
    }
    return result;
}

/**
 * Check if a number is prime
 */
bool ModularNumber::is_prime(long long n) {
    if (n <= 1) return false;
    if (n == 2 || n == 3 || n == 5) return true;
    if (n % 2 == 0) return false;
    for (long long i = 3; i * i <= n; i += 2) {
        if (n % i == 0) return false;
    }
    return true;
}


/**
 * Constructor: creates a ModularNumber with given value and modulus
 * Normalizes value to be in range [0, modulus)
 */
ModularNumber::ModularNumber(long long value, long long modulus)
    : modulus(modulus) {
    if (modulus <= 0) {
        throw std::invalid_argument("Modulus must be positive");
    }
    this->value = ((value % modulus) + modulus) % modulus;
}

/**
 * Get the value
 */
long long ModularNumber::get_value() const {
    return value;
}

/**
 * Get the modulus
 */
long long ModularNumber::get_modulus() const {
    return modulus;
}

/**
 * Addition in modular arithmetic: (a + b) mod n
 */
ModularNumber ModularNumber::add(const ModularNumber& other) const {
    if (modulus != other.modulus) {
        throw std::invalid_argument("Cannot add numbers with different moduli");
    }
    return ModularNumber(value + other.value, modulus);
}

/**
 * Subtraction in modular arithmetic: (a - b) mod n
 */
ModularNumber ModularNumber::subtract(const ModularNumber& other) const {
    if (modulus != other.modulus) {
        throw std::invalid_argument(
            "Cannot subtract numbers with different moduli");
    }
    return ModularNumber(value - other.value, modulus);
}

/**
 * Multiplication in modular arithmetic: (a * b) mod n
 */
ModularNumber ModularNumber::multiply(const ModularNumber& other) const {
    if (modulus != other.modulus) {
        throw std::invalid_argument(
            "Cannot multiply numbers with different moduli");
    }
    return ModularNumber(value * other.value, modulus);
}

/**
 * Division in modular arithmetic: (a / b) mod n
 * Only works if n is prime (uses Fermat's little theorem)
 */
ModularNumber ModularNumber::divide(const ModularNumber& other) const {
    if (modulus != other.modulus) {
        throw std::invalid_argument(
            "Cannot divide numbers with different moduli");
    }
    if (!is_prime(modulus)) {
        throw std::invalid_argument("Division only works if modulus is prime");
    }
    if (other.value == 0) {
        throw std::invalid_argument("Cannot divide by zero");
    }
    // For prime modulus, a^(-1) ≡ a^(p-2) (mod p) by Fermat's little theorem
    long long inverse = modular_pow(other.value, modulus - 2, modulus);
    return ModularNumber(value * inverse, modulus);
}

/**
 * Equality operator
 */
bool ModularNumber::operator==(const ModularNumber& other) const {
    return value == other.value && modulus == other.modulus;
}

/**
 * Inequality operator
 */
bool ModularNumber::operator!=(const ModularNumber& other) const {
    return !(*this == other);
}

/**
 * Addition operator: a + b
 */
ModularNumber ModularNumber::operator+(const ModularNumber& other) const {
    return add(other);
}

/**
 * Subtraction operator: a - b
 */
ModularNumber ModularNumber::operator-(const ModularNumber& other) const {
    return subtract(other);
}

/**
 * Multiplication operator: a * b
 */
ModularNumber ModularNumber::operator*(const ModularNumber& other) const {
    return multiply(other);
}

/**
 * Division operator: a / b
 */
ModularNumber ModularNumber::operator/(const ModularNumber& other) const {
    return divide(other);
}

/**
 * String representation
 */
std::string ModularNumber::to_string() const {
    return std::to_string(value) + " (mod " + std::to_string(modulus) + ")";
}
