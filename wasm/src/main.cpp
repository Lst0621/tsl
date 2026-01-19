#include <emscripten/emscripten.h>
#include "add.h"
#include "minus.h"

extern "C" {

EMSCRIPTEN_KEEPALIVE
int wasm_add(int a, int b) {
    return add(a, b);
}

EMSCRIPTEN_KEEPALIVE
int wasm_minus(int a, int b) {
    return minus(a, b);
}

}
