#include <emscripten/emscripten.h>

#include <cstddef>
#include <functional>
#include <numeric>
#include <vector>

#include "combinatorics/comb.h"

extern "C" {

EMSCRIPTEN_KEEPALIVE
int wasm_number_of_sequences(int* arr, int arr_len, int* seq, int seq_len) {
    std::vector<int> arr_vec(arr, arr + arr_len);
    std::vector<int> seq_vec(seq, seq + seq_len);
    return number_of_sequences(arr_vec, seq_vec);
}

EMSCRIPTEN_KEEPALIVE
int* wasm_number_of_sequences_all(int* arr, int arr_len, int* sequence,
                                  int seq_len) {
    std::vector<int> arr_vec(arr, arr + arr_len);
    std::vector<int> sequence_vec(sequence, sequence + seq_len);

    auto results = number_of_sequences_all(arr_vec, sequence_vec);

    const int total_size =
        std::accumulate(sequence_vec.begin(), sequence_vec.end(), 1,
                        [](int acc, int val) { return acc * (val + 1); });

    int* output = new int[total_size];

    std::function<void(std::vector<int>&, std::size_t, int&)> fill_output =
        [&](std::vector<int>& seq, std::size_t pos, int& idx) {
            if (pos == sequence_vec.size()) {
                output[idx++] = results[seq];
                return;
            }

            for (int val = 0; val <= sequence_vec[pos]; ++val) {
                seq[pos] = val;
                fill_output(seq, pos + 1, idx);
            }
        };

    std::vector<int> seq(sequence_vec.size(), 0);
    int idx = 0;
    fill_output(seq, 0, idx);

    return output;
}
}

