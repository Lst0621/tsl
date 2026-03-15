#pragma once

#include <vector>

/**
 * Merge two cartesian-product results: each row of left concatenated with
 * each row of right. (Used for divide-and-conquer.)
 */
template <typename T>
std::vector<std::vector<T>> cartesian_product_merge(
    const std::vector<std::vector<T>>& left,
    const std::vector<std::vector<T>>& right) {
    std::vector<std::vector<T>> out;
    out.reserve(left.size() * right.size());
    for (const std::vector<T>& lrow : left) {
        for (const std::vector<T>& rrow : right) {
            std::vector<T> row = lrow;
            row.insert(row.end(), rrow.begin(), rrow.end());
            out.push_back(std::move(row));
        }
    }
    return out;
}

/**
 * Cartesian product of N sets (vectors).
 * Input: inputs[0], inputs[1], ... each a vector of T.
 * Output: vector of tuples, each tuple is a vector<T> of length N.
 *
 * For N=1 returns singletons; for N=2 uses two nested for loops.
 * For N>=3 uses divide-and-conquer: product(first half) and product(second
 * half), then merge (e.g. 4 sets -> product(set0,set1) x product(set2,set3)).
 * All inputs and outputs are vectors.
 */
template <typename T>
std::vector<std::vector<T>> cartesian_product(
    const std::vector<std::vector<T>>& inputs) {
    if (inputs.empty()) {
        return {{}};
    }
    for (const auto& v : inputs) {
        if (v.empty()) {
            return {};
        }
    }
    if (inputs.size() == 1) {
        std::vector<std::vector<T>> out;
        for (const T& x : inputs[0]) {
            out.push_back({x});
        }
        return out;
    }
    if (inputs.size() == 2) {
        std::vector<std::vector<T>> out;
        for (const T& a : inputs[0]) {
            for (const T& b : inputs[1]) {
                out.push_back({a, b});
            }
        }
        return out;
    }
    // N >= 3: break in the middle — product(first half), product(second half),
    // merge
    size_t mid = inputs.size() / 2;
    std::vector<std::vector<T>> left_inputs(
        inputs.begin(), inputs.begin() + static_cast<ptrdiff_t>(mid));
    std::vector<std::vector<T>> right_inputs(
        inputs.begin() + static_cast<ptrdiff_t>(mid), inputs.end());
    std::vector<std::vector<T>> left_product = cartesian_product(left_inputs);
    std::vector<std::vector<T>> right_product = cartesian_product(right_inputs);
    return cartesian_product_merge(left_product, right_product);
}
