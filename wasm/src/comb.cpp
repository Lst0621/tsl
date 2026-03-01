#include "comb.h"
#include <algorithm>

// NOLINTNEXTLINE(misc-no-recursion)
int number_of_sequences(const std::vector<int>& arr, const std::vector<int>& seq) {
    // Check if seq is all zeros using std::all_of
    if (std::all_of(seq.begin(), seq.end(), [](int val) { return val == 0; })) {
        return 1;
    }

    // Check if any element in seq is negative using std::any_of
    if (std::any_of(seq.begin(), seq.end(), [](int val) { return val < 0; })) {
        return 0;
    }

    int total_result = 0;

    // Create a local copy since we need to modify it
    std::vector<int> local_seq = seq;

    // Go through all entries of seq using reference
    for (int& seq_val : local_seq) {
        // If it is non-negative
        if (seq_val >= 0) {
            // Go through all entries of arr
            for (int arr_val : arr) {
                if (seq_val >= arr_val) {
                    seq_val -= arr_val;

                    // Call recursively and add to total result
                    total_result += number_of_sequences(arr, local_seq);

                    // Restore the original value for next iteration
                    seq_val += arr_val;
                }
            }
        }
    }

    return total_result;
}
