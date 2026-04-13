#include <emscripten/emscripten.h>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <numeric>
#include <queue>
#include <vector>

#include "bars_game.h"
#include "comb.h"
#include "game_of_life.h"
#include "general_linear_group.h"
#include "linear_recurrence.h"
#include "matrix.h"
#include "semigroup.h"
#include "graph/graph.h"
#include "graph/metric_dimension.h"

namespace {

inline bool adj01_has_edge_directed(const int* adj01, int n, int from, int to) {
    return adj01[from * n + to] != 0;
}

inline bool adj01_has_edge_undirected_or(const int* adj01, int n, int a, int b) {
    return adj01[a * n + b] != 0 || adj01[b * n + a] != 0;
}

inline Graph<int> build_undirected_graph_from_adj01_or(const int* adj01, int n) {
    Graph<int> g(false);
    for (int i = 0; i < n; i++) {
        g.add_vertex(i);
    }
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            if (adj01_has_edge_undirected_or(adj01, n, i, j)) {
                g.add_edge(i, j);
            }
        }
    }
    return g;
}

}  // namespace

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
int* wasm_get_gl_n_zm(int n, int m, int* out_count) {
    auto gl = get_gl_n_zm(n, m);
    *out_count = static_cast<int>(gl.size());
    int elems = *out_count * n * n;
    int* buf = static_cast<int*>(malloc(elems * sizeof(int)));
    for (size_t k = 0; k < gl.size(); k++) {
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                buf[k * n * n + i * n + j] =
                    static_cast<int>(gl[k](i, j).get_value());
            }
        }
    }
    return buf;
}

/**
 * Check whether a caller-supplied set of n×n matrices forms a group
 * under multiplication.
 *
 * @param data       Flat row-major int32 array: count matrices, each n×n
 * @param count      Number of matrices
 * @param n          Matrix dimension (n×n)
 * @param modulus    >0  → elements are in Z_modulus (ModularNumber)
 *                    0  → plain integer arithmetic (characteristic 0)
 * @return 1 if the set is a group, 0 otherwise
 */
EMSCRIPTEN_KEEPALIVE
int wasm_is_matrix_group(int* data, int count, int n, int modulus) {
    int elems_per = n * n;
    if (modulus > 0) {
        ModularNumber zero(0, modulus);
        std::vector<Matrix<ModularNumber>> matrices;
        matrices.reserve(count);
        for (int k = 0; k < count; k++) {
            std::vector<std::vector<ModularNumber>> rows(
                n, std::vector<ModularNumber>(n, zero));
            for (int i = 0; i < n; i++) {
                for (int j = 0; j < n; j++) {
                    rows[i][j] = ModularNumber(
                        data[k * elems_per + i * n + j], modulus);
                }
            }
            matrices.emplace_back(rows);
        }
        return is_group(matrices).has_value() ? 1 : 0;
    } else {
        std::vector<Matrix<long long>> matrices;
        matrices.reserve(count);
        for (int k = 0; k < count; k++) {
            std::vector<long long> flat(elems_per);
            for (int i = 0; i < elems_per; i++) {
                flat[i] = static_cast<long long>(data[k * elems_per + i]);
            }
            matrices.push_back(to_matrix<long long>(flat, n, n));
        }
        return is_group(matrices).has_value() ? 1 : 0;
    }
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

EMSCRIPTEN_KEEPALIVE
void wasm_matrix_inverse_mod(int* data, int n, int m, int* out) {
    int size = n * n;
    ModularNumber zero(0, m);
    std::vector<std::vector<ModularNumber>> mat_data(n, std::vector<ModularNumber>(n, zero));
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            mat_data[i][j] = ModularNumber(data[i * n + j], m);
        }
    }
    Matrix<ModularNumber> mat(mat_data);
    Matrix<ModularNumber> inv = mat.inverse();
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            out[i * n + j] = static_cast<int>(inv(i, j).get_value());
        }
    }
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
// Recurrence coefficients / initial values are exact rationals.
// WASM wire format uses HEAP32 (int32) with packed int64 parts:
// - int64 is two int32 words: low32, high32 (two's complement).
// - Rational is four int32 words: m_lo, m_hi, n_lo, n_hi.

static inline std::int64_t read_i64(const int* p) {
    const std::uint64_t lo = static_cast<std::uint32_t>(p[0]);
    const std::uint64_t hi = static_cast<std::uint32_t>(p[1]);
    const std::uint64_t u = lo | (hi << 32);
    return static_cast<std::int64_t>(u);
}

static inline void write_i64(int* p, std::int64_t v) {
    const std::uint64_t u = static_cast<std::uint64_t>(v);
    p[0] = static_cast<int>(static_cast<std::uint32_t>(u & 0xFFFFFFFFu));
    p[1] = static_cast<int>(
        static_cast<std::uint32_t>((u >> 32) & 0xFFFFFFFFu));
}

static inline LinearRecurrence::Coeff read_rational(const int* p) {
    const std::int64_t m = read_i64(p);
    const std::int64_t n = read_i64(p + 2);
    return LinearRecurrence::Coeff(static_cast<long long>(m),
                                   static_cast<long long>(n));
}

static inline void write_rational(int* p, const LinearRecurrence::Coeff& r) {
    write_i64(p, static_cast<std::int64_t>(r.m()));
    write_i64(p + 2, static_cast<std::int64_t>(r.n()));
}

EMSCRIPTEN_KEEPALIVE
void* lr_create(const int* coeffs_ptr, int coeffs_len,
                int recursive_threshold) {
    std::vector<LinearRecurrence::Coeff> coeffs;
    coeffs.reserve(static_cast<size_t>(coeffs_len));
    for (int i = 0; i < coeffs_len; ++i) {
        coeffs.push_back(read_rational(coeffs_ptr + 4 * i));
    }
    return new LinearRecurrence(
        coeffs,
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

// Writes result as packed rational (4 int32 words).
EMSCRIPTEN_KEEPALIVE
void lr_evaluate(void* handle, const int* init_ptr, int init_len, int n,
                 int* result_ptr) {
    std::vector<LinearRecurrence::Coeff> init;
    init.reserve(static_cast<size_t>(init_len));
    for (int i = 0; i < init_len; ++i) {
        init.push_back(read_rational(init_ptr + 4 * i));
    }
    LinearRecurrence::Coeff val = static_cast<LinearRecurrence*>(handle)->evaluate(
        init, static_cast<size_t>(n));
    write_rational(result_ptr, val);
}

// Writes characteristic polynomial coefficients (ascending degree) as packed
// rationals; returns count in coefficients.
EMSCRIPTEN_KEEPALIVE
int lr_characteristic_polynomial(void* handle, int* out_ptr, int max_len) {
    const auto& coeffs = static_cast<LinearRecurrence*>(handle)
                             ->characteristic_polynomial()
                             .get_coefficients();
    int len = static_cast<int>(coeffs.size());
    if (len > max_len) len = max_len;
    for (int i = 0; i < len; ++i) {
        write_rational(out_ptr + 4 * i, coeffs[static_cast<size_t>(i)]);
    }
    return len;
}

EMSCRIPTEN_KEEPALIVE
int lr_transition_matrix_size(void* handle) {
    return static_cast<int>(
        static_cast<LinearRecurrence*>(handle)->transition_matrix().get_rows());
}

// Writes transition matrix row-major as packed rationals. out_ptr must have at
// least 4*order² int32 elements; max_len counts matrix entries (not int32
// words).
EMSCRIPTEN_KEEPALIVE
void lr_transition_matrix_data(void* handle, int* out_ptr, int max_len) {
    const Matrix<LinearRecurrence::Coeff>& M =
        static_cast<LinearRecurrence*>(handle)->transition_matrix();
    size_t rows = M.get_rows();
    size_t cols = M.get_cols();
    size_t idx = 0;
    for (size_t i = 0; i < rows && idx < static_cast<size_t>(max_len); ++i) {
        for (size_t j = 0; j < cols && idx < static_cast<size_t>(max_len);
             ++j) {
            write_rational(out_ptr + 4 * idx, M.at(i, j));
            idx++;
        }
    }
}

// p(M) for characteristic polynomial; writes n×n row-major as packed rationals.
EMSCRIPTEN_KEEPALIVE
void lr_evaluate_poly_at_matrix(void* handle, int* out_ptr, int max_len) {
    LinearRecurrence* lr = static_cast<LinearRecurrence*>(handle);
    Matrix<LinearRecurrence::Coeff> pM =
        lr->characteristic_polynomial().apply(lr->transition_matrix());
    size_t rows = pM.get_rows();
    size_t cols = pM.get_cols();
    size_t idx = 0;
    for (size_t i = 0; i < rows && idx < static_cast<size_t>(max_len); ++i) {
        for (size_t j = 0; j < cols && idx < static_cast<size_t>(max_len);
             ++j) {
            write_rational(out_ptr + 4 * idx, pM.at(i, j));
            idx++;
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
        monoid_power(M, static_cast<unsigned long long>(exponent), identity);
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

// --- Graph demo helpers (adjacency-matrix input) ---

EMSCRIPTEN_KEEPALIVE
int wasm_graph_edge_count(int n, int directed, const int* adj01) {
    if (n < 0) {
        return 0;
    }
    if (!adj01) {
        return 0;
    }

    int count = 0;
    if (directed != 0) {
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                if (i == j) {
                    continue;
                }
                if (adj01_has_edge_directed(adj01, n, i, j)) {
                    count++;
                }
            }
        }
        return count;
    }

    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            if (adj01_has_edge_undirected_or(adj01, n, i, j)) {
                count++;
            }
        }
    }
    return count;
}

EMSCRIPTEN_KEEPALIVE
void wasm_graph_all_pairs_bfs_distances(int n, int directed, const int* adj01,
                                       int* out_dist) {
    if (n <= 0) {
        return;
    }
    if (!adj01 || !out_dist) {
        return;
    }

    std::vector<int> dist(static_cast<std::size_t>(n), -1);
    std::queue<int> q;

    for (int s = 0; s < n; s++) {
        for (int i = 0; i < n; i++) {
            dist[static_cast<std::size_t>(i)] = -1;
        }
        while (!q.empty()) {
            q.pop();
        }

        dist[static_cast<std::size_t>(s)] = 0;
        q.push(s);

        while (!q.empty()) {
            const int u = q.front();
            q.pop();
            const int du = dist[static_cast<std::size_t>(u)];

            for (int v = 0; v < n; v++) {
                if (u == v) {
                    continue;
                }
                bool has = false;
                if (directed != 0) {
                    has = adj01_has_edge_directed(adj01, n, u, v);
                } else {
                    has = adj01_has_edge_undirected_or(adj01, n, u, v);
                }
                if (!has) {
                    continue;
                }
                if (dist[static_cast<std::size_t>(v)] != -1) {
                    continue;
                }
                dist[static_cast<std::size_t>(v)] = du + 1;
                q.push(v);
            }
        }

        for (int t = 0; t < n; t++) {
            out_dist[s * n + t] = dist[static_cast<std::size_t>(t)];
        }
    }
}

EMSCRIPTEN_KEEPALIVE
int wasm_graph_metric_dimension(int n, const int* adj01, int* out_dim,
                                int* out_basis, int basis_max) {
    if (n < 0) {
        return 0;
    }
    if (!adj01 || !out_dim) {
        return 0;
    }
    if (basis_max < 0) {
        return 0;
    }

    Graph<int> g = build_undirected_graph_from_adj01_or(adj01, n);
    MetricDimensionResult md = metric_dimension_brute_force(g);
    *out_dim = md.dimension;

    if (out_basis) {
        const int write_n =
            md.dimension < basis_max ? md.dimension : basis_max;
        for (int i = 0; i < write_n; i++) {
            out_basis[i] = md.basis[static_cast<std::size_t>(i)];
        }
    }

    return md.dimension;
}
}
