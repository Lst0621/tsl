#include <emscripten/emscripten.h>

#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <queue>
#include <random>
#include <vector>

#include "algebra/matrix.h"
#include "graph/graph.h"
#include "graph/metric_dimension.h"
#include "util/scoped_timer.h"

namespace {

inline bool adj01_has_edge_directed(const int* adj01, int n, int from, int to) {
    return adj01[from * n + to] != 0;
}

inline bool adj01_has_edge_undirected_or(const int* adj01, int n, int a,
                                         int b) {
    return adj01[a * n + b] != 0 || adj01[b * n + a] != 0;
}

inline Graph<int> build_undirected_graph_from_adj01_or(const int* adj01, int n) {
    Graph<int> g(false);
    for (int i = 0; i < n; i++) {
        g.add_vertex(i);
    }
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            if (adj01_has_edge_undirected_or(adj01, n, i, j)) {
                g.add_edge(i, j);
            }
        }
    }
    return g;
}

inline std::vector<UndirectedEdge01> build_undirected_edges_from_adj01_or(
    const int* adj01, int n) {
    std::vector<UndirectedEdge01> edges;
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            if (adj01_has_edge_undirected_or(adj01, n, i, j)) {
                edges.push_back(UndirectedEdge01{i, j});
            }
        }
    }
    return edges;
}

inline Matrix<int> to_matrix_int_row_major(const int* flat, int n) {
    Matrix<int> M(static_cast<std::size_t>(n), static_cast<std::size_t>(n), 0);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            M(static_cast<std::size_t>(i), static_cast<std::size_t>(j)) =
                flat[i * n + j];
        }
    }
    return M;
}

inline bool encode_graph_resolving_subsets_result_to_buffers(
    const GraphResolvingSubsetsResult& r,
    int use_non_resolving_size_minus_one_filter, int* out_min_dim,
    int* out_smallest_basis, int basis_max, int* out_list_count,
    int* out_list_used_ints, int* out_list_flat, int list_flat_max_ints,
    int* out_list_truncated) {
    if (!out_min_dim || !out_smallest_basis || basis_max < 0) {
        return false;
    }
    if (!out_list_count || !out_list_used_ints || !out_list_flat ||
        list_flat_max_ints < 0) {
        return false;
    }
    if (!out_list_truncated) {
        return false;
    }

    *out_min_dim = r.min_dimension;

    const int write_basis_n =
        static_cast<int>(r.smallest_basis.size()) < basis_max
            ? static_cast<int>(r.smallest_basis.size())
            : basis_max;
    for (int i = 0; i < write_basis_n; i++) {
        out_smallest_basis[i] =
            r.smallest_basis[static_cast<std::size_t>(i)];
    }

    const std::vector<std::vector<int>>& subset_list =
        (use_non_resolving_size_minus_one_filter != 0)
            ? r.subsets_with_non_resolving_size_minus_one_upto_k
            : r.subsets_upto_k;

    int used = 0;
    int count = 0;
    int truncated = 0;
    for (const auto& subset : subset_list) {
        const int k = static_cast<int>(subset.size());
        const int need = 1 + k;
        if (used + need > list_flat_max_ints) {
            truncated = 1;
            break;
        }
        out_list_flat[used] = k;
        used++;
        for (int i = 0; i < k; i++) {
            out_list_flat[used] = subset[static_cast<std::size_t>(i)];
            used++;
        }
        count++;
    }
    *out_list_count = count;
    *out_list_used_ints = used;
    *out_list_truncated = truncated;
    return true;
}

inline int wasm_graph_resolving_subsets_from_dist_impl(
    int n, const int* adj01, const int* dist_flat, int mode, int list_max_k,
    int* out_min_dim, int* out_smallest_basis, int basis_max,
    int* out_list_count, int* out_list_used_ints, int* out_list_flat,
    int list_flat_max_ints, int* out_list_truncated,
    int use_non_resolving_size_minus_one_filter) {
    if (n <= 0) {
        return 0;
    }
    if (!adj01 || !dist_flat) {
        return 0;
    }
    if (!out_min_dim || !out_smallest_basis || basis_max < 0) {
        return 0;
    }
    if (!out_list_count || !out_list_used_ints || !out_list_flat ||
        list_flat_max_ints < 0) {
        return 0;
    }
    if (!out_list_truncated) {
        return 0;
    }

    const GraphResolveMode m = static_cast<GraphResolveMode>(mode);
    if (!(m == GraphResolveMode::Nodes || m == GraphResolveMode::Edges ||
          m == GraphResolveMode::NodesAndEdges)) {
        return 0;
    }

    ScopedTimer timer_total("graph_resolving_subsets_from_dist_total");

    const Matrix<int> dist = to_matrix_int_row_major(dist_flat, n);
    const std::vector<UndirectedEdge01> edges =
        build_undirected_edges_from_adj01_or(adj01, n);

    ScopedTimer timer_enum("graph_resolving_subsets_enumerate");
    const GraphResolvingSubsetsResult r =
        resolving_subsets_bruteforce_upto_k(dist, edges, m, list_max_k);

    const bool ok = encode_graph_resolving_subsets_result_to_buffers(
        r, use_non_resolving_size_minus_one_filter, out_min_dim,
        out_smallest_basis, basis_max, out_list_count, out_list_used_ints,
        out_list_flat, list_flat_max_ints, out_list_truncated);
    if (!ok) {
        return 0;
    }
    return 1;
}

struct GraphResolvePaginationCache {
    int n = 0;
    bool ready = false;
    GraphResolvingSubsetsAllModesPageResult all_modes_full;
};

}  // namespace

extern "C" {

EMSCRIPTEN_KEEPALIVE
int wasm_graph_edge_count(int n, int directed, const int* adj01) {
    if (n < 0) {
        return 0;
    }
    if (!adj01) {
        return 0;
    }

    int count = 0;
    if (directed != 0) {
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                if (i == j) {
                    continue;
                }
                if (adj01_has_edge_directed(adj01, n, i, j)) {
                    count++;
                }
            }
        }
        return count;
    }

    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            if (adj01_has_edge_undirected_or(adj01, n, i, j)) {
                count++;
            }
        }
    }
    return count;
}

EMSCRIPTEN_KEEPALIVE
int wasm_graph_randomize_undirected_adj01(int n, int seed, int* out_adj01,
                                         int* out_edge_count,
                                         int* out_threshold_milli) {
    if (n < 0) {
        return 0;
    }
    if (!out_adj01) {
        return 0;
    }

    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            out_adj01[i * n + j] = 0;
        }
    }

    std::mt19937 rng(static_cast<std::uint32_t>(seed));
    std::uniform_real_distribution<double> threshold_dist(0.4, 0.6);
    std::uniform_real_distribution<double> roll_dist(0.0, 1.0);
    const double threshold = threshold_dist(rng);

    int edge_count = 0;
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            const double roll = roll_dist(rng);
            if (roll > threshold) {
                out_adj01[i * n + j] = 1;
                out_adj01[j * n + i] = 1;
                edge_count++;
            }
        }
    }

    if (out_edge_count) {
        *out_edge_count = edge_count;
    }
    if (out_threshold_milli) {
        *out_threshold_milli =
            static_cast<int>(std::round(threshold * 1000.0));
    }
    return edge_count;
}

EMSCRIPTEN_KEEPALIVE
void wasm_graph_all_pairs_bfs_distances(int n, int directed, const int* adj01,
                                       int* out_dist) {
    if (n <= 0) {
        return;
    }
    if (!adj01 || !out_dist) {
        return;
    }

    std::vector<int> dist(static_cast<std::size_t>(n), -1);
    std::queue<int> q;

    for (int s = 0; s < n; s++) {
        for (int i = 0; i < n; i++) {
            dist[static_cast<std::size_t>(i)] = -1;
        }
        while (!q.empty()) {
            q.pop();
        }

        dist[static_cast<std::size_t>(s)] = 0;
        q.push(s);

        while (!q.empty()) {
            const int u = q.front();
            q.pop();
            const int du = dist[static_cast<std::size_t>(u)];

            for (int v = 0; v < n; v++) {
                if (u == v) {
                    continue;
                }
                bool has = false;
                if (directed != 0) {
                    has = adj01_has_edge_directed(adj01, n, u, v);
                } else {
                    has = adj01_has_edge_undirected_or(adj01, n, u, v);
                }
                if (!has) {
                    continue;
                }
                if (dist[static_cast<std::size_t>(v)] != -1) {
                    continue;
                }
                dist[static_cast<std::size_t>(v)] = du + 1;
                q.push(v);
            }
        }

        for (int t = 0; t < n; t++) {
            out_dist[s * n + t] = dist[static_cast<std::size_t>(t)];
        }
    }
}

EMSCRIPTEN_KEEPALIVE
int wasm_graph_metric_dimension(int n, const int* adj01, int* out_dim,
                                int* out_basis, int basis_max) {
    if (n < 0) {
        return 0;
    }
    if (!adj01 || !out_dim) {
        return 0;
    }
    if (basis_max < 0) {
        return 0;
    }

    Graph<int> g = build_undirected_graph_from_adj01_or(adj01, n);
    MetricDimensionResult md = metric_dimension_brute_force(g);
    *out_dim = md.dimension;

    if (out_basis) {
        const int write_n = md.dimension < basis_max ? md.dimension : basis_max;
        for (int i = 0; i < write_n; i++) {
            out_basis[i] = md.basis[static_cast<std::size_t>(i)];
        }
    }

    return md.dimension;
}

EMSCRIPTEN_KEEPALIVE
int wasm_graph_resolving_subsets_from_dist(
    int n, const int* adj01, const int* dist_flat, int mode, int list_max_k,
    int* out_min_dim, int* out_smallest_basis, int basis_max,
    int* out_list_count, int* out_list_used_ints, int* out_list_flat,
    int list_flat_max_ints, int* out_list_truncated) {
    return wasm_graph_resolving_subsets_from_dist_impl(
        n, adj01, dist_flat, mode, list_max_k, out_min_dim, out_smallest_basis,
        basis_max, out_list_count, out_list_used_ints, out_list_flat,
        list_flat_max_ints, out_list_truncated, 0);
}

EMSCRIPTEN_KEEPALIVE
int wasm_graph_resolving_subsets_with_non_resolving_size_minus_one_from_dist(
    int n, const int* adj01, const int* dist_flat, int mode, int list_max_k,
    int* out_min_dim, int* out_smallest_basis, int basis_max,
    int* out_list_count, int* out_list_used_ints, int* out_list_flat,
    int list_flat_max_ints, int* out_list_truncated) {
    return wasm_graph_resolving_subsets_from_dist_impl(
        n, adj01, dist_flat, mode, list_max_k, out_min_dim, out_smallest_basis,
        basis_max, out_list_count, out_list_used_ints, out_list_flat,
        list_flat_max_ints, out_list_truncated, 1);
}

EMSCRIPTEN_KEEPALIVE
int wasm_graph_resolving_subsets_all_modes_with_non_resolving_size_minus_one_from_dist(
    int n, const int* adj01, const int* dist_flat, int list_max_k,
    int* out_min_dim_3, int* out_smallest_basis_3, int basis_max,
    int* out_list_count_3, int* out_list_used_ints_3, int* out_list_flat_3,
    int list_flat_max_ints_per_mode, int* out_list_truncated_3) {
    if (n <= 0) {
        return 0;
    }
    if (!adj01 || !dist_flat) {
        return 0;
    }
    if (!out_min_dim_3 || !out_smallest_basis_3 || basis_max < 0) {
        return 0;
    }
    if (!out_list_count_3 || !out_list_used_ints_3 || !out_list_flat_3 ||
        list_flat_max_ints_per_mode < 0) {
        return 0;
    }
    if (!out_list_truncated_3) {
        return 0;
    }

    ScopedTimer timer_total("graph_resolving_subsets_all_modes_total");
    const Matrix<int> dist = to_matrix_int_row_major(dist_flat, n);
    const std::vector<UndirectedEdge01> edges =
        build_undirected_edges_from_adj01_or(adj01, n);

    ScopedTimer timer_enum("graph_resolving_subsets_all_modes_enumerate");
    const GraphResolvingSubsetsAllModesResult all_r =
        resolving_subsets_bruteforce_upto_k_all_modes(dist, edges, list_max_k);

    const std::array<const GraphResolvingSubsetsResult*, 3> mode_results = {
        &all_r.nodes, &all_r.edges, &all_r.nodes_and_edges};
    for (int mode_i = 0; mode_i < 3; mode_i++) {
        const int basis_offset = mode_i * basis_max;
        const int list_flat_offset = mode_i * list_flat_max_ints_per_mode;
        const bool ok = encode_graph_resolving_subsets_result_to_buffers(
            *mode_results[static_cast<std::size_t>(mode_i)], 1,
            out_min_dim_3 + mode_i, out_smallest_basis_3 + basis_offset,
            basis_max, out_list_count_3 + mode_i, out_list_used_ints_3 + mode_i,
            out_list_flat_3 + list_flat_offset, list_flat_max_ints_per_mode,
            out_list_truncated_3 + mode_i);
        if (!ok) {
            return 0;
        }
    }
    return 1;
}

EMSCRIPTEN_KEEPALIVE
int wasm_graph_resolving_subsets_all_modes_paginated_with_non_resolving_size_minus_one_from_dist(
    int n, const int* adj01, const int* dist_flat, int page_size,
    const int* page_index_3, int* out_min_dim_3, int* out_smallest_basis_3,
    int basis_max, int* out_total_count_3, int* out_page_count_3,
    int* out_page_list_count_3, int* out_page_list_used_ints_3,
    int* out_page_list_flat_3, int page_list_flat_max_ints_per_mode,
    int* out_page_list_truncated_3) {
    if (n <= 0) {
        return 0;
    }
    if (!adj01 || !dist_flat || !page_index_3) {
        return 0;
    }
    if (!out_min_dim_3 || !out_smallest_basis_3 || basis_max < 0) {
        return 0;
    }
    if (!out_total_count_3 || !out_page_count_3 || !out_page_list_count_3 ||
        !out_page_list_used_ints_3 || !out_page_list_flat_3 ||
        page_list_flat_max_ints_per_mode < 0 || !out_page_list_truncated_3) {
        return 0;
    }

    ScopedTimer timer_total("graph_resolving_subsets_all_modes_paginated_total");
    const Matrix<int> dist = to_matrix_int_row_major(dist_flat, n);
    const std::vector<UndirectedEdge01> edges =
        build_undirected_edges_from_adj01_or(adj01, n);

    std::array<int, 3> page_index_by_mode = {page_index_3[0], page_index_3[1],
                                             page_index_3[2]};
    ScopedTimer timer_enum(
        "graph_resolving_subsets_all_modes_paginated_enumerate");
    const GraphResolvingSubsetsAllModesPageResult all_r =
        resolving_subsets_bruteforce_paginated_all_modes(
            dist, edges, page_index_by_mode, page_size);

    const std::array<const GraphResolvingSubsetsPageResult*, 3> mode_results = {
        &all_r.nodes, &all_r.edges, &all_r.nodes_and_edges};
    for (int mode_i = 0; mode_i < 3; mode_i++) {
        const GraphResolvingSubsetsPageResult& r =
            *mode_results[static_cast<std::size_t>(mode_i)];
        out_min_dim_3[mode_i] = r.min_dimension;

        const int basis_offset = mode_i * basis_max;
        const int write_basis_n =
            static_cast<int>(r.smallest_basis.size()) < basis_max
                ? static_cast<int>(r.smallest_basis.size())
                : basis_max;
        for (int bi = 0; bi < write_basis_n; bi++) {
            out_smallest_basis_3[basis_offset + bi] =
                r.smallest_basis[static_cast<std::size_t>(bi)];
        }

        const std::uint64_t total_count_u64 = r.total_count;
        const std::uint64_t int_max_u64 =
            static_cast<std::uint64_t>(std::numeric_limits<int>::max());
        const int total_count_clamped =
            total_count_u64 > int_max_u64 ? std::numeric_limits<int>::max()
                                          : static_cast<int>(total_count_u64);
        out_total_count_3[mode_i] = total_count_clamped;

        int page_count = 0;
        if (page_size > 0) {
            const std::uint64_t pages_u64 =
                (total_count_u64 + static_cast<std::uint64_t>(page_size) - 1ULL) /
                static_cast<std::uint64_t>(page_size);
            page_count = pages_u64 > int_max_u64 ? std::numeric_limits<int>::max()
                                                 : static_cast<int>(pages_u64);
        }
        out_page_count_3[mode_i] = page_count;

        const int list_flat_offset = mode_i * page_list_flat_max_ints_per_mode;
        int used = 0;
        int count = 0;
        int truncated = 0;
        for (const auto& subset : r.page_subsets) {
            const int k = static_cast<int>(subset.size());
            const int need = 1 + k;
            if (used + need > page_list_flat_max_ints_per_mode) {
                truncated = 1;
                break;
            }
            out_page_list_flat_3[list_flat_offset + used] = k;
            used++;
            for (int i = 0; i < k; i++) {
                out_page_list_flat_3[list_flat_offset + used] =
                    subset[static_cast<std::size_t>(i)];
                used++;
            }
            count++;
        }
        out_page_list_count_3[mode_i] = count;
        out_page_list_used_ints_3[mode_i] = used;
        out_page_list_truncated_3[mode_i] = truncated;
    }
    return 1;
}

EMSCRIPTEN_KEEPALIVE
void* wasm_graph_resolving_subsets_cache_create() {
    auto* cache = new GraphResolvePaginationCache();
    return cache;
}

EMSCRIPTEN_KEEPALIVE
void wasm_graph_resolving_subsets_cache_destroy(void* handle) {
    delete static_cast<GraphResolvePaginationCache*>(handle);
}

EMSCRIPTEN_KEEPALIVE
int wasm_graph_resolving_subsets_cache_set_graph(void* handle, int n,
                                                 const int* adj01,
                                                 const int* dist_flat) {
    if (!handle || n <= 0 || !adj01 || !dist_flat) {
        return 0;
    }
    auto* cache = static_cast<GraphResolvePaginationCache*>(handle);

    ScopedTimer timer_total("graph_resolving_subsets_cache_set_graph_total");
    const Matrix<int> dist = to_matrix_int_row_major(dist_flat, n);
    const std::vector<UndirectedEdge01> edges =
        build_undirected_edges_from_adj01_or(adj01, n);
    const std::array<int, 3> first_page = {0, 0, 0};

    ScopedTimer timer_enum("graph_resolving_subsets_cache_set_graph_enumerate");
    cache->all_modes_full = resolving_subsets_bruteforce_paginated_all_modes(
        dist, edges, first_page, std::numeric_limits<int>::max());
    cache->n = n;
    cache->ready = true;
    return 1;
}

EMSCRIPTEN_KEEPALIVE
int wasm_graph_resolving_subsets_cache_get_page(
    void* handle, int page_size, const int* page_index_3, int* out_min_dim_3,
    int* out_smallest_basis_3, int basis_max, int* out_total_count_3,
    int* out_page_count_3, int* out_page_list_count_3,
    int* out_page_list_used_ints_3, int* out_page_list_flat_3,
    int page_list_flat_max_ints_per_mode, int* out_page_list_truncated_3,
    int* out_min_list_count_3, int* out_min_list_used_ints_3,
    int* out_min_list_flat_3, int min_list_flat_max_ints_per_mode,
    int* out_min_list_truncated_3) {
    if (!handle || !page_index_3 || !out_min_dim_3 || !out_smallest_basis_3 ||
        basis_max < 0 || !out_total_count_3 || !out_page_count_3 ||
        !out_page_list_count_3 || !out_page_list_used_ints_3 ||
        !out_page_list_flat_3 || page_list_flat_max_ints_per_mode < 0 ||
        !out_page_list_truncated_3 || !out_min_list_count_3 ||
        !out_min_list_used_ints_3 || !out_min_list_flat_3 ||
        min_list_flat_max_ints_per_mode < 0 || !out_min_list_truncated_3) {
        return 0;
    }
    auto* cache = static_cast<GraphResolvePaginationCache*>(handle);
    if (!cache->ready || cache->n <= 0) {
        return 0;
    }

    if (page_size < 0) {
        page_size = 0;
    }

    const std::array<const GraphResolvingSubsetsPageResult*, 3> mode_results = {
        &cache->all_modes_full.nodes, &cache->all_modes_full.edges,
        &cache->all_modes_full.nodes_and_edges};
    const std::uint64_t int_max_u64 =
        static_cast<std::uint64_t>(std::numeric_limits<int>::max());

    for (int mode_i = 0; mode_i < 3; mode_i++) {
        const GraphResolvingSubsetsPageResult& r =
            *mode_results[static_cast<std::size_t>(mode_i)];
        out_min_dim_3[mode_i] = r.min_dimension;

        const int basis_offset = mode_i * basis_max;
        const int write_basis_n =
            static_cast<int>(r.smallest_basis.size()) < basis_max
                ? static_cast<int>(r.smallest_basis.size())
                : basis_max;
        for (int bi = 0; bi < write_basis_n; bi++) {
            out_smallest_basis_3[basis_offset + bi] =
                r.smallest_basis[static_cast<std::size_t>(bi)];
        }

        const std::uint64_t total_count_u64 = r.total_count;
        const int total_count_clamped =
            total_count_u64 > int_max_u64 ? std::numeric_limits<int>::max()
                                          : static_cast<int>(total_count_u64);
        out_total_count_3[mode_i] = total_count_clamped;

        int page_count = 0;
        if (page_size > 0) {
            const std::uint64_t pages_u64 =
                (total_count_u64 + static_cast<std::uint64_t>(page_size) - 1ULL) /
                static_cast<std::uint64_t>(page_size);
            page_count = pages_u64 > int_max_u64 ? std::numeric_limits<int>::max()
                                                 : static_cast<int>(pages_u64);
        }
        out_page_count_3[mode_i] = page_count;

        int page_index = page_index_3[mode_i];
        if (page_index < 0) {
            page_index = 0;
        }
        std::uint64_t page_start = 0ULL;
        std::uint64_t page_end = 0ULL;
        if (page_size > 0) {
            page_start = static_cast<std::uint64_t>(page_index) *
                         static_cast<std::uint64_t>(page_size);
            page_end = page_start + static_cast<std::uint64_t>(page_size);
        }

        const int list_flat_offset = mode_i * page_list_flat_max_ints_per_mode;
        int used = 0;
        int count = 0;
        int truncated = 0;

        if (page_size > 0) {
            for (std::uint64_t idx = page_start;
                 idx < page_end && idx < r.page_subsets.size(); idx++) {
                const auto& subset =
                    r.page_subsets[static_cast<std::size_t>(idx)];
                const int k = static_cast<int>(subset.size());
                const int need = 1 + k;
                if (used + need > page_list_flat_max_ints_per_mode) {
                    truncated = 1;
                    break;
                }
                out_page_list_flat_3[list_flat_offset + used] = k;
                used++;
                for (int i = 0; i < k; i++) {
                    out_page_list_flat_3[list_flat_offset + used] =
                        subset[static_cast<std::size_t>(i)];
                    used++;
                }
                count++;
            }
        }

        out_page_list_count_3[mode_i] = count;
        out_page_list_used_ints_3[mode_i] = used;
        out_page_list_truncated_3[mode_i] = truncated;

        const int min_list_flat_offset =
            mode_i * min_list_flat_max_ints_per_mode;
        int min_used = 0;
        int min_count = 0;
        int min_truncated = 0;
        for (const auto& subset : r.min_size_subsets) {
            const int k = static_cast<int>(subset.size());
            const int need = 1 + k;
            if (min_used + need > min_list_flat_max_ints_per_mode) {
                min_truncated = 1;
                break;
            }
            out_min_list_flat_3[min_list_flat_offset + min_used] = k;
            min_used++;
            for (int i = 0; i < k; i++) {
                out_min_list_flat_3[min_list_flat_offset + min_used] =
                    subset[static_cast<std::size_t>(i)];
                min_used++;
            }
            min_count++;
        }
        out_min_list_count_3[mode_i] = min_count;
        out_min_list_used_ints_3[mode_i] = min_used;
        out_min_list_truncated_3[mode_i] = min_truncated;
    }
    return 1;
}
}

