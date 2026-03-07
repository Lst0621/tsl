#pragma once

#include <algorithm>
#include <cassert>
#include <cmath>
#include <concepts>
#include <vector>

#include "coordinate.h"

/**
 * Neighbor type enumeration
 * Defines different types of neighbor relationships based on geometric sharing
 */
enum class NeighborType {
    // 2D: 4 neighbors sharing an edge (cardinal directions: N, S, E, W)
    // 3D: 6 neighbors sharing a face
    EdgeSharing,

    // 3D only: 18 neighbors sharing an edge or vertex (6 face + 12 edge)
    EdgeAndVertexSharing,

    // 2D: 8 neighbors sharing a vertex (cardinal + diagonal)
    // 3D: 26 neighbors sharing a vertex (all adjacent cells)
    VertexSharing
};

/**
 * Topology concept
 * Defines the required interface for a topology type
 */
template <typename T, typename CoordType>
concept Topology =
    Coordinate<CoordType> &&
    requires(T topology, const T const_topology, const CoordType& coord1,
             const CoordType& coord2, NeighborType neighbor_type) {
        // Must be able to get neighbors of a coordinate
        {
            const_topology.get_neighbors(coord1, neighbor_type)
        } -> std::convertible_to<std::vector<CoordType>>;

        // Must be able to calculate L1 distance between two coordinates
        {
            const_topology.get_distance(coord1, coord2)
        } -> std::convertible_to<int>;

        // Must be able to validate a coordinate
        { const_topology.is_valid(coord1) } -> std::convertible_to<bool>;

        // Must be able to normalize a coordinate (for periodic topologies)
        { const_topology.normalize(coord1) } -> std::convertible_to<CoordType>;
    };

/**
 * Finite 2D topology
 * Rectangular grid with finite bounds [0, width) × [0, height)
 * Coordinates outside bounds are invalid
 */
class Finite2D {
   private:
    int width_;
    int height_;

   public:
    using CoordType = Coord2D;

    Finite2D(int width, int height) : width_(width), height_(height) {
        assert(width > 0 && height > 0);
    }

    int width() const {
        return width_;
    }
    int height() const {
        return height_;
    }

    /**
     * Check if a coordinate is valid (within bounds)
     */
    bool is_valid(const Coord2D& coord) const {
        return coord.x >= 0 && coord.x < width_ && coord.y >= 0 &&
               coord.y < height_;
    }

    /**
     * Normalize coordinate (no-op for finite topology)
     */
    Coord2D normalize(const Coord2D& coord) const {
        return coord;
    }

    /**
     * Get neighbors of a coordinate
     * Returns only valid neighbors within bounds
     */
    std::vector<Coord2D> get_neighbors(const Coord2D& coord,
                                       NeighborType type) const {
        std::vector<Coord2D> neighbors;

        if (type == NeighborType::EdgeSharing) {
            // 4 edge-sharing neighbors (cardinal directions)
            const std::vector<Coord2D> offsets = {
                {0, 1}, {0, -1}, {1, 0}, {-1, 0}};
            for (const auto& offset : offsets) {
                Coord2D neighbor = coord + offset;
                if (is_valid(neighbor)) {
                    neighbors.push_back(neighbor);
                }
            }
        } else if (type == NeighborType::VertexSharing) {
            // 8 vertex-sharing neighbors (cardinal + diagonal)
            for (int dx = -1; dx <= 1; ++dx) {
                for (int dy = -1; dy <= 1; ++dy) {
                    if (dx == 0 && dy == 0) continue;
                    Coord2D neighbor = coord + Coord2D(dx, dy);
                    if (is_valid(neighbor)) {
                        neighbors.push_back(neighbor);
                    }
                }
            }
        }

        return neighbors;
    }

    /**
     * Calculate L1 (Manhattan) distance between two coordinates
     */
    int get_distance(const Coord2D& coord1, const Coord2D& coord2) const {
        return std::abs(coord1.x - coord2.x) + std::abs(coord1.y - coord2.y);
    }
};

/**
 * Torus 2D topology
 * Rectangular grid with periodic boundary conditions in both directions
 * Wraps around: x mod width, y mod height
 * Accepts congruent coordinates
 */
class Torus2D {
   private:
    int width_;
    int height_;

    /**
     * Modulo operation that handles negative numbers correctly
     */
    static int mod(int a, int b) {
        int result = a % b;
        return result < 0 ? result + b : result;
    }

   public:
    using CoordType = Coord2D;

    Torus2D(int width, int height) : width_(width), height_(height) {
        assert(width > 0 && height > 0);
    }

    int width() const {
        return width_;
    }
    int height() const {
        return height_;
    }

    /**
     * All coordinates are valid on a torus (wraps around)
     */
    bool is_valid(const Coord2D& coord) const {
        (void)coord;  // Suppress unused parameter warning
        return true;
    }

    /**
     * Normalize coordinate to canonical form [0, width) × [0, height)
     */
    Coord2D normalize(const Coord2D& coord) const {
        return Coord2D(mod(coord.x, width_), mod(coord.y, height_));
    }

    /**
     * Get neighbors of a coordinate
     * All neighbors are valid (wrapping handled by normalization)
     */
    std::vector<Coord2D> get_neighbors(const Coord2D& coord,
                                       NeighborType type) const {
        std::vector<Coord2D> neighbors;
        Coord2D normalized = normalize(coord);

        if (type == NeighborType::EdgeSharing) {
            // 4 edge-sharing neighbors
            const std::vector<Coord2D> offsets = {
                {0, 1}, {0, -1}, {1, 0}, {-1, 0}};
            for (const auto& offset : offsets) {
                neighbors.push_back(normalize(normalized + offset));
            }
        } else if (type == NeighborType::VertexSharing) {
            // 8 vertex-sharing neighbors
            for (int dx = -1; dx <= 1; ++dx) {
                for (int dy = -1; dy <= 1; ++dy) {
                    if (dx == 0 && dy == 0) continue;
                    neighbors.push_back(
                        normalize(normalized + Coord2D(dx, dy)));
                }
            }
        }

        return neighbors;
    }

    /**
     * Calculate minimum L1 distance considering wrapping
     * Returns shortest path distance on the torus
     */
    int get_distance(const Coord2D& coord1, const Coord2D& coord2) const {
        Coord2D c1 = normalize(coord1);
        Coord2D c2 = normalize(coord2);

        // Calculate minimum distance in each dimension considering wrapping
        int dx = std::abs(c1.x - c2.x);
        int dy = std::abs(c1.y - c2.y);

        // Consider wrapping around
        dx = std::min(dx, width_ - dx);
        dy = std::min(dy, height_ - dy);

        return dx + dy;
    }
};

/**
 * Cylinder 2D topology
 * Rectangular grid with periodic boundary in x-direction, finite in y-direction
 * Wraps x: x mod width, y: [0, height)
 * Accepts congruent coordinates in x-direction
 */
class Cylinder2D {
   private:
    int width_;
    int height_;

    /**
     * Modulo operation that handles negative numbers correctly
     */
    static int mod(int a, int b) {
        int result = a % b;
        return result < 0 ? result + b : result;
    }

   public:
    using CoordType = Coord2D;

    Cylinder2D(int width, int height) : width_(width), height_(height) {
        assert(width > 0 && height > 0);
    }

    int width() const {
        return width_;
    }
    int height() const {
        return height_;
    }

    /**
     * Check if coordinate is valid
     * x can be any value (wraps), y must be in [0, height)
     */
    bool is_valid(const Coord2D& coord) const {
        return coord.y >= 0 && coord.y < height_;
    }

    /**
     * Normalize coordinate
     * x is normalized to [0, width), y stays as is
     */
    Coord2D normalize(const Coord2D& coord) const {
        return Coord2D(mod(coord.x, width_), coord.y);
    }

    /**
     * Get neighbors of a coordinate
     * Returns neighbors with valid y-coordinates
     */
    std::vector<Coord2D> get_neighbors(const Coord2D& coord,
                                       NeighborType type) const {
        std::vector<Coord2D> neighbors;
        Coord2D normalized = normalize(coord);

        if (type == NeighborType::EdgeSharing) {
            // 4 edge-sharing neighbors
            const std::vector<Coord2D> offsets = {
                {0, 1}, {0, -1}, {1, 0}, {-1, 0}};
            for (const auto& offset : offsets) {
                Coord2D neighbor = normalized + offset;
                if (is_valid(neighbor)) {
                    neighbors.push_back(normalize(neighbor));
                }
            }
        } else if (type == NeighborType::VertexSharing) {
            // 8 vertex-sharing neighbors
            for (int dx = -1; dx <= 1; ++dx) {
                for (int dy = -1; dy <= 1; ++dy) {
                    if (dx == 0 && dy == 0) continue;
                    Coord2D neighbor = normalized + Coord2D(dx, dy);
                    if (is_valid(neighbor)) {
                        neighbors.push_back(normalize(neighbor));
                    }
                }
            }
        }

        return neighbors;
    }

    /**
     * Calculate L1 distance considering x-wrapping
     * Returns shortest path distance on the cylinder
     */
    int get_distance(const Coord2D& coord1, const Coord2D& coord2) const {
        Coord2D c1 = normalize(coord1);
        Coord2D c2 = normalize(coord2);

        // x-direction: consider wrapping
        int dx = std::abs(c1.x - c2.x);
        dx = std::min(dx, width_ - dx);

        // y-direction: no wrapping
        int dy = std::abs(c1.y - c2.y);

        return dx + dy;
    }
};

/**
 * Finite 3D topology
 * Rectangular grid with finite bounds [0, width) × [0, height) × [0, depth)
 * Coordinates outside bounds are invalid
 */
class Finite3D {
   private:
    int width_;
    int height_;
    int depth_;

   public:
    using CoordType = Coord3D;

    Finite3D(int width, int height, int depth)
        : width_(width), height_(height), depth_(depth) {
        assert(width > 0 && height > 0 && depth > 0);
    }

    int width() const {
        return width_;
    }
    int height() const {
        return height_;
    }
    int depth() const {
        return depth_;
    }

    /**
     * Check if a coordinate is valid (within bounds)
     */
    bool is_valid(const Coord3D& coord) const {
        return coord.x >= 0 && coord.x < width_ && coord.y >= 0 &&
               coord.y < height_ && coord.z >= 0 && coord.z < depth_;
    }

    /**
     * Normalize coordinate (no-op for finite topology)
     */
    Coord3D normalize(const Coord3D& coord) const {
        return coord;
    }

    /**
     * Get neighbors of a coordinate
     * Returns only valid neighbors within bounds
     */
    std::vector<Coord3D> get_neighbors(const Coord3D& coord,
                                       NeighborType type) const {
        std::vector<Coord3D> neighbors;

        if (type == NeighborType::EdgeSharing) {
            // 6 face-sharing neighbors
            const std::vector<Coord3D> offsets = {{1, 0, 0}, {-1, 0, 0},
                                                  {0, 1, 0}, {0, -1, 0},
                                                  {0, 0, 1}, {0, 0, -1}};
            for (const auto& offset : offsets) {
                Coord3D neighbor = coord + offset;
                if (is_valid(neighbor)) {
                    neighbors.push_back(neighbor);
                }
            }
        } else if (type == NeighborType::EdgeAndVertexSharing) {
            // 18 neighbors: 6 face-sharing + 12 edge-sharing
            for (int dx = -1; dx <= 1; ++dx) {
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dz = -1; dz <= 1; ++dz) {
                        // Skip center
                        if (dx == 0 && dy == 0 && dz == 0) continue;

                        // Skip pure diagonal (vertex only, not edge)
                        int nonzero_count = (dx != 0 ? 1 : 0) +
                                            (dy != 0 ? 1 : 0) +
                                            (dz != 0 ? 1 : 0);
                        if (nonzero_count == 3) continue;

                        Coord3D neighbor = coord + Coord3D(dx, dy, dz);
                        if (is_valid(neighbor)) {
                            neighbors.push_back(neighbor);
                        }
                    }
                }
            }
        } else if (type == NeighborType::VertexSharing) {
            // 26 vertex-sharing neighbors (all adjacent)
            for (int dx = -1; dx <= 1; ++dx) {
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dz = -1; dz <= 1; ++dz) {
                        if (dx == 0 && dy == 0 && dz == 0) continue;
                        Coord3D neighbor = coord + Coord3D(dx, dy, dz);
                        if (is_valid(neighbor)) {
                            neighbors.push_back(neighbor);
                        }
                    }
                }
            }
        }

        return neighbors;
    }

    /**
     * Calculate L1 (Manhattan) distance between two coordinates
     */
    int get_distance(const Coord3D& coord1, const Coord3D& coord2) const {
        return std::abs(coord1.x - coord2.x) + std::abs(coord1.y - coord2.y) +
               std::abs(coord1.z - coord2.z);
    }
};

// Verify that our topology types satisfy the concept
static_assert(Topology<Finite2D, Coord2D>);
static_assert(Topology<Torus2D, Coord2D>);
static_assert(Topology<Cylinder2D, Coord2D>);
static_assert(Topology<Finite3D, Coord3D>);
