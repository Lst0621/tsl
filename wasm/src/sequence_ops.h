#pragma once

#include <iterator>
#include <numeric>
#include <stdexcept>
#include <vector>

/**
 * Compute the full Cauchy convolution of two finite sequences using
 * inner product + reverse.
 *
 * For sequences a[0..m-1] and b[0..n-1], the output c has length m+n-1 where:
 *
 *   c[k] = sum_{j=0}^{k} a[j] * b[k-j]
 *        = inner_product( a[lo..hi],
 *                         reverse( b[k-lo .. k-hi] ) )
 *
 * @tparam T  Element type. Must support T(0), +, *.
 * @param a   First sequence.
 * @param b   Second sequence.
 * @return    Convolution c of length a.size() + b.size() - 1.
 */
template <typename T>
std::vector<T> sequence_convolve(const std::vector<T>& a,
                                 const std::vector<T>& b) {
    if (a.empty() || b.empty()) {
        return {};
    }

    const size_t na = a.size();
    const size_t nb = b.size();
    const size_t nc = na + nb - 1;
    std::vector<T> c(nc, T(0));

    for (size_t k = 0; k < nc; ++k) {
        // Valid range of j in a: max(0, k-(nb-1)) .. min(k, na-1)
        const size_t j_lo = (k + 1 > nb) ? k + 1 - nb : 0;
        const size_t j_hi = (k < na - 1) ? k : na - 1;

        // a[j_lo..j_hi] forward,  b[k-j_lo..k-j_hi] (= b reversed over that slice)
        c[k] = std::inner_product(
            a.begin() + static_cast<std::ptrdiff_t>(j_lo),
            a.begin() + static_cast<std::ptrdiff_t>(j_hi + 1),
            std::make_reverse_iterator(
                b.begin() + static_cast<std::ptrdiff_t>(k - j_lo + 1)),
            T(0));
    }

    return c;
}
