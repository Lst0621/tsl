#include <emscripten/emscripten.h>

#include <cmath>
#include <numeric>

#include "comb.h"
#include "general_linear_group.h"
#include "matrix.h"

extern "C" {

EMSCRIPTEN_KEEPALIVE
int wasm_number_of_sequences(int* arr, int arr_len, int* seq, int seq_len) {
    std::vector arr_vec(arr, arr + arr_len);
    std::vector seq_vec(seq, seq + seq_len);
    return number_of_sequences(arr_vec, seq_vec);
}

EMSCRIPTEN_KEEPALIVE
int* wasm_number_of_sequences_all(int* arr, int arr_len, int* sequence,
                                  int seq_len) {
    std::vector arr_vec(arr, arr + arr_len);
    std::vector sequence_vec(sequence, sequence + seq_len);

    auto results = number_of_sequences_all(arr_vec, sequence_vec);

    // Calculate total size (product of all sequence values + 1 for each, since
    // ranges are 0 to value inclusive)
    int total_size =
        std::accumulate(sequence_vec.begin(), sequence_vec.end(), 1,
                        [](int acc, int val) { return acc * (val + 1); });

    // Allocate output array
    int* output = new int[total_size];

    // Fill output array in lexicographic order
    std::function<void(std::vector<int>&, size_t, int&)> fill_output =
        [&](std::vector<int>& seq, size_t pos, int& idx) {
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

EMSCRIPTEN_KEEPALIVE
int wasm_get_gl_n_zm_size(int n, int m) {
    return static_cast<int>(get_gl_n_zm_size(n, m));
}

EMSCRIPTEN_KEEPALIVE
int wasm_matrix_det(int* data, int n) {
    // For square matrices: data should contain n*n elements
    int size = n * n;

    // Convert raw int array to vector
    std::vector<int> data_vec(data, data + size);

    // Convert vector to Matrix<int>
    Matrix<int> mat = to_matrix<int, std::vector<int>>(data_vec, n, n);

    // Calculate determinant
    int det = mat.determinant();

    return det;
}
}
