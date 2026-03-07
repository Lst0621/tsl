#include "game_of_life.h"

#include <random>

GameOfLife::GameOfLife(int size)
    : size_(size),
      finite2d_(size, size),
      torus2d_(size, size),
      cylinder2d_(size, size),
      topo_(TopologyMode::Torus2D),
      last_seed_(0) {
    grid_.resize(static_cast<size_t>(size));
    for (int i = 0; i < size; ++i) {
        grid_[static_cast<size_t>(i)].resize(static_cast<size_t>(size), 0);
    }
}

void GameOfLife::init() {
    for (int i = 0; i < size_; ++i) {
        for (int j = 0; j < size_; ++j) {
            grid_[static_cast<size_t>(i)][static_cast<size_t>(j)] = 0;
        }
    }
}

void GameOfLife::random_init(double live_prob) {
    std::random_device rd;
    random_init(live_prob, static_cast<uint32_t>(rd()));
}

void GameOfLife::random_init(double live_prob, uint32_t seed) {
    last_seed_ = seed;
    std::mt19937 gen(seed);
    std::bernoulli_distribution dist(live_prob);
    for (int i = 0; i < size_; ++i) {
        for (int j = 0; j < size_; ++j) {
            grid_[static_cast<size_t>(i)][static_cast<size_t>(j)] =
                dist(gen) ? 1 : 0;
        }
    }
}

uint32_t GameOfLife::last_seed() const {
    return last_seed_;
}

int GameOfLife::neighbor_count(int i, int j) const {
    std::vector<Coord2D> neighbors;
    switch (topo_) {
        case TopologyMode::Finite2D:
            neighbors = finite2d_.get_neighbors(Coord2D(i, j),
                                                NeighborType::VertexSharing);
            break;
        case TopologyMode::Torus2D:
            neighbors = torus2d_.get_neighbors(Coord2D(i, j),
                                               NeighborType::VertexSharing);
            break;
        case TopologyMode::Cylinder2D:
            neighbors = cylinder2d_.get_neighbors(Coord2D(i, j),
                                                  NeighborType::VertexSharing);
            break;
    }
    int sum = 0;
    for (const auto& n : neighbors) {
        sum += grid_[static_cast<size_t>(n.x)][static_cast<size_t>(n.y)];
    }
    return sum;
}

void GameOfLife::evolve() {
    std::vector<std::vector<int>> next(
        static_cast<size_t>(size_),
        std::vector<int>(static_cast<size_t>(size_), 0));
    for (int i = 0; i < size_; ++i) {
        for (int j = 0; j < size_; ++j) {
            int sum = neighbor_count(i, j);
            int cell = grid_[static_cast<size_t>(i)][static_cast<size_t>(j)];
            if ((cell == 1 && sum == 2) || sum == 3) {
                next[static_cast<size_t>(i)][static_cast<size_t>(j)] = 1;
            }
        }
    }
    grid_ = std::move(next);
}

std::vector<Coord2D> GameOfLife::get_live_cells() const {
    std::vector<Coord2D> out;
    for (int i = 0; i < size_; ++i) {
        for (int j = 0; j < size_; ++j) {
            if (grid_[static_cast<size_t>(i)][static_cast<size_t>(j)] == 1) {
                out.push_back(Coord2D(i, j));
            }
        }
    }
    return out;
}

void GameOfLife::set_topology(TopologyMode mode) {
    topo_ = mode;
}

TopologyMode GameOfLife::topology() const {
    return topo_;
}
