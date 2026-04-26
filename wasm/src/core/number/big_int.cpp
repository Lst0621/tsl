#include "big_int.h"

#include <algorithm>
#include <cctype>
#include <limits>
#include <stdexcept>

BigInt::BigInt() : sign_(0), limbs_() {
}

BigInt::BigInt(std::int64_t v) : sign_(0), limbs_() {
    if (v == 0) {
        return;
    }
    std::uint64_t mag;
    if (v < 0) {
        sign_ = -1;
        // handle INT64_MIN safely
        mag = static_cast<std::uint64_t>(-(v + 1)) + 1u;
    } else {
        sign_ = 1;
        mag = static_cast<std::uint64_t>(v);
    }
    while (mag != 0) {
        limbs_.push_back(static_cast<std::uint32_t>(mag % LIMB_BASE));
        mag /= LIMB_BASE;
    }
    normalize();
}

bool BigInt::is_zero() const {
    return sign_ == 0;
}

int BigInt::signum() const {
    return sign_;
}

void BigInt::normalize() {
    while (!limbs_.empty() && limbs_.back() == 0u) {
        limbs_.pop_back();
    }
    if (limbs_.empty()) {
        sign_ = 0;
    } else {
        if (sign_ == 0) {
            sign_ = 1;
        }
    }
}

int BigInt::cmp_abs(const BigInt& a, const BigInt& b) {
    if (a.limbs_.size() != b.limbs_.size()) {
        return (a.limbs_.size() < b.limbs_.size()) ? -1 : 1;
    }
    for (size_t i = a.limbs_.size(); i-- > 0;) {
        if (a.limbs_[i] != b.limbs_[i]) {
            return (a.limbs_[i] < b.limbs_[i]) ? -1 : 1;
        }
    }
    return 0;
}

int BigInt::cmp(const BigInt& other) const {
    if (sign_ != other.sign_) {
        return (sign_ < other.sign_) ? -1 : 1;
    }
    if (sign_ == 0) {
        return 0;
    }
    const int c = cmp_abs(*this, other);
    return sign_ > 0 ? c : -c;
}

BigInt BigInt::operator-() const {
    BigInt out = *this;
    out.sign_ = -out.sign_;
    return out;
}

BigInt::operator double() const {
    if (sign_ == 0) {
        return 0.0;
    }
    // Accumulate the top up-to-3 limbs (enough to saturate a double's 53-bit
    // mantissa since each limb holds up to ~30 bits), then scale by LIMB_BASE
    // for each skipped low limb.
    const double base = static_cast<double>(LIMB_BASE);
    const std::size_t n = limbs_.size();
    const std::size_t top = (n >= 3) ? 3 : n;
    const std::size_t start = n - top;

    double acc = 0.0;
    for (std::size_t i = n; i-- > start;) {
        acc = acc * base + static_cast<double>(limbs_[i]);
    }
    for (std::size_t i = 0; i < start; i++) {
        acc *= base;
    }
    return sign_ < 0 ? -acc : acc;
}

BigInt BigInt::add_abs(const BigInt& a, const BigInt& b) {
    BigInt out;
    out.sign_ = 1;
    const size_t n = std::max(a.limbs_.size(), b.limbs_.size());
    out.limbs_.assign(n, 0u);
    std::uint64_t carry = 0;
    for (size_t i = 0; i < n; i++) {
        const std::uint64_t av = (i < a.limbs_.size()) ? a.limbs_[i] : 0u;
        const std::uint64_t bv = (i < b.limbs_.size()) ? b.limbs_[i] : 0u;
        const std::uint64_t sum = av + bv + carry;
        out.limbs_[i] = static_cast<std::uint32_t>(sum % LIMB_BASE);
        carry = sum / LIMB_BASE;
    }
    if (carry != 0) {
        out.limbs_.push_back(static_cast<std::uint32_t>(carry));
    }
    out.normalize();
    return out;
}

BigInt BigInt::sub_abs(const BigInt& a, const BigInt& b) {
    // assumes |a| >= |b|
    BigInt out;
    out.sign_ = 1;
    out.limbs_.assign(a.limbs_.size(), 0u);
    std::int64_t borrow = 0;
    for (size_t i = 0; i < a.limbs_.size(); i++) {
        std::int64_t av = static_cast<std::int64_t>(a.limbs_[i]);
        std::int64_t bv = (i < b.limbs_.size()) ? static_cast<std::int64_t>(b.limbs_[i]) : 0;
        std::int64_t diff = av - bv - borrow;
        if (diff < 0) {
            diff += static_cast<std::int64_t>(LIMB_BASE);
            borrow = 1;
        } else {
            borrow = 0;
        }
        out.limbs_[i] = static_cast<std::uint32_t>(diff);
    }
    out.normalize();
    return out;
}

BigInt BigInt::add(const BigInt& other) const {
    if (sign_ == 0) {
        return other;
    }
    if (other.sign_ == 0) {
        return *this;
    }
    if (sign_ == other.sign_) {
        BigInt out = add_abs(*this, other);
        out.sign_ = sign_;
        out.normalize();
        return out;
    }
    const int c = cmp_abs(*this, other);
    if (c == 0) {
        return BigInt();
    }
    if (c > 0) {
        BigInt out = sub_abs(*this, other);
        out.sign_ = sign_;
        out.normalize();
        return out;
    }
    BigInt out = sub_abs(other, *this);
    out.sign_ = other.sign_;
    out.normalize();
    return out;
}

BigInt BigInt::sub(const BigInt& other) const {
    return add(-other);
}

BigInt BigInt::mul_abs(const BigInt& a, const BigInt& b) {
    BigInt out;
    if (a.sign_ == 0 || b.sign_ == 0) {
        return out;
    }
    out.sign_ = 1;
    const size_t n = a.limbs_.size();
    const size_t m = b.limbs_.size();
    out.limbs_.assign(n + m, 0u);
    for (size_t i = 0; i < n; i++) {
        std::uint64_t carry = 0;
        for (size_t j = 0; j < m; j++) {
            const std::uint64_t cur = out.limbs_[i + j];
            const std::uint64_t prod = static_cast<std::uint64_t>(a.limbs_[i]) * static_cast<std::uint64_t>(b.limbs_[j]);
            const std::uint64_t sum = cur + prod + carry;
            out.limbs_[i + j] = static_cast<std::uint32_t>(sum % LIMB_BASE);
            carry = sum / LIMB_BASE;
        }
        size_t k = i + m;
        while (carry != 0) {
            if (k >= out.limbs_.size()) {
                out.limbs_.push_back(0u);
            }
            const std::uint64_t sum = static_cast<std::uint64_t>(out.limbs_[k]) + carry;
            out.limbs_[k] = static_cast<std::uint32_t>(sum % LIMB_BASE);
            carry = sum / LIMB_BASE;
            k++;
        }
    }
    out.normalize();
    return out;
}

BigInt BigInt::mul(const BigInt& other) const {
    BigInt out = mul_abs(*this, other);
    if (out.sign_ == 0) {
        return out;
    }
    out.sign_ = sign_ * other.sign_;
    out.normalize();
    return out;
}

void BigInt::mul_small(std::uint32_t m) {
    if (sign_ == 0) {
        return;
    }
    if (m == 0u) {
        sign_ = 0;
        limbs_.clear();
        return;
    }
    std::uint64_t carry = 0;
    for (size_t i = 0; i < limbs_.size(); i++) {
        const std::uint64_t prod = static_cast<std::uint64_t>(limbs_[i]) * m + carry;
        limbs_[i] = static_cast<std::uint32_t>(prod % LIMB_BASE);
        carry = prod / LIMB_BASE;
    }
    while (carry != 0) {
        limbs_.push_back(static_cast<std::uint32_t>(carry % LIMB_BASE));
        carry /= LIMB_BASE;
    }
    normalize();
}

void BigInt::add_small(std::uint32_t a) {
    if (a == 0u) {
        return;
    }
    if (sign_ == 0) {
        sign_ = 1;
        limbs_.push_back(a % LIMB_BASE);
        std::uint32_t carry = a / LIMB_BASE;
        if (carry != 0u) {
            limbs_.push_back(carry);
        }
        normalize();
        return;
    }
    if (sign_ < 0) {
        // For parsing we only use on non-negative numbers.
        throw std::logic_error("add_small on negative BigInt not supported");
    }
    std::uint64_t carry = a;
    size_t i = 0;
    while (carry != 0) {
        if (i >= limbs_.size()) {
            limbs_.push_back(0u);
        }
        const std::uint64_t sum = static_cast<std::uint64_t>(limbs_[i]) + carry;
        limbs_[i] = static_cast<std::uint32_t>(sum % LIMB_BASE);
        carry = sum / LIMB_BASE;
        i++;
    }
    normalize();
}

std::pair<BigInt, std::uint32_t> BigInt::divmod_abs_small(const BigInt& a, std::uint32_t d) {
    if (d == 0u) {
        throw std::invalid_argument("division by zero");
    }
    BigInt q;
    if (a.sign_ == 0) {
        return {q, 0u};
    }
    q.sign_ = 1;
    q.limbs_.assign(a.limbs_.size(), 0u);
    std::uint64_t rem = 0;
    for (size_t i = a.limbs_.size(); i-- > 0;) {
        const std::uint64_t cur = a.limbs_[i] + rem * LIMB_BASE;
        const std::uint64_t digit = cur / d;
        rem = cur % d;
        q.limbs_[i] = static_cast<std::uint32_t>(digit);
    }
    q.normalize();
    return {q, static_cast<std::uint32_t>(rem)};
}

BigInt BigInt::abs_copy(const BigInt& x) {
    BigInt out = x;
    if (out.sign_ < 0) {
        out.sign_ = 1;
    }
    out.normalize();
    return out;
}

std::pair<BigInt, BigInt> BigInt::divmod_abs(const BigInt& a, const BigInt& b) {
    // a,b are non-negative, b != 0.
    if (b.sign_ == 0) {
        throw std::invalid_argument("division by zero");
    }
    if (a.sign_ == 0) {
        return {BigInt(), BigInt()};
    }
    if (cmp_abs(a, b) < 0) {
        return {BigInt(), a};
    }

    // Fast path: small divisor (one limb).
    if (b.limbs_.size() == 1) {
        const auto [q, r] = divmod_abs_small(a, b.limbs_[0]);
        BigInt rr(static_cast<std::int64_t>(r));
        return {q, rr};
    }

    // Knuth long division in base LIMB_BASE.
    const std::uint32_t f = LIMB_BASE / (b.limbs_.back() + 1u);
    BigInt a_norm = a;
    BigInt b_norm = b;
    a_norm.mul_small(f);
    b_norm.mul_small(f);

    const size_t n = b_norm.limbs_.size();
    const size_t m = a_norm.limbs_.size() - n;

    // Ensure a_norm has one extra limb for convenience.
    a_norm.limbs_.push_back(0u);

    BigInt q;
    q.sign_ = 1;
    q.limbs_.assign(m + 1, 0u);

    for (size_t j = m + 1; j-- > 0;) {
        const std::uint64_t a2 = (j + n < a_norm.limbs_.size()) ? a_norm.limbs_[j + n] : 0u;
        const std::uint64_t a1 = a_norm.limbs_[j + n - 1];
        const std::uint64_t a0 = a_norm.limbs_[j + n - 2];

        const std::uint64_t v1 = b_norm.limbs_[n - 1];
        const std::uint64_t v0 = b_norm.limbs_[n - 2];

        std::uint64_t numerator = a2 * LIMB_BASE + a1;
        std::uint64_t qhat = numerator / v1;
        std::uint64_t rhat = numerator % v1;
        if (qhat >= LIMB_BASE) {
            qhat = LIMB_BASE - 1;
        }

        while (qhat * v0 > (rhat * LIMB_BASE + a0)) {
            qhat--;
            rhat += v1;
            if (rhat >= LIMB_BASE) {
                break;
            }
        }

        // Multiply and subtract qhat * b_norm shifted by j.
        std::int64_t borrow = 0;
        std::uint64_t carry = 0;
        for (size_t i = 0; i < n; i++) {
            const std::uint64_t p = qhat * static_cast<std::uint64_t>(b_norm.limbs_[i]) + carry;
            carry = p / LIMB_BASE;
            const std::int64_t sub = static_cast<std::int64_t>(a_norm.limbs_[j + i]) -
                                     static_cast<std::int64_t>(p % LIMB_BASE) -
                                     borrow;
            if (sub < 0) {
                a_norm.limbs_[j + i] = static_cast<std::uint32_t>(sub + static_cast<std::int64_t>(LIMB_BASE));
                borrow = 1;
            } else {
                a_norm.limbs_[j + i] = static_cast<std::uint32_t>(sub);
                borrow = 0;
            }
        }
        {
            const std::int64_t sub = static_cast<std::int64_t>(a_norm.limbs_[j + n]) -
                                     static_cast<std::int64_t>(carry) -
                                     borrow;
            if (sub < 0) {
                a_norm.limbs_[j + n] = static_cast<std::uint32_t>(sub + static_cast<std::int64_t>(LIMB_BASE));
                borrow = 1;
            } else {
                a_norm.limbs_[j + n] = static_cast<std::uint32_t>(sub);
                borrow = 0;
            }
        }

        if (borrow != 0) {
            // qhat was too large; decrement and add back divisor.
            qhat--;
            std::uint64_t carry2 = 0;
            for (size_t i = 0; i < n; i++) {
                const std::uint64_t sum = static_cast<std::uint64_t>(a_norm.limbs_[j + i]) +
                                          static_cast<std::uint64_t>(b_norm.limbs_[i]) +
                                          carry2;
                a_norm.limbs_[j + i] = static_cast<std::uint32_t>(sum % LIMB_BASE);
                carry2 = sum / LIMB_BASE;
            }
            a_norm.limbs_[j + n] = static_cast<std::uint32_t>(
                (static_cast<std::uint64_t>(a_norm.limbs_[j + n]) + carry2) % LIMB_BASE);
        }

        q.limbs_[j] = static_cast<std::uint32_t>(qhat);
    }

    q.normalize();

    // Remainder is a_norm / f (undo normalization factor).
    BigInt r;
    r.sign_ = 1;
    r.limbs_.assign(a_norm.limbs_.begin(), a_norm.limbs_.begin() + n);
    r.normalize();
    if (f != 1u) {
        const auto [rq, rr] = divmod_abs_small(r, f);
        (void)rr;
        r = rq;
        r.normalize();
    }

    return {q, r};
}

std::pair<BigInt, BigInt> BigInt::divmod(const BigInt& other) const {
    if (other.sign_ == 0) {
        throw std::invalid_argument("division by zero");
    }
    if (sign_ == 0) {
        return {BigInt(), BigInt()};
    }

    BigInt a_abs = abs_copy(*this);
    BigInt b_abs = abs_copy(other);
    const auto [q_abs, r_abs] = divmod_abs(a_abs, b_abs);

    BigInt q = q_abs;
    BigInt r = r_abs;

    const bool a_neg = sign_ < 0;
    const bool b_neg = other.sign_ < 0;
    const bool q_neg = (a_neg != b_neg);

    if (!a_neg) {
        q.sign_ = q.is_zero() ? 0 : (q_neg ? -1 : 1);
        r.sign_ = r.is_zero() ? 0 : 1;
        q.normalize();
        r.normalize();
        return {q, r};
    }

    // For negative dividend, adjust to floor division semantics.
    if (r.is_zero()) {
        q.sign_ = q.is_zero() ? 0 : (q_neg ? -1 : 1);
        r.sign_ = 0;
        q.normalize();
        r.normalize();
        return {q, r};
    }

    // q = -q_abs - 1 if signs differ, else q = q_abs + 1 (because a negative)
    BigInt one(1);
    if (q_neg) {
        q = q_abs.add(one);
        q.sign_ = -1;
    } else {
        q = q_abs.add(one);
        q.sign_ = 1;
    }
    q.normalize();

    // r = |b| - r_abs
    BigInt r2 = b_abs.sub(r_abs);
    r2.sign_ = r2.is_zero() ? 0 : 1;
    r2.normalize();
    return {q, r2};
}

BigInt BigInt::floordiv(const BigInt& other) const {
    return divmod(other).first;
}

BigInt BigInt::mod(const BigInt& other) const {
    return divmod(other).second;
}

int BigInt::char_to_digit(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return 10 + (c - 'a');
    }
    if (c >= 'A' && c <= 'F') {
        return 10 + (c - 'A');
    }
    return -1;
}

void BigInt::parse_prefix_base(const std::string& s_in, int base_in, std::string& out_digits, int& out_base, bool& out_neg) {
    std::string s = s_in;
    // trim whitespace
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start])) != 0) {
        start++;
    }
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1])) != 0) {
        end--;
    }
    s = s.substr(start, end - start);
    if (s.empty()) {
        throw std::invalid_argument("empty string");
    }

    out_neg = false;
    size_t idx = 0;
    if (s[idx] == '+' || s[idx] == '-') {
        out_neg = (s[idx] == '-');
        idx++;
    }
    if (idx >= s.size()) {
        throw std::invalid_argument("invalid integer string");
    }

    int base = base_in;
    if (base != 0 && base != 2 && base != 10 && base != 16) {
        throw std::invalid_argument("base must be 0,2,10,16");
    }
    if (base == 0) {
        base = 10;
        if (s.size() - idx >= 2 && s[idx] == '0' && (s[idx + 1] == 'x' || s[idx + 1] == 'X')) {
            base = 16;
            idx += 2;
        } else if (s.size() - idx >= 2 && s[idx] == '0' && (s[idx + 1] == 'b' || s[idx + 1] == 'B')) {
            base = 2;
            idx += 2;
        }
    }

    out_base = base;
    if (idx >= s.size()) {
        throw std::invalid_argument("invalid integer string");
    }
    out_digits = s.substr(idx);
    if (out_digits.empty()) {
        throw std::invalid_argument("invalid integer string");
    }
}

BigInt BigInt::from_string(const std::string& s, int base) {
    std::string digits;
    int b = 10;
    bool neg = false;
    parse_prefix_base(s, base, digits, b, neg);

    BigInt acc;
    acc.sign_ = 1;
    acc.limbs_.clear();

    for (char c : digits) {
        const int d = char_to_digit(c);
        if (d < 0 || d >= b) {
            throw std::invalid_argument("invalid digit for base");
        }
        acc.mul_small(static_cast<std::uint32_t>(b));
        acc.add_small(static_cast<std::uint32_t>(d));
    }
    acc.normalize();
    if (acc.is_zero()) {
        return acc;
    }
    acc.sign_ = neg ? -1 : 1;
    acc.normalize();
    return acc;
}

std::string BigInt::to_string(int base) const {
    if (base != 2 && base != 10 && base != 16) {
        throw std::invalid_argument("base must be 2,10,16");
    }
    if (sign_ == 0) {
        return "0";
    }
    BigInt mag = *this;
    mag.sign_ = 1;
    mag.normalize();
    std::string s;
    if (base == 10) {
        std::string out;
        out.reserve(mag.limbs_.size() * 9);
        const std::uint32_t ms = mag.limbs_.back();
        out += std::to_string(ms);
        for (size_t i = mag.limbs_.size() - 1; i-- > 0;) {
            std::uint32_t limb = mag.limbs_[i];
            std::string part = std::to_string(limb);
            if (part.size() < 9) {
                out.append(9 - part.size(), '0');
            }
            out += part;
        }
        s = std::move(out);
    } else {
        const std::uint32_t d = static_cast<std::uint32_t>(base);
        std::string out_rev;
        BigInt t = mag;
        while (!t.is_zero()) {
            const auto [q, r] = divmod_abs_small(t, d);
            const std::uint32_t rem = r;
            char ch;
            if (rem < 10) {
                ch = static_cast<char>('0' + rem);
            } else {
                ch = static_cast<char>('a' + (rem - 10));
            }
            out_rev.push_back(ch);
            t = q;
        }
        if (out_rev.empty()) {
            out_rev = "0";
        } else {
            std::reverse(out_rev.begin(), out_rev.end());
        }
        s = std::move(out_rev);
    }
    if (sign_ < 0) {
        return "-" + s;
    }
    return s;
}

BigInt BigInt::gcd(BigInt a, BigInt b) {
    a.sign_ = a.sign_ < 0 ? 1 : a.sign_;
    b.sign_ = b.sign_ < 0 ? 1 : b.sign_;
    a.normalize();
    b.normalize();
    while (!b.is_zero()) {
        BigInt r = a.mod(b);
        a = b;
        b = r;
    }
    return a;
}

BigInt BigInt::powmod(BigInt base, BigInt exp, const BigInt& mod) {
    if (mod.sign_ == 0) {
        throw std::invalid_argument("powmod: modulus is zero");
    }
    BigInt m = mod;
    if (m.sign_ < 0) {
        m.sign_ = 1;
        m.normalize();
    }
    if (exp.sign_ < 0) {
        throw std::invalid_argument("powmod: negative exponent");
    }
    base = base.mod(m);
    BigInt result(1);

    BigInt e = exp;
    while (!e.is_zero()) {
        const auto [q, r] = divmod_abs_small(e, 2u);
        if (r != 0u) {
            result = result.mul(base).mod(m);
        }
        base = base.mul(base).mod(m);
        e = q;
    }
    result.normalize();
    return result;
}

static std::pair<BigInt, BigInt> egcd(BigInt a, BigInt b) {
    // returns (g, x) where g=gcd(a,b) and x satisfies a*x + b*y = g for some y
    BigInt old_r = a;
    BigInt r = b;
    BigInt old_s(1);
    BigInt s(0);

    while (!r.is_zero()) {
        const auto [q, rr] = old_r.divmod(r);
        BigInt tmp_r = r;
        r = rr;
        old_r = tmp_r;

        BigInt tmp_s = s;
        s = old_s.sub(q.mul(s));
        old_s = tmp_s;
    }
    return {old_r, old_s};
}

BigInt BigInt::invmod(const BigInt& a, const BigInt& mod) {
    if (mod.sign_ == 0) {
        throw std::invalid_argument("invmod: modulus is zero");
    }
    BigInt m = mod;
    if (m.sign_ < 0) {
        m.sign_ = 1;
        m.normalize();
    }
    BigInt aa = a.mod(m);
    const auto [g, x] = egcd(aa, m);
    if (!(g.cmp(BigInt(1)) == 0)) {
        throw std::invalid_argument("invmod: not invertible");
    }
    BigInt inv = x.mod(m);
    inv.normalize();
    return inv;
}

