#include <gtest/gtest.h>

#include "graph/graph.h"

TEST(GraphDataStructureTest, UndirectedSingleEdgeStoredOnce) {
    Graph<int, char, int> g(false);
    int e = g.add_edge(1, 2, 'a', 7);

    EXPECT_EQ(g.vertex_count(), 2u);
    EXPECT_EQ(g.edge_count(), 1u);
    EXPECT_TRUE(g.has_edge(e));

    EXPECT_EQ(g.edge(e).from, 1);
    EXPECT_EQ(g.edge(e).to, 2);
    EXPECT_EQ(g.edge(e).label, 'a');
    EXPECT_EQ(g.edge(e).weight, 7);

    ASSERT_EQ(g.out_edge_ids(1).size(), 1u);
    ASSERT_EQ(g.out_edge_ids(2).size(), 1u);
    EXPECT_EQ(g.out_edge_ids(1)[0], e);
    EXPECT_EQ(g.out_edge_ids(2)[0], e);

    ASSERT_EQ(g.in_edge_ids(1).size(), 1u);
    ASSERT_EQ(g.in_edge_ids(2).size(), 1u);
    EXPECT_EQ(g.in_edge_ids(1)[0], e);
    EXPECT_EQ(g.in_edge_ids(2)[0], e);

    EXPECT_EQ(g.other(e, 1), 2);
    EXPECT_EQ(g.other(e, 2), 1);
}

TEST(GraphDataStructureTest, DirectedUsesOutAndInMaps) {
    Graph<int, char, int> g(true);
    int e = g.add_edge(1, 2, 'x', 3);

    EXPECT_EQ(g.vertex_count(), 2u);
    EXPECT_EQ(g.edge_count(), 1u);

    ASSERT_EQ(g.out_edge_ids(1).size(), 1u);
    EXPECT_EQ(g.out_edge_ids(1)[0], e);
    ASSERT_EQ(g.in_edge_ids(2).size(), 1u);
    EXPECT_EQ(g.in_edge_ids(2)[0], e);

    EXPECT_EQ(g.out_edge_ids(2).size(), 0u);
    EXPECT_EQ(g.in_edge_ids(1).size(), 0u);

    EXPECT_THROW(g.other(e, 1), std::logic_error);
}

TEST(GraphDataStructureTest, SelfLoopUndirectedWorks) {
    Graph<int, int, int> g(false);
    int e = g.add_edge(5, 5, 11, 2);

    EXPECT_EQ(g.vertex_count(), 1u);
    EXPECT_EQ(g.edge_count(), 1u);
    ASSERT_EQ(g.out_edge_ids(5).size(), 2u);
    ASSERT_EQ(g.in_edge_ids(5).size(), 2u);

    EXPECT_EQ(g.edge(e).from, 5);
    EXPECT_EQ(g.edge(e).to, 5);
    EXPECT_EQ(g.other(e, 5), 5);
}

TEST(GraphDataStructureTest, CanMutateEdgeWeightAndLabel) {
    Graph<int, char, int> g(true);
    int e = g.add_edge(1, 2, 'a', 1);

    g.edge(e).label = 'b';
    g.edge(e).weight = 42;

    EXPECT_EQ(g.edge(e).label, 'b');
    EXPECT_EQ(g.edge(e).weight, 42);
}

TEST(GraphDataStructureTest, EdgesBetweenFiltersCorrectly) {
    Graph<int, char, int> g(true);
    int e1 = g.add_edge(1, 2, 'a', 1);
    int e2 = g.add_edge(1, 2, 'b', 2);
    (void)e1;
    (void)e2;

    auto between = g.edges_between(1, 2);
    EXPECT_EQ(between.size(), 2u);

    auto reverse = g.edges_between(2, 1);
    EXPECT_EQ(reverse.size(), 0u);
}

TEST(GraphDataStructureTest, RemoveEdgeUpdatesIndices) {
    Graph<int, char, int> g(true);
    int e1 = g.add_edge(1, 2, 'a', 1);
    int e2 = g.add_edge(1, 3, 'b', 1);

    EXPECT_EQ(g.edge_count(), 2u);
    ASSERT_EQ(g.out_edge_ids(1).size(), 2u);

    g.remove_edge(e1);
    EXPECT_EQ(g.edge_count(), 1u);
    EXPECT_FALSE(g.has_edge(e1));
    EXPECT_TRUE(g.has_edge(e2));

    ASSERT_EQ(g.out_edge_ids(1).size(), 1u);
    EXPECT_EQ(g.out_edge_ids(1)[0], e2);

    ASSERT_EQ(g.in_edge_ids(2).size(), 0u);
    ASSERT_EQ(g.in_edge_ids(3).size(), 1u);
    EXPECT_EQ(g.in_edge_ids(3)[0], e2);

    EXPECT_THROW(g.edge(e1), std::out_of_range);
}

TEST(GraphDataStructureTest, RemoveVertexRemovesIncidentEdges) {
    Graph<int, char, int> g(false);
    int e1 = g.add_edge(1, 2, 'a', 1);
    int e2 = g.add_edge(2, 3, 'b', 1);

    EXPECT_EQ(g.vertex_count(), 3u);
    EXPECT_EQ(g.edge_count(), 2u);

    g.remove_vertex(2);
    EXPECT_EQ(g.vertex_count(), 2u);
    EXPECT_EQ(g.edge_count(), 0u);
    EXPECT_FALSE(g.has_edge(e1));
    EXPECT_FALSE(g.has_edge(e2));

    EXPECT_THROW(g.out_edge_ids(2), std::out_of_range);
    EXPECT_THROW(g.in_edge_ids(2), std::out_of_range);
}

