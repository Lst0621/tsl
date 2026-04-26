#pragma once

#include <algorithm>
#include <cstddef>
#include <queue>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include "algebra/matrix.h"
#include "graph/graph.h"

struct GraphDistances {
    Matrix<int> dist;
    std::vector<int> idx_to_node;
    std::unordered_map<int, int> node_to_idx;
};

inline GraphDistances bfs_distances(const Graph<int>& g) {
    if (g.is_directed()) {
        throw std::invalid_argument("bfs_distances: graph must be undirected");
    }

    GraphDistances result;

    result.idx_to_node = g.vertices();
    std::sort(result.idx_to_node.begin(), result.idx_to_node.end());

    const std::size_t n = result.idx_to_node.size();
    result.node_to_idx.reserve(n);
    for (std::size_t i = 0; i < n; i++) {
        result.node_to_idx.emplace(result.idx_to_node[i],
                                   static_cast<int>(i));
    }

    if (n == 0) {
        result.dist = Matrix<int>(1, 1, 0);
        return result;
    }

    result.dist = Matrix<int>(n, n, -1);

    std::vector<int> dist_idx(n, -1);
    std::queue<int> q;

    for (std::size_t s = 0; s < n; s++) {
        std::fill(dist_idx.begin(), dist_idx.end(), -1);
        while (!q.empty()) {
            q.pop();
        }

        dist_idx[s] = 0;
        q.push(static_cast<int>(s));

        while (!q.empty()) {
            const int u_idx = q.front();
            q.pop();
            const int u = result.idx_to_node[static_cast<std::size_t>(u_idx)];

            const auto& eids = g.out_edge_ids(u);
            for (const auto& eid : eids) {
                const int v = g.other(eid, u);
                const auto it = result.node_to_idx.find(v);
                if (it == result.node_to_idx.end()) {
                    continue;
                }
                const int v_idx = it->second;
                if (dist_idx[static_cast<std::size_t>(v_idx)] != -1) {
                    continue;
                }
                dist_idx[static_cast<std::size_t>(v_idx)] =
                    dist_idx[static_cast<std::size_t>(u_idx)] + 1;
                q.push(v_idx);
            }
        }

        for (std::size_t t = 0; t < n; t++) {
            result.dist(s, t) = dist_idx[t];
        }
    }

    return result;
}

