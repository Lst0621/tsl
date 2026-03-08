#pragma once

#include <string>

/**
 * Modular arithmetic class for numbers mod n.
 * Supports addition, subtraction, multiplication, and division (if n is prime).
 *
 * Supports unknown-modulus sentinel mode: ModularNumber() creates a value with
 * unknown modulus, which can only be used with known-modulus values.
 * Operations adopt the known modulus when combining with known-modulus values.
 */
class ModularNumber {
    long long value;
    long long modulus;
    bool is_unknown_mod;

    /**
     * Fast modular exponentiation: base^exp mod mod
     */
    static long long modular_pow(long long base, long long exp, long long mod);

    /**
     * Check if a number is prime
     */
    static bool is_prime(long long n);

   public:
    /**
     * Default constructor: creates ModularNumber with unknown modulus
     * value defaults to 0, modulus is marked as unknown
     */
    ModularNumber();

    /**
     * Constructor: creates a ModularNumber with given value and modulus
     * Normalizes value to be in range [0, modulus)
     */
    ModularNumber(long long value, long long modulus);

    /**
     * Get the value
     */
    long long get_value() const;

    /**
     * Get the modulus
     */
    long long get_modulus() const;

    /**
     * Check if modulus is unknown
     */
    bool is_unknown_modulus() const;

    /**
     * Addition in modular arithmetic: (a + b) mod n
     */
    ModularNumber add(const ModularNumber& other) const;

    /**
     * Subtraction in modular arithmetic: (a - b) mod n
     */
    ModularNumber subtract(const ModularNumber& other) const;

    /**
     * Multiplication in modular arithmetic: (a * b) mod n
     */
    ModularNumber multiply(const ModularNumber& other) const;

    /**
     * Division in modular arithmetic: (a / b) mod n
     * Only works if n is prime (uses Fermat's little theorem)
     */
    ModularNumber divide(const ModularNumber& other) const;

    /**
     * Equality operator
     */
    bool operator==(const ModularNumber& other) const;

    /**
     * Inequality operator
     */
    bool operator!=(const ModularNumber& other) const;

    /**
     * Addition operator: a + b
     */
    ModularNumber operator+(const ModularNumber& other) const;

    /**
     * Subtraction operator: a - b
     */
    ModularNumber operator-(const ModularNumber& other) const;

    /**
     * Multiplication operator: a * b
     */
    ModularNumber operator*(const ModularNumber& other) const;

    /**
     * Division operator: a / b
     */
    ModularNumber operator/(const ModularNumber& other) const;

    /**
     * String representation
     */
    std::string to_string() const;
};
