#include <gtest/gtest.h>

#include "graph/graph_distances.h"
#include "graph/graph_generators.h"

TEST(GraphDistancesTest, P4Distances) {
    Graph<int> g = path_graph(4, false);
    const GraphDistances gd = bfs_distances(g);

    ASSERT_EQ(gd.idx_to_node.size(), 4u);

    const int expected[4][4] = {
        {0, 1, 2, 3},
        {1, 0, 1, 2},
        {2, 1, 0, 1},
        {3, 2, 1, 0},
    };

    for (std::size_t i = 0; i < 4; i++) {
        for (std::size_t j = 0; j < 4; j++) {
            EXPECT_EQ(gd.dist(i, j), expected[i][j]);
            EXPECT_EQ(gd.dist(i, j), gd.dist(j, i));
        }
    }
}

TEST(GraphDistancesTest, C5Distances) {
    Graph<int> g = cycle_graph(5, false);
    const GraphDistances gd = bfs_distances(g);

    ASSERT_EQ(gd.idx_to_node.size(), 5u);

    const int expected[5][5] = {
        {0, 1, 2, 2, 1},
        {1, 0, 1, 2, 2},
        {2, 1, 0, 1, 2},
        {2, 2, 1, 0, 1},
        {1, 2, 2, 1, 0},
    };

    for (std::size_t i = 0; i < 5; i++) {
        for (std::size_t j = 0; j < 5; j++) {
            EXPECT_EQ(gd.dist(i, j), expected[i][j]);
            EXPECT_EQ(gd.dist(i, j), gd.dist(j, i));
        }
    }
}

TEST(GraphDistancesTest, IdxMappingRoundTrip) {
    Graph<int> g(false);
    g.add_edge(10, 20);
    g.add_edge(20, 30);

    const GraphDistances gd = bfs_distances(g);

    ASSERT_EQ(gd.idx_to_node.size(), 3u);

    for (std::size_t i = 0; i < gd.idx_to_node.size(); i++) {
        const int node = gd.idx_to_node[i];
        const auto it = gd.node_to_idx.find(node);
        ASSERT_NE(it, gd.node_to_idx.end());
        EXPECT_EQ(it->second, static_cast<int>(i));
        EXPECT_EQ(gd.idx_to_node[static_cast<std::size_t>(it->second)], node);
    }
}

