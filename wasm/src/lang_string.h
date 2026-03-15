#pragma once

#include <string>
#include <vector>

#include "semigroup.h"
#include "set_utils.h"

namespace lang {

std::string get_prefix(const std::string& str, size_t n);
std::string get_suffix(const std::string& str, size_t n);
std::vector<std::string> get_all_prefixes(const std::string& str);
std::string concat_and_get_suffix_k(const std::string& str1,
                                    const std::string& str2, size_t k);
std::vector<std::string> get_all_chars(const std::string& s);
std::vector<std::string> get_alphabet_from_strings(
    const std::vector<std::string>& strs);
std::vector<std::string> concat_string_lists(
    const std::vector<std::string>& s1, const std::vector<std::string>& s2);
std::vector<std::string> concat_same_string_lists_n_times(
    const std::vector<std::string>& s1, size_t n);
std::vector<std::string> concat_same_string_lists_leq_n_times(
    const std::vector<std::string>& s1, size_t n);

constexpr int NO_LENGTH_LIMIT = -1;

std::vector<std::string> cat_subseq_leq_q(const std::vector<std::string>& s1,
                                          const std::vector<std::string>& s2,
                                          int k = NO_LENGTH_LIMIT);
bool is_sub_seq(const std::string& str, const std::string& pattern);
std::vector<std::string> cat_subseq_of_blocklist(
    const std::vector<std::string>& s1, const std::vector<std::string>& s2,
    int k, const std::vector<std::string>& blocks);
std::vector<std::string> get_all_subseq_leq_k(const std::string& str,
                                              int k = NO_LENGTH_LIMIT);
std::vector<std::string> get_subseq_of_blocklist(
    const std::string& str, const std::vector<std::string>& blocks);

struct SubseqElement {
    std::vector<std::string> value;
    std::vector<std::string> blocks;
    size_t k = 0;

    SubseqElement() = default;
    SubseqElement(std::vector<std::string> value_,
                  std::vector<std::string> blocks_, size_t k_);

    SubseqElement operator*(const SubseqElement& other) const;
    bool operator==(const SubseqElement& other) const;
};

std::vector<std::vector<std::string>> get_all_subseq_for_blocks(
    const std::vector<std::string>& blocks);
std::vector<std::string> subseq_remove_short(
    const std::vector<std::string>& subs);

}  // namespace lang
