#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

#include "graph/graph.h"
#include "semigroup.h"

inline Graph<int> path_graph(int n, bool directed = false) {
    if (n < 0) {
        throw std::invalid_argument("path_graph: n must be >= 0");
    }
    Graph<int> g(directed);
    for (int i = 0; i < n; i++) {
        g.add_vertex(i);
    }
    for (int i = 0; i + 1 < n; i++) {
        g.add_edge(i, i + 1);
    }
    return g;
}

inline Graph<int> cycle_graph(int n, bool directed = false) {
    if (n < 0) {
        throw std::invalid_argument("cycle_graph: n must be >= 0");
    }
    Graph<int> g(directed);
    for (int i = 0; i < n; i++) {
        g.add_vertex(i);
    }
    if (n <= 1) {
        return g;
    }

    for (int i = 0; i + 1 < n; i++) {
        g.add_edge(i, i + 1);
    }

    if (directed) {
        g.add_edge(n - 1, 0);
    } else {
        // Avoid multi-edge for n == 2 in an undirected simple graph.
        if (n > 2) {
            g.add_edge(n - 1, 0);
        }
    }

    return g;
}

inline Graph<int> complete_graph(int n, bool directed = false) {
    if (n < 0) {
        throw std::invalid_argument("complete_graph: n must be >= 0");
    }
    Graph<int> g(directed);
    for (int i = 0; i < n; i++) {
        g.add_vertex(i);
    }

    if (directed) {
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                if (i != j) {
                    g.add_edge(i, j);
                }
            }
        }
    } else {
        for (int i = 0; i < n; i++) {
            for (int j = i + 1; j < n; j++) {
                g.add_edge(i, j);
            }
        }
    }

    return g;
}

inline Graph<int> grid_product(const std::vector<std::pair<int, bool>>& dims,
                               bool directed = false) {
    if (dims.empty()) {
        return Graph<int>(directed);
    }

    std::vector<std::size_t> sizes;
    sizes.reserve(dims.size());
    for (const auto& dim : dims) {
        if (dim.first <= 0) {
            throw std::invalid_argument("grid_product: dim size must be > 0");
        }
        sizes.push_back(static_cast<std::size_t>(dim.first));
    }

    std::vector<std::size_t> stride(sizes.size(), 1);
    for (std::size_t i = sizes.size(); i-- > 0;) {
        if (i + 1 < sizes.size()) {
            if (sizes[i + 1] != 0 &&
                stride[i + 1] >
                    (std::numeric_limits<std::size_t>::max() / sizes[i + 1])) {
                throw std::overflow_error("grid_product: node count overflow");
            }
            stride[i] = stride[i + 1] * sizes[i + 1];
        }
    }

    std::size_t total = 1;
    for (std::size_t i = 0; i < sizes.size(); i++) {
        if (sizes[i] != 0 &&
            total > (std::numeric_limits<std::size_t>::max() / sizes[i])) {
            throw std::overflow_error("grid_product: node count overflow");
        }
        total *= sizes[i];
    }

    if (total > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
        throw std::overflow_error("grid_product: too many nodes for int NodeId");
    }

    Graph<int> g(directed);
    for (int id = 0; id < static_cast<int>(total); id++) {
        g.add_vertex(id);
    }

    for (int id = 0; id < static_cast<int>(total); id++) {
        for (std::size_t dim = 0; dim < sizes.size(); dim++) {
            const std::size_t s = stride[dim];
            const std::size_t size = sizes[dim];

            std::size_t coord = (static_cast<std::size_t>(id) / s) % size;

            if (coord + 1 < size) {
                int nb = static_cast<int>(static_cast<std::size_t>(id) + s);
                g.add_edge(id, nb);
                continue;
            }

            bool wrap = dims[dim].second;
            if (wrap) {
                // Treat very small wrapped dimensions as simple paths to avoid
                // generating multi-edges (e.g. cycle of length 2 in undirected).
                if (size > 2) {
                    int nb = static_cast<int>(
                        static_cast<std::size_t>(id) - (size - 1) * s);
                    g.add_edge(id, nb);
                }
            }
        }
    }

    return g;
}

inline Graph<int> grid_graph(int m, int n, bool directed = false) {
    return grid_product({{m, false}, {n, false}}, directed);
}

inline Graph<int> torus_graph(int m, int n, bool directed = false) {
    return grid_product({{m, true}, {n, true}}, directed);
}

inline Graph<int> cylinder_graph(int m, int n, bool directed = false) {
    return grid_product({{m, false}, {n, true}}, directed);
}

inline Graph<int> random_graph(int n, double density, std::uint32_t seed = 0) {
    if (n < 0) {
        throw std::invalid_argument("random_graph: n must be >= 0");
    }
    if (density < 0.0 || density > 1.0) {
        throw std::invalid_argument("random_graph: density must be in [0, 1]");
    }

    Graph<int> g(false);
    for (int i = 0; i < n; i++) {
        g.add_vertex(i);
    }

    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            if (dist(rng) < density) {
                g.add_edge(i, j);
            }
        }
    }

    return g;
}

namespace graph_generators_internal {

template <typename T>
int find_index_or_throw(const std::vector<T>& elements, const T& value,
                        const char* context) {
    for (std::size_t i = 0; i < elements.size(); i++) {
        if (elements[i] == value) {
            return static_cast<int>(i);
        }
    }
    throw std::invalid_argument(context);
}

}  // namespace graph_generators_internal

/**
 * Monoid (or semigroup) multiplication graph.
 *
 * Vertices: indices 0..n-1 representing elements[i].
 * Directed edges:
 *   - left:  i -> idx(m * x)
 *   - right: i -> idx(x * m)
 * Edge label: multiplier index (in the same index namespace as vertices).
 * Edge weight: 1.
 *
 * Preconditions:
 * - elements must be closed under operator* (checked via is_closure()).
 * - multiplier_indices must be in-range.
 */
template <typename T>
Graph<int, int, int> monoid_multiplication_graph(
    const std::vector<T>& elements, const std::vector<int>& multiplier_indices,
    bool left_multiply = true, bool right_multiply = true) {
    if (!left_multiply && !right_multiply) {
        throw std::invalid_argument(
            "monoid_multiplication_graph: at least one of left_multiply or "
            "right_multiply must be true");
    }
    if (!is_closure(elements, is_abelian(elements))) {
        throw std::invalid_argument(
            "monoid_multiplication_graph: elements must be closed under "
            "multiplication");
    }

    const int n = static_cast<int>(elements.size());
    Graph<int, int, int> g(true);
    for (int i = 0; i < n; i++) {
        g.add_vertex(i);
    }

    for (int midx : multiplier_indices) {
        if (midx < 0 || midx >= n) {
            throw std::invalid_argument(
                "monoid_multiplication_graph: multiplier index out of range");
        }
    }

    for (int i = 0; i < n; i++) {
        const T& x = elements[static_cast<std::size_t>(i)];
        for (int midx : multiplier_indices) {
            const T& m = elements[static_cast<std::size_t>(midx)];
            if (left_multiply) {
                T y = m * x;
                int j = graph_generators_internal::find_index_or_throw(
                    elements, y,
                    "monoid_multiplication_graph: left product not found in "
                    "elements");
                g.add_edge(i, j, midx, 1);
            }
            if (right_multiply) {
                T y = x * m;
                int j = graph_generators_internal::find_index_or_throw(
                    elements, y,
                    "monoid_multiplication_graph: right product not found in "
                    "elements");
                g.add_edge(i, j, midx, 1);
            }
        }
    }

    return g;
}

/**
 * Convenience wrapper: use all elements as multipliers.
 */
template <typename T>
Graph<int, int, int> monoid_multiplication_graph_all(
    const std::vector<T>& elements, bool left_multiply = true,
    bool right_multiply = true) {
    std::vector<int> mult;
    mult.reserve(elements.size());
    for (std::size_t i = 0; i < elements.size(); i++) {
        mult.push_back(static_cast<int>(i));
    }
    return monoid_multiplication_graph(elements, mult, left_multiply,
                                       right_multiply);
}

