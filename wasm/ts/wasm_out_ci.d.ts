declare module "../wasm_out_ci/wasm_sequences" {
    export interface WasmSequencesModule {
        _wasm_number_of_sequences(arr_ptr: number, arr_len: number, seq_ptr: number, seq_len: number): number;
        _wasm_number_of_sequences_all(arr_ptr: number, arr_len: number, sequence_ptr: number, seq_len: number): number;
        _malloc(size: number): number;
        _free(ptr: number): void;
    }

    const init: (moduleOverrides?: { wasmBinary?: Buffer | Uint8Array }) => Promise<WasmSequencesModule>;
    export default init;
}

declare module "../wasm_out_ci/wasm_matrix" {
    export interface WasmMatrixModule {
        _wasm_get_gl_n_zm_size(n: number, m: number): number;
        _wasm_get_gl_n_zm(n: number, m: number, out_count_ptr: number): number;
        _wasm_is_matrix_group(data_ptr: number, count: number, n: number, modulus: number): number;
        _wasm_matrix_det(data_ptr: number, n: number): number;
        _wasm_matrix_inverse_mod(data_ptr: number, n: number, m: number, out_ptr: number): void;
        _malloc(size: number): number;
        _free(ptr: number): void;
    }

    const init: (moduleOverrides?: { wasmBinary?: Buffer | Uint8Array }) => Promise<WasmMatrixModule>;
    export default init;
}

declare module "../wasm_out_ci/wasm_gol" {
    export interface WasmGolModule {
        _gol_create(size: number): number;
        _gol_destroy(handle: number): void;
        _gol_init(handle: number): void;
        _gol_random_init(handle: number, live_prob: number): void;
        _gol_random_init_seed(handle: number, live_prob: number, seed: number): void;
        _gol_get_seed(handle: number): number;
        _gol_evolve(handle: number): void;
        _gol_set_topology(handle: number, mode: number): void;
        _gol_set_wormhole_seed(handle: number, seed: number): void;
        _gol_set_wormhole_count(handle: number, count: number): void;
        _gol_get_wormhole_edges(handle: number): number;
        _gol_set_cut_seed(handle: number, seed: number): void;
        _gol_set_cut_count(handle: number, count: number): void;
        _gol_get_cut_edges(handle: number): number;
        _gol_get_live_cells(handle: number, out_xy_ptr: number, max_count: number): number;
        _malloc(size: number): number;
        _free(ptr: number): void;
    }

    const init: (moduleOverrides?: { wasmBinary?: Buffer | Uint8Array }) => Promise<WasmGolModule>;
    export default init;
}

declare module "../wasm_out_ci/wasm_bars_game" {
    export interface WasmBarsGameModule {
        _bars_game_create(): number;
        _bars_game_destroy(handle: number): void;
        _bars_game_set_seed(handle: number, seed: number): void;
        _bars_game_init(handle: number): void;
        _bars_game_get_state(handle: number, out_ptr: number): void;
        _bars_game_get_future_state(handle: number, choice_index: number, out_ptr: number): void;
        _bars_game_apply_choice(handle: number, index: number): void;
        _bars_game_is_ended(handle: number): number;
        _bars_game_state_size(handle: number): number;
        _bars_game_num_choices(handle: number): number;
        _bars_game_min_val(handle: number): number;
        _bars_game_max_val(handle: number): number;
        _malloc(size: number): number;
        _free(ptr: number): void;
    }

    const init: (moduleOverrides?: { wasmBinary?: Buffer | Uint8Array }) => Promise<WasmBarsGameModule>;
    export default init;
}

declare module "../wasm_out_ci/wasm_graph" {
    export interface WasmGraphModule {
        _wasm_graph_edge_count(n: number, directed: number, adj01_ptr: number): number;
        _wasm_graph_randomize_undirected_adj01(
            n: number,
            seed: number,
            out_adj01_ptr: number,
            out_edge_count_ptr: number,
            out_threshold_milli_ptr: number,
        ): number;
        _wasm_graph_all_pairs_bfs_distances(
            n: number,
            directed: number,
            adj01_ptr: number,
            out_dist_ptr: number,
        ): void;
        _wasm_graph_metric_dimension(
            n: number,
            adj01_ptr: number,
            out_dim_ptr: number,
            out_basis_ptr: number,
            basis_max: number,
        ): number;
        _wasm_graph_resolving_subsets_from_dist(
            n: number,
            adj01_ptr: number,
            dist_flat_ptr: number,
            mode: number,
            list_max_k: number,
            out_min_dim_ptr: number,
            out_smallest_basis_ptr: number,
            basis_max: number,
            out_list_count_ptr: number,
            out_list_used_ints_ptr: number,
            out_list_flat_ptr: number,
            list_flat_max_ints: number,
            out_list_truncated_ptr: number,
        ): number;
        _wasm_graph_resolving_subsets_with_non_resolving_size_minus_one_from_dist(
            n: number,
            adj01_ptr: number,
            dist_flat_ptr: number,
            mode: number,
            list_max_k: number,
            out_min_dim_ptr: number,
            out_smallest_basis_ptr: number,
            basis_max: number,
            out_list_count_ptr: number,
            out_list_used_ints_ptr: number,
            out_list_flat_ptr: number,
            list_flat_max_ints: number,
            out_list_truncated_ptr: number,
        ): number;
        _wasm_graph_resolving_subsets_all_modes_with_non_resolving_size_minus_one_from_dist(
            n: number,
            adj01_ptr: number,
            dist_flat_ptr: number,
            list_max_k: number,
            out_min_dim_3_ptr: number,
            out_smallest_basis_3_ptr: number,
            basis_max: number,
            out_list_count_3_ptr: number,
            out_list_used_ints_3_ptr: number,
            out_list_flat_3_ptr: number,
            list_flat_max_ints_per_mode: number,
            out_list_truncated_3_ptr: number,
        ): number;
        _wasm_graph_resolving_subsets_all_modes_paginated_with_non_resolving_size_minus_one_from_dist(
            n: number,
            adj01_ptr: number,
            dist_flat_ptr: number,
            page_size: number,
            page_index_3_ptr: number,
            out_min_dim_3_ptr: number,
            out_smallest_basis_3_ptr: number,
            basis_max: number,
            out_total_count_3_ptr: number,
            out_page_count_3_ptr: number,
            out_page_list_count_3_ptr: number,
            out_page_list_used_ints_3_ptr: number,
            out_page_list_flat_3_ptr: number,
            page_list_flat_max_ints_per_mode: number,
            out_page_list_truncated_3_ptr: number,
        ): number;
        _wasm_graph_resolving_subsets_cache_create(): number;
        _wasm_graph_resolving_subsets_cache_destroy(handle: number): void;
        _wasm_graph_resolving_subsets_cache_set_graph(
            handle: number,
            n: number,
            adj01_ptr: number,
            dist_flat_ptr: number,
        ): number;
        _wasm_graph_resolving_subsets_cache_get_page(
            handle: number,
            page_size: number,
            page_index_3_ptr: number,
            out_min_dim_3_ptr: number,
            out_smallest_basis_3_ptr: number,
            basis_max: number,
            out_total_count_3_ptr: number,
            out_page_count_3_ptr: number,
            out_page_list_count_3_ptr: number,
            out_page_list_used_ints_3_ptr: number,
            out_page_list_flat_3_ptr: number,
            page_list_flat_max_ints_per_mode: number,
            out_page_list_truncated_3_ptr: number,
            out_min_list_count_3_ptr: number,
            out_min_list_used_ints_3_ptr: number,
            out_min_list_flat_3_ptr: number,
            min_list_flat_max_ints_per_mode: number,
            out_min_list_truncated_3_ptr: number,
        ): number;
        _malloc(size: number): number;
        _free(ptr: number): void;
    }

    const init: (moduleOverrides?: { wasmBinary?: Buffer | Uint8Array }) => Promise<WasmGraphModule>;
    export default init;
}

declare module "../wasm_out_ci/wasm_linear_recur" {
    export interface WasmLinearRecurModule {
        _lr_create(coeffs_ptr: number, coeffs_len: number, recursive_threshold: number): number;
        _lr_destroy(handle: number): void;
        _lr_order(handle: number): number;
        _lr_evaluate(handle: number, init_ptr: number, init_len: number, n: number, result_ptr: number): void;
        _lr_characteristic_polynomial(handle: number, out_ptr: number, max_len: number): number;
        _lr_transition_matrix_size(handle: number): number;
        _lr_transition_matrix_data(handle: number, out_ptr: number, max_len: number): void;
        _lr_evaluate_poly_at_matrix(handle: number, out_ptr: number, max_len: number): void;
        _wasm_matrix_power(data_ptr: number, n: number, exponent: number, out_ptr: number): void;
        _wasm_matrix_times_const(data_ptr: number, n: number, scalar: number, out_ptr: number): void;
        _wasm_matrix_add(data_a_ptr: number, data_b_ptr: number, n: number, out_ptr: number): void;
        _malloc(size: number): number;
        _free(ptr: number): void;
    }

    const init: (moduleOverrides?: { wasmBinary?: Buffer | Uint8Array }) => Promise<WasmLinearRecurModule>;
    export default init;
}

