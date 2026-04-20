#include <gtest/gtest.h>

#include <map>
#include <vector>

#include "graph/graph_distances.h"
#include "graph/graph_generators.h"
#include "graph/metric_dimension.h"

static bool is_resolving_set_indices_reference(const Matrix<int>& dist,
                                               const std::vector<int>& comb) {
    const std::size_t n = dist.get_rows();
    std::map<std::vector<int>, int> seen;
    for (std::size_t v = 0; v < n; v++) {
        std::vector<int> rep;
        rep.reserve(comb.size());
        for (const int b : comb) {
            rep.push_back(dist(static_cast<std::size_t>(b), v));
        }
        if (seen.find(rep) != seen.end()) {
            return false;
        }
        seen.emplace(std::move(rep), static_cast<int>(v));
    }
    return true;
}

static bool next_combination_in_place_reference(std::vector<int>& comb, int n) {
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

static MetricDimensionResult metric_dimension_brute_force_reference(
    const Graph<int>& g) {
    if (g.is_directed()) {
        throw std::invalid_argument(
            "metric_dimension_brute_force_reference: graph must be undirected");
    }

    const GraphDistances gd = bfs_distances(g);
    const std::size_t n = gd.idx_to_node.size();
    if (n <= 1) {
        return MetricDimensionResult{0, {}};
    }

    const Matrix<int>& dist = gd.dist;
    for (std::size_t k = 1; k <= n; k++) {
        std::vector<int> comb(k);
        for (std::size_t i = 0; i < k; i++) {
            comb[i] = static_cast<int>(i);
        }
        while (true) {
            if (is_resolving_set_indices_reference(dist, comb)) {
                std::vector<int> basis;
                basis.reserve(k);
                for (std::size_t i = 0; i < k; i++) {
                    basis.push_back(gd.idx_to_node[static_cast<std::size_t>(
                        comb[i])]);
                }
                return MetricDimensionResult{static_cast<int>(k), basis};
            }
            if (!next_combination_in_place_reference(comb, static_cast<int>(n))) {
                break;
            }
        }
    }

    return MetricDimensionResult{static_cast<int>(n), gd.idx_to_node};
}

static bool resolves(const Graph<int>& g, const std::vector<int>& basis) {
    const GraphDistances gd = bfs_distances(g);
    const std::size_t n = gd.idx_to_node.size();

    std::vector<int> basis_idx;
    basis_idx.reserve(basis.size());
    for (const int node : basis) {
        auto it = gd.node_to_idx.find(node);
        if (it == gd.node_to_idx.end()) {
            return false;
        }
        basis_idx.push_back(it->second);
    }

    std::map<std::vector<int>, int> seen;
    for (std::size_t v = 0; v < n; v++) {
        std::vector<int> rep;
        rep.reserve(basis_idx.size());
        for (const int b : basis_idx) {
            rep.push_back(gd.dist(static_cast<std::size_t>(b), v));
        }
        if (seen.find(rep) != seen.end()) {
            return false;
        }
        seen.emplace(std::move(rep), static_cast<int>(v));
    }

    return true;
}

TEST(MetricDimensionTest, PathGraphs) {
    {
        Graph<int> g = path_graph(1, false);
        auto r = metric_dimension_brute_force(g);
        EXPECT_EQ(r.dimension, 0);
        EXPECT_EQ(r.basis.size(), 0u);
    }
    {
        Graph<int> g = path_graph(2, false);
        auto r = metric_dimension_brute_force(g);
        EXPECT_EQ(r.dimension, 1);
        EXPECT_EQ(r.basis.size(), 1u);
        EXPECT_TRUE(resolves(g, r.basis));
    }
    {
        Graph<int> g = path_graph(3, false);
        auto r = metric_dimension_brute_force(g);
        EXPECT_EQ(r.dimension, 1);
        EXPECT_EQ(r.basis.size(), 1u);
        EXPECT_TRUE(resolves(g, r.basis));
    }
    {
        Graph<int> g = path_graph(5, false);
        auto r = metric_dimension_brute_force(g);
        EXPECT_EQ(r.dimension, 1);
        EXPECT_EQ(r.basis.size(), 1u);
        EXPECT_TRUE(resolves(g, r.basis));
    }
}

TEST(MetricDimensionTest, CycleGraphs) {
    for (int n = 3; n <= 6; n++) {
        Graph<int> g = cycle_graph(n, false);
        auto r = metric_dimension_brute_force(g);
        EXPECT_EQ(r.dimension, 2);
        EXPECT_EQ(r.basis.size(), 2u);
        EXPECT_TRUE(resolves(g, r.basis));
    }
}

TEST(MetricDimensionTest, GridGraphs) {
    {
        Graph<int> g = grid_graph(2, 3, false);
        auto r = metric_dimension_brute_force(g);
        EXPECT_EQ(r.dimension, 2);
        EXPECT_EQ(r.basis.size(), 2u);
        EXPECT_TRUE(resolves(g, r.basis));
    }
    {
        Graph<int> g = grid_graph(3, 3, false);
        auto r = metric_dimension_brute_force(g);
        EXPECT_EQ(r.dimension, 2);
        EXPECT_EQ(r.basis.size(), 2u);
        EXPECT_TRUE(resolves(g, r.basis));
    }
}

TEST(MetricDimensionTest, PathGraphN10AllModesMinDimension) {
    Graph<int> g = path_graph(10, false);
    const GraphDistances gd = bfs_distances(g);
    const Matrix<int>& dist = gd.dist;
    std::vector<UndirectedEdge01> edges;
    edges.reserve(9);
    for (int i = 0; i < 9; i++) {
        edges.push_back(UndirectedEdge01{i, i + 1});
    }

    const GraphResolvingSubsetsResult r_nodes =
        resolving_subsets_bruteforce_upto_k(dist, edges, GraphResolveMode::Nodes,
                                            3);
    const GraphResolvingSubsetsResult r_edges =
        resolving_subsets_bruteforce_upto_k(dist, edges, GraphResolveMode::Edges,
                                            3);
    const GraphResolvingSubsetsResult r_mixed =
        resolving_subsets_bruteforce_upto_k(
            dist, edges, GraphResolveMode::NodesAndEdges, 3);

    EXPECT_EQ(r_nodes.min_dimension, 1);
    EXPECT_EQ(r_edges.min_dimension, 1);
    EXPECT_EQ(r_mixed.min_dimension, 2);

    EXPECT_EQ(r_nodes.smallest_basis.size(),
              static_cast<std::size_t>(r_nodes.min_dimension));
    EXPECT_EQ(r_edges.smallest_basis.size(),
              static_cast<std::size_t>(r_edges.min_dimension));
    EXPECT_EQ(r_mixed.smallest_basis.size(),
              static_cast<std::size_t>(r_mixed.min_dimension));

    EXPECT_TRUE(is_resolving_set_for_mode(dist, edges, GraphResolveMode::Nodes,
                                          r_nodes.smallest_basis));
    EXPECT_TRUE(is_resolving_set_for_mode(dist, edges, GraphResolveMode::Edges,
                                          r_edges.smallest_basis));
    EXPECT_TRUE(is_resolving_set_for_mode(
        dist, edges, GraphResolveMode::NodesAndEdges, r_mixed.smallest_basis));
}

TEST(MetricDimensionTest, RandomGraphsN15ReferenceMatch) {
    const int n = 15;
    const std::vector<double> densities = {0.2, 0.4, 0.6};
    for (std::size_t di = 0; di < densities.size(); di++) {
        const double density = densities[di];
        for (std::uint32_t seed = 0; seed < 30; seed++) {
            Graph<int> g = random_graph(n, density, seed);
            const MetricDimensionResult r_new = metric_dimension_brute_force(g);
            const MetricDimensionResult r_ref =
                metric_dimension_brute_force_reference(g);

            EXPECT_EQ(r_new.dimension, r_ref.dimension)
                << "n=15 density=" << density << " seed=" << seed;
            EXPECT_EQ(r_new.basis, r_ref.basis)
                << "n=15 density=" << density << " seed=" << seed;

            EXPECT_TRUE(resolves(g, r_new.basis))
                << "new basis should resolve by reference check; density="
                << density << " seed=" << seed;
        }
    }
}

