#pragma once

#include <algorithm>
#include <optional>
#include <vector>

#include "set_utils.h"

/**
 * Generic semigroup generator template.
 *
 * Generates all elements of a semigroup from a set of generators.
 * Assumes that operator== and operator* are already defined for type T.
 *
 * @tparam T The element type. Must support operator== and operator*.
 * @param generators Vector of initial generator elements.
 * @param limit Maximum number of elements to generate. -1 (default) means no
 * limit.
 * @param is_abelian If true, only compute i*j (skips j*i). Default is false.
 * @return Vector containing all generated semigroup elements.
 */
template <typename T>
std::vector<T> generate_semigroup(const std::vector<T>& generators,
                                  const int limit = -1,
                                  const bool is_abelian = false);

/**
 * Semigroup power: base^exp for exp >= 1 (no identity). Default op = operator*.
 * Binary exponentiation. exp must be >= 1; if exp == 0, returns base
 * (documented as invalid).
 */
template <typename T>
T semigroup_power(const T& base, unsigned long long exp) {
    return semigroup_power(base, exp,
                           [](const T& a, const T& b) { return a * b; });
}

/**
 * Semigroup power with custom binary op. exp >= 1.
 */
template <typename T, typename BinaryOp>
T semigroup_power(const T& base, unsigned long long exp, BinaryOp op) {
    if (exp == 0) {
        return base;
    }
    T result = base;
    T current = base;
    exp--;
    while (exp > 0) {
        if (exp % 2 == 1) {
            result = op(result, current);
        }
        current = op(current, current);
        exp /= 2;
    }
    return result;
}

/**
 * Monoid power: base^exp for exp >= 0. Default identity T(), default op =
 * operator*. exp == 0 returns identity; exp >= 1 returns semigroup_power(base,
 * exp).
 */
template <typename T>
T monoid_power(const T& base, unsigned long long exp) {
    if (exp == 0) {
        return T();
    }
    return semigroup_power(base, exp);
}

/**
 * Monoid power with explicit identity. Default op = operator*.
 */
template <typename T>
T monoid_power(const T& base, unsigned long long exp, const T& identity) {
    if (exp == 0) {
        return identity;
    }
    return semigroup_power(base, exp);
}

/**
 * Monoid power with explicit identity and custom op.
 */
template <typename T, typename BinaryOp>
T monoid_power(const T& base, unsigned long long exp, const T& identity,
               BinaryOp op) {
    if (exp == 0) {
        return identity;
    }
    return semigroup_power(base, exp, op);
}

/**
 * Group power: base^exp for any integer exp. Uses identity and inverse.
 * exp >= 0: monoid_power(base, exp, identity) [or with op].
 * exp < 0: monoid_power(inverse(base), -exp, identity) i.e. (base^(-1))^|exp|.
 * Default op = operator*; inverse is a callable T -> T (no default;
 * type-specific).
 */
template <typename T, typename InverseOp>
T group_power(const T& base, long long exp, const T& identity,
              InverseOp inverse_op) {
    if (exp >= 0) {
        return monoid_power(base, static_cast<unsigned long long>(exp),
                            identity);
    }
    return monoid_power(inverse_op(base), static_cast<unsigned long long>(-exp),
                        identity);
}

/**
 * Group power with custom op.
 */
template <typename T, typename InverseOp, typename BinaryOp>
T group_power(const T& base, long long exp, const T& identity,
              InverseOp inverse_op, BinaryOp op) {
    if (exp >= 0) {
        return monoid_power(base, static_cast<unsigned long long>(exp),
                            identity, op);
    }
    return monoid_power(inverse_op(base), static_cast<unsigned long long>(-exp),
                        identity, op);
}

/**
 * Generate all elements of a semigroup from a set of generators.
 * Assumes that operator== and operator* are already defined for type T.
 *
 * @tparam T The element type. Must support operator== and operator*.
 * @param generators Vector of initial generator elements.
 * @param limit Maximum number of elements to generate. -1 (default) means no
 * limit.
 * @param is_abelian If true, only compute i*j (skips j*i). Default is false.
 * @return Vector containing all generated semigroup elements.
 */
template <typename T>
std::vector<T> generate_semigroup(const std::vector<T>& generators,
                                  const int limit, const bool is_abelian) {
    std::vector<T> ret(generators.begin(), generators.end());
    size_t last_length = 0;
    size_t current_length = ret.size();

    while (last_length < current_length) {
        // Multiply existing elements with newly generated elements (forward
        // order)
        for (size_t i = 0; i < current_length; i++) {
            for (size_t j = last_length; j < current_length; j++) {
                T product_ij = ret[i] * ret[j];

                // Check if product already exists
                if (std::find(ret.begin(), ret.end(), product_ij) ==
                    ret.end()) {
                    ret.push_back(product_ij);
                }
            }
        }

        // Multiply newly generated elements with existing elements (reverse
        // order) Skip this if the semigroup is abelian (commutative)
        if (!is_abelian) {
            for (size_t i = 0; i < current_length; i++) {
                for (size_t j = last_length; j < current_length; j++) {
                    T product_ji = ret[j] * ret[i];

                    // Check if product already exists
                    if (std::find(ret.begin(), ret.end(), product_ji) ==
                        ret.end()) {
                        ret.push_back(product_ji);
                    }
                }
            }
        }

        last_length = current_length;
        current_length = ret.size();

        // Check if limit exceeded
        if (limit > 0 && current_length > static_cast<size_t>(limit)) {
            break;
        }
    }

    return ret;
}

/**
 * Check if a set of generators is closed under multiplication.
 *
 * A set is closed if the semigroup generated from it has the same size as the
 * generators. (i.e., no new elements are created by multiplication)
 *
 * @tparam T The element type. Must support operator== and operator*.
 * @param generators Vector of generator elements to check.
 * @param is_abelian If true, only compute i*j (skips j*i). Default is false.
 * @return True if the generators form a closed set, false otherwise.
 */
template <typename T>
bool is_closure(const std::vector<T>& generators,
                const bool is_abelian = false) {
    // Set limit to generators.size() + 1 to avoid generating the whole
    // semigroup We only need to check if any new elements are created
    const int limit = static_cast<int>(generators.size()) + 1;
    std::vector<T> closure = generate_semigroup(generators, limit, is_abelian);

    // If closure size equals generators size, no new elements were created
    // (closed)
    return closure.size() == generators.size();
}

/**
 * Check associativity: for all a,b,c, (a*b)*c == a*(b*c).
 * Uses cartesian product of three copies of elements.
 */
template <typename T>
bool is_associative(const std::vector<T>& elements) {
    if (elements.empty()) {
        return true;
    }
    std::vector<std::vector<T>> triple_input = {elements, elements, elements};
    std::vector<std::vector<T>> triples = cartesian_product(triple_input);
    for (const std::vector<T>& row : triples) {
        const T& a = row[0];
        const T& b = row[1];
        const T& c = row[2];
        T ab = a * b;
        T abc_1 = ab * c;
        T bc = b * c;
        T abc_2 = a * bc;
        if (!(abc_1 == abc_2)) {
            return false;
        }
    }
    return true;
}

/**
 * Idempotent power (Floyd-style): smallest i >= 1 such that item^i ==
 * item^(2i). limit == -1 means no limit. Returns optional (power, idempotent);
 * nullopt if not found within limit.
 */
template <typename T>
std::optional<std::pair<size_t, T>> get_idempotent_power(const T& item,
                                                         const int limit = -1) {
    T power_t = item;
    T square = item * item;
    T power_2t = square;
    for (size_t i = 1; limit < 0 || i < static_cast<size_t>(limit); i++) {
        if (power_t == power_2t) {
            return std::make_pair(i, power_t);
        }
        power_t = power_t * item;
        power_2t = power_2t * square;
    }
    return std::nullopt;
}

/**
 * Indices of all elements x such that x*x == x.
 */
template <typename T>
std::vector<size_t> get_all_idempotent_indices(const std::vector<T>& elements) {
    std::vector<size_t> out;
    for (size_t i = 0; i < elements.size(); i++) {
        if (elements[i] * elements[i] == elements[i]) {
            out.push_back(i);
        }
    }
    return out;
}

/**
 * All elements x such that x*x == x.
 */
template <typename T>
std::vector<T> get_all_idempotent_elements(const std::vector<T>& elements) {
    std::vector<T> out;
    for (size_t i : get_all_idempotent_indices(elements)) {
        out.push_back(elements[i]);
    }
    return out;
}

/**
 * Maximum idempotent power over all elements (or 0 if none).
 */
template <typename T>
size_t get_highest_idempotent_power(const std::vector<T>& elements,
                                    const int limit = -1) {
    size_t max_power = 0;
    for (const T& item : elements) {
        auto opt = get_idempotent_power(item, limit);
        if (opt.has_value()) {
            if (opt->first > max_power) {
                max_power = opt->first;
            }
        }
    }
    return max_power;
}

/**
 * True iff for all i,j: elements[i]*elements[j] == elements[j]*elements[i].
 */
template <typename T>
bool is_abelian(const std::vector<T>& elements) {
    size_t len = elements.size();
    for (size_t i = 0; i < len; i++) {
        for (size_t j = 0; j < i; j++) {
            if (!(elements[i] * elements[j] == elements[j] * elements[i])) {
                return false;
            }
        }
    }
    return true;
}

namespace internal {

template <typename T>
int get_definite_k_common(const std::vector<T>& elements,
                          bool multiply_idempotent_on_right) {
    size_t highest = get_highest_idempotent_power(elements);
    if (highest == 0) {
        return -1;
    }
    std::vector<T> candidates;
    candidates.reserve(elements.size());
    for (const T& item : elements) {
        candidates.push_back(
            semigroup_power(item, static_cast<unsigned long long>(highest)));
    }
    std::vector<std::vector<T>> pair_input = {elements, candidates};
    std::vector<std::vector<T>> pairs = cartesian_product(pair_input);
    for (const std::vector<T>& row : pairs) {
        const T& element = row[0];
        const T& candidate = row[1];
        T product = multiply_idempotent_on_right ? (element * candidate)
                                                 : (candidate * element);
        if (!(product == candidate)) {
            return -1;
        }
    }
    return static_cast<int>(highest);
}

}  // namespace internal

/**
 * Definite k (right ideal of idempotents).
 */
template <typename T>
int get_definite_k(const std::vector<T>& elements) {
    return internal::get_definite_k_common(elements, true);
}

/**
 * Reverse definite k (left ideal of idempotents).
 */
template <typename T>
int get_reverse_definite_k(const std::vector<T>& elements) {
    return internal::get_definite_k_common(elements, false);
}

/**
 * Aperiodic: every element has idempotent power and e*x == e for that
 * idempotent. Returns max such power, or -1 if not aperiodic.
 */
template <typename T>
int is_aperiodic(const std::vector<T>& elements, const int limit = -1) {
    size_t max_power = 1;
    for (const T& element : elements) {
        auto opt = get_idempotent_power(element, limit);
        if (!opt.has_value()) {
            return -1;
        }
        T idem = opt->second;
        if (!(idem * element == idem)) {
            return -1;
        }
        if (opt->first > max_power) {
            max_power = opt->first;
        }
    }
    return static_cast<int>(max_power);
}

/**
 * Monoid: closure and existence of identity e with e*a == a*e == a for all a.
 * Returns optional identity; nullopt if not a monoid.
 */
template <typename T>
std::optional<T> is_monoid(const std::vector<T>& elements,
                           const bool is_abelian = false) {
    if (!is_closure(elements, is_abelian)) {
        return std::nullopt;
    }
    for (size_t idx : get_all_idempotent_indices(elements)) {
        const T& idem = elements[idx];
        bool is_identity = true;
        for (const T& a : elements) {
            if (!(idem * a == a) || !(a * idem == a)) {
                is_identity = false;
                break;
            }
        }
        if (is_identity) {
            return idem;
        }
    }
    return std::nullopt;
}

/**
 * Group: monoid and every element has an inverse.
 * Returns optional identity; nullopt if not a group.
 */
template <typename T>
std::optional<T> is_group(const std::vector<T>& elements,
                          const bool is_abelian = false) {
    std::optional<T> identity_opt = is_monoid(elements, is_abelian);
    if (!identity_opt.has_value()) {
        return std::nullopt;
    }
    T identity = *identity_opt;
    for (const T& x : elements) {
        bool has_inverse = false;
        for (const T& y : elements) {
            if (x * y == identity) {
                if (y * x == identity) {
                    has_inverse = true;
                }
                break;
            }
        }
        if (!has_inverse) {
            return std::nullopt;
        }
    }
    return identity;
}
