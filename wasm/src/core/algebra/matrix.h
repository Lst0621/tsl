#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "util/small_vector.h"

/**
 * Helper to detect if T has a default constructor
 * Used for compile-time selection of zero initialization method
 */
template <typename T>
class has_default_constructor {
    template <typename U, typename = decltype(U())>
    static std::true_type test(int);

    template <typename U>
    static std::false_type test(...);

   public:
    static constexpr bool value = decltype(test<T>(0))::value;
};

/**
 * Helper to create a zero value for type T
 * Tries default constructor first, falls back to subtraction
 * Note: For subtraction fallback, pass any existing element
 */
template <typename T, bool HasDefaultConstructor>
struct ZeroValue;

// Specialization: T has default constructor
template <typename T>
struct ZeroValue<T, true> {
    static T create() {
        return T();
    }
};

// Specialization: T doesn't have default constructor, use subtraction
// This version requires a sample element
template <typename T>
struct ZeroValue<T, false> {
    static T create(const T& sample) {
        return T(0);
    }
};

/**
 * Generic matrix with contiguous row-major storage.
 * Elements live in SmallVector<T, InlineCap> (inline up to InlineCap entries,
 * then heap).
 *
 * @tparam T The element type (must support default construction and copying)
 * @tparam InlineCap Inline storage cap (in entries), default 16 (= 4x4).
 */
template <typename T, size_t InlineCap = 16>
class Matrix {
    SmallVector<T, InlineCap> flat_;
    size_t rows;
    size_t cols;
    std::optional<T> unknown_diag_scalar;

   private:
    bool is_unknown_scalar() const {
        return unknown_diag_scalar.has_value();
    }

    const T& unknown_scalar_value() const {
        if (!unknown_diag_scalar.has_value()) {
            throw std::logic_error(
                "Unknown-size scalar diagonal is not initialized");
        }
        return *unknown_diag_scalar;
    }

    Matrix<T, InlineCap> materialize_as_diag(size_t n) const {
        if (!is_unknown_scalar()) {
            return *this;
        }
        if (n == 0) {
            throw std::invalid_argument(
                "Cannot materialize unknown matrix with size 0");
        }
        Matrix<T, InlineCap> result(n, n);
        for (size_t i = 0; i < n; i++) {
            result(i, i) = unknown_scalar_value();
        }
        return result;
    }

   public:
    // Unknown-size diagonal scalar sentinel, default scalar is 0.
    Matrix() : rows(0), cols(0), unknown_diag_scalar(T()) {
    }

    // Unknown-size diagonal scalar sentinel with arbitrary scalar on diagonal.
    explicit Matrix(const T& scalar)
        : rows(0), cols(0), unknown_diag_scalar(scalar) {
    }

    /**
     * Constructor: creates an m x n matrix with smart zero initialization
     * @param m Number of rows
     * @param n Number of columns
     * Uses compile-time check: if T has default constructor, uses T();
     * otherwise uses element - element
     */
    explicit Matrix(size_t m, size_t n)
        : rows(m), cols(n), unknown_diag_scalar(std::nullopt) {
        if (m == 0 || n == 0) {
            throw std::invalid_argument("Matrix dimensions must be positive");
        }
        // Use compile-time check to determine how to create zero value
        if constexpr (has_default_constructor<T>::value) {
            T zero = ZeroValue<T, true>::create();
            flat_.assign(m * n, zero);
        } else {
            // This path requires at least one element to exist - would need to
            // be called differently For now, we throw an error if no default
            // constructor and no explicit value provided
            throw std::invalid_argument(
                "Cannot create matrix without explicit default value for types "
                "without default constructor");
        }
    }

    /**
     * Constructor: creates an m x n matrix with explicit default value
     * @param m Number of rows
     * @param n Number of columns
     * @param default_value Default value for all elements
     */
    explicit Matrix(size_t m, size_t n, const T& default_value)
        : rows(m), cols(n), unknown_diag_scalar(std::nullopt) {
        if (m == 0 || n == 0) {
            throw std::invalid_argument("Matrix dimensions must be positive");
        }
        flat_.assign(m * n, default_value);
    }

    /**
     * Constructor from 2D vector of the same type (simplified version)
     * @param source Source matrix data of type T
     */
    Matrix(const std::vector<std::vector<T>>& source)
        : rows(0), cols(0), unknown_diag_scalar(std::nullopt) {
        if (source.empty()) {
            throw std::invalid_argument("Matrix cannot be empty");
        }
        rows = source.size();
        cols = source[0].size();

        // Verify all rows have same size
        for (const auto& row : source) {
            if (row.size() != cols) {
                throw std::invalid_argument(
                    "All rows must have the same number of columns");
            }
        }

        flat_.reserve(rows * cols);
        for (const auto& row : source) {
            flat_.insert_back(row.begin(), row.end());
        }
    }

    /**
     * Constructor from 2D vector of any type U with optional transform function
     * @tparam U Source element type (defaults to T for same-type construction)
     * @param source Source matrix data
     * @param transform Function to convert U to T. Defaults to T's constructor
     * from U.
     */
    template <typename U>
    Matrix(
        const std::vector<std::vector<U>>& source,
        std::function<T(const U&)> transform = [](const U& u) { return T(u); })
        : rows(0), cols(0), unknown_diag_scalar(std::nullopt) {
        if (source.empty()) {
            throw std::invalid_argument("Matrix cannot be empty");
        }
        rows = source.size();
        cols = source[0].size();

        // Verify all rows have same size
        for (const auto& row : source) {
            if (row.size() != cols) {
                throw std::invalid_argument(
                    "All rows must have the same number of columns");
            }
        }

        flat_.reserve(rows * cols);
        for (size_t i = 0; i < rows; i++) {
            for (size_t j = 0; j < cols; j++) {
                flat_.push_back(transform(source[i][j]));
            }
        }
    }

    /**
     * Get number of rows
     */
    size_t get_rows() const {
        if (is_unknown_scalar()) {
            throw std::invalid_argument(
                "Unknown-size diagonal scalar matrix has unresolved rows");
        }
        return rows;
    }

    /**
     * Get number of columns
     */
    size_t get_cols() const {
        if (is_unknown_scalar()) {
            throw std::invalid_argument(
                "Unknown-size diagonal scalar matrix has unresolved cols");
        }
        return cols;
    }

    /**
     * Access element at (i, j) - const version
     */
    const T& at(size_t i, size_t j) const {
        if (is_unknown_scalar()) {
            throw std::invalid_argument(
                "Cannot index unknown-size diagonal scalar matrix");
        }
        if (i >= rows || j >= cols) {
            throw std::out_of_range("Matrix index out of bounds");
        }
        return flat_[i * cols + j];
    }

    /**
     * Access element at (i, j) - non-const version
     */
    T& at(size_t i, size_t j) {
        if (is_unknown_scalar()) {
            throw std::invalid_argument(
                "Cannot index unknown-size diagonal scalar matrix");
        }
        if (i >= rows || j >= cols) {
            throw std::out_of_range("Matrix index out of bounds");
        }
        return flat_[i * cols + j];
    }

    /**
     * Access element using operator (i, j)
     * Note: Uses at() internally for bounds checking
     */
    T& operator()(size_t i, size_t j) {
        return at(i, j);
    }

    /**
     * Access element using operator (i, j) - const version
     */
    const T& operator()(size_t i, size_t j) const {
        return at(i, j);
    }

    /**
     * Copy underlying entries as vector-of-rows (row-major).
     */
    std::vector<std::vector<T>> get_data() const {
        if (is_unknown_scalar()) {
            throw std::invalid_argument(
                "Unknown-size diagonal scalar matrix has no concrete data");
        }
        std::vector<std::vector<T>> out;
        out.reserve(rows);
        for (size_t i = 0; i < rows; i++) {
            out.emplace_back(flat_.begin() + static_cast<ptrdiff_t>(i * cols),
                             flat_.begin() +
                                 static_cast<ptrdiff_t>((i + 1) * cols));
        }
        return out;
    }

    /**
     * Check if matrix is square
     */
    bool is_square() const {
        if (is_unknown_scalar()) {
            return true;
        }
        return rows == cols;
    }

    /**
     * Transpose the matrix (returns new matrix)
     */
    Matrix<T, InlineCap> transpose() const {
        if (is_unknown_scalar()) {
            return *this;
        }
        Matrix<T, InlineCap> result(cols, rows);
        for (size_t i = 0; i < rows; i++) {
            for (size_t j = 0; j < cols; j++) {
                result(j, i) = flat_[i * cols + j];
            }
        }
        return result;
    }

    /**
     * String representation
     */
    std::string to_string() const {
        if (is_unknown_scalar()) {
            return "Matrix(?x?, diag=" +
                   std::to_string(unknown_scalar_value()) + ")";
        }
        std::string result = "Matrix(" + std::to_string(rows) + "x" +
                             std::to_string(cols) + ")[\n";
        for (size_t i = 0; i < rows; i++) {
            result += "  [";
            for (size_t j = 0; j < cols; j++) {
                if (j > 0) {
                    result += ", ";
                }
                result += std::to_string(flat_[i * cols + j]);
            }
            result += "]\n";
        }
        result += "]";
        return result;
    }

    /**
     * Matrix addition: A + B
     * Requires: both matrices have same dimensions
     */
    Matrix<T, InlineCap> operator+(const Matrix<T, InlineCap>& other) const {
        if (is_unknown_scalar() && other.is_unknown_scalar()) {
            return Matrix<T, InlineCap>(unknown_scalar_value() +
                                        other.unknown_scalar_value());
        }
        if (is_unknown_scalar()) {
            if (!other.is_square()) {
                throw std::invalid_argument(
                    "Cannot add unknown-size diagonal scalar matrix to "
                    "non-square matrix");
            }
            return this->materialize_as_diag(other.rows) + other;
        }
        if (other.is_unknown_scalar()) {
            if (!this->is_square()) {
                throw std::invalid_argument(
                    "Cannot add non-square matrix to unknown-size diagonal "
                    "scalar matrix");
            }
            return (*this) + other.materialize_as_diag(this->rows);
        }
        if (rows != other.rows || cols != other.cols) {
            throw std::invalid_argument(
                "Cannot add matrices with different dimensions");
        }
        Matrix<T, InlineCap> result(rows, cols);
        for (size_t i = 0; i < rows; i++) {
            for (size_t j = 0; j < cols; j++) {
                const size_t idx = i * cols + j;
                result(i, j) = flat_[idx] + other.flat_[idx];
            }
        }
        return result;
    }

    /**
     * Matrix subtraction: A - B
     * Requires: both matrices have same dimensions
     */
    Matrix<T, InlineCap> operator-(const Matrix<T, InlineCap>& other) const {
        if (is_unknown_scalar() && other.is_unknown_scalar()) {
            return Matrix<T, InlineCap>(unknown_scalar_value() -
                                        other.unknown_scalar_value());
        }
        if (is_unknown_scalar()) {
            if (!other.is_square()) {
                throw std::invalid_argument(
                    "Cannot subtract non-square matrix from unknown-size "
                    "diagonal scalar matrix");
            }
            return this->materialize_as_diag(other.rows) - other;
        }
        if (other.is_unknown_scalar()) {
            if (!this->is_square()) {
                throw std::invalid_argument(
                    "Cannot subtract unknown-size diagonal scalar matrix from "
                    "non-square matrix");
            }
            return (*this) - other.materialize_as_diag(this->rows);
        }
        if (rows != other.rows || cols != other.cols) {
            throw std::invalid_argument(
                "Cannot subtract matrices with different dimensions");
        }
        Matrix<T, InlineCap> result(rows, cols);
        for (size_t i = 0; i < rows; i++) {
            for (size_t j = 0; j < cols; j++) {
                const size_t idx = i * cols + j;
                result(i, j) = flat_[idx] - other.flat_[idx];
            }
        }
        return result;
    }

    /**
     * Matrix multiplication: A * B
     * Requires: cols of A == rows of B
     * Returns: (rows x other.cols) matrix
     */
    Matrix<T, InlineCap> operator*(const Matrix<T, InlineCap>& other) const {
        if (is_unknown_scalar() && other.is_unknown_scalar()) {
            return Matrix<T, InlineCap>(unknown_scalar_value() *
                                        other.unknown_scalar_value());
        }
        if (is_unknown_scalar()) {
            return this->materialize_as_diag(other.rows) * other;
        }
        if (other.is_unknown_scalar()) {
            return (*this) * other.materialize_as_diag(this->cols);
        }
        if (cols != other.rows) {
            throw std::invalid_argument(
                "Cannot multiply matrices: cols of A must equal rows of B");
        }
        Matrix<T, InlineCap> result(rows, other.cols);
        for (size_t i = 0; i < rows; i++) {
            for (size_t j = 0; j < other.cols; j++) {
                T sum = flat_[i * cols + 0] * other.flat_[0 * other.cols + j];
                for (size_t k = 1; k < cols; k++) {
                    sum = sum + (flat_[i * cols + k] *
                                 other.flat_[k * other.cols + j]);
                }
                result(i, j) = sum;
            }
        }
        return result;
    }

    /**
     * Scalar multiplication: A * scalar
     * Delegates to scalar * A so scalar-left path is the single implementation.
     */
    Matrix<T, InlineCap> operator*(const T& scalar) const {
        return scalar * (*this);
    }

    /**
     * Scalar multiplication (commutative): scalar * A
     * Friend function to allow scalar * matrix
     */
    friend Matrix<T, InlineCap> operator*(const T& scalar,
                                          const Matrix<T, InlineCap>& matrix) {
        if (matrix.is_unknown_scalar()) {
            return Matrix<T, InlineCap>(scalar * matrix.unknown_scalar_value());
        }
        Matrix<T, InlineCap> result(matrix.rows, matrix.cols);
        for (size_t i = 0; i < matrix.rows; i++) {
            for (size_t j = 0; j < matrix.cols; j++) {
                result(i, j) = scalar * matrix.flat_[i * matrix.cols + j];
            }
        }
        return result;
    }

    /**
     * Matrix division by scalar: A / scalar
     * Divides every element by the scalar
     */
    Matrix<T, InlineCap> operator/(const T& scalar) const {
        if (is_unknown_scalar()) {
            return Matrix<T, InlineCap>(unknown_scalar_value() / scalar);
        }
        Matrix<T, InlineCap> result(rows, cols);
        for (size_t i = 0; i < rows; i++) {
            for (size_t j = 0; j < cols; j++) {
                result(i, j) = flat_[i * cols + j] / scalar;
            }
        }
        return result;
    }

    /**
     * Equality operator
     * Sentinel-sentinel compares diagonal scalar value.
     * Sentinel-concrete is always false.
     */
    bool operator==(const Matrix<T, InlineCap>& other) const {
        if (is_unknown_scalar() && other.is_unknown_scalar()) {
            return unknown_scalar_value() == other.unknown_scalar_value();
        }
        if (is_unknown_scalar() || other.is_unknown_scalar()) {
            return false;
        }
        if (rows != other.rows || cols != other.cols) {
            return false;
        }
        for (size_t i = 0; i < rows; i++) {
            for (size_t j = 0; j < cols; j++) {
                const size_t idx = i * cols + j;
                if (!(flat_[idx] == other.flat_[idx])) {
                    return false;
                }
            }
        }
        return true;
    }

    /**
     * Inequality operator
     */
    bool operator!=(const Matrix<T, InlineCap>& other) const {
        return !(*this == other);
    }

    /**
     * Calculate the trace of the matrix (sum of diagonal elements)
     * Only valid for square matrices
     */
    T trace() const {
        if (!is_square()) {
            throw std::invalid_argument(
                "Trace is only defined for square matrices");
        }
        T result = flat_[0];
        for (size_t i = 1; i < rows; i++) {
            result = result + flat_[i * cols + i];
        }
        return result;
    }

    /**
     * Calculate the determinant using Laplace expansion (recursive)
     * Only valid for square matrices
     * Note: For large matrices, this is computationally expensive O(n!)
     */
    T determinant() const {
        if (!is_square()) {
            throw std::invalid_argument(
                "Determinant is only defined for square matrices");
        }

        std::vector<int> skip_rows(rows, 0);
        std::vector<int> skip_cols(cols, 0);
        return determinant_recursive(skip_rows, skip_cols);
    }

    /**
     * Cofactor C(i,j) = (-1)^(i+j) * det(minor(i,j))
     * Only needs +, -, * on T (ring operations). No division required.
     */
    T cofactor(size_t i, size_t j) const {
        if (!is_square()) {
            throw std::invalid_argument(
                "Cofactor is only defined for square matrices");
        }
        if (rows < 2) {
            throw std::invalid_argument(
                "Cofactor requires matrix of size >= 2");
        }
        if (i >= rows || j >= cols) {
            throw std::out_of_range("Cofactor index out of bounds");
        }
        std::vector<int> skip_rows(rows, 0);
        std::vector<int> skip_cols(cols, 0);
        skip_rows[i] = 1;
        skip_cols[j] = 1;
        T minor_det = determinant_recursive(skip_rows, skip_cols);
        if ((i + j) % 2 == 1) {
            return T() - minor_det;
        }
        return minor_det;
    }

    /**
     * Classical adjugate (adjoint): transpose of the cofactor matrix.
     * adj(A)_{ij} = C(j,i)
     * Only needs +, -, * on T (ring operations). No division required.
     *
     * Key property: A * adj(A) = det(A) * I  (holds over any commutative ring)
     */
    Matrix<T, InlineCap> adjugate() const {
        if (!is_square()) {
            throw std::invalid_argument(
                "Adjugate is only defined for square matrices");
        }
        if (rows < 2) {
            throw std::invalid_argument(
                "Adjugate requires matrix of size >= 2 "
                "(use inverse() which handles 1x1 directly)");
        }
        Matrix<T, InlineCap> result(rows, cols);
        for (size_t i = 0; i < rows; i++) {
            for (size_t j = 0; j < cols; j++) {
                result(i, j) = cofactor(j, i);
            }
        }
        return result;
    }

    /**
     * Matrix inverse via adjugate/determinant: A^{-1} = adj(A) / det(A)
     *
     * Requires T to support operator/ (i.e. T is a field or det is a unit).
     * For ModularNumber with prime modulus: always works when det != 0.
     * For integers: only correct when |det| = 1 (unimodular matrices).
     * For composite modulus: throws if gcd(det, m) > 1.
     *
     * @throws std::invalid_argument if matrix is not square or det is zero
     */
    Matrix<T, InlineCap> inverse() const {
        if (!is_square()) {
            throw std::invalid_argument(
                "Inverse is only defined for square matrices");
        }
        T det = determinant();
        if (rows == 1) {
            Matrix<T, InlineCap> result(1, 1);
            result(0, 0) = det / det;
            return result;
        }
        return adjugate() / det;
    }

   private:
    /**
     * Helper function to calculate determinant for a submatrix
     * Uses integer arrays to track which rows/columns are skipped
     * 0 = not skipped, 1 = skipped
     * This properly handles nested recursive calls
     *
     * @param skip_rows Integer vector marking which rows to skip (0 or 1)
     * @param skip_cols Integer vector marking which columns to skip (0 or 1)
     */
    T determinant_recursive(std::vector<int>& skip_rows,
                            std::vector<int>& skip_cols) const {
        // Count non-skipped rows and columns
        size_t active_size = 0;
        for (size_t i = 0; i < rows; i++) {
            if (skip_rows[i] == 0) {
                active_size++;
            }
        }

        // Base case: 1x1 submatrix
        if (active_size == 1) {
            for (size_t i = 0; i < rows; i++) {
                if (skip_rows[i] == 0) {
                    for (size_t j = 0; j < cols; j++) {
                        if (skip_cols[j] == 0) {
                            return flat_[i * cols + j];
                        }
                    }
                }
            }
        }

        // Base case: 2x2 submatrix
        if (active_size == 2) {
            std::vector<size_t> active_rows, active_cols;
            for (size_t i = 0; i < rows; i++) {
                if (skip_rows[i] == 0) {
                    active_rows.push_back(i);
                }
            }
            for (size_t j = 0; j < cols; j++) {
                if (skip_cols[j] == 0) {
                    active_cols.push_back(j);
                }
            }

            if (active_rows.size() == 2 && active_cols.size() == 2) {
                const size_t r0 = active_rows[0];
                const size_t r1 = active_rows[1];
                const size_t c0 = active_cols[0];
                const size_t c1 = active_cols[1];
                return flat_[r0 * cols + c0] * flat_[r1 * cols + c1] -
                       flat_[r0 * cols + c1] * flat_[r1 * cols + c0];
            }
        }

        // Recursive case: Laplace expansion along first active row
        // Find first active row
        size_t expand_row = 0;
        for (size_t i = 0; i < rows; i++) {
            if (skip_rows[i] == 0) {
                expand_row = i;
                break;
            }
        }

        // Expand along this row - find first active column
        size_t first_col = 0;
        for (size_t j = 0; j < cols; j++) {
            if (skip_cols[j] == 0) {
                first_col = j;
                break;
            }
        }

        // Initialize det with zero value using smart helper
        // Uses element - element since ModularNumber and other custom types may
        // not have default constructor
        // T det = ZeroValue<T, false>::create(data[expand_row][0]);
        T det = T();

        // Mark and compute first term
        skip_rows[expand_row] = 1;
        skip_cols[first_col] = 1;
        T cofactor = determinant_recursive(skip_rows, skip_cols);
        det = det + (flat_[expand_row * cols + first_col] * cofactor);
        skip_rows[expand_row] = 0;
        skip_cols[first_col] = 0;

        // Expand along remaining columns in this row
        for (size_t j = first_col + 1; j < cols; j++) {
            if (skip_cols[j] == 1) {
                continue;
            }

            // Mark this row and column as skipped
            skip_rows[expand_row] = 1;
            skip_cols[j] = 1;

            T cofactor_j = determinant_recursive(skip_rows, skip_cols);
            T term = flat_[expand_row * cols + j] * cofactor_j;

            // Unmark for next iteration
            skip_rows[expand_row] = 0;
            skip_cols[j] = 0;

            // Calculate sign based on position in submatrix
            size_t row_pos = 0, col_pos = 0;
            for (size_t ii = 0; ii < expand_row; ii++) {
                if (skip_rows[ii] == 0) {
                    row_pos++;
                }
            }
            for (size_t jj = 0; jj < j; jj++) {
                if (skip_cols[jj] == 0) {
                    col_pos++;
                }
            }

            if ((row_pos + col_pos) % 2 == 0) {
                det = det + term;
            } else {
                det = det - term;
            }
        }

        return det;
    }

   private:
};

/**
 * Convert a 1D container to Matrix
 * Requires both m and n with verification that size == m*n
 *
 * @tparam T Element type
 * @tparam ContainerT Container type (any type with .size() and [] access)
 * @param data 1D data container
 * @param m Number of rows
 * @param n Number of columns
 * @return Matrix with dimensions m x n
 */
template <typename T, typename ContainerT>
Matrix<T> to_matrix(const ContainerT& data, size_t m, size_t n) {
    if (data.size() != m * n) {
        throw std::invalid_argument("Container size must equal m * n");
    }

    Matrix<T> result(m, n);
    for (size_t i = 0; i < m; i++) {
        for (size_t j = 0; j < n; j++) {
            result(i, j) = data[i * n + j];
        }
    }
    return result;
}

/**
 * Convert a 1D container to Matrix
 * Infers n from size/m (assumes size is divisible by m)
 *
 * @tparam T Element type
 * @tparam ContainerT Container type (any type with .size() and [] access)
 * @param data 1D data container
 * @param m Number of rows
 * @return Matrix with dimensions m x (size/m)
 */
template <typename T, typename ContainerT>
Matrix<T> to_matrix_infer_cols(const ContainerT& data, size_t m) {
    if (data.size() % m != 0) {
        throw std::invalid_argument("Container size must be divisible by m");
    }
    size_t n = data.size() / m;

    // Call to_matrix with inferred n
    return to_matrix<T, ContainerT>(data, m, n);
}
