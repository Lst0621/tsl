#include "game_of_life.h"

#include <algorithm>
#include <cstdint>
#include <cmath>
#include <random>
#include <unordered_set>
#include <utility>

GameOfLife::GameOfLife(int size)
    : size_(size),
      finite2d_(size, size),
      torus2d_(size, size),
      cylinder2d_(size, size),
      topo_(TopologyMode::Torus2D),
      last_seed_(0),
      wormholes_(),
      wormhole_seed_(0),
      wormhole_target_count_(0),
      wormhole_edge_count_(0),
      cut_nodes_(),
      cut_seed_(0),
      cut_target_count_(0),
      cut_node_count_(0) {
    grid_.resize(static_cast<size_t>(size));
    for (int i = 0; i < size; ++i) {
        grid_[static_cast<size_t>(i)].resize(static_cast<size_t>(size), 0);
    }
    cut_nodes_.assign(static_cast<size_t>(size_ * size_), 0);
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

int GameOfLife::flat_index(int i, int j) const {
    return i * size_ + j;
}

static uint32_t mix_seed(uint32_t x) {
    // SplitMix-like avalanche (deterministic, fast).
    uint32_t z = x + 0x9e3779b9u;
    z ^= z >> 16;
    z *= 0x85ebca6bu;
    z ^= z >> 13;
    z *= 0xc2b2ae35u;
    z ^= z >> 16;
    return z;
}

void GameOfLife::rebuild_wormholes() {
    const int n = size_ * size_;
    wormholes_.clear();
    wormholes_.resize(static_cast<size_t>(n));
    wormhole_edge_count_ = 0;

    std::uint32_t seed = wormhole_seed_;
    if (seed == 0) {
        seed = mix_seed(last_seed_);
        if (seed == 0) {
            seed = 1;
        }
    }
    std::mt19937 gen(seed);

    const auto wrap_mod = [](int a, int m) -> int {
        int r = a % m;
        if (r < 0) {
            r += m;
        }
        return r;
    };

    // Wormholes: add exactly K random edges (uniform over pairs), without
    // replacement, excluding existing base edges for the current topology.
    int k = wormhole_target_count_;
    if (k < 0) {
        k = 0;
    }
    // Hard cap for stability: prevents pathological allocations/runtime if UI
    // requests an enormous number of wormholes.
    // (WASM memory growth is limited; huge k can easily OOM via chosen.reserve.)
    const int K_HARD_CAP = 1'000'000;
    if (k > K_HARD_CAP) {
        k = K_HARD_CAP;
    }
    const long long all_pairs = (static_cast<long long>(n) * (n - 1)) / 2;
    // Conservative clamp: we don't know exact forbidden count cheaply, but this
    // prevents pathological huge K values.
    const long long max_possible = all_pairs;
    if (static_cast<long long>(k) > max_possible) {
        k = static_cast<int>(max_possible);
    }

    const auto is_base_edge = [this, wrap_mod](int u, int v) -> bool {
        if (u == v) {
            return false;
        }
        // If either endpoint is cut, its base edges are removed, so allow
        // wormholes even if they would be a base-neighborhood pair.
        if (!cut_nodes_.empty()) {
            if (cut_nodes_[static_cast<size_t>(u)] != 0 ||
                cut_nodes_[static_cast<size_t>(v)] != 0) {
                return false;
            }
        }
        const int ux = u / size_;
        const int uy = u % size_;
        const int vx = v / size_;
        const int vy = v % size_;
        int dx = std::abs(ux - vx);
        int dy = std::abs(uy - vy);
        if (topo_ == TopologyMode::Torus2D) {
            dx = std::min(dx, size_ - dx);
            dy = std::min(dy, size_ - dy);
            return dx <= 1 && dy <= 1 && !(dx == 0 && dy == 0);
        }
        if (topo_ == TopologyMode::Finite2D) {
            // Finite bounds: only edges between in-bounds coords exist.
            if (ux < 0 || ux >= size_ || uy < 0 || uy >= size_ ||
                vx < 0 || vx >= size_ || vy < 0 || vy >= size_) {
                return false;
            }
            return dx <= 1 && dy <= 1 && !(dx == 0 && dy == 0);
        }
        // Cylinder2D: x wraps, y finite. In our square grid: x = row (ux),
        // wraps; y = col (uy) finite.
        dx = std::min(dx, size_ - dx);
        if (uy < 0 || uy >= size_ || vy < 0 || vy >= size_) {
            return false;
        }
        return dx <= 1 && dy <= 1 && !(dx == 0 && dy == 0);
    };

    auto edge_key = [](int a, int b) -> std::uint64_t {
        const std::uint32_t lo = static_cast<std::uint32_t>(std::min(a, b));
        const std::uint32_t hi = static_cast<std::uint32_t>(std::max(a, b));
        return (static_cast<std::uint64_t>(hi) << 32) | lo;
    };

    std::unordered_set<std::uint64_t> chosen;
    // Reserve conservatively to avoid large single allocations in WASM.
    const size_t want_reserve = static_cast<size_t>(k) * 2u + 8u;
    const size_t max_reserve = 1'000'000u;
    chosen.reserve(std::min(want_reserve, max_reserve));
    std::uniform_int_distribution<int> uni(0, n - 1);

    int added = 0;
    while (added < k) {
        int u = uni(gen);
        int v = uni(gen);
        if (u == v) {
            continue;
        }
        if (u > v) {
            std::swap(u, v);
        }
        if (is_base_edge(u, v)) {
            continue;
        }
        const std::uint64_t key = edge_key(u, v);
        if (chosen.insert(key).second) {
            wormholes_[static_cast<size_t>(u)].push_back(v);
            wormholes_[static_cast<size_t>(v)].push_back(u);
            added += 1;
        }
    }

    // De-duplicate wormholes (should already be clean, but keep it robust).
    for (auto& adj : wormholes_) {
        std::sort(adj.begin(), adj.end());
        adj.erase(std::unique(adj.begin(), adj.end()), adj.end());
    }

    int wormhole_edges = 0;
    for (const auto& adj : wormholes_) {
        wormhole_edges += static_cast<int>(adj.size());
    }
    wormhole_edges /= 2;
    wormhole_edge_count_ = wormhole_edges;
}

void GameOfLife::rebuild_cuts() {
    const int n = size_ * size_;
    if (static_cast<int>(cut_nodes_.size()) != n) {
        cut_nodes_.assign(static_cast<size_t>(n), 0);
    } else {
        std::fill(cut_nodes_.begin(), cut_nodes_.end(), 0);
    }
    cut_node_count_ = 0;

    std::uint32_t seed = cut_seed_;
    if (seed == 0) {
        seed = mix_seed(last_seed_ ^ 0x7f4a7c15u);
        if (seed == 0) {
            seed = 1;
        }
    }
    std::mt19937 gen(seed);

    int k = cut_target_count_;
    if (k < 0) {
        k = 0;
    }
    const int K_HARD_CAP = 1'000'000;
    if (k > K_HARD_CAP) {
        k = K_HARD_CAP;
    }

    if (k > n) {
        k = n;
    }
    std::uniform_int_distribution<int> uni(0, n - 1);

    int added = 0;
    while (added < k) {
        const int u = uni(gen);
        if (cut_nodes_[static_cast<size_t>(u)] != 0) {
            continue;
        }
        cut_nodes_[static_cast<size_t>(u)] = 1;
        added += 1;
    }
    cut_node_count_ = added;
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
    const int u = flat_index(i, j);
    if (!cut_nodes_.empty() && cut_nodes_[static_cast<size_t>(u)] != 0) {
        // Cut nodes have no base-neighborhood edges (wormholes still apply).
        neighbors.clear();
    }
    for (const auto& n : neighbors) {
        const int v = flat_index(n.x, n.y);
        if (!cut_nodes_.empty() && cut_nodes_[static_cast<size_t>(v)] != 0) {
            continue;
        }
        sum += grid_[static_cast<size_t>(n.x)][static_cast<size_t>(n.y)];
    }
    if (!wormholes_.empty() && u >= 0 && u < size_ * size_) {
        for (int v : wormholes_[static_cast<size_t>(u)]) {
            const int x = v / size_;
            const int y = v % size_;
            sum += grid_[static_cast<size_t>(x)][static_cast<size_t>(y)];
        }
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
    rebuild_cuts();
    rebuild_wormholes();
}

void GameOfLife::set_wormhole_seed(uint32_t seed) {
    wormhole_seed_ = seed;
    if (wormhole_seed_ == 0) {
        wormhole_seed_ = 1;
    }
    rebuild_wormholes();
}

void GameOfLife::set_wormhole_count(int count) {
    wormhole_target_count_ = std::max(0, count);
    rebuild_wormholes();
}

void GameOfLife::set_cut_seed(uint32_t seed) {
    cut_seed_ = seed;
    if (cut_seed_ == 0) {
        cut_seed_ = 1;
    }
    rebuild_cuts();
    rebuild_wormholes();
}

void GameOfLife::set_cut_count(int count) {
    cut_target_count_ = std::max(0, count);
    rebuild_cuts();
    rebuild_wormholes();
}

TopologyMode GameOfLife::topology() const {
    return topo_;
}

int GameOfLife::debug_wormhole_edge_count() const {
    return wormhole_edge_count_;
}

int GameOfLife::debug_cut_node_count() const {
    return cut_node_count_;
}
