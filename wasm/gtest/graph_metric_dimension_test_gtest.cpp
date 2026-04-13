#include <gtest/gtest.h>

#include <map>
#include <vector>

#include "graph/graph_distances.h"
#include "graph/graph_generators.h"
#include "graph/metric_dimension.h"

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

