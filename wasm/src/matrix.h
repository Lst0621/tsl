#pragma once

#include <stdexcept>
#include <string>
#include <vector>
#include <functional>

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
     * Constructor: creates an m x n matrix
     * @param m Number of rows
     * @param n Number of columns
     * @param default_value Default value for all elements (default-constructed
     * if not provided)
     */
    Matrix(size_t m, size_t n, const T& default_value = T())
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
            throw std::invalid_argument("Trace is only defined for square matrices");
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
            throw std::invalid_argument("Determinant is only defined for square matrices");
        }

        size_t n = rows;

        // Base cases
        if (n == 1) {
            return data[0][0];
        }
        if (n == 2) {
            return data[0][0] * data[1][1] - data[0][1] * data[1][0];
        }

        // Laplace expansion along first row
        T det = data[0][0] * determinant_recursive(0, 0, n - 1);
        for (size_t j = 1; j < n; j++) {
            T cofactor = determinant_recursive(0, j, n - 1);

            // Alternate signs based on column index
            if (j % 2 == 0) {
                det = det + (data[0][j] * cofactor);
            } else {
                det = det - (data[0][j] * cofactor);
            }
        }

        return det;
    }

   private:
    /**
     * Helper function to calculate determinant for a submatrix
     * Skips the specified row and column
     * Used internally to avoid creating intermediate minor matrices
     *
     * @param skip_row Row to skip
     * @param skip_col Column to skip
     * @param size Size of the original matrix (before removing row/col)
     */
    T determinant_recursive(size_t skip_row, size_t skip_col, size_t size) const {
        // Base case: 1x1 submatrix
        if (size == 1) {
            // Find the one remaining element (not in skip_row or skip_col)
            for (size_t i = 0; i < rows; i++) {
                if (i == skip_row) {
                    continue;
                }
                for (size_t j = 0; j < cols; j++) {
                    if (j == skip_col) {
                        continue;
                    }
                    return data[i][j];
                }
            }
        }

        // Base case: 2x2 submatrix
        if (size == 2) {
            std::vector<std::vector<T>> elements;
            for (size_t i = 0; i < rows; i++) {
                if (i == skip_row) {
                    continue;
                }
                std::vector<T> row;
                for (size_t j = 0; j < cols; j++) {
                    if (j == skip_col) {
                        continue;
                    }
                    row.push_back(data[i][j]);
                }
                if (!row.empty()) {
                    elements.push_back(row);
                }
            }
            if (elements.size() == 2 && elements[0].size() == 2) {
                return elements[0][0] * elements[1][1] - elements[0][1] * elements[1][0];
            }
        }

        // Recursive case: use Laplace expansion
        T det = data[0][0] * determinant_recursive(0, 0, size - 1);  // Initialize with first term

        // Iterate through first available row (not skip_row)
        bool first_term = true;
        for (size_t i = 0; i < rows; i++) {
            if (i == skip_row) {
                continue;
            }

            // Expand along this row
            for (size_t j = 0; j < cols; j++) {
                if (j == skip_col) {
                    continue;
                }

                // Skip first element as it's already processed
                if (first_term) {
                    first_term = false;
                    continue;
                }

                T cofactor = determinant_recursive(i, j, size - 1);
                T term = data[i][j] * cofactor;

                // Alternate signs based on position
                size_t pos_i = 0, pos_j = 0;
                for (size_t ii = 0; ii < i; ii++) {
                    if (ii != skip_row) {
                        pos_i++;
                    }
                }
                for (size_t jj = 0; jj < j; jj++) {
                    if (jj != skip_col) {
                        pos_j++;
                    }
                }

                if ((pos_i + pos_j) % 2 == 0) {
                    det = det + term;
                } else {
                    det = det - term;
                }
            }
            break;  // Only use first available row
        }

        return det;
    }

   private:
};
