#include <emscripten/emscripten.h>

#include <cstdlib>
#include <vector>

#include "algebra/general_linear_group.h"
#include "algebra/matrix.h"
#include "algebra/semigroup.h"
#include "number/modular_number.h"

extern "C" {

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
    const int size = n * n;
    std::vector<int> data_vec(data, data + size);
    Matrix<int> mat = to_matrix<int, std::vector<int>>(data_vec, n, n);
    return mat.determinant();
}

EMSCRIPTEN_KEEPALIVE
void wasm_matrix_inverse_mod(int* data, int n, int m, int* out) {
    ModularNumber zero(0, m);
    std::vector<std::vector<ModularNumber>> mat_data(
        n, std::vector<ModularNumber>(n, zero));
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
}

