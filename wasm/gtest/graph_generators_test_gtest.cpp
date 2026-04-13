#include <gtest/gtest.h>

#include "graph/graph_generators.h"
#include "graph/graph_utils.h"
#include "modular_number.h"

TEST(GraphGeneratorsTest, PathGraphUndirected) {
    Graph<int> g = path_graph(4, false);
    EXPECT_EQ(g.vertex_count(), 4u);
    EXPECT_EQ(g.edge_count(), 3u);

    EXPECT_EQ(g.out_edge_ids(0).size(), 1u);
    EXPECT_EQ(g.out_edge_ids(1).size(), 2u);
    EXPECT_EQ(g.out_edge_ids(2).size(), 2u);
    EXPECT_EQ(g.out_edge_ids(3).size(), 1u);
}

TEST(GraphGeneratorsTest, PathGraphDirected) {
    Graph<int> g = path_graph(4, true);
    EXPECT_EQ(g.vertex_count(), 4u);
    EXPECT_EQ(g.edge_count(), 3u);

    EXPECT_EQ(g.out_edge_ids(0).size(), 1u);
    EXPECT_EQ(g.in_edge_ids(0).size(), 0u);

    EXPECT_EQ(g.out_edge_ids(1).size(), 1u);
    EXPECT_EQ(g.in_edge_ids(1).size(), 1u);
}

TEST(GraphGeneratorsTest, CycleGraphUndirected) {
    Graph<int> g = cycle_graph(5, false);
    EXPECT_EQ(g.vertex_count(), 5u);
    EXPECT_EQ(g.edge_count(), 5u);

    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(g.out_edge_ids(i).size(), 2u);
    }
}

TEST(GraphGeneratorsTest, CycleGraphUndirectedN2AvoidsMultiEdge) {
    Graph<int> g = cycle_graph(2, false);
    EXPECT_EQ(g.vertex_count(), 2u);
    EXPECT_EQ(g.edge_count(), 1u);
}

TEST(GraphGeneratorsTest, CompleteGraphUndirected) {
    Graph<int> g = complete_graph(4, false);
    EXPECT_EQ(g.vertex_count(), 4u);
    EXPECT_EQ(g.edge_count(), 6u);
    for (int i = 0; i < 4; i++) {
        EXPECT_EQ(g.out_edge_ids(i).size(), 3u);
    }
}

TEST(GraphGeneratorsTest, CompleteGraphDirected) {
    Graph<int> g = complete_graph(4, true);
    EXPECT_EQ(g.vertex_count(), 4u);
    EXPECT_EQ(g.edge_count(), 12u);
    for (int i = 0; i < 4; i++) {
        EXPECT_EQ(g.out_edge_ids(i).size(), 3u);
        EXPECT_EQ(g.in_edge_ids(i).size(), 3u);
    }
}

TEST(GraphGeneratorsTest, GridGraph2x3Undirected) {
    Graph<int> g = grid_graph(2, 3, false);
    EXPECT_EQ(g.vertex_count(), 6u);
    EXPECT_EQ(g.edge_count(), 7u);
}

TEST(GraphGeneratorsTest, TorusGraph3x4Undirected) {
    Graph<int> g = torus_graph(3, 4, false);
    EXPECT_EQ(g.vertex_count(), 12u);
    EXPECT_EQ(g.edge_count(), 24u);
    for (int i = 0; i < 12; i++) {
        EXPECT_EQ(g.out_edge_ids(i).size(), 4u);
    }
}

TEST(GraphGeneratorsTest, CylinderGraph3x4Undirected) {
    Graph<int> g = cylinder_graph(3, 4, false);
    EXPECT_EQ(g.vertex_count(), 12u);
    EXPECT_EQ(g.edge_count(), 20u);
}

TEST(GraphGeneratorsTest, RandomGraphSeedReproducible) {
    const int n = 8;
    const double density = 0.5;
    Graph<int> g1 = random_graph(n, density, 123u);
    Graph<int> g2 = random_graph(n, density, 123u);
    EXPECT_EQ(g1.vertex_count(), g2.vertex_count());
    EXPECT_EQ(g1.edge_count(), g2.edge_count());

    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            const bool e1 = !g1.edges_between(i, j).empty();
            const bool e2 = !g2.edges_between(i, j).empty();
            EXPECT_EQ(e1, e2);
        }
    }
}

TEST(GraphGeneratorsTest, RandomGraphDifferentSeeds) {
    const int n = 8;
    const double density = 0.5;
    Graph<int> g1 = random_graph(n, density, 1u);
    Graph<int> g2 = random_graph(n, density, 2u);

    bool any_diff = false;
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            const bool e1 = !g1.edges_between(i, j).empty();
            const bool e2 = !g2.edges_between(i, j).empty();
            if (e1 != e2) {
                any_diff = true;
            }
        }
    }
    EXPECT_TRUE(any_diff);
}

TEST(GraphGeneratorsTest, RandomGraphDensityBoundaries) {
    const int n = 6;
    Graph<int> g0 = random_graph(n, 0.0, 999u);
    EXPECT_EQ(g0.vertex_count(), static_cast<std::size_t>(n));
    EXPECT_EQ(g0.edge_count(), 0u);

    Graph<int> g1 = random_graph(n, 1.0, 999u);
    EXPECT_EQ(g1.vertex_count(), static_cast<std::size_t>(n));
    EXPECT_EQ(g1.edge_count(),
              static_cast<std::size_t>(n) *
                  static_cast<std::size_t>(n - 1) / 2u);

    for (int i = 0; i < n; i++) {
        EXPECT_TRUE(g1.edges_between(i, i).empty());
    }
}

TEST(GraphGeneratorsTest, MonoidMultiplicationGraphMod3RightMultiplyBy2) {
    std::vector<ModularNumber> elems = {ModularNumber(0, 3), ModularNumber(1, 3),
                                        ModularNumber(2, 3)};
    Graph<int, int, int> g =
        monoid_multiplication_graph(elems, {2}, false, true);

    // x -> x*2 mod 3: 0->0, 1->2, 2->1
    auto e01 = g.edges_between(0, 0);
    ASSERT_EQ(e01.size(), 1u);
    EXPECT_EQ(g.edge(e01[0]).label, 2);

    auto e12 = g.edges_between(1, 2);
    ASSERT_EQ(e12.size(), 1u);
    EXPECT_EQ(g.edge(e12[0]).label, 2);

    auto e21 = g.edges_between(2, 1);
    ASSERT_EQ(e21.size(), 1u);
    EXPECT_EQ(g.edge(e21[0]).label, 2);
}

namespace {

struct Endo2 {
    int f0;
    int f1;

    int apply(int x) const {
        if (x == 0) {
            return f0;
        } else {
            return f1;
        }
    }

    // Composition: (this * other)(x) = other(this(x))
    Endo2 operator*(const Endo2& other) const {
        return Endo2{other.apply(apply(0)), other.apply(apply(1))};
    }

    bool operator==(const Endo2& other) const {
        return f0 == other.f0 && f1 == other.f1;
    }
};

}  // namespace

TEST(GraphGeneratorsTest, MonoidMultiplicationGraphLeftVsRightNonCommutative) {
    // Endomorphisms on {0,1} form a monoid under composition.
    // Use the full 4-element monoid of functions on {0,1}:
    //   id:     0->0, 1->1
    //   swap:   0->1, 1->0
    //   const0: 0->0, 1->0
    //   const1: 0->1, 1->1
    Endo2 id{0, 1};
    Endo2 swap{1, 0};
    Endo2 const0{0, 0};
    Endo2 const1{1, 1};
    std::vector<Endo2> elems = {id, swap, const0, const1};

    // Left multiply by swap (index 1): x -> swap * x
    Graph<int, int, int> left_g =
        monoid_multiplication_graph(elems, {1}, true, false);
    // Right multiply by swap (index 1): x -> x * swap
    Graph<int, int, int> right_g =
        monoid_multiplication_graph(elems, {1}, false, true);

    // For x = const0 (index 2):
    // swap * const0 = const0
    // const0 * swap = const1
    auto left_edge = left_g.edges_between(2, 2);
    ASSERT_EQ(left_edge.size(), 1u);
    EXPECT_EQ(left_g.edge(left_edge[0]).label, 1);

    auto right_edge = right_g.edges_between(2, 3);
    ASSERT_EQ(right_edge.size(), 1u);
    EXPECT_EQ(right_g.edge(right_edge[0]).label, 1);
}

TEST(GraphGeneratorsTest, GraphUtilsCollapseParallelEdgesCountAndCustomReducer) {
    Graph<int, int, int> g(true);
    g.add_vertex(0);
    g.add_vertex(1);
    g.add_edge(0, 1, 5, 10);
    g.add_edge(0, 1, 6, 20);
    g.add_edge(0, 1, 7, 30);

    auto collapsed_count = graph_utils::collapse_parallel_edges_count(g);
    auto ids = collapsed_count.edges_between(0, 1);
    ASSERT_EQ(ids.size(), 1u);
    EXPECT_EQ(collapsed_count.edge(ids[0]).weight, 3);

    auto collapsed_sum_labels =
        graph_utils::collapse_parallel_edges<int, int, int, int>(
            g, 0,
            [](int cur, const Graph<int, int, int>::Edge& e) {
                return cur + e.label;
            });
    auto ids2 = collapsed_sum_labels.edges_between(0, 1);
    ASSERT_EQ(ids2.size(), 1u);
    EXPECT_EQ(collapsed_sum_labels.edge(ids2[0]).weight, 5 + 6 + 7);
}

