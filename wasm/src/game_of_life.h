#pragma once

#include <cstdint>
#include <vector>

#include "coordinate.h"
#include "topology.h"

/**
 * Topology mode for Game of Life grid.
 * Same grid size used for all; neighbor lookups and boundary behavior depend on
 * mode.
 */
enum class TopologyMode { Finite2D, Torus2D, Cylinder2D };

/**
 * Conway's Game of Life on a square grid.
 * Supports Finite2D, Torus2D, and Cylinder2D topologies with runtime switching.
 */
class GameOfLife {
   public:
    explicit GameOfLife(int size);

    void init();
    void random_init(double live_prob);
    void random_init(double live_prob, uint32_t seed);
    void evolve();

    uint32_t last_seed() const;

    std::vector<Coord2D> get_live_cells() const;

    void set_topology(TopologyMode mode);
    TopologyMode topology() const;

   private:
    int neighbor_count(int i, int j) const;

    int size_;
    std::vector<std::vector<int>> grid_;
    Finite2D finite2d_;
    Torus2D torus2d_;
    Cylinder2D cylinder2d_;
    TopologyMode topo_;
    uint32_t last_seed_;
};
