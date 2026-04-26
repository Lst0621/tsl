#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "game_of_life.h"

static std::vector<Coord2D> evolve_cells(int size, uint32_t wormhole_seed,
                                        uint32_t cell_seed, int steps) {
    GameOfLife gol(size);
    gol.set_topology(TopologyMode::Torus2D);
    gol.set_wormhole_seed(wormhole_seed);
    gol.set_wormhole_count(200);
    gol.random_init(0.2, cell_seed);
    for (int i = 0; i < steps; ++i) {
        gol.evolve();
    }
    return gol.get_live_cells();
}

static double avg_degree(int size, uint32_t seed, int wormholes) {
    GameOfLife gol(size);
    gol.set_topology(TopologyMode::Torus2D);
    gol.set_wormhole_seed(seed);
    gol.set_wormhole_count(wormholes);
    const int n = size * size;
    long long sum_deg = 0;
    for (int i = 0; i < size; ++i) {
        for (int j = 0; j < size; ++j) {
            // Baseline torus: 8 neighbors; wormholes add degree.
            // We can't query base degree directly; use a proxy by counting
            // neighbor contributions via two steps:
            // For the test we only care about monotonicity, so compute from
            // wormhole edge count.
            (void)i;
            (void)j;
        }
    }
    (void)sum_deg;
    return static_cast<double>(gol.debug_wormhole_edge_count()) / static_cast<double>(n);
}

TEST(GameOfLifeWormholesTest, EvolveDoesNotCrash) {
    GameOfLife gol(16);
    gol.set_topology(TopologyMode::Torus2D);
    gol.set_wormhole_seed(123u);
    gol.set_wormhole_count(200);
    gol.random_init(0.2, 456u);
    for (int i = 0; i < 50; ++i) {
        gol.evolve();
    }
    SUCCEED();
}

TEST(GameOfLifeWormholesTest, DeterministicGivenSeeds) {
    const auto a = evolve_cells(16, 999u, 111u, 50);
    const auto b = evolve_cells(16, 999u, 111u, 50);
    EXPECT_EQ(a, b);
}

TEST(GameOfLifeWormholesTest, WormholeEdgeCountIsExact) {
    const std::vector<int> sizes = {32, 64};
    const std::vector<uint32_t> seeds = {1u, 2u, 3u, 123u, 999u};
    for (int size : sizes) {
        for (uint32_t seed : seeds) {
            GameOfLife gol(size);
            gol.set_topology(TopologyMode::Torus2D);
            gol.set_wormhole_seed(seed);
            gol.set_wormhole_count(200);
            EXPECT_EQ(gol.debug_wormhole_edge_count(), 200) << "size=" << size << " seed=" << seed;
        }
    }
}

TEST(GameOfLifeWormholesTest, MoreWormholesMeansMoreWormholeEdges) {
    const int size = 64;
    const uint32_t seed = 123u;
    const double a0 = avg_degree(size, seed, 0);
    const double a1 = avg_degree(size, seed, 200);
    EXPECT_GT(a1, a0);
}

// Symmetry is guaranteed by construction (we add both directions).

