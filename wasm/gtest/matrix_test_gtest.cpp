#include <gtest/gtest.h>

#include "general_linear_group.h"
#include "matrix.h"
#include "modular_number.h"

#include <vector>

static Matrix<ModularNumber> identity_mod(size_t n, long long m) {
    ModularNumber zero(0, m);
    ModularNumber one(1, m);
    Matrix<ModularNumber> I(n, n, zero);
    for (size_t i = 0; i < n; i++) {
        I(i, i) = one;
    }
    return I;
}

TEST(MatrixInverseTest, Cofactor2x2) {
    ModularNumber a(3, 7), b(1, 7), c(5, 7), d(2, 7);
    Matrix<ModularNumber> M({{a, b}, {c, d}});
    EXPECT_EQ(M.cofactor(0, 0), d);
    EXPECT_EQ(M.cofactor(0, 1), ModularNumber(0, 7) - c);
    EXPECT_EQ(M.cofactor(1, 0), ModularNumber(0, 7) - b);
    EXPECT_EQ(M.cofactor(1, 1), a);
}

TEST(MatrixInverseTest, Adjugate2x2) {
    ModularNumber a(3, 7), b(1, 7), c(5, 7), d(2, 7);
    Matrix<ModularNumber> M({{a, b}, {c, d}});
    Matrix<ModularNumber> adj = M.adjugate();
    EXPECT_EQ(adj(0, 0), d);
    EXPECT_EQ(adj(0, 1), ModularNumber(0, 7) - b);
    EXPECT_EQ(adj(1, 0), ModularNumber(0, 7) - c);
    EXPECT_EQ(adj(1, 1), a);
}

TEST(MatrixInverseTest, AdjugateIdentity) {
    Matrix<ModularNumber> M = identity_mod(2, 5);
    Matrix<ModularNumber> adj = M.adjugate();
    EXPECT_EQ(adj, M);
}

TEST(MatrixInverseTest, Inverse2x2PrimeMod) {
    ModularNumber a(3, 5), b(1, 5), c(4, 5), d(2, 5);
    Matrix<ModularNumber> M({{a, b}, {c, d}});

    // det = 3*2 - 1*4 = 2 mod 5, which is invertible
    Matrix<ModularNumber> inv = M.inverse();
    Matrix<ModularNumber> I = identity_mod(2, 5);
    EXPECT_EQ(M * inv, I);
    EXPECT_EQ(inv * M, I);
}

TEST(MatrixInverseTest, Inverse3x3PrimeMod) {
    long long p = 7;
    Matrix<ModularNumber> M({
        {ModularNumber(1, p), ModularNumber(2, p), ModularNumber(3, p)},
        {ModularNumber(0, p), ModularNumber(1, p), ModularNumber(4, p)},
        {ModularNumber(5, p), ModularNumber(6, p), ModularNumber(0, p)},
    });

    Matrix<ModularNumber> inv = M.inverse();
    Matrix<ModularNumber> I = identity_mod(3, p);
    EXPECT_EQ(M * inv, I);
    EXPECT_EQ(inv * M, I);
}

TEST(MatrixInverseTest, AllGL2Z5HaveInverse) {
    long long p = 5;
    auto gl = get_gl_n_zm(2, p);
    Matrix<ModularNumber> I = identity_mod(2, p);
    for (const auto& M : gl) {
        Matrix<ModularNumber> inv = M.inverse();
        EXPECT_EQ(M * inv, I);
    }
}

TEST(MatrixInverseTest, IntegerDet) {
    Matrix<int> M({{1, 2, 3}, {0, 1, 4}, {5, 6, 0}});
    int det = M.determinant();
    EXPECT_EQ(det, 1);

    // det = 1, so integer inverse is exact
    Matrix<int> inv = M.inverse();
    Matrix<int> I({{1, 0, 0}, {0, 1, 0}, {0, 0, 1}});
    EXPECT_EQ(M * inv, I);
}

TEST(MatrixInverseTest, IntegerNonUnimodular) {
    // det = 3*2 - 1*4 = 2 -- integer division will truncate
    Matrix<int> M({{3, 1}, {4, 2}});
    EXPECT_EQ(M.determinant(), 2);
    // inverse() won't throw, but M * inv != I due to truncation
    Matrix<int> inv = M.inverse();
    Matrix<int> I({{1, 0}, {0, 1}});
    EXPECT_NE(M * inv, I);
}
