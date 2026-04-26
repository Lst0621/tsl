#include "bars_game.h"

#include <algorithm>
#include <random>

namespace {

int uniform_int(std::mt19937& gen, int lo, int hi) {
    if (hi <= lo) {
        return lo;
    }
    std::uniform_int_distribution<int> dist(lo, hi);
    return dist(gen);
}

}  // namespace

int BarsGame::clamp(int val, int lo, int hi) {
    return std::max(lo, std::min(hi, val));
}

BarsGame::BarsGame(const BarsGameConfig& config)
    : config_(config), ended_(false), rng_seed_(1), gen_(1) {
    state_.resize(static_cast<size_t>(config_.state_size), 0);
    future_states_.resize(static_cast<size_t>(config_.num_choices));
    for (int i = 0; i < config_.num_choices; ++i) {
        future_states_[static_cast<size_t>(i)].resize(
            static_cast<size_t>(config_.state_size), 0);
    }
}

void BarsGame::set_seed(uint32_t seed) {
    rng_seed_ = seed;
}

void BarsGame::init() {
    ended_ = false;
    gen_.seed(rng_seed_);
    const int c = config_.init_center;
    const int v = config_.init_variance;
    const int lo = config_.min_val;
    const int hi = config_.max_val;
    for (size_t i = 0; i < state_.size(); ++i) {
        state_[i] = clamp(c + uniform_int(gen_, -v, v), lo, hi);
    }
    generate_future_states();
}

void BarsGame::generate_future_states() {
    const int v = config_.move_variance;
    const int lo = config_.min_val;
    const int hi = config_.max_val;
    for (int c = 0; c < config_.num_choices; ++c) {
        auto& fut = future_states_[static_cast<size_t>(c)];
        for (size_t i = 0; i < state_.size(); ++i) {
            int delta = uniform_int(gen_, -v, v);
            fut[i] = clamp(state_[i] + delta, lo, hi);
        }
    }
}

void BarsGame::get_state(int* out) const {
    for (size_t i = 0; i < state_.size(); ++i) {
        out[i] = state_[i];
    }
}

void BarsGame::get_future_state(int choice_index, int* out) const {
    if (choice_index < 0 || choice_index >= config_.num_choices) {
        return;
    }
    const auto& fut = future_states_[static_cast<size_t>(choice_index)];
    for (size_t i = 0; i < fut.size(); ++i) {
        out[i] = fut[i];
    }
}

void BarsGame::apply_choice(int index) {
    if (ended_ || index < 0 || index >= config_.num_choices) {
        return;
    }
    const auto& fut = future_states_[static_cast<size_t>(index)];
    const int lo = config_.min_val;
    const int hi = config_.max_val;
    for (size_t i = 0; i < state_.size(); ++i) {
        state_[i] = fut[i];
        if (state_[i] == lo || state_[i] == hi) {
            ended_ = true;
        }
    }
    if (!ended_) {
        generate_future_states();
    }
}

bool BarsGame::is_ended() const {
    return ended_;
}
