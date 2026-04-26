#pragma once

#include <cmath>
#include <functional>
#include <vector>

// Custom vector hash function for unordered containers
template <typename T>
struct vector_hash {
    size_t operator()(const std::vector<T>& v) const {
        // Golden ratio constant: 2^32 / φ where φ = (1 + sqrt(5)) / 2
        const double phi = (1.0 + std::sqrt(5.0)) / 2.0;
        const size_t golden_ratio =
            static_cast<size_t>(std::pow(2.0, 32.0) / phi);

        size_t hash = 0;
        for (const auto& element : v) {
            hash ^= std::hash<T>()(element) + golden_ratio + (hash << 6) +
                    (hash >> 2);
        }
        return hash;
    }
};
