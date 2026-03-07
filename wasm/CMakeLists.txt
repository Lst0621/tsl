cmake_minimum_required(VERSION 3.15)
project(wasm_sample)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXECUTABLE_SUFFIX ".js")

add_executable(wasm_sample
        src/main.cpp
        src/comb.cpp
        src/comb.h
        src/game_of_life.cpp
        src/game_of_life.h
        src/helper.h
        src/modular_number.cpp
        src/modular_number.h
        src/semigroup.h
        src/matrix.h
        src/general_linear_group.h
        src/general_linear_group.cpp
        src/coordinate.h
        src/topology.h
)


target_link_options(wasm_sample PRIVATE
        -sWASM=1
        -sMODULARIZE=1
        -sEXPORT_ES6=1
        -sENVIRONMENT=web
        -O1
)

target_link_options(wasm_sample PRIVATE
    -sEXPORTED_FUNCTIONS=_wasm_number_of_sequences,_wasm_number_of_sequences_all,_wasm_get_gl_n_zm_size,_wasm_matrix_det,_gol_create,_gol_destroy,_gol_init,_gol_random_init,_gol_random_init_seed,_gol_get_seed,_gol_evolve,_gol_set_topology,_gol_get_live_cells,_malloc,_free
)