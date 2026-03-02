#pragma once
#include <unordered_map>
#include <vector>

#include "helper.h"
int number_of_sequences(const std::vector<int>& arr,
                        const std::vector<int>& sums);

std::unordered_map<std::vector<int>, int, vector_hash<int>>
number_of_sequences_all(const std::vector<int>& arr,
                        const std::vector<int>& sums);
