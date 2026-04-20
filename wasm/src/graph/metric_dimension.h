#pragma once

#include <cstddef>
#include <cstdint>
#include <array>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "graph/graph.h"
#include "graph/graph_distances.h"

struct MetricDimensionResult {
    int dimension;
    std::vector<int> basis;
};

enum class GraphResolveMode : int {
    Nodes = 0,
    Edges = 1,
    NodesAndEdges = 2,  // strict union: vertices ∪ edges, including vertex-vs-edge
};

struct UndirectedEdge01 {
    int u;
    int v;
};

struct GraphResolvingSubsetsResult {
    int min_dimension;
    std::vector<int> smallest_basis;  // indices in [0, n)
    std::vector<std::vector<int>> subsets_upto_k;  // all resolving subsets
    std::vector<std::vector<int>>
        subsets_with_non_resolving_size_minus_one_upto_k;
};

struct GraphResolvingSubsetsAllModesResult {
    GraphResolvingSubsetsResult nodes;
    GraphResolvingSubsetsResult edges;
    GraphResolvingSubsetsResult nodes_and_edges;
};

struct GraphResolvingSubsetsPageResult {
    int min_dimension;
    std::vector<int> smallest_basis;  // indices in [0, n)
    std::uint64_t total_count;
    std::vector<std::vector<int>> page_subsets;
    std::vector<std::vector<int>> min_size_subsets;
};

struct GraphResolvingSubsetsAllModesPageResult {
    GraphResolvingSubsetsPageResult nodes;
    GraphResolvingSubsetsPageResult edges;
    GraphResolvingSubsetsPageResult nodes_and_edges;
};

namespace metric_dimension_detail {

inline std::uint64_t splitmix64(std::uint64_t x) {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

enum class ObjectKind : std::uint64_t {
    Node = 0x6e6f6465ULL,  // "node"
    Edge = 0x65646765ULL,  // "edge"
};

inline std::uint64_t coord_contrib_kindless(int basis_idx, int dist_val) {
    const std::uint64_t b = static_cast<std::uint64_t>(
        static_cast<std::uint32_t>(basis_idx));
    const std::uint64_t d = static_cast<std::uint64_t>(
        static_cast<std::uint32_t>(dist_val + 1));
    return splitmix64((b * 0x9e3779b97f4a7c15ULL) ^ (d * 0xbf58476d1ce4e5b9ULL));
}

inline std::uint64_t coord_contrib(ObjectKind kind, int basis_idx, int dist_val) {
    // Deterministic contribution; XOR-composable so we can add/remove basis
    // indices when iterating combinations.
    const std::uint64_t b = static_cast<std::uint64_t>(
        static_cast<std::uint32_t>(basis_idx));
    const std::uint64_t d = static_cast<std::uint64_t>(
        static_cast<std::uint32_t>(dist_val + 1));
    const std::uint64_t k = static_cast<std::uint64_t>(kind);
    return splitmix64((b * 0x9e3779b97f4a7c15ULL) ^ (d * 0xbf58476d1ce4e5b9ULL) ^
                      (k * 0x94d049bb133111ebULL));
}

inline void combination_diff_sorted(const std::vector<int>& prev,
                                    const std::vector<int>& curr,
                                    std::vector<int>& out_removed,
                                    std::vector<int>& out_added) {
    out_removed.clear();
    out_added.clear();
    std::size_t i = 0;
    std::size_t j = 0;
    while (i < prev.size() && j < curr.size()) {
        const int a = prev[i];
        const int b = curr[j];
        if (a == b) {
            i++;
            j++;
        } else if (a < b) {
            out_removed.push_back(a);
            i++;
        } else {
            out_added.push_back(b);
            j++;
        }
    }
    while (i < prev.size()) {
        out_removed.push_back(prev[i]);
        i++;
    }
    while (j < curr.size()) {
        out_added.push_back(curr[j]);
        j++;
    }
}

inline bool same_coords_nodes(const Matrix<int>& dist,
                              const std::vector<int>& basis_indices,
                              std::size_t v1, std::size_t v2) {
    for (std::size_t t = 0; t < basis_indices.size(); t++) {
        const int b = basis_indices[t];
        if (dist(static_cast<std::size_t>(b), v1) !=
            dist(static_cast<std::size_t>(b), v2)) {
            return false;
        }
    }
    return true;
}

inline bool same_coords_edges(const Matrix<int>& dist,
                              const std::vector<UndirectedEdge01>& edges,
                              const std::vector<int>& basis_indices,
                              std::size_t e1, std::size_t e2) {
    const UndirectedEdge01 a = edges[e1];
    const UndirectedEdge01 b = edges[e2];
    for (std::size_t t = 0; t < basis_indices.size(); t++) {
        const int bi = basis_indices[t];
        const int a_du = dist(static_cast<std::size_t>(bi),
                              static_cast<std::size_t>(a.u));
        const int a_dv = dist(static_cast<std::size_t>(bi),
                              static_cast<std::size_t>(a.v));
        const int a_d = (a_du < a_dv) ? a_du : a_dv;

        const int b_du = dist(static_cast<std::size_t>(bi),
                              static_cast<std::size_t>(b.u));
        const int b_dv = dist(static_cast<std::size_t>(bi),
                              static_cast<std::size_t>(b.v));
        const int b_d = (b_du < b_dv) ? b_du : b_dv;

        if (a_d != b_d) {
            return false;
        }
    }
    return true;
}

inline bool same_coords_node_edge(const Matrix<int>& dist,
                                  const std::vector<UndirectedEdge01>& edges,
                                  const std::vector<int>& basis_indices,
                                  std::size_t v, std::size_t ei) {
    const UndirectedEdge01 e = edges[ei];
    for (std::size_t t = 0; t < basis_indices.size(); t++) {
        const int bi = basis_indices[t];
        const int node_d =
            dist(static_cast<std::size_t>(bi), static_cast<std::size_t>(v));
        const int du = dist(static_cast<std::size_t>(bi),
                            static_cast<std::size_t>(e.u));
        const int dv = dist(static_cast<std::size_t>(bi),
                            static_cast<std::size_t>(e.v));
        const int edge_d = (du < dv) ? du : dv;
        if (node_d != edge_d) {
            return false;
        }
    }
    return true;
}

}  // namespace metric_dimension_detail

inline bool is_resolving_set_indices(const Matrix<int>& dist,
                                    const std::vector<int>& basis_indices) {
    const std::size_t n = dist.get_rows();
    std::unordered_map<std::uint64_t, std::vector<std::size_t>> buckets;
    buckets.reserve(n);
    for (std::size_t v = 0; v < n; v++) {
        std::uint64_t sig = 0ULL;
        for (std::size_t t = 0; t < basis_indices.size(); t++) {
            const int b = basis_indices[t];
            const int d = dist(static_cast<std::size_t>(b), v);
            sig ^= metric_dimension_detail::coord_contrib(
                metric_dimension_detail::ObjectKind::Node, b, d);
        }
        auto& bucket = buckets[sig];
        for (std::size_t bi = 0; bi < bucket.size(); bi++) {
            const std::size_t rep_v = bucket[bi];
            if (metric_dimension_detail::same_coords_nodes(dist, basis_indices,
                                                          rep_v, v)) {
                return false;
            }
        }
        bucket.push_back(v);
    }
    return true;
}

inline bool is_resolving_set_edges_indices(
    const Matrix<int>& dist, const std::vector<UndirectedEdge01>& edges,
    const std::vector<int>& basis_indices) {
    const std::size_t m = edges.size();
    std::unordered_map<std::uint64_t, std::vector<std::size_t>> buckets;
    buckets.reserve(m);
    for (std::size_t ei = 0; ei < m; ei++) {
        const UndirectedEdge01 e = edges[ei];
        std::uint64_t sig = 0ULL;
        for (std::size_t t = 0; t < basis_indices.size(); t++) {
            const int b = basis_indices[t];
            const int du = dist(static_cast<std::size_t>(b),
                                static_cast<std::size_t>(e.u));
            const int dv = dist(static_cast<std::size_t>(b),
                                static_cast<std::size_t>(e.v));
            const int d = (du < dv) ? du : dv;
            sig ^= metric_dimension_detail::coord_contrib(
                metric_dimension_detail::ObjectKind::Edge, b, d);
        }
        auto& bucket = buckets[sig];
        for (std::size_t bi = 0; bi < bucket.size(); bi++) {
            const std::size_t rep_ei = bucket[bi];
            if (metric_dimension_detail::same_coords_edges(dist, edges,
                                                          basis_indices, rep_ei,
                                                          ei)) {
                return false;
            }
        }
        bucket.push_back(ei);
    }
    return true;
}

inline bool is_resolving_set_nodes_and_edges_indices(
    const Matrix<int>& dist, const std::vector<UndirectedEdge01>& edges,
    const std::vector<int>& basis_indices) {
    const std::size_t n = dist.get_rows();
    const std::size_t m = edges.size();
    std::unordered_map<std::uint64_t, std::vector<std::size_t>> buckets;
    buckets.reserve(n + m);

    // First vertices
    for (std::size_t v = 0; v < n; v++) {
        std::uint64_t sig = 0ULL;
        for (std::size_t t = 0; t < basis_indices.size(); t++) {
            const int b = basis_indices[t];
            const int d = dist(static_cast<std::size_t>(b), v);
            sig ^= metric_dimension_detail::coord_contrib_kindless(b, d);
        }
        auto& bucket = buckets[sig];
        for (std::size_t bi = 0; bi < bucket.size(); bi++) {
            const std::size_t rep_obj = bucket[bi];
            if (rep_obj < n) {
                if (metric_dimension_detail::same_coords_nodes(
                        dist, basis_indices, rep_obj, v)) {
                    return false;
                }
            } else {
                const std::size_t rep_ei = rep_obj - n;
                if (metric_dimension_detail::same_coords_node_edge(
                        dist, edges, basis_indices, v, rep_ei)) {
                    return false;
                }
            }
        }
        bucket.push_back(v);
    }

    // Then edges
    for (std::size_t ei = 0; ei < m; ei++) {
        const UndirectedEdge01 e = edges[ei];
        std::uint64_t sig = 0ULL;
        for (std::size_t t = 0; t < basis_indices.size(); t++) {
            const int b = basis_indices[t];
            const int du = dist(static_cast<std::size_t>(b),
                                static_cast<std::size_t>(e.u));
            const int dv = dist(static_cast<std::size_t>(b),
                                static_cast<std::size_t>(e.v));
            const int d = (du < dv) ? du : dv;
            sig ^= metric_dimension_detail::coord_contrib_kindless(b, d);
        }
        auto& bucket = buckets[sig];
        for (std::size_t bi = 0; bi < bucket.size(); bi++) {
            const std::size_t rep_obj = bucket[bi];
            if (rep_obj < n) {
                if (metric_dimension_detail::same_coords_node_edge(
                        dist, edges, basis_indices, rep_obj, ei)) {
                    return false;
                }
            } else {
                const std::size_t rep_ei = rep_obj - n;
                if (metric_dimension_detail::same_coords_edges(
                        dist, edges, basis_indices, rep_ei, ei)) {
                    return false;
                }
            }
        }
        bucket.push_back(n + ei);
    }

    return true;
}

inline bool is_resolving_set_for_mode(
    const Matrix<int>& dist, const std::vector<UndirectedEdge01>& edges,
    GraphResolveMode mode, const std::vector<int>& basis_indices) {
    if (mode == GraphResolveMode::Nodes) {
        return is_resolving_set_indices(dist, basis_indices);
    } else if (mode == GraphResolveMode::Edges) {
        return is_resolving_set_edges_indices(dist, edges, basis_indices);
    } else {
        return is_resolving_set_nodes_and_edges_indices(dist, edges,
                                                        basis_indices);
    }
}

inline bool has_non_resolving_size_minus_one_subset_for_mode(
    const Matrix<int>& dist, const std::vector<UndirectedEdge01>& edges,
    GraphResolveMode mode, const std::vector<int>& basis_indices) {
    const int k = static_cast<int>(basis_indices.size());
    if (k <= 0) {
        return false;
    }

    std::vector<int> subset;
    subset.reserve(static_cast<std::size_t>(k - 1));
    for (int drop_i = 0; drop_i < k; drop_i++) {
        subset.clear();
        for (int i = 0; i < k; i++) {
            if (i == drop_i) {
                continue;
            }
            subset.push_back(basis_indices[static_cast<std::size_t>(i)]);
        }
        if (!is_resolving_set_for_mode(dist, edges, mode, subset)) {
            return true;
        }
    }
    return false;
}

inline std::uint64_t subset_indices_to_mask64(
    const std::vector<int>& subset_indices) {
    std::uint64_t mask = 0ULL;
    for (std::size_t i = 0; i < subset_indices.size(); i++) {
        const int idx = subset_indices[i];
        mask |= (1ULL << static_cast<unsigned int>(idx));
    }
    return mask;
}

inline bool has_non_resolving_size_minus_one_subset_from_cache64(
    std::uint64_t subset_mask,
    const std::unordered_map<std::uint64_t, bool>& resolving_cache,
    bool* out_cache_complete) {
    if (!out_cache_complete) {
        return false;
    }
    *out_cache_complete = true;
    std::uint64_t bits = subset_mask;
    while (bits != 0ULL) {
        const std::uint64_t lsb = bits & (0ULL - bits);
        const std::uint64_t submask = subset_mask ^ lsb;
        auto it = resolving_cache.find(submask);
        if (it == resolving_cache.end()) {
            *out_cache_complete = false;
            return false;
        }
        if (!it->second) {
            return true;
        }
        bits ^= lsb;
    }
    return false;
}

struct SubsetMaskWords {
    std::vector<std::uint64_t> words;

    bool operator==(const SubsetMaskWords& other) const {
        return words == other.words;
    }
};

struct SubsetMaskWordsHasher {
    std::size_t operator()(const SubsetMaskWords& key) const {
        std::size_t h = 0;
        if (sizeof(std::size_t) >= sizeof(std::uint64_t)) {
            h = static_cast<std::size_t>(0xcbf29ce484222325ULL);
        } else {
            h = static_cast<std::size_t>(2166136261u);
        }
        for (std::size_t i = 0; i < key.words.size(); i++) {
            const std::uint64_t x = key.words[i];
            h ^= static_cast<std::size_t>(x + 0x9e3779b97f4a7c15ULL +
                                          (h << 6) + (h >> 2));
        }
        return h;
    }
};

inline SubsetMaskWords subset_indices_to_mask_words(
    const std::vector<int>& subset_indices, int n) {
    const std::size_t word_count =
        static_cast<std::size_t>((n + 63) / 64);
    SubsetMaskWords key;
    key.words.assign(word_count, 0ULL);
    for (std::size_t i = 0; i < subset_indices.size(); i++) {
        const int idx = subset_indices[i];
        const std::size_t word_i =
            static_cast<std::size_t>(idx / 64);
        const int bit_i = idx % 64;
        key.words[word_i] |= (1ULL << static_cast<unsigned int>(bit_i));
    }
    return key;
}

inline bool has_non_resolving_size_minus_one_subset_from_cache_words(
    const SubsetMaskWords& subset_mask,
    const std::unordered_map<SubsetMaskWords, bool, SubsetMaskWordsHasher>&
        resolving_cache,
    bool* out_cache_complete) {
    if (!out_cache_complete) {
        return false;
    }
    *out_cache_complete = true;

    SubsetMaskWords submask = subset_mask;
    for (std::size_t wi = 0; wi < subset_mask.words.size(); wi++) {
        std::uint64_t bits = subset_mask.words[wi];
        while (bits != 0ULL) {
            const std::uint64_t lsb = bits & (0ULL - bits);
            submask.words[wi] ^= lsb;
            auto it = resolving_cache.find(submask);
            if (it == resolving_cache.end()) {
                *out_cache_complete = false;
                return false;
            }
            if (!it->second) {
                return true;
            }
            submask.words[wi] ^= lsb;
            bits ^= lsb;
        }
    }
    return false;
}

inline bool next_combination_in_place(std::vector<int>& comb, int n) {
    const int k = static_cast<int>(comb.size());
    if (k <= 0) {
        return false;
    }
    int i = k - 1;
    while (i >= 0) {
        const int max_val = n - k + i;
        if (comb[static_cast<std::size_t>(i)] < max_val) {
            break;
        }
        i--;
    }
    if (i < 0) {
        return false;
    }
    comb[static_cast<std::size_t>(i)]++;
    for (int j = i + 1; j < k; j++) {
        comb[static_cast<std::size_t>(j)] =
            comb[static_cast<std::size_t>(j - 1)] + 1;
    }
    return true;
}

inline bool has_non_resolving_parent_from_set64(
    std::uint64_t subset_mask,
    const std::unordered_set<std::uint64_t>& non_resolving_prev) {
    std::uint64_t bits = subset_mask;
    while (bits != 0ULL) {
        const std::uint64_t lsb = bits & (0ULL - bits);
        const std::uint64_t parent_mask = subset_mask ^ lsb;
        if (non_resolving_prev.find(parent_mask) != non_resolving_prev.end()) {
            return true;
        }
        bits ^= lsb;
    }
    return false;
}

inline bool has_non_resolving_parent_from_set_words(
    const SubsetMaskWords& subset_mask,
    const std::unordered_set<SubsetMaskWords, SubsetMaskWordsHasher>&
        non_resolving_prev) {
    SubsetMaskWords parent_mask = subset_mask;
    for (std::size_t wi = 0; wi < subset_mask.words.size(); wi++) {
        std::uint64_t bits = subset_mask.words[wi];
        while (bits != 0ULL) {
            const std::uint64_t lsb = bits & (0ULL - bits);
            parent_mask.words[wi] ^= lsb;
            if (non_resolving_prev.find(parent_mask) != non_resolving_prev.end()) {
                return true;
            }
            parent_mask.words[wi] ^= lsb;
            bits ^= lsb;
        }
    }
    return false;
}

inline GraphResolvingSubsetsResult resolving_subsets_bruteforce_upto_k(
    const Matrix<int>& dist, const std::vector<UndirectedEdge01>& edges,
    GraphResolveMode mode, int list_max_k) {
    const int n = static_cast<int>(dist.get_rows());
    const std::size_t m = edges.size();
    if (n <= 1) {
        return GraphResolvingSubsetsResult{0, {}, {}, {}};
    }
    if (list_max_k < 0) {
        list_max_k = 0;
    }
    if (list_max_k > n) {
        list_max_k = n;
    }

    GraphResolvingSubsetsResult out;
    out.min_dimension = n;
    out.smallest_basis = {};
    out.subsets_upto_k = {};
    out.subsets_with_non_resolving_size_minus_one_upto_k = {};
    const bool use_subset_mask_cache64 = (n <= 63);
    const bool use_subset_mask_cache_words = !use_subset_mask_cache64;
    std::unordered_map<std::uint64_t, bool> resolving_cache64;
    std::unordered_map<SubsetMaskWords, bool, SubsetMaskWordsHasher>
        resolving_cache_words;
    if (use_subset_mask_cache64) {
        const std::vector<int> empty_subset{};
        const bool empty_is_resolving =
            is_resolving_set_for_mode(dist, edges, mode, empty_subset);
        resolving_cache64.emplace(0ULL, empty_is_resolving);
    }
    if (use_subset_mask_cache_words) {
        const std::vector<int> empty_subset{};
        const bool empty_is_resolving =
            is_resolving_set_for_mode(dist, edges, mode, empty_subset);
        SubsetMaskWords empty_key;
        empty_key.words.assign(static_cast<std::size_t>((n + 63) / 64), 0ULL);
        resolving_cache_words.emplace(std::move(empty_key), empty_is_resolving);
    }

    // Find minimum dimension and one smallest basis.
    for (int k = 1; k <= n; k++) {
        std::vector<int> comb(static_cast<std::size_t>(k));
        for (int i = 0; i < k; i++) {
            comb[static_cast<std::size_t>(i)] = i;
        }

        std::vector<int> prev_comb = comb;
        std::vector<int> removed{};
        std::vector<int> added{};
        const std::size_t obj_count =
            (mode == GraphResolveMode::Nodes)
                ? static_cast<std::size_t>(n)
                : ((mode == GraphResolveMode::Edges)
                       ? static_cast<std::size_t>(m)
                       : (static_cast<std::size_t>(n) + static_cast<std::size_t>(m)));
        std::vector<std::uint64_t> sigs(obj_count, 0ULL);
        auto apply_basis_xor = [&](int b, bool is_add) {
            (void)is_add;  // XOR is same for add/remove.
            if (mode == GraphResolveMode::Nodes || mode == GraphResolveMode::NodesAndEdges) {
                for (int v = 0; v < n; v++) {
                    const int d = dist(static_cast<std::size_t>(b),
                                       static_cast<std::size_t>(v));
                    const std::uint64_t contrib =
                        (mode == GraphResolveMode::NodesAndEdges)
                            ? metric_dimension_detail::coord_contrib_kindless(b, d)
                            : metric_dimension_detail::coord_contrib(
                                  metric_dimension_detail::ObjectKind::Node, b, d);
                    sigs[static_cast<std::size_t>(v)] ^= contrib;
                }
            }
            if (mode == GraphResolveMode::Edges || mode == GraphResolveMode::NodesAndEdges) {
                for (std::size_t ei = 0; ei < m; ei++) {
                    const UndirectedEdge01 e = edges[ei];
                    const int du = dist(static_cast<std::size_t>(b),
                                        static_cast<std::size_t>(e.u));
                    const int dv = dist(static_cast<std::size_t>(b),
                                        static_cast<std::size_t>(e.v));
                    const int d = (du < dv) ? du : dv;
                    const std::size_t idx =
                        (mode == GraphResolveMode::Edges)
                            ? ei
                            : (static_cast<std::size_t>(n) + ei);
                    const std::uint64_t contrib =
                        (mode == GraphResolveMode::NodesAndEdges)
                            ? metric_dimension_detail::coord_contrib_kindless(b, d)
                            : metric_dimension_detail::coord_contrib(
                                  metric_dimension_detail::ObjectKind::Edge, b, d);
                    sigs[idx] ^= contrib;
                }
            }
        };
        for (std::size_t t = 0; t < comb.size(); t++) {
            apply_basis_xor(comb[t], true);
        }

        auto is_resolving_from_sigs = [&]() -> bool {
            std::unordered_map<std::uint64_t, std::vector<std::size_t>> buckets;
            buckets.reserve(obj_count);
            for (std::size_t obj = 0; obj < obj_count; obj++) {
                const std::uint64_t sig = sigs[obj];
                auto& bucket = buckets[sig];
                for (std::size_t bi = 0; bi < bucket.size(); bi++) {
                    const std::size_t rep_obj = bucket[bi];
                    if (mode == GraphResolveMode::Nodes) {
                        if (metric_dimension_detail::same_coords_nodes(
                                dist, comb, rep_obj, obj)) {
                            return false;
                        }
                    } else if (mode == GraphResolveMode::Edges) {
                        if (metric_dimension_detail::same_coords_edges(
                                dist, edges, comb, rep_obj, obj)) {
                            return false;
                        }
                    } else {
                        const bool rep_is_edge =
                            (rep_obj >= static_cast<std::size_t>(n));
                        const bool obj_is_edge =
                            (obj >= static_cast<std::size_t>(n));
                        if (!rep_is_edge && !obj_is_edge) {
                            if (metric_dimension_detail::same_coords_nodes(
                                    dist, comb, rep_obj, obj)) {
                                return false;
                            }
                        } else if (rep_is_edge && obj_is_edge) {
                            const std::size_t rep_e = rep_obj - static_cast<std::size_t>(n);
                            const std::size_t obj_e = obj - static_cast<std::size_t>(n);
                            if (metric_dimension_detail::same_coords_edges(
                                    dist, edges, comb, rep_e, obj_e)) {
                                return false;
                            }
                        } else if (!rep_is_edge && obj_is_edge) {
                            const std::size_t obj_e =
                                obj - static_cast<std::size_t>(n);
                            if (metric_dimension_detail::same_coords_node_edge(
                                    dist, edges, comb, rep_obj, obj_e)) {
                                return false;
                            }
                        } else {  // rep_is_edge && !obj_is_edge
                            const std::size_t rep_e =
                                rep_obj - static_cast<std::size_t>(n);
                            if (metric_dimension_detail::same_coords_node_edge(
                                    dist, edges, comb, obj, rep_e)) {
                                return false;
                            }
                        }
                    }
                }
                bucket.push_back(obj);
            }
            return true;
        };

        while (true) {
            if (is_resolving_from_sigs()) {
                out.min_dimension = k;
                out.smallest_basis = comb;
                k = n + 1;  // break outer
                break;
            }
            prev_comb = comb;
            if (!next_combination_in_place(comb, n)) {
                break;
            }
            metric_dimension_detail::combination_diff_sorted(prev_comb, comb,
                                                            removed, added);
            for (std::size_t i = 0; i < removed.size(); i++) {
                apply_basis_xor(removed[i], false);
            }
            for (std::size_t i = 0; i < added.size(); i++) {
                apply_basis_xor(added[i], true);
            }
        }
    }

    // List all resolving subsets up to list_max_k (typically <= 3).
    for (int k = 1; k <= list_max_k; k++) {
        std::vector<int> comb(static_cast<std::size_t>(k));
        for (int i = 0; i < k; i++) {
            comb[static_cast<std::size_t>(i)] = i;
        }

        std::vector<int> prev_comb = comb;
        std::vector<int> removed{};
        std::vector<int> added{};
        const std::size_t obj_count =
            (mode == GraphResolveMode::Nodes)
                ? static_cast<std::size_t>(n)
                : ((mode == GraphResolveMode::Edges)
                       ? static_cast<std::size_t>(m)
                       : (static_cast<std::size_t>(n) + static_cast<std::size_t>(m)));
        std::vector<std::uint64_t> sigs(obj_count, 0ULL);
        auto apply_basis_xor = [&](int b, bool is_add) {
            (void)is_add;
            if (mode == GraphResolveMode::Nodes || mode == GraphResolveMode::NodesAndEdges) {
                for (int v = 0; v < n; v++) {
                    const int d = dist(static_cast<std::size_t>(b),
                                       static_cast<std::size_t>(v));
                    const std::uint64_t contrib =
                        (mode == GraphResolveMode::NodesAndEdges)
                            ? metric_dimension_detail::coord_contrib_kindless(b, d)
                            : metric_dimension_detail::coord_contrib(
                                  metric_dimension_detail::ObjectKind::Node, b, d);
                    sigs[static_cast<std::size_t>(v)] ^= contrib;
                }
            }
            if (mode == GraphResolveMode::Edges || mode == GraphResolveMode::NodesAndEdges) {
                for (std::size_t ei = 0; ei < m; ei++) {
                    const UndirectedEdge01 e = edges[ei];
                    const int du = dist(static_cast<std::size_t>(b),
                                        static_cast<std::size_t>(e.u));
                    const int dv = dist(static_cast<std::size_t>(b),
                                        static_cast<std::size_t>(e.v));
                    const int d = (du < dv) ? du : dv;
                    const std::size_t idx =
                        (mode == GraphResolveMode::Edges)
                            ? ei
                            : (static_cast<std::size_t>(n) + ei);
                    const std::uint64_t contrib =
                        (mode == GraphResolveMode::NodesAndEdges)
                            ? metric_dimension_detail::coord_contrib_kindless(b, d)
                            : metric_dimension_detail::coord_contrib(
                                  metric_dimension_detail::ObjectKind::Edge, b, d);
                    sigs[idx] ^= contrib;
                }
            }
        };
        for (std::size_t t = 0; t < comb.size(); t++) {
            apply_basis_xor(comb[t], true);
        }

        auto is_resolving_from_sigs = [&]() -> bool {
            std::unordered_map<std::uint64_t, std::vector<std::size_t>> buckets;
            buckets.reserve(obj_count);
            for (std::size_t obj = 0; obj < obj_count; obj++) {
                const std::uint64_t sig = sigs[obj];
                auto& bucket = buckets[sig];
                for (std::size_t bi = 0; bi < bucket.size(); bi++) {
                    const std::size_t rep_obj = bucket[bi];
                    if (mode == GraphResolveMode::Nodes) {
                        if (metric_dimension_detail::same_coords_nodes(
                                dist, comb, rep_obj, obj)) {
                            return false;
                        }
                    } else if (mode == GraphResolveMode::Edges) {
                        if (metric_dimension_detail::same_coords_edges(
                                dist, edges, comb, rep_obj, obj)) {
                            return false;
                        }
                    } else {
                        const bool rep_is_edge =
                            (rep_obj >= static_cast<std::size_t>(n));
                        const bool obj_is_edge =
                            (obj >= static_cast<std::size_t>(n));
                        if (!rep_is_edge && !obj_is_edge) {
                            if (metric_dimension_detail::same_coords_nodes(
                                    dist, comb, rep_obj, obj)) {
                                return false;
                            }
                        } else if (rep_is_edge && obj_is_edge) {
                            const std::size_t rep_e = rep_obj - static_cast<std::size_t>(n);
                            const std::size_t obj_e = obj - static_cast<std::size_t>(n);
                            if (metric_dimension_detail::same_coords_edges(
                                    dist, edges, comb, rep_e, obj_e)) {
                                return false;
                            }
                        } else if (!rep_is_edge && obj_is_edge) {
                            const std::size_t obj_e =
                                obj - static_cast<std::size_t>(n);
                            if (metric_dimension_detail::same_coords_node_edge(
                                    dist, edges, comb, rep_obj, obj_e)) {
                                return false;
                            }
                        } else {  // rep_is_edge && !obj_is_edge
                            const std::size_t rep_e =
                                rep_obj - static_cast<std::size_t>(n);
                            if (metric_dimension_detail::same_coords_node_edge(
                                    dist, edges, comb, obj, rep_e)) {
                                return false;
                            }
                        }
                    }
                }
                bucket.push_back(obj);
            }
            return true;
        };

        while (true) {
            std::uint64_t comb_mask64 = 0ULL;
            SubsetMaskWords comb_mask_words{};
            if (use_subset_mask_cache64) {
                comb_mask64 = subset_indices_to_mask64(comb);
            }
            if (use_subset_mask_cache_words) {
                comb_mask_words = subset_indices_to_mask_words(comb, n);
            }
            const bool is_resolving = is_resolving_from_sigs();
            if (use_subset_mask_cache64) {
                resolving_cache64[comb_mask64] = is_resolving;
            }
            if (use_subset_mask_cache_words) {
                resolving_cache_words[comb_mask_words] = is_resolving;
            }
            if (is_resolving) {
                out.subsets_upto_k.push_back(comb);
                bool has_non_resolving_size_minus_one_subset = false;
                if (use_subset_mask_cache64) {
                    bool cache_complete = false;
                    has_non_resolving_size_minus_one_subset =
                        has_non_resolving_size_minus_one_subset_from_cache64(
                            comb_mask64, resolving_cache64, &cache_complete);
                    if (!cache_complete) {
                        has_non_resolving_size_minus_one_subset =
                            has_non_resolving_size_minus_one_subset_for_mode(
                                dist, edges, mode, comb);
                    }
                } else if (use_subset_mask_cache_words) {
                    bool cache_complete = false;
                    has_non_resolving_size_minus_one_subset =
                        has_non_resolving_size_minus_one_subset_from_cache_words(
                            comb_mask_words, resolving_cache_words,
                            &cache_complete);
                    if (!cache_complete) {
                        has_non_resolving_size_minus_one_subset =
                            has_non_resolving_size_minus_one_subset_for_mode(
                                dist, edges, mode, comb);
                    }
                } else {
                    has_non_resolving_size_minus_one_subset =
                        has_non_resolving_size_minus_one_subset_for_mode(
                            dist, edges, mode, comb);
                }
                if (has_non_resolving_size_minus_one_subset) {
                    out.subsets_with_non_resolving_size_minus_one_upto_k
                        .push_back(comb);
                }
            }
            prev_comb = comb;
            if (!next_combination_in_place(comb, n)) {
                break;
            }
            metric_dimension_detail::combination_diff_sorted(prev_comb, comb,
                                                            removed, added);
            for (std::size_t i = 0; i < removed.size(); i++) {
                apply_basis_xor(removed[i], false);
            }
            for (std::size_t i = 0; i < added.size(); i++) {
                apply_basis_xor(added[i], true);
            }
        }
    }

    return out;
}

inline GraphResolvingSubsetsAllModesResult
resolving_subsets_bruteforce_upto_k_all_modes(
    const Matrix<int>& dist, const std::vector<UndirectedEdge01>& edges,
    int list_max_k) {
    const int n = static_cast<int>(dist.get_rows());
    GraphResolvingSubsetsAllModesResult all_out;
    if (n <= 1) {
        all_out.nodes = GraphResolvingSubsetsResult{0, {}, {}, {}};
        all_out.edges = GraphResolvingSubsetsResult{0, {}, {}, {}};
        all_out.nodes_and_edges = GraphResolvingSubsetsResult{0, {}, {}, {}};
        return all_out;
    }
    if (list_max_k < 0) {
        list_max_k = 0;
    }
    if (list_max_k > n) {
        list_max_k = n;
    }

    all_out.nodes = GraphResolvingSubsetsResult{n, {}, {}, {}};
    all_out.edges = GraphResolvingSubsetsResult{n, {}, {}, {}};
    all_out.nodes_and_edges = GraphResolvingSubsetsResult{n, {}, {}, {}};

    struct ModeState {
        GraphResolveMode mode;
        GraphResolvingSubsetsResult* out;
        bool min_found;
        std::unordered_map<std::uint64_t, bool> resolving_cache64;
        std::unordered_map<SubsetMaskWords, bool, SubsetMaskWordsHasher>
            resolving_cache_words;
    };

    std::array<ModeState, 3> states = {{
        ModeState{GraphResolveMode::Nodes, &all_out.nodes, false, {}, {}},
        ModeState{GraphResolveMode::Edges, &all_out.edges, false, {}, {}},
        ModeState{GraphResolveMode::NodesAndEdges, &all_out.nodes_and_edges,
                  false, {}, {}},
    }};

    const bool use_subset_mask_cache64 = (n <= 63);
    const bool use_subset_mask_cache_words = !use_subset_mask_cache64;
    if (use_subset_mask_cache64) {
        const std::vector<int> empty_subset{};
        for (std::size_t si = 0; si < states.size(); si++) {
            const bool empty_is_resolving = is_resolving_set_for_mode(
                dist, edges, states[si].mode, empty_subset);
            states[si].resolving_cache64.emplace(0ULL, empty_is_resolving);
        }
    }
    if (use_subset_mask_cache_words) {
        const std::vector<int> empty_subset{};
        for (std::size_t si = 0; si < states.size(); si++) {
            const bool empty_is_resolving = is_resolving_set_for_mode(
                dist, edges, states[si].mode, empty_subset);
            SubsetMaskWords empty_key;
            empty_key.words.assign(static_cast<std::size_t>((n + 63) / 64),
                                   0ULL);
            states[si].resolving_cache_words.emplace(std::move(empty_key),
                                                     empty_is_resolving);
        }
    }

    for (int k = 1; k <= n; k++) {
        std::vector<int> comb(static_cast<std::size_t>(k));
        for (int i = 0; i < k; i++) {
            comb[static_cast<std::size_t>(i)] = i;
        }
        while (true) {
            std::uint64_t comb_mask64 = 0ULL;
            SubsetMaskWords comb_mask_words{};
            bool has_mask64 = false;
            bool has_mask_words = false;

            for (std::size_t si = 0; si < states.size(); si++) {
                const bool need_for_min = !states[si].min_found;
                const bool need_for_list = (k <= list_max_k);
                if (!need_for_min && !need_for_list) {
                    continue;
                }

                if (need_for_list) {
                    if (use_subset_mask_cache64 && !has_mask64) {
                        comb_mask64 = subset_indices_to_mask64(comb);
                        has_mask64 = true;
                    }
                    if (use_subset_mask_cache_words && !has_mask_words) {
                        comb_mask_words = subset_indices_to_mask_words(comb, n);
                        has_mask_words = true;
                    }
                }

                const bool is_resolving = is_resolving_set_for_mode(
                    dist, edges, states[si].mode, comb);
                if (need_for_list) {
                    if (use_subset_mask_cache64) {
                        states[si].resolving_cache64[comb_mask64] = is_resolving;
                    }
                    if (use_subset_mask_cache_words) {
                        states[si].resolving_cache_words[comb_mask_words] =
                            is_resolving;
                    }
                }

                if (is_resolving && !states[si].min_found) {
                    states[si].out->min_dimension = k;
                    states[si].out->smallest_basis = comb;
                    states[si].min_found = true;
                }

                if (need_for_list && is_resolving) {
                    states[si].out->subsets_upto_k.push_back(comb);
                    bool has_non_resolving_size_minus_one_subset = false;
                    if (use_subset_mask_cache64) {
                        bool cache_complete = false;
                        has_non_resolving_size_minus_one_subset =
                            has_non_resolving_size_minus_one_subset_from_cache64(
                                comb_mask64, states[si].resolving_cache64,
                                &cache_complete);
                        if (!cache_complete) {
                            has_non_resolving_size_minus_one_subset =
                                has_non_resolving_size_minus_one_subset_for_mode(
                                    dist, edges, states[si].mode, comb);
                        }
                    } else if (use_subset_mask_cache_words) {
                        bool cache_complete = false;
                        has_non_resolving_size_minus_one_subset =
                            has_non_resolving_size_minus_one_subset_from_cache_words(
                                comb_mask_words,
                                states[si].resolving_cache_words,
                                &cache_complete);
                        if (!cache_complete) {
                            has_non_resolving_size_minus_one_subset =
                                has_non_resolving_size_minus_one_subset_for_mode(
                                    dist, edges, states[si].mode, comb);
                        }
                    } else {
                        has_non_resolving_size_minus_one_subset =
                            has_non_resolving_size_minus_one_subset_for_mode(
                                dist, edges, states[si].mode, comb);
                    }
                    if (has_non_resolving_size_minus_one_subset) {
                        states[si]
                            .out->subsets_with_non_resolving_size_minus_one_upto_k
                            .push_back(comb);
                    }
                }
            }

            if (!next_combination_in_place(comb, n)) {
                break;
            }
        }
    }

    return all_out;
}

inline GraphResolvingSubsetsAllModesPageResult
resolving_subsets_bruteforce_paginated_all_modes(
    const Matrix<int>& dist, const std::vector<UndirectedEdge01>& edges,
    const std::array<int, 3>& page_index_by_mode, int page_size) {
    const int n = static_cast<int>(dist.get_rows());
    const std::size_t m = edges.size();
    GraphResolvingSubsetsAllModesPageResult all_out;
    if (n <= 1) {
        all_out.nodes = GraphResolvingSubsetsPageResult{0, {}, 0ULL, {}, {}};
        all_out.edges = GraphResolvingSubsetsPageResult{0, {}, 0ULL, {}, {}};
        all_out.nodes_and_edges =
            GraphResolvingSubsetsPageResult{0, {}, 0ULL, {}, {}};
        return all_out;
    }

    if (page_size < 0) {
        page_size = 0;
    }
    std::array<std::uint64_t, 3> page_start = {0ULL, 0ULL, 0ULL};
    std::array<std::uint64_t, 3> page_end = {0ULL, 0ULL, 0ULL};
    for (int mode_i = 0; mode_i < 3; mode_i++) {
        int page_index = page_index_by_mode[static_cast<std::size_t>(mode_i)];
        if (page_index < 0) {
            page_index = 0;
        }
        page_start[static_cast<std::size_t>(mode_i)] =
            static_cast<std::uint64_t>(page_index) *
            static_cast<std::uint64_t>(page_size);
        page_end[static_cast<std::size_t>(mode_i)] =
            page_start[static_cast<std::size_t>(mode_i)] +
            static_cast<std::uint64_t>(page_size);
    }

    all_out.nodes = GraphResolvingSubsetsPageResult{n, {}, 0ULL, {}, {}};
    all_out.edges = GraphResolvingSubsetsPageResult{n, {}, 0ULL, {}, {}};
    all_out.nodes_and_edges = GraphResolvingSubsetsPageResult{n, {}, 0ULL, {}, {}};

    const bool use_mask64 = (n <= 63);
    struct ModeState {
        GraphResolveMode mode;
        GraphResolvingSubsetsPageResult* out;
        bool min_found;
        bool active;
        bool empty_is_non_resolving;
        std::uint64_t page_start;
        std::uint64_t page_end;
        std::unordered_set<std::uint64_t> non_resolving_prev64;
        std::unordered_set<std::uint64_t> non_resolving_curr64;
        std::unordered_set<SubsetMaskWords, SubsetMaskWordsHasher>
            non_resolving_prev_words;
        std::unordered_set<SubsetMaskWords, SubsetMaskWordsHasher>
            non_resolving_curr_words;
    };

    std::array<ModeState, 3> states{};
    states[0].mode = GraphResolveMode::Nodes;
    states[1].mode = GraphResolveMode::Edges;
    states[2].mode = GraphResolveMode::NodesAndEdges;
    states[0].out = &all_out.nodes;
    states[1].out = &all_out.edges;
    states[2].out = &all_out.nodes_and_edges;
    for (std::size_t si = 0; si < states.size(); si++) {
        states[si].min_found = false;
        states[si].active = false;
        states[si].empty_is_non_resolving = false;
        states[si].page_start = page_start[si];
        states[si].page_end = page_end[si];
    }

    const std::vector<int> empty_subset{};
    SubsetMaskWords empty_words{};
    if (!use_mask64) {
        empty_words.words.assign(static_cast<std::size_t>((n + 63) / 64), 0ULL);
    }
    for (std::size_t si = 0; si < states.size(); si++) {
        const bool empty_is_resolving = is_resolving_set_for_mode(
            dist, edges, states[si].mode, empty_subset);
        if (empty_is_resolving) {
            states[si].min_found = true;
            states[si].out->min_dimension = 0;
            states[si].out->smallest_basis = {};
            states[si].active = false;
            states[si].empty_is_non_resolving = false;
        } else {
            states[si].min_found = false;
            states[si].active = true;
            states[si].empty_is_non_resolving = true;
            if (use_mask64) {
                states[si].non_resolving_prev64.insert(0ULL);
            } else {
                states[si].non_resolving_prev_words.insert(empty_words);
            }
        }
    }

    for (int k = 1; k <= n; k++) {
        bool any_mode_active = false;
        for (std::size_t si = 0; si < states.size(); si++) {
            if (states[si].active) {
                any_mode_active = true;
                break;
            }
        }
        if (!any_mode_active) {
            break;
        }

        for (std::size_t si = 0; si < states.size(); si++) {
            if (!states[si].active) {
                continue;
            }
            if (use_mask64) {
                states[si].non_resolving_curr64.clear();
            } else {
                states[si].non_resolving_curr_words.clear();
            }
        }

        std::vector<int> comb(static_cast<std::size_t>(k));
        for (int i = 0; i < k; i++) {
            comb[static_cast<std::size_t>(i)] = i;
        }
        std::vector<int> prev_comb = comb;
        std::vector<int> removed{};
        std::vector<int> added{};

        std::array<std::vector<std::uint64_t>, 3> sigs_by_state{};
        for (std::size_t si = 0; si < states.size(); si++) {
            if (!states[si].active) {
                continue;
            }
            const std::size_t obj_count =
                (states[si].mode == GraphResolveMode::Nodes)
                    ? static_cast<std::size_t>(n)
                    : ((states[si].mode == GraphResolveMode::Edges)
                           ? static_cast<std::size_t>(m)
                           : (static_cast<std::size_t>(n) +
                              static_cast<std::size_t>(m)));
            sigs_by_state[si].assign(obj_count, 0ULL);
        }

        auto apply_basis_xor_state = [&](std::size_t si, int b) {
            if (!states[si].active) {
                return;
            }
            if (states[si].mode == GraphResolveMode::Nodes ||
                states[si].mode == GraphResolveMode::NodesAndEdges) {
                for (int v = 0; v < n; v++) {
                    const int d = dist(static_cast<std::size_t>(b),
                                       static_cast<std::size_t>(v));
                    const std::uint64_t contrib =
                        (states[si].mode == GraphResolveMode::NodesAndEdges)
                            ? metric_dimension_detail::coord_contrib_kindless(b, d)
                            : metric_dimension_detail::coord_contrib(
                                  metric_dimension_detail::ObjectKind::Node, b, d);
                    sigs_by_state[si][static_cast<std::size_t>(v)] ^= contrib;
                }
            }
            if (states[si].mode == GraphResolveMode::Edges ||
                states[si].mode == GraphResolveMode::NodesAndEdges) {
                for (std::size_t ei = 0; ei < m; ei++) {
                    const UndirectedEdge01 e = edges[ei];
                    const int du = dist(static_cast<std::size_t>(b),
                                        static_cast<std::size_t>(e.u));
                    const int dv = dist(static_cast<std::size_t>(b),
                                        static_cast<std::size_t>(e.v));
                    const int d = (du < dv) ? du : dv;
                    const std::size_t idx =
                        (states[si].mode == GraphResolveMode::Edges)
                            ? ei
                            : (static_cast<std::size_t>(n) + ei);
                    const std::uint64_t contrib =
                        (states[si].mode == GraphResolveMode::NodesAndEdges)
                            ? metric_dimension_detail::coord_contrib_kindless(b, d)
                            : metric_dimension_detail::coord_contrib(
                                  metric_dimension_detail::ObjectKind::Edge, b, d);
                    sigs_by_state[si][idx] ^= contrib;
                }
            }
        };

        for (std::size_t si = 0; si < states.size(); si++) {
            if (!states[si].active) {
                continue;
            }
            for (std::size_t t = 0; t < comb.size(); t++) {
                apply_basis_xor_state(si, comb[t]);
            }
        }

        auto is_resolving_state_from_sigs = [&](std::size_t si) -> bool {
            const std::size_t obj_count = sigs_by_state[si].size();
            std::unordered_map<std::uint64_t, std::vector<std::size_t>> buckets;
            buckets.reserve(obj_count);
            for (std::size_t obj = 0; obj < obj_count; obj++) {
                const std::uint64_t sig = sigs_by_state[si][obj];
                auto& bucket = buckets[sig];
                for (std::size_t bi = 0; bi < bucket.size(); bi++) {
                    const std::size_t rep_obj = bucket[bi];
                    if (states[si].mode == GraphResolveMode::Nodes) {
                        if (metric_dimension_detail::same_coords_nodes(
                                dist, comb, rep_obj, obj)) {
                            return false;
                        }
                    } else if (states[si].mode == GraphResolveMode::Edges) {
                        if (metric_dimension_detail::same_coords_edges(
                                dist, edges, comb, rep_obj, obj)) {
                            return false;
                        }
                    } else {
                        const bool rep_is_edge =
                            (rep_obj >= static_cast<std::size_t>(n));
                        const bool obj_is_edge =
                            (obj >= static_cast<std::size_t>(n));
                        if (!rep_is_edge && !obj_is_edge) {
                            if (metric_dimension_detail::same_coords_nodes(
                                    dist, comb, rep_obj, obj)) {
                                return false;
                            }
                        } else if (rep_is_edge && obj_is_edge) {
                            const std::size_t rep_e =
                                rep_obj - static_cast<std::size_t>(n);
                            const std::size_t obj_e =
                                obj - static_cast<std::size_t>(n);
                            if (metric_dimension_detail::same_coords_edges(
                                    dist, edges, comb, rep_e, obj_e)) {
                                return false;
                            }
                        } else if (!rep_is_edge && obj_is_edge) {
                            const std::size_t obj_e =
                                obj - static_cast<std::size_t>(n);
                            if (metric_dimension_detail::same_coords_node_edge(
                                    dist, edges, comb, rep_obj, obj_e)) {
                                return false;
                            }
                        } else {  // rep_is_edge && !obj_is_edge
                            const std::size_t rep_e =
                                rep_obj - static_cast<std::size_t>(n);
                            if (metric_dimension_detail::same_coords_node_edge(
                                    dist, edges, comb, obj, rep_e)) {
                                return false;
                            }
                        }
                    }
                }
                bucket.push_back(obj);
            }
            return true;
        };

        while (true) {
            std::uint64_t comb_mask64 = 0ULL;
            SubsetMaskWords comb_mask_words{};
            if (use_mask64) {
                comb_mask64 = subset_indices_to_mask64(comb);
            } else {
                comb_mask_words = subset_indices_to_mask_words(comb, n);
            }

            for (std::size_t si = 0; si < states.size(); si++) {
                if (!states[si].active) {
                    continue;
                }

                bool has_non_resolving_parent = false;
                if (k == 1) {
                    has_non_resolving_parent = states[si].empty_is_non_resolving;
                } else if (use_mask64) {
                    has_non_resolving_parent = has_non_resolving_parent_from_set64(
                        comb_mask64, states[si].non_resolving_prev64);
                } else {
                    has_non_resolving_parent =
                        has_non_resolving_parent_from_set_words(
                            comb_mask_words, states[si].non_resolving_prev_words);
                }
                if (!has_non_resolving_parent) {
                    continue;
                }

                const bool is_resolving = is_resolving_state_from_sigs(si);
                if (is_resolving && !states[si].min_found) {
                    states[si].out->min_dimension = k;
                    states[si].out->smallest_basis = comb;
                    states[si].min_found = true;
                }
                if (!is_resolving) {
                    if (use_mask64) {
                        states[si].non_resolving_curr64.insert(comb_mask64);
                    } else {
                        states[si].non_resolving_curr_words.insert(comb_mask_words);
                    }
                    continue;
                }

                const std::uint64_t idx = states[si].out->total_count;
                states[si].out->total_count = idx + 1ULL;
                if (k == states[si].out->min_dimension) {
                    states[si].out->min_size_subsets.push_back(comb);
                }
                if (idx >= states[si].page_start && idx < states[si].page_end) {
                    states[si].out->page_subsets.push_back(comb);
                }
            }

            prev_comb = comb;
            if (!next_combination_in_place(comb, n)) {
                break;
            }
            metric_dimension_detail::combination_diff_sorted(prev_comb, comb,
                                                            removed, added);
            for (std::size_t si = 0; si < states.size(); si++) {
                if (!states[si].active) {
                    continue;
                }
                for (std::size_t i = 0; i < removed.size(); i++) {
                    apply_basis_xor_state(si, removed[i]);
                }
                for (std::size_t i = 0; i < added.size(); i++) {
                    apply_basis_xor_state(si, added[i]);
                }
            }
        }

        for (std::size_t si = 0; si < states.size(); si++) {
            if (!states[si].active) {
                continue;
            }
            if (use_mask64) {
                states[si].non_resolving_prev64.swap(states[si].non_resolving_curr64);
                states[si].non_resolving_curr64.clear();
                if (states[si].non_resolving_prev64.empty()) {
                    states[si].active = false;
                }
            } else {
                states[si].non_resolving_prev_words.swap(
                    states[si].non_resolving_curr_words);
                states[si].non_resolving_curr_words.clear();
                if (states[si].non_resolving_prev_words.empty()) {
                    states[si].active = false;
                }
            }
        }
    }

    return all_out;
}

inline MetricDimensionResult metric_dimension_brute_force(const Graph<int>& g) {
    if (g.is_directed()) {
        throw std::invalid_argument(
            "metric_dimension_brute_force: graph must be undirected");
    }

    const GraphDistances gd = bfs_distances(g);
    const std::size_t n = gd.idx_to_node.size();

    if (n <= 1) {
        return MetricDimensionResult{0, {}};
    }

    const Matrix<int>& dist = gd.dist;

    for (std::size_t k = 0; k <= n; k++) {
        if (k == 0) {
            continue;
        }

        std::vector<int> comb(k);
        for (std::size_t i = 0; i < k; i++) {
            comb[i] = static_cast<int>(i);
        }

        while (true) {
            if (is_resolving_set_indices(dist, comb)) {
                std::vector<int> basis;
                basis.reserve(k);
                for (std::size_t i = 0; i < k; i++) {
                    basis.push_back(gd.idx_to_node[static_cast<std::size_t>(
                        comb[i])]);
                }
                return MetricDimensionResult{static_cast<int>(k), basis};
            }

            if (!next_combination_in_place(comb, static_cast<int>(n))) {
                break;
            }
        }
    }

    return MetricDimensionResult{static_cast<int>(n), gd.idx_to_node};
}

