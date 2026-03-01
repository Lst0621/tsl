#include "comb.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <unordered_map>

// Custom hash function for std::vector<int>
namespace std {
template <>
struct hash<std::vector<int>> {
    size_t operator()(const std::vector<int>& v) const {
        // Golden ratio constant: 2^32 / φ where φ = (1 + sqrt(5)) / 2
        const double phi = (1.0 + std::sqrt(5.0)) / 2.0;
        const size_t golden_ratio =
            static_cast<size_t>(std::pow(2.0, 32.0) / phi);

        size_t hash = 0;
        for (int i : v) {
            hash ^= std::hash<int>()(i) + golden_ratio + (hash << 6) +
                    (hash >> 2);
        }
        return hash;
    }
};
}  // namespace std

// Recursive helper function that modifies seq in-place with memoization
static int number_of_sequences_helper(
    const std::vector<int>& arr, std::vector<int>& seq,
    std::unordered_map<std::vector<int>, int>& memo) {
    // Check if result is already cached
    auto it = memo.find(seq);
    if (it != memo.end()) {
        return it->second;
    }

    // Check if seq is all zeros using std::all_of
    if (std::all_of(seq.begin(), seq.end(),
                    [](const int x) { return x == 0; })) {
        return 1;
    }

    // Check if any element in seq is negative using std::any_of
    if (std::any_of(seq.begin(), seq.end(),
                    [](const int x) { return x < 0; })) {
        return 0;
    }

    int total_result = 0;

    // Go through all entries of seq using reference (modifying in-place)
    for (int& seq_val : seq) {
        // If it is non-negative
        if (seq_val >= 0) {
            // Go through all entries of arr
            for (int arr_val : arr) {
                if (seq_val >= arr_val) {
                    seq_val -= arr_val;

                    // Call recursively and add to total result
                    total_result += number_of_sequences_helper(arr, seq, memo);

                    // Restore the original value for next iteration
                    seq_val += arr_val;
                }
            }
        }
    }

    // Cache the result before returning
    memo[seq] = total_result;
    return total_result;
}

// Top-level function - makes ONE copy then delegates to helper
int number_of_sequences(const std::vector<int>& arr,
                        const std::vector<int>& seq) {
    std::vector<int> seq_copy = seq;
    std::unordered_map<std::vector<int>, int> memo;
    return number_of_sequences_helper(arr, seq_copy, memo);
}
