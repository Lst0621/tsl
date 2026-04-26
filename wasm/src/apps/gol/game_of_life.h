#pragma once

#include <cstdint>
#include <vector>

#include "topology/coordinate.h"
#include "topology/topology.h"

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
    void set_wormhole_seed(uint32_t seed);
    void set_wormhole_count(int count);
    void set_cut_seed(uint32_t seed);
    void set_cut_count(int count);
    TopologyMode topology() const;

    // Debug/test helper.
    int debug_wormhole_edge_count() const;
    int debug_cut_node_count() const;

   private:
    int neighbor_count(int i, int j) const;
    int flat_index(int i, int j) const;
    void rebuild_wormholes();
    void rebuild_cuts();

    int size_;
    std::vector<std::vector<int>> grid_;
    Finite2D finite2d_;
    Torus2D torus2d_;
    Cylinder2D cylinder2d_;
    TopologyMode topo_;
    uint32_t last_seed_;

    std::vector<std::vector<int>> wormholes_;
    uint32_t wormhole_seed_;
    int wormhole_target_count_;
    int wormhole_edge_count_;

    std::vector<std::uint8_t> cut_nodes_;
    uint32_t cut_seed_;
    int cut_target_count_;
    int cut_node_count_;
};
