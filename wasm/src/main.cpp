#include <emscripten/emscripten.h>

#include <cmath>
#include <numeric>

#include "comb.h"
#include "game_of_life.h"
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

// Game of Life C API (opaque handle = GameOfLife*)
EMSCRIPTEN_KEEPALIVE
void* gol_create(int size) {
    return new GameOfLife(size);
}

EMSCRIPTEN_KEEPALIVE
void gol_destroy(void* handle) {
    delete static_cast<GameOfLife*>(handle);
}

EMSCRIPTEN_KEEPALIVE
void gol_init(void* handle) {
    static_cast<GameOfLife*>(handle)->init();
}

EMSCRIPTEN_KEEPALIVE
void gol_random_init(void* handle, double live_prob) {
    static_cast<GameOfLife*>(handle)->random_init(live_prob);
}

EMSCRIPTEN_KEEPALIVE
void gol_random_init_seed(void* handle, double live_prob, uint32_t seed) {
    static_cast<GameOfLife*>(handle)->random_init(live_prob, seed);
}

EMSCRIPTEN_KEEPALIVE
uint32_t gol_get_seed(void* handle) {
    return static_cast<GameOfLife*>(handle)->last_seed();
}

EMSCRIPTEN_KEEPALIVE
void gol_evolve(void* handle) {
    static_cast<GameOfLife*>(handle)->evolve();
}

EMSCRIPTEN_KEEPALIVE
void gol_set_topology(void* handle, int mode) {
    static_cast<GameOfLife*>(handle)->set_topology(
        static_cast<TopologyMode>(mode));
}

EMSCRIPTEN_KEEPALIVE
int gol_get_live_cells(void* handle, int* out_xy, int max_count) {
    std::vector<Coord2D> cells =
        static_cast<GameOfLife*>(handle)->get_live_cells();
    int n = static_cast<int>(cells.size());
    if (n > max_count) n = max_count;
    for (int i = 0; i < n; ++i) {
        out_xy[2 * i] = cells[i].x;
        out_xy[2 * i + 1] = cells[i].y;
    }
    return static_cast<int>(cells.size());
}
}
