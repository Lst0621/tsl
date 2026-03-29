#include "comb.h"

#include <algorithm>
#include <unordered_map>

#include "helper.h"

// Recursive helper function that modifies seq in-place with memoization
static int number_of_sequences_helper(
    const std::vector<int>& arr, std::vector<int>& seq,
    std::unordered_map<std::vector<int>, int, vector_hash<int>>& memo) {
    auto it = memo.find(seq);
    if (it != memo.end()) {
        return it->second;
    }

    // ...existing code...
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
                        const std::vector<int>& sums) {
    std::vector<int> seq_copy = sums;
    std::unordered_map<std::vector<int>, int, vector_hash<int>> memo;
    return number_of_sequences_helper(arr, seq_copy, memo);
}

// Helper function to recursively generate all cartesian product combinations
static void generate_combinations(
    const std::vector<int>& arr, const std::vector<int>& sums,
    std::vector<int>& seq, size_t pos,
    std::unordered_map<std::vector<int>, int, vector_hash<int>>& memo,
    std::unordered_map<std::vector<int>, int, vector_hash<int>>& results) {
    if (pos == sums.size()) {
        // Call number_of_sequences_helper directly with shared memo cache
        std::vector<int> seq_copy = seq;
        results[seq] = number_of_sequences_helper(arr, seq_copy, memo);
        return;
    }

    // Try all values from 0 to sums[pos] (inclusive)
    for (int val = 0; val <= sums[pos]; ++val) {
        seq[pos] = val;
        generate_combinations(arr, sums, seq, pos + 1, memo, results);
    }
}

// Generate all cartesian product combinations of sequences and compute results
std::unordered_map<std::vector<int>, int, vector_hash<int>>
number_of_sequences_all(const std::vector<int>& arr,
                        const std::vector<int>& sums) {
    std::unordered_map<std::vector<int>, int, vector_hash<int>> results;
    std::unordered_map<std::vector<int>, int, vector_hash<int>> memo;

    // Start generation with a vector sized to match sums
    std::vector<int> seq(sums.size(), 0);
    generate_combinations(arr, sums, seq, 0, memo, results);

    return results;
}
