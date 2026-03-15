#pragma once

#include <cstdint>
#include <random>
#include <vector>

/**
 * Configurable parameters for the bars game.
 */
struct BarsGameConfig {
    int state_size = 4;
    int min_val = 0;
    int max_val = 2000;
    int init_center = 1236;   // e.g. 2000 / golden_ratio
    int init_variance = 400;
    int move_variance = 280;  // variance when generating each future state (larger = easier to hit 0/2000)
    int num_choices = 2;
};

/**
 * Bars game: state is an int array (e.g. size 4) in [min_val, max_val].
 * Each turn the game offers num_choices future states (each = current + delta + variance).
 * User picks one; new state = that future state (no extra randomness).
 * Game ends when any value hits min_val or max_val.
 */
class BarsGame {
   public:
    explicit BarsGame(const BarsGameConfig& config);

    void init();
    void set_seed(uint32_t seed);
    void get_state(int* out) const;
    void get_future_state(int choice_index, int* out) const;
    void apply_choice(int index);
    bool is_ended() const;

    int state_size() const { return config_.state_size; }
    int num_choices() const { return config_.num_choices; }
    int min_val() const { return config_.min_val; }
    int max_val() const { return config_.max_val; }

   private:
    static int clamp(int val, int lo, int hi);
    void generate_future_states();

    BarsGameConfig config_;
    std::vector<int> state_;
    std::vector<std::vector<int>> future_states_;
    bool ended_;
    uint32_t rng_seed_;
    std::mt19937 gen_;
};
