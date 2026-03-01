#include <emscripten/emscripten.h>
#include "add.h"
#include "minus.h"
#include "comb.h"

extern "C" {
EMSCRIPTEN_KEEPALIVE
int wasm_add(int a, int b) {
    return add(a, b);
}

EMSCRIPTEN_KEEPALIVE
int wasm_minus(int a, int b) {
    return minus(a, b);
}


EMSCRIPTEN_KEEPALIVE
int wasm_number_of_sequences(int* arr, int arr_len, int* seq, int seq_len) {
    std::vector arr_vec(arr, arr + arr_len);
    std::vector seq_vec(seq, seq + seq_len);
    return number_of_sequences(arr_vec, seq_vec);
}

}
