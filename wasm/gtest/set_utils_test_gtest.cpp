#include <gtest/gtest.h>

#include "util/set_utils.h"

#include <vector>

TEST(SetUtilsTest, CartesianProductEmpty) {
    std::vector<std::vector<int>> empty;
    auto out = cartesian_product(empty);
    EXPECT_EQ(out.size(), 1u);
    EXPECT_TRUE(out[0].empty());
}

TEST(SetUtilsTest, CartesianProductOneSet) {
    std::vector<std::vector<int>> input = {{1, 2, 3}};
    auto out = cartesian_product(input);
    EXPECT_EQ(out.size(), 3u);
    EXPECT_EQ(out[0], std::vector<int>({1}));
    EXPECT_EQ(out[1], std::vector<int>({2}));
    EXPECT_EQ(out[2], std::vector<int>({3}));
}

TEST(SetUtilsTest, CartesianProductTwoSets) {
    std::vector<std::vector<int>> input = {{1, 2}, {3, 4}};
    auto out = cartesian_product(input);
    EXPECT_EQ(out.size(), 4u);
    EXPECT_EQ(out[0], std::vector<int>({1, 3}));
    EXPECT_EQ(out[1], std::vector<int>({1, 4}));
    EXPECT_EQ(out[2], std::vector<int>({2, 3}));
    EXPECT_EQ(out[3], std::vector<int>({2, 4}));
}

TEST(SetUtilsTest, CartesianProductThreeSets) {
    std::vector<std::vector<int>> input = {{0, 1}, {0, 1}, {0, 1}};
    auto out = cartesian_product(input);
    EXPECT_EQ(out.size(), 8u);
}

TEST(SetUtilsTest, CartesianProductFourSetsSplitMiddle) {
    std::vector<std::vector<int>> input = {{1, 2}, {10, 20}, {100, 200}, {1000, 2000}};
    auto out = cartesian_product(input);
    EXPECT_EQ(out.size(), 16u);
    std::vector<int> first = {1, 10, 100, 1000};
    std::vector<int> last = {2, 20, 200, 2000};
    EXPECT_EQ(out[0], first);
    EXPECT_EQ(out.back(), last);
}

TEST(SetUtilsTest, CartesianProductSixSetsLimitedElements) {
    std::vector<std::vector<int>> input = {
        {0, 1},
        {0, 1},
        {0, 1},
        {0, 1},
        {0, 1},
        {0, 1},
    };
    auto out = cartesian_product(input);
    EXPECT_EQ(out.size(), 64u);
    EXPECT_EQ(out[0].size(), 6u);
    EXPECT_EQ(out[0], std::vector<int>({0, 0, 0, 0, 0, 0}));
    EXPECT_EQ(out.back(), std::vector<int>({1, 1, 1, 1, 1, 1}));
}

TEST(SetUtilsTest, CartesianProductSixSetsSmallSets) {
    std::vector<std::vector<int>> input = {
        {1},
        {2, 3},
        {4},
        {5, 6},
        {7},
        {8, 9},
    };
    auto out = cartesian_product(input);
    EXPECT_EQ(out.size(), 8u);
    EXPECT_EQ(out[0].size(), 6u);
    EXPECT_EQ(out[0], std::vector<int>({1, 2, 4, 5, 7, 8}));
    EXPECT_EQ(out.back(), std::vector<int>({1, 3, 4, 6, 7, 9}));
}

TEST(SetUtilsTest, CartesianProductAnyEmptyReturnsEmpty) {
    std::vector<std::vector<int>> input = {{1, 2}, {}, {5}};
    auto out = cartesian_product(input);
    EXPECT_TRUE(out.empty());
}

TEST(SetUtilsTest, CartesianProductMerge) {
    std::vector<std::vector<int>> left = {{1, 2}, {1, 3}};
    std::vector<std::vector<int>> right = {{10}, {20}};
    auto out = cartesian_product_merge(left, right);
    EXPECT_EQ(out.size(), 4u);
    EXPECT_EQ(out[0], std::vector<int>({1, 2, 10}));
    EXPECT_EQ(out[1], std::vector<int>({1, 2, 20}));
    EXPECT_EQ(out[2], std::vector<int>({1, 3, 10}));
    EXPECT_EQ(out[3], std::vector<int>({1, 3, 20}));
}
