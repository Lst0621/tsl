#include <emscripten/emscripten.h>

#include <cstdint>
#include <vector>

#include "game_of_life.h"
#include "topology/coordinate.h"
#include "topology/topology.h"

extern "C" {

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
void gol_random_init_seed(void* handle, double live_prob, std::uint32_t seed) {
    static_cast<GameOfLife*>(handle)->random_init(live_prob, seed);
}

EMSCRIPTEN_KEEPALIVE
std::uint32_t gol_get_seed(void* handle) {
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
void gol_set_wormhole_seed(void* handle, std::uint32_t seed) {
    static_cast<GameOfLife*>(handle)->set_wormhole_seed(seed);
}

EMSCRIPTEN_KEEPALIVE
void gol_set_wormhole_count(void* handle, int count) {
    static_cast<GameOfLife*>(handle)->set_wormhole_count(count);
}

EMSCRIPTEN_KEEPALIVE
int gol_get_wormhole_edges(void* handle) {
    return static_cast<GameOfLife*>(handle)->debug_wormhole_edge_count();
}

EMSCRIPTEN_KEEPALIVE
void gol_set_cut_seed(void* handle, std::uint32_t seed) {
    static_cast<GameOfLife*>(handle)->set_cut_seed(seed);
}

EMSCRIPTEN_KEEPALIVE
void gol_set_cut_count(void* handle, int count) {
    static_cast<GameOfLife*>(handle)->set_cut_count(count);
}

EMSCRIPTEN_KEEPALIVE
int gol_get_cut_edges(void* handle) {
    return static_cast<GameOfLife*>(handle)->debug_cut_node_count();
}

EMSCRIPTEN_KEEPALIVE
int gol_get_live_cells(void* handle, int* out_xy, int max_count) {
    std::vector<Coord2D> cells =
        static_cast<GameOfLife*>(handle)->get_live_cells();
    int n = static_cast<int>(cells.size());
    if (n > max_count) {
        n = max_count;
    }
    for (int i = 0; i < n; ++i) {
        out_xy[2 * i] = cells[static_cast<std::size_t>(i)].x;
        out_xy[2 * i + 1] = cells[static_cast<std::size_t>(i)].y;
    }
    return static_cast<int>(cells.size());
}
}

