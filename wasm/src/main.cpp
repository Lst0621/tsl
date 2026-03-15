#include <emscripten/emscripten.h>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <numeric>
#include <vector>

#include "bars_game.h"
#include "comb.h"
#include "game_of_life.h"
#include "general_linear_group.h"
#include "linear_recurrence.h"
#include "matrix.h"
#include "semigroup.h"

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

// --- Linear recurrence (opaque handle = LinearRecurrence*) ---
// All recurrence-related data uses long long in C++; we pass int32 ptrs and
// convert. Output buffers are int32 (truncated from long long for v1).

EMSCRIPTEN_KEEPALIVE
void* lr_create(const int* coeffs_ptr, int coeffs_len,
                int recursive_threshold) {
    std::vector<long long> coeffs(coeffs_ptr, coeffs_ptr + coeffs_len);
    return new LinearRecurrence(coeffs,
                                static_cast<size_t>(recursive_threshold));
}

EMSCRIPTEN_KEEPALIVE
void lr_destroy(void* handle) {
    delete static_cast<LinearRecurrence*>(handle);
}

EMSCRIPTEN_KEEPALIVE
int lr_order(void* handle) {
    return static_cast<int>(static_cast<LinearRecurrence*>(handle)->order());
}

// Writes result as int64 (low 32 bits at result_ptr, high 32 at result_ptr+4).
EMSCRIPTEN_KEEPALIVE
void lr_evaluate(void* handle, const int* init_ptr, int init_len, int n,
                 int* result_ptr) {
    std::vector<long long> init(init_ptr, init_ptr + init_len);
    long long val = static_cast<LinearRecurrence*>(handle)->evaluate(
        init, static_cast<size_t>(n));
    std::uint32_t low = static_cast<std::uint32_t>(val & 0xFFFFFFFFu);
    std::uint32_t high = static_cast<std::uint32_t>((val >> 32) & 0xFFFFFFFFu);
    result_ptr[0] = static_cast<int>(low);
    result_ptr[1] = static_cast<int>(high);
}

// Writes characteristic polynomial coefficients (ascending degree) as int32;
// returns count.
EMSCRIPTEN_KEEPALIVE
int lr_characteristic_polynomial(void* handle, int* out_ptr, int max_len) {
    const auto& coeffs = static_cast<LinearRecurrence*>(handle)
                             ->characteristic_polynomial()
                             .get_coefficients();
    int len = static_cast<int>(coeffs.size());
    if (len > max_len) len = max_len;
    for (int i = 0; i < len; ++i) {
        out_ptr[i] = static_cast<int>(coeffs[i]);
    }
    return len;
}

EMSCRIPTEN_KEEPALIVE
int lr_transition_matrix_size(void* handle) {
    return static_cast<int>(
        static_cast<LinearRecurrence*>(handle)->transition_matrix().get_rows());
}

// Writes transition matrix row-major as int32 (truncated). out_ptr must have at
// least order² elements.
EMSCRIPTEN_KEEPALIVE
void lr_transition_matrix_data(void* handle, int* out_ptr, int max_len) {
    const Matrix<long long>& M =
        static_cast<LinearRecurrence*>(handle)->transition_matrix();
    size_t rows = M.get_rows();
    size_t cols = M.get_cols();
    size_t idx = 0;
    for (size_t i = 0; i < rows && idx < static_cast<size_t>(max_len); ++i) {
        for (size_t j = 0; j < cols && idx < static_cast<size_t>(max_len);
             ++j) {
            out_ptr[idx++] = static_cast<int>(M.at(i, j));
        }
    }
}

// p(M) for characteristic polynomial; writes n×n row-major as int32.
EMSCRIPTEN_KEEPALIVE
void lr_evaluate_poly_at_matrix(void* handle, int* out_ptr, int max_len) {
    LinearRecurrence* lr = static_cast<LinearRecurrence*>(handle);
    Matrix<long long> pM =
        lr->characteristic_polynomial().apply(lr->transition_matrix());
    size_t rows = pM.get_rows();
    size_t cols = pM.get_cols();
    size_t idx = 0;
    for (size_t i = 0; i < rows && idx < static_cast<size_t>(max_len); ++i) {
        for (size_t j = 0; j < cols && idx < static_cast<size_t>(max_len);
             ++j) {
            out_ptr[idx++] = static_cast<int>(pM.at(i, j));
        }
    }
}

// Standalone: M^exponent. data_ptr and out_ptr are n×n row-major int32; we use
// long long internally.
EMSCRIPTEN_KEEPALIVE
void wasm_matrix_power(const int* data_ptr, int n, int exponent, int* out_ptr) {
    std::vector<long long> data(n * n);
    for (int i = 0; i < n * n; ++i)
        data[i] = static_cast<long long>(data_ptr[i]);
    Matrix<long long> M =
        to_matrix<long long, std::vector<long long>>(data, n, n);
    std::vector<std::vector<long long>> id_data(n,
                                                std::vector<long long>(n, 0LL));
    for (int i = 0; i < n; ++i) id_data[i][i] = 1LL;
    Matrix<long long> identity(id_data);
    Matrix<long long> result =
        power(M, static_cast<unsigned long long>(exponent), identity);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            out_ptr[i * n + j] = static_cast<int>(result.at(i, j));
        }
    }
}

// Standalone: scalar * M. data_ptr and out_ptr are n×n row-major int32; scalar
// is int32.
EMSCRIPTEN_KEEPALIVE
void wasm_matrix_times_const(const int* data_ptr, int n, int scalar,
                             int* out_ptr) {
    std::vector<long long> data(n * n);
    for (int i = 0; i < n * n; ++i)
        data[i] = static_cast<long long>(data_ptr[i]);
    Matrix<long long> M =
        to_matrix<long long, std::vector<long long>>(data, n, n);
    Matrix<long long> result = static_cast<long long>(scalar) * M;
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            out_ptr[i * n + j] = static_cast<int>(result.at(i, j));
        }
    }
}

// Standalone: A + B. data_a_ptr, data_b_ptr, out_ptr are n×n row-major int32.
EMSCRIPTEN_KEEPALIVE
void wasm_matrix_add(const int* data_a_ptr, const int* data_b_ptr, int n,
                     int* out_ptr) {
    std::vector<long long> a_data(n * n), b_data(n * n);
    for (int i = 0; i < n * n; ++i) {
        a_data[i] = static_cast<long long>(data_a_ptr[i]);
        b_data[i] = static_cast<long long>(data_b_ptr[i]);
    }
    Matrix<long long> A =
        to_matrix<long long, std::vector<long long>>(a_data, n, n);
    Matrix<long long> B =
        to_matrix<long long, std::vector<long long>>(b_data, n, n);
    Matrix<long long> result = A + B;
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            out_ptr[i * n + j] = static_cast<int>(result.at(i, j));
        }
    }
}

// --- Bars game (opaque handle = BarsGame*) ---
EMSCRIPTEN_KEEPALIVE
void* bars_game_create() {
    return new BarsGame(BarsGameConfig{});
}

EMSCRIPTEN_KEEPALIVE
void bars_game_destroy(void* handle) {
    delete static_cast<BarsGame*>(handle);
}

EMSCRIPTEN_KEEPALIVE
void bars_game_set_seed(void* handle, uint32_t seed) {
    static_cast<BarsGame*>(handle)->set_seed(seed);
}

EMSCRIPTEN_KEEPALIVE
void bars_game_init(void* handle) {
    static_cast<BarsGame*>(handle)->init();
}

EMSCRIPTEN_KEEPALIVE
void bars_game_get_state(void* handle, int* out) {
    static_cast<BarsGame*>(handle)->get_state(out);
}

EMSCRIPTEN_KEEPALIVE
void bars_game_get_future_state(void* handle, int choice_index, int* out) {
    static_cast<BarsGame*>(handle)->get_future_state(choice_index, out);
}

EMSCRIPTEN_KEEPALIVE
void bars_game_apply_choice(void* handle, int index) {
    static_cast<BarsGame*>(handle)->apply_choice(index);
}

EMSCRIPTEN_KEEPALIVE
int bars_game_is_ended(void* handle) {
    return static_cast<BarsGame*>(handle)->is_ended() ? 1 : 0;
}

EMSCRIPTEN_KEEPALIVE
int bars_game_state_size(void* handle) {
    return static_cast<BarsGame*>(handle)->state_size();
}

EMSCRIPTEN_KEEPALIVE
int bars_game_num_choices(void* handle) {
    return static_cast<BarsGame*>(handle)->num_choices();
}

EMSCRIPTEN_KEEPALIVE
int bars_game_min_val(void* handle) {
    return static_cast<BarsGame*>(handle)->min_val();
}

EMSCRIPTEN_KEEPALIVE
int bars_game_max_val(void* handle) {
    return static_cast<BarsGame*>(handle)->max_val();
}
}
