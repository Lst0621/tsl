cmake_minimum_required(VERSION 3.15)
project(wasm_sample)

set(CMAKE_CXX_STANDARD 20)

# Shared implementation sources compiled for each active toolchain/profile.
set(TSL_CORE_SOURCES
        src/bars_game.cpp
        src/big_int.cpp
        src/comb.cpp
        src/game_of_life.cpp
        src/lang_string.cpp
        src/modular_number.cpp
        src/general_linear_group.cpp
)

set(TSL_CORE_HEADERS
        src/bars_game.h
        src/big_int.h
        src/comb.h
        src/game_of_life.h
        src/graph/graph.h
        src/graph/graph_distances.h
        src/graph/graph_generators.h
        src/graph/metric_dimension.h
        src/scoped_timer.h
        src/helper.h
        src/lang_string.h
        src/modular_number.h
        src/rational.h
        src/rational_function.h
        src/semigroup.h
        src/set_utils.h
        src/matrix.h
        src/small_vector.h
        src/general_linear_group.h
        src/coordinate.h
        src/topology.h
        src/polynomial.h
        src/linear_recurrence.h
        src/complex.h
        src/sequence_ops.h
)

add_library(tsl_core STATIC
        ${TSL_CORE_SOURCES}
        ${TSL_CORE_HEADERS}
)

target_include_directories(tsl_core PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Only set WASM executable suffix when using Emscripten.
if(CMAKE_C_COMPILER MATCHES "emcc")
    set(CMAKE_EXECUTABLE_SUFFIX ".js")
endif()

# Main WASM executable - only build when using Emscripten.
if(CMAKE_C_COMPILER MATCHES "emcc")
    add_executable(wasm_sample
            src/main.cpp
    )

    target_link_libraries(wasm_sample PRIVATE tsl_core)

    target_link_options(wasm_sample PRIVATE
            -sWASM=1
            -sMODULARIZE=1
            -sEXPORT_ES6=1
            -sENVIRONMENT=web
            -O1
    )

    target_link_options(wasm_sample PRIVATE
            -sEXPORTED_FUNCTIONS=_wasm_number_of_sequences,_wasm_number_of_sequences_all,_wasm_get_gl_n_zm_size,_wasm_get_gl_n_zm,_wasm_is_matrix_group,_wasm_matrix_det,_wasm_matrix_inverse_mod,_gol_create,_gol_destroy,_gol_init,_gol_random_init,_gol_random_init_seed,_gol_get_seed,_gol_evolve,_gol_set_topology,_gol_set_wormhole_seed,_gol_set_wormhole_count,_gol_get_wormhole_edges,_gol_set_cut_seed,_gol_set_cut_count,_gol_get_cut_edges,_gol_get_live_cells,_lr_create,_lr_destroy,_lr_order,_lr_evaluate,_lr_characteristic_polynomial,_lr_transition_matrix_size,_lr_transition_matrix_data,_lr_evaluate_poly_at_matrix,_wasm_matrix_power,_wasm_matrix_times_const,_wasm_matrix_add,_bars_game_create,_bars_game_destroy,_bars_game_set_seed,_bars_game_init,_bars_game_get_state,_bars_game_get_future_state,_bars_game_apply_choice,_bars_game_is_ended,_bars_game_state_size,_bars_game_num_choices,_bars_game_min_val,_bars_game_max_val,_wasm_graph_edge_count,_wasm_graph_randomize_undirected_adj01,_wasm_graph_all_pairs_bfs_distances,_wasm_graph_metric_dimension,_wasm_graph_resolving_subsets_from_dist,_wasm_graph_resolving_subsets_with_non_resolving_size_minus_one_from_dist,_wasm_graph_resolving_subsets_all_modes_with_non_resolving_size_minus_one_from_dist,_wasm_graph_resolving_subsets_all_modes_paginated_with_non_resolving_size_minus_one_from_dist,_wasm_graph_resolving_subsets_cache_create,_wasm_graph_resolving_subsets_cache_destroy,_wasm_graph_resolving_subsets_cache_set_graph,_wasm_graph_resolving_subsets_cache_get_page,_malloc,_free
    )
endif()

# Google Test Configuration (for native C++ testing, not WASM).
if(NOT CMAKE_C_COMPILER MATCHES "emcc")
    enable_testing()

    include(FetchContent)
    FetchContent_Declare(
            googletest
            URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
            DOWNLOAD_EXTRACT_TIMESTAMP ON
    )

    # For Windows: Prevent overriding the parent project's compiler/linker settings.
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    include(GoogleTest)

    # Create GoogleTest-based executable when sources exist.
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/gtest/polynomial_test_gtest.cpp")
        add_executable(tsl_core_tests
                gtest/big_int_test_gtest.cpp
                gtest/big_rational_test_gtest.cpp
                gtest/polynomial_test_gtest.cpp
                gtest/linear_recurrence_test_gtest.cpp
                gtest/set_utils_test_gtest.cpp
                gtest/semigroup_test_gtest.cpp
                gtest/lang_string_test_gtest.cpp
                gtest/rational_test_gtest.cpp
                gtest/rational_function_test_gtest.cpp
                gtest/complex_test_gtest.cpp
                gtest/complex_qi_test_gtest.cpp
                gtest/game_of_life_test_gtest.cpp
                gtest/graph_distances_test_gtest.cpp
                gtest/graph_generators_test_gtest.cpp
                gtest/graph_metric_dimension_test_gtest.cpp
                gtest/graph_test_gtest.cpp
                gtest/matrix_test_gtest.cpp
        )

        target_link_libraries(tsl_core_tests
                tsl_core
                GTest::gtest
                GTest::gtest_main
        )

        gtest_discover_tests(tsl_core_tests)
    endif()

endif()
