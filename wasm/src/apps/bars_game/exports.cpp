#include <emscripten/emscripten.h>

#include <cstdint>

#include "bars_game.h"

extern "C" {

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
void bars_game_set_seed(void* handle, std::uint32_t seed) {
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

