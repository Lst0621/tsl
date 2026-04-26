#include <emscripten/emscripten.h>

#include <cstdint>
#include <vector>

#include "algebra/linear_recurrence.h"
#include "algebra/matrix.h"
#include "algebra/semigroup.h"

namespace {

// Packed int64: [lo32, hi32] in two's complement.
inline std::int64_t read_i64(const int* p) {
    const std::uint64_t lo = static_cast<std::uint32_t>(p[0]);
    const std::uint64_t hi = static_cast<std::uint32_t>(p[1]);
    const std::uint64_t u = lo | (hi << 32);
    return static_cast<std::int64_t>(u);
}

inline void write_i64(int* p, std::int64_t v) {
    const std::uint64_t u = static_cast<std::uint64_t>(v);
    p[0] = static_cast<int>(static_cast<std::uint32_t>(u & 0xFFFFFFFFu));
    p[1] = static_cast<int>(
        static_cast<std::uint32_t>((u >> 32) & 0xFFFFFFFFu));
}

inline LinearRecurrence::Coeff read_rational(const int* p) {
    const std::int64_t m = read_i64(p);
    const std::int64_t n = read_i64(p + 2);
    return LinearRecurrence::Coeff(static_cast<long long>(m),
                                   static_cast<long long>(n));
}

inline void write_rational(int* p, const LinearRecurrence::Coeff& r) {
    write_i64(p, static_cast<std::int64_t>(r.m()));
    write_i64(p + 2, static_cast<std::int64_t>(r.n()));
}

}  // namespace

extern "C" {

// --- Linear recurrence (opaque handle = LinearRecurrence*) ---
EMSCRIPTEN_KEEPALIVE
void* lr_create(const int* coeffs_ptr, int coeffs_len,
                int recursive_threshold) {
    std::vector<LinearRecurrence::Coeff> coeffs;
    coeffs.reserve(static_cast<std::size_t>(coeffs_len));
    for (int i = 0; i < coeffs_len; ++i) {
        coeffs.push_back(read_rational(coeffs_ptr + 4 * i));
    }
    return new LinearRecurrence(coeffs,
                                static_cast<std::size_t>(recursive_threshold));
}

EMSCRIPTEN_KEEPALIVE
void lr_destroy(void* handle) {
    delete static_cast<LinearRecurrence*>(handle);
}

EMSCRIPTEN_KEEPALIVE
int lr_order(void* handle) {
    return static_cast<int>(static_cast<LinearRecurrence*>(handle)->order());
}

EMSCRIPTEN_KEEPALIVE
void lr_evaluate(void* handle, const int* init_ptr, int init_len, int n,
                 int* result_ptr) {
    std::vector<LinearRecurrence::Coeff> init;
    init.reserve(static_cast<std::size_t>(init_len));
    for (int i = 0; i < init_len; ++i) {
        init.push_back(read_rational(init_ptr + 4 * i));
    }
    LinearRecurrence::Coeff val =
        static_cast<LinearRecurrence*>(handle)->evaluate(
            init, static_cast<std::size_t>(n));
    write_rational(result_ptr, val);
}

EMSCRIPTEN_KEEPALIVE
int lr_characteristic_polynomial(void* handle, int* out_ptr, int max_len) {
    const auto& coeffs = static_cast<LinearRecurrence*>(handle)
                             ->characteristic_polynomial()
                             .get_coefficients();
    int len = static_cast<int>(coeffs.size());
    if (len > max_len) {
        len = max_len;
    }
    for (int i = 0; i < len; ++i) {
        write_rational(out_ptr + 4 * i, coeffs[static_cast<std::size_t>(i)]);
    }
    return len;
}

EMSCRIPTEN_KEEPALIVE
int lr_transition_matrix_size(void* handle) {
    return static_cast<int>(
        static_cast<LinearRecurrence*>(handle)->transition_matrix().get_rows());
}

EMSCRIPTEN_KEEPALIVE
void lr_transition_matrix_data(void* handle, int* out_ptr, int max_len) {
    const Matrix<LinearRecurrence::Coeff>& M =
        static_cast<LinearRecurrence*>(handle)->transition_matrix();
    std::size_t rows = M.get_rows();
    std::size_t cols = M.get_cols();
    std::size_t idx = 0;
    for (std::size_t i = 0; i < rows && idx < static_cast<std::size_t>(max_len);
         ++i) {
        for (std::size_t j = 0;
             j < cols && idx < static_cast<std::size_t>(max_len); ++j) {
            write_rational(out_ptr + 4 * idx, M.at(i, j));
            idx++;
        }
    }
}

EMSCRIPTEN_KEEPALIVE
void lr_evaluate_poly_at_matrix(void* handle, int* out_ptr, int max_len) {
    LinearRecurrence* lr = static_cast<LinearRecurrence*>(handle);
    Matrix<LinearRecurrence::Coeff> pM =
        lr->characteristic_polynomial().apply(lr->transition_matrix());
    std::size_t rows = pM.get_rows();
    std::size_t cols = pM.get_cols();
    std::size_t idx = 0;
    for (std::size_t i = 0; i < rows && idx < static_cast<std::size_t>(max_len);
         ++i) {
        for (std::size_t j = 0;
             j < cols && idx < static_cast<std::size_t>(max_len); ++j) {
            write_rational(out_ptr + 4 * idx, pM.at(i, j));
            idx++;
        }
    }
}

// --- Matrix helpers used by linear_recur page ---
EMSCRIPTEN_KEEPALIVE
void wasm_matrix_power(const int* data_ptr, int n, int exponent, int* out_ptr) {
    std::vector<long long> data(static_cast<std::size_t>(n) *
                                static_cast<std::size_t>(n));
    for (int i = 0; i < n * n; ++i) {
        data[static_cast<std::size_t>(i)] = static_cast<long long>(data_ptr[i]);
    }
    Matrix<long long> M =
        to_matrix<long long, std::vector<long long>>(data, n, n);
    std::vector<std::vector<long long>> id_data(
        static_cast<std::size_t>(n),
        std::vector<long long>(static_cast<std::size_t>(n), 0LL));
    for (int i = 0; i < n; ++i) {
        id_data[static_cast<std::size_t>(i)][static_cast<std::size_t>(i)] = 1LL;
    }
    Matrix<long long> identity(id_data);
    Matrix<long long> result =
        monoid_power(M, static_cast<unsigned long long>(exponent), identity);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            out_ptr[i * n + j] = static_cast<int>(result.at(
                static_cast<std::size_t>(i), static_cast<std::size_t>(j)));
        }
    }
}

EMSCRIPTEN_KEEPALIVE
void wasm_matrix_times_const(const int* data_ptr, int n, int scalar,
                             int* out_ptr) {
    std::vector<long long> data(static_cast<std::size_t>(n) *
                                static_cast<std::size_t>(n));
    for (int i = 0; i < n * n; ++i) {
        data[static_cast<std::size_t>(i)] = static_cast<long long>(data_ptr[i]);
    }
    Matrix<long long> M =
        to_matrix<long long, std::vector<long long>>(data, n, n);
    Matrix<long long> result = static_cast<long long>(scalar) * M;
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            out_ptr[i * n + j] = static_cast<int>(result.at(
                static_cast<std::size_t>(i), static_cast<std::size_t>(j)));
        }
    }
}

EMSCRIPTEN_KEEPALIVE
void wasm_matrix_add(const int* data_a_ptr, const int* data_b_ptr, int n,
                     int* out_ptr) {
    std::vector<long long> a_data(static_cast<std::size_t>(n) *
                                  static_cast<std::size_t>(n));
    std::vector<long long> b_data(static_cast<std::size_t>(n) *
                                  static_cast<std::size_t>(n));
    for (int i = 0; i < n * n; ++i) {
        a_data[static_cast<std::size_t>(i)] =
            static_cast<long long>(data_a_ptr[i]);
        b_data[static_cast<std::size_t>(i)] =
            static_cast<long long>(data_b_ptr[i]);
    }
    Matrix<long long> A =
        to_matrix<long long, std::vector<long long>>(a_data, n, n);
    Matrix<long long> B =
        to_matrix<long long, std::vector<long long>>(b_data, n, n);
    Matrix<long long> result = A + B;
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            out_ptr[i * n + j] = static_cast<int>(result.at(
                static_cast<std::size_t>(i), static_cast<std::size_t>(j)));
        }
    }
}
}

