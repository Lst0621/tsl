cmake_minimum_required(VERSION 3.15)
project(tsl_wasm_apps)

set(CMAKE_CXX_STANDARD 20)

# Shared implementation sources compiled for each active toolchain/profile.
set(TSL_CORE_SOURCES
        src/core/combinatorics/comb.cpp
        src/core/algebra/general_linear_group.cpp
        src/core/number/big_int.cpp
        src/core/number/modular_number.cpp
        src/core/string/lang_string.cpp
)

set(TSL_CORE_HEADERS
        src/core/combinatorics/comb.h
        src/core/algebra/general_linear_group.h
        src/core/algebra/linear_recurrence.h
        src/core/algebra/matrix.h
        src/core/algebra/polynomial.h
        src/core/algebra/rational_function.h
        src/core/algebra/semigroup.h
        src/core/algebra/sequence_ops.h
        src/core/graph/graph.h
        src/core/graph/graph_distances.h
        src/core/graph/graph_generators.h
        src/core/graph/graph_utils.h
        src/core/graph/metric_dimension.h
        src/core/number/big_int.h
        src/core/number/complex.h
        src/core/number/modular_number.h
        src/core/number/rational.h
        src/core/string/lang_string.h
        src/core/topology/coordinate.h
        src/core/topology/topology.h
        src/core/util/helper.h
        src/core/util/scoped_timer.h
        src/core/util/set_utils.h
        src/core/util/small_vector.h
)

add_library(tsl_core STATIC
        ${TSL_CORE_SOURCES}
        ${TSL_CORE_HEADERS}
)

target_include_directories(tsl_core PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core
)

add_library(tsl_apps STATIC
        src/apps/gol/game_of_life.cpp
        src/apps/gol/game_of_life.h
        src/apps/bars_game/bars_game.cpp
        src/apps/bars_game/bars_game.h
)

target_include_directories(tsl_apps PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core
        ${CMAKE_CURRENT_SOURCE_DIR}/src/apps/gol
        ${CMAKE_CURRENT_SOURCE_DIR}/src/apps/bars_game
)

# Only set WASM executable suffix when using Emscripten.
if(CMAKE_C_COMPILER MATCHES "emcc")
    set(CMAKE_EXECUTABLE_SUFFIX ".js")
endif()

# WASM executables - only build when using Emscripten.
if(CMAKE_C_COMPILER MATCHES "emcc")
    function(tsl_add_wasm_app target entry_source exported_functions_csv)
        add_executable(${target} ${entry_source})
        target_link_libraries(${target} PRIVATE tsl_core)
        target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/core)

        target_link_options(${target} PRIVATE
                -sWASM=1
                -sMODULARIZE=1
                -sEXPORT_ES6=1
                -sENVIRONMENT=web
                -O1
        )

        target_link_options(${target} PRIVATE
                -sEXPORTED_FUNCTIONS=${exported_functions_csv}
        )
    endfunction()

    # Reusable WASM modules (domain-based exports).

    tsl_add_wasm_app(wasm_sequences
            src/modules/sequences/exports.cpp
            _wasm_number_of_sequences,_wasm_number_of_sequences_all,_malloc,_free
    )

    tsl_add_wasm_app(wasm_gol
            src/apps/gol/exports.cpp
            _gol_create,_gol_destroy,_gol_init,_gol_random_init,_gol_random_init_seed,_gol_get_seed,_gol_evolve,_gol_set_topology,_gol_set_wormhole_seed,_gol_set_wormhole_count,_gol_get_wormhole_edges,_gol_set_cut_seed,_gol_set_cut_count,_gol_get_cut_edges,_gol_get_live_cells,_malloc,_free
    )
    target_link_libraries(wasm_gol PRIVATE tsl_apps)
    target_include_directories(wasm_gol PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/apps/gol)

    tsl_add_wasm_app(wasm_bars_game
            src/apps/bars_game/exports.cpp
            _bars_game_create,_bars_game_destroy,_bars_game_set_seed,_bars_game_init,_bars_game_get_state,_bars_game_get_future_state,_bars_game_apply_choice,_bars_game_is_ended,_bars_game_state_size,_bars_game_num_choices,_bars_game_min_val,_bars_game_max_val,_malloc,_free
    )
    target_link_libraries(wasm_bars_game PRIVATE tsl_apps)
    target_include_directories(wasm_bars_game PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src/apps/bars_game)

    tsl_add_wasm_app(wasm_linear_recur
            src/modules/linear_recur/exports.cpp
            _lr_create,_lr_destroy,_lr_order,_lr_evaluate,_lr_characteristic_polynomial,_lr_transition_matrix_size,_lr_transition_matrix_data,_lr_evaluate_poly_at_matrix,_wasm_matrix_power,_wasm_matrix_times_const,_wasm_matrix_add,_malloc,_free
    )

    tsl_add_wasm_app(wasm_graph
            src/modules/graph/exports.cpp
            _wasm_graph_edge_count,_wasm_graph_randomize_undirected_adj01,_wasm_graph_all_pairs_bfs_distances,_wasm_graph_metric_dimension,_wasm_graph_resolving_subsets_from_dist,_wasm_graph_resolving_subsets_with_non_resolving_size_minus_one_from_dist,_wasm_graph_resolving_subsets_all_modes_with_non_resolving_size_minus_one_from_dist,_wasm_graph_resolving_subsets_all_modes_paginated_with_non_resolving_size_minus_one_from_dist,_wasm_graph_resolving_subsets_cache_create,_wasm_graph_resolving_subsets_cache_destroy,_wasm_graph_resolving_subsets_cache_set_graph,_wasm_graph_resolving_subsets_cache_get_page,_malloc,_free
    )

    # Algebra utilities: GL(n, Z_m) generation and matrix helpers.
    tsl_add_wasm_app(wasm_matrix
            src/modules/matrix/exports.cpp
            _wasm_get_gl_n_zm_size,_wasm_get_gl_n_zm,_wasm_is_matrix_group,_wasm_matrix_det,_wasm_matrix_inverse_mod,_malloc,_free
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
                tsl_apps
                GTest::gtest
                GTest::gtest_main
        )

        gtest_discover_tests(tsl_core_tests)
    endif()

endif()
