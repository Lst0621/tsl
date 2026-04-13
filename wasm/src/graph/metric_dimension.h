#pragma once

#include <cstddef>
#include <map>
#include <stdexcept>
#include <vector>

#include "graph/graph.h"
#include "graph/graph_distances.h"

struct MetricDimensionResult {
    int dimension;
    std::vector<int> basis;
};

inline bool is_resolving_set_indices(const Matrix<int>& dist,
                                    const std::vector<int>& basis_indices) {
    const std::size_t n = dist.get_rows();
    const std::size_t k = basis_indices.size();

    std::map<std::vector<int>, int> seen;
    for (std::size_t v = 0; v < n; v++) {
        std::vector<int> rep;
        rep.reserve(k);
        for (std::size_t t = 0; t < k; t++) {
            const int b = basis_indices[t];
            rep.push_back(dist(static_cast<std::size_t>(b), v));
        }
        auto it = seen.find(rep);
        if (it != seen.end()) {
            return false;
        }
        seen.emplace(std::move(rep), static_cast<int>(v));
    }
    return true;
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

            int i = static_cast<int>(k) - 1;
            while (i >= 0) {
                const int max_val =
                    static_cast<int>(n - k + static_cast<std::size_t>(i));
                if (comb[static_cast<std::size_t>(i)] < max_val) {
                    break;
                }
                i--;
            }
            if (i < 0) {
                break;
            }

            comb[static_cast<std::size_t>(i)]++;
            for (std::size_t j = static_cast<std::size_t>(i) + 1; j < k; j++) {
                comb[j] = comb[j - 1] + 1;
            }
        }
    }

    return MetricDimensionResult{static_cast<int>(n), gd.idx_to_node};
}

