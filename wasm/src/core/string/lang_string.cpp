#include "lang_string.h"

#include <algorithm>
#include <set>

namespace lang {

std::string get_prefix(const std::string& str, size_t n) {
    if (n >= str.size()) {
        return str;
    }
    return str.substr(0, n);
}

std::string get_suffix(const std::string& str, size_t n) {
    if (n >= str.size()) {
        return str;
    }
    return str.substr(str.size() - n, n);
}

std::vector<std::string> get_all_prefixes(const std::string& str) {
    std::vector<std::string> out;
    out.reserve(str.size() + 1);
    for (size_t i = 0; i <= str.size(); i++) {
        out.push_back(get_prefix(str, i));
    }
    return out;
}

std::string concat_and_get_suffix_k(const std::string& str1,
                                    const std::string& str2, size_t k) {
    std::string concat_str = str1 + str2;
    return get_suffix(concat_str, k);
}

std::vector<std::string> get_all_chars(const std::string& s) {
    std::set<std::string> seen;
    for (unsigned char c : s) {
        seen.insert(std::string(1, static_cast<char>(c)));
    }
    return std::vector<std::string>(seen.begin(), seen.end());
}

std::vector<std::string> get_alphabet_from_strings(
    const std::vector<std::string>& strs) {
    std::set<std::string> out;
    for (const std::string& s : strs) {
        std::vector<std::string> chars = get_all_chars(s);
        for (const std::string& c : chars) {
            out.insert(c);
        }
    }
    return std::vector<std::string>(out.begin(), out.end());
}

std::vector<std::string> concat_string_lists(
    const std::vector<std::string>& s1, const std::vector<std::string>& s2) {
    if (s1.empty() || s2.empty()) {
        return {};
    }
    std::vector<std::vector<std::string>> input = {s1, s2};
    std::vector<std::vector<std::string>> pairs = cartesian_product(input);
    std::set<std::string> unique;
    for (const std::vector<std::string>& row : pairs) {
        unique.insert(row[0] + row[1]);
    }
    std::vector<std::string> out(unique.begin(), unique.end());
    std::sort(out.begin(), out.end(),
              [](const std::string& a, const std::string& b) {
                  if (a.size() != b.size()) {
                      return a.size() < b.size();
                  }
                  return a < b;
              });
    return out;
}

std::vector<std::string> concat_same_string_lists_n_times(
    const std::vector<std::string>& s1, size_t n) {
    if (n == 0) {
        return {};
    }
    std::vector<std::string> result = s1;
    for (size_t i = 1; i < n; i++) {
        result = concat_string_lists(result, s1);
    }
    return result;
}

std::vector<std::string> concat_same_string_lists_leq_n_times(
    const std::vector<std::string>& s1, size_t n) {
    std::vector<std::string> s_with_empty = s1;
    s_with_empty.push_back("");
    return concat_same_string_lists_n_times(s_with_empty, n);
}

std::vector<std::string> cat_subseq_leq_q(const std::vector<std::string>& s1,
                                          const std::vector<std::string>& s2,
                                          int k) {
    std::vector<std::string> combined = concat_string_lists(s1, s2);
    if (k == NO_LENGTH_LIMIT) {
        return combined;
    }
    std::vector<std::string> out;
    for (const std::string& s : combined) {
        if (static_cast<int>(s.size()) <= k) {
            out.push_back(s);
        }
    }
    return out;
}

bool is_sub_seq(const std::string& str, const std::string& pattern) {
    if (str.size() < pattern.size()) {
        return false;
    }
    if (pattern.empty()) {
        return true;
    }
    if (str[0] == pattern[0]) {
        return is_sub_seq(str.substr(1), pattern.substr(1));
    }
    return is_sub_seq(str.substr(1), pattern);
}

std::vector<std::string> cat_subseq_of_blocklist(
    const std::vector<std::string>& s1, const std::vector<std::string>& s2,
    int k, const std::vector<std::string>& blocks) {
    std::vector<std::string> combined = cat_subseq_leq_q(s1, s2, k);
    std::vector<std::string> out;
    for (const std::string& s : combined) {
        for (const std::string& block : blocks) {
            if (is_sub_seq(block, s)) {
                out.push_back(s);
                break;
            }
        }
    }
    return out;
}

std::vector<std::string> get_all_subseq_leq_k(const std::string& str, int k) {
    if (str.empty()) {
        return {""};
    }
    if (str.size() == 1) {
        if (k == NO_LENGTH_LIMIT || k >= 1) {
            return {str, ""};
        }
        return {""};
    }
    size_t half = str.size() / 2;
    std::vector<std::string> first_half =
        get_all_subseq_leq_k(str.substr(0, half), k);
    std::vector<std::string> second_half =
        get_all_subseq_leq_k(str.substr(half), k);
    return cat_subseq_leq_q(first_half, second_half, k);
}

std::vector<std::string> get_subseq_of_blocklist(
    const std::string& str, const std::vector<std::string>& blocks) {
    if (blocks.empty()) {
        return {};
    }
    size_t k = 0;
    for (const std::string& b : blocks) {
        if (b.size() > k) {
            k = b.size();
        }
    }
    int k_int = static_cast<int>(k);
    if (str.size() <= 1) {
        std::vector<std::string> all = get_all_subseq_leq_k(str);
        std::vector<std::string> out;
        for (const std::string& s : all) {
            for (const std::string& block : blocks) {
                if (is_sub_seq(block, s)) {
                    out.push_back(s);
                    break;
                }
            }
        }
        return out;
    }
    size_t half = str.size() / 2;
    std::vector<std::string> first_half =
        get_subseq_of_blocklist(str.substr(0, half), blocks);
    std::vector<std::string> second_half =
        get_subseq_of_blocklist(str.substr(half), blocks);
    return cat_subseq_of_blocklist(first_half, second_half, k_int, blocks);
}

SubseqElement::SubseqElement(std::vector<std::string> value_,
                             std::vector<std::string> blocks_, size_t k_)
    : value(std::move(value_)), blocks(std::move(blocks_)), k(k_) {
}

SubseqElement SubseqElement::operator*(const SubseqElement& other) const {
    std::vector<std::string> product = cat_subseq_of_blocklist(
        value, other.value, static_cast<int>(k), blocks);
    return SubseqElement(std::move(product), blocks, k);
}

bool SubseqElement::operator==(const SubseqElement& other) const {
    return value == other.value;
}

std::vector<std::vector<std::string>> get_all_subseq_for_blocks(
    const std::vector<std::string>& blocks) {
    if (blocks.empty()) {
        return {};
    }
    std::vector<std::string> alphabet = get_alphabet_from_strings(blocks);
    size_t k = 0;
    for (const std::string& b : blocks) {
        if (b.size() > k) {
            k = b.size();
        }
    }
    std::vector<SubseqElement> generators;
    for (const std::string& x : alphabet) {
        generators.push_back(SubseqElement({x, ""}, blocks, k));
    }
    generators.push_back(SubseqElement({""}, blocks, k));
    std::vector<SubseqElement> all_subs = generate_semigroup(generators);
    std::vector<std::vector<std::string>> out;
    out.reserve(all_subs.size());
    for (const SubseqElement& e : all_subs) {
        out.push_back(e.value);
    }
    return out;
}

std::vector<std::string> subseq_remove_short(
    const std::vector<std::string>& subs) {
    std::vector<std::string> out;
    for (const std::string& x : subs) {
        bool removed = false;
        for (const std::string& y : subs) {
            if (y.size() > x.size() && is_sub_seq(y, x)) {
                removed = true;
                break;
            }
        }
        if (!removed) {
            out.push_back(x);
        }
    }
    return out;
}

}  // namespace lang
