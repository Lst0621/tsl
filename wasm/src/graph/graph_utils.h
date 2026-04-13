#pragma once

#include <algorithm>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "graph/graph.h"

namespace graph_utils {

namespace internal {

template <typename NodeId>
struct PairHash {
    std::size_t operator()(const std::pair<NodeId, NodeId>& p) const {
        const std::size_t h1 = std::hash<NodeId>{}(p.first);
        const std::size_t h2 = std::hash<NodeId>{}(p.second);
        // hash combine
        return h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
    }
};

template <typename NodeId>
std::pair<NodeId, NodeId> normalize_pair(const NodeId& a, const NodeId& b) {
    if (std::less<NodeId>{}(b, a)) {
        return {b, a};
    }
    return {a, b};
}

}  // namespace internal

/**
 * Collapse parallel edges into a single edge per (from,to), aggregating the new
 * edge weight via a reducer callback.
 *
 * - For directed graphs: key is (from,to).
 * - For undirected graphs: key is normalized {min(from,to), max(from,to)}.
 *
 * The output graph:
 * - preserves directedness and vertex set
 * - uses std::monostate edge labels
 * - stores the aggregated value as edge weight
 *
 * Reducer signature:
 *   AccWeight reducer(AccWeight current,
 *                     const Graph<NodeId,Label,Weight>::Edge& edge)
 */
template <typename NodeId, typename Label, typename Weight, typename AccWeight,
          typename Reducer>
Graph<NodeId, std::monostate, AccWeight> collapse_parallel_edges(
    const Graph<NodeId, Label, Weight>& g, const AccWeight& initial_value,
    Reducer reducer) {
    using InGraph = Graph<NodeId, Label, Weight>;
    using EdgeId = typename InGraph::EdgeId;

    Graph<NodeId, std::monostate, AccWeight> out(g.is_directed());
    for (const NodeId& v : g.vertices()) {
        out.add_vertex(v);
    }

    std::unordered_set<EdgeId> seen;
    std::unordered_map<std::pair<NodeId, NodeId>, AccWeight,
                       internal::PairHash<NodeId>>
        acc;

    for (const NodeId& u : g.vertices()) {
        for (EdgeId eid : g.out_edge_ids(u)) {
            if (seen.find(eid) != seen.end()) {
                continue;
            }
            seen.insert(eid);
            if (!g.has_edge(eid)) {
                continue;
            }
            const typename InGraph::Edge& e = g.edge(eid);

            std::pair<NodeId, NodeId> key;
            if (g.is_directed()) {
                key = {e.from, e.to};
            } else {
                key = internal::normalize_pair(e.from, e.to);
            }

            auto it = acc.find(key);
            if (it == acc.end()) {
                it = acc.emplace(key, initial_value).first;
            }
            it->second = reducer(it->second, e);
        }
    }

    for (const auto& kv : acc) {
        const NodeId& from = kv.first.first;
        const NodeId& to = kv.first.second;
        out.add_edge(from, to, std::monostate{}, kv.second);
    }

    return out;
}

/**
 * Convenience wrapper: collapse parallel edges and store multiplicity as weight.
 */
template <typename NodeId, typename Label, typename Weight>
Graph<NodeId, std::monostate, int> collapse_parallel_edges_count(
    const Graph<NodeId, Label, Weight>& g) {
    using InGraph = Graph<NodeId, Label, Weight>;
    return collapse_parallel_edges<NodeId, Label, Weight, int>(
        g, 0, [](int cur, const typename InGraph::Edge&) { return cur + 1; });
}

}  // namespace graph_utils

