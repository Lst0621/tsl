#pragma once

#include <functional>
#include <stdexcept>
#include <string>
#include <vector>

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
        return sample - sample;
    }
};

/**
 * Generic matrix class using vector<vector<T>> storage.
 * Provides efficient 2D element access and standard matrix operations.
 *
 * @tparam T The element type (must support default construction and copying)
 */
template <typename T>
class Matrix {
    std::vector<std::vector<T>> data;
    size_t rows;
    size_t cols;

   public:
    /**
     * Constructor: creates an m x n matrix with smart zero initialization
     * @param m Number of rows
     * @param n Number of columns
     * Uses compile-time check: if T has default constructor, uses T();
     * otherwise uses element - element
     */
    explicit Matrix(size_t m, size_t n) : rows(m), cols(n) {
        if (m == 0 || n == 0) {
            throw std::invalid_argument("Matrix dimensions must be positive");
        }
        // Use compile-time check to determine how to create zero value
        if constexpr (has_default_constructor<T>::value) {
            T zero = ZeroValue<T, true>::create();
            data.assign(m, std::vector<T>(n, zero));
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
        : rows(m), cols(n) {
        if (m == 0 || n == 0) {
            throw std::invalid_argument("Matrix dimensions must be positive");
        }
        data.assign(m, std::vector<T>(n, default_value));
    }

    /**
     * Constructor from 2D vector of the same type (simplified version)
     * @param source Source matrix data of type T
     */
    Matrix(const std::vector<std::vector<T>>& source) {
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

        // Copy data directly
        for (const auto& row : source) {
            std::vector<T> new_row;
            for (const auto& elem : row) {
                new_row.push_back(elem);
            }
            data.push_back(new_row);
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
        std::function<T(const U&)> transform = [](const U& u) {
            return T(u);
        }) {
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

        // Transform and copy data
        for (size_t i = 0; i < rows; i++) {
            std::vector<T> new_row;
            for (size_t j = 0; j < cols; j++) {
                new_row.push_back(transform(source[i][j]));
            }
            data.push_back(new_row);
        }
    }

    /**
     * Get number of rows
     */
    size_t get_rows() const {
        return rows;
    }

    /**
     * Get number of columns
     */
    size_t get_cols() const {
        return cols;
    }

    /**
     * Access element at (i, j) - const version
     */
    const T& at(size_t i, size_t j) const {
        if (i >= rows || j >= cols) {
            throw std::out_of_range("Matrix index out of bounds");
        }
        return data[i][j];
    }

    /**
     * Access element at (i, j) - non-const version
     */
    T& at(size_t i, size_t j) {
        if (i >= rows || j >= cols) {
            throw std::out_of_range("Matrix index out of bounds");
        }
        return data[i][j];
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
     * Get underlying data as vector<vector<T>>
     */
    const std::vector<std::vector<T>>& get_data() const {
        return data;
    }

    /**
     * Get mutable reference to underlying data
     */
    std::vector<std::vector<T>>& get_data_mut() {
        return data;
    }

    /**
     * Check if matrix is square
     */
    bool is_square() const {
        return rows == cols;
    }

    /**
     * Transpose the matrix (returns new matrix)
     */
    Matrix<T> transpose() const {
        Matrix<T> result(cols, rows);
        for (size_t i = 0; i < rows; i++) {
            for (size_t j = 0; j < cols; j++) {
                result(j, i) = data[i][j];
            }
        }
        return result;
    }

    /**
     * String representation
     */
    std::string to_string() const {
        std::string result = "Matrix(" + std::to_string(rows) + "x" +
                             std::to_string(cols) + ")[\n";
        for (size_t i = 0; i < rows; i++) {
            result += "  [";
            for (size_t j = 0; j < cols; j++) {
                if (j > 0) {
                    result += ", ";
                }
                result += std::to_string(data[i][j]);
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
    Matrix<T> operator+(const Matrix<T>& other) const {
        if (rows != other.rows || cols != other.cols) {
            throw std::invalid_argument(
                "Cannot add matrices with different dimensions");
        }
        Matrix<T> result(rows, cols);
        for (size_t i = 0; i < rows; i++) {
            for (size_t j = 0; j < cols; j++) {
                result(i, j) = data[i][j] + other.data[i][j];
            }
        }
        return result;
    }

    /**
     * Matrix subtraction: A - B
     * Requires: both matrices have same dimensions
     */
    Matrix<T> operator-(const Matrix<T>& other) const {
        if (rows != other.rows || cols != other.cols) {
            throw std::invalid_argument(
                "Cannot subtract matrices with different dimensions");
        }
        Matrix<T> result(rows, cols);
        for (size_t i = 0; i < rows; i++) {
            for (size_t j = 0; j < cols; j++) {
                result(i, j) = data[i][j] - other.data[i][j];
            }
        }
        return result;
    }

    /**
     * Matrix multiplication: A * B
     * Requires: cols of A == rows of B
     * Returns: (rows x other.cols) matrix
     */
    Matrix<T> operator*(const Matrix<T>& other) const {
        if (cols != other.rows) {
            throw std::invalid_argument(
                "Cannot multiply matrices: cols of A must equal rows of B");
        }
        // Create result matrix with uninitialized storage, then fill it
        std::vector<std::vector<T>> result_data;
        for (size_t i = 0; i < rows; i++) {
            std::vector<T> new_row;
            for (size_t j = 0; j < other.cols; j++) {
                T sum = data[i][0] * other.data[0][j];
                for (size_t k = 1; k < cols; k++) {
                    sum = sum + (data[i][k] * other.data[k][j]);
                }
                new_row.push_back(sum);
            }
            result_data.push_back(new_row);
        }
        return Matrix<T>(result_data);
    }

    /**
     * Scalar multiplication: A * scalar
     * Multiplies every element by the scalar
     */
    Matrix<T> operator*(const T& scalar) const {
        Matrix<T> result(rows, cols);
        for (size_t i = 0; i < rows; i++) {
            for (size_t j = 0; j < cols; j++) {
                result(i, j) = data[i][j] * scalar;
            }
        }
        return result;
    }

    /**
     * Scalar multiplication (commutative): scalar * A
     * Friend function to allow scalar * matrix
     */
    friend Matrix<T> operator*(const T& scalar, const Matrix<T>& matrix) {
        return matrix * scalar;
    }

    /**
     * Matrix division by scalar: A / scalar
     * Divides every element by the scalar
     */
    Matrix<T> operator/(const T& scalar) const {
        Matrix<T> result(rows, cols);
        for (size_t i = 0; i < rows; i++) {
            for (size_t j = 0; j < cols; j++) {
                result(i, j) = data[i][j] / scalar;
            }
        }
        return result;
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
        T result = data[0][0];
        for (size_t i = 1; i < rows; i++) {
            result = result + data[i][i];
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
                            return data[i][j];
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
                return data[active_rows[0]][active_cols[0]] *
                           data[active_rows[1]][active_cols[1]] -
                       data[active_rows[0]][active_cols[1]] *
                           data[active_rows[1]][active_cols[0]];
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
        T det = ZeroValue<T, false>::create(data[expand_row][0]);

        // Mark and compute first term
        skip_rows[expand_row] = 1;
        skip_cols[first_col] = 1;
        T cofactor = determinant_recursive(skip_rows, skip_cols);
        det = det + (data[expand_row][first_col] * cofactor);
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
            T term = data[expand_row][j] * cofactor_j;

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

    std::vector<std::vector<T>> mat_data;
    for (size_t i = 0; i < m; i++) {
        std::vector<T> row;
        for (size_t j = 0; j < n; j++) {
            row.push_back(data[i * n + j]);
        }
        mat_data.push_back(row);
    }
    return Matrix<T>(mat_data);
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
