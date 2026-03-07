#pragma once

#include <concepts>
#include <cstddef>
#include <functional>

/**
 * 2D Coordinate structure
 * Represents a point in 2D space with integer coordinates
 */
struct Coord2D {
    int x;
    int y;

    // Default constructor
    Coord2D() : x(0), y(0) {
    }

    // Parameterized constructor
    Coord2D(int x, int y) : x(x), y(y) {
    }

    // Equality operators
    bool operator==(const Coord2D& other) const {
        return x == other.x && y == other.y;
    }

    bool operator!=(const Coord2D& other) const {
        return !(*this == other);
    }

    // Arithmetic operators
    Coord2D operator+(const Coord2D& other) const {
        return Coord2D(x + other.x, y + other.y);
    }

    Coord2D operator-(const Coord2D& other) const {
        return Coord2D(x - other.x, y - other.y);
    }

    Coord2D& operator+=(const Coord2D& other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    Coord2D& operator-=(const Coord2D& other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    // Scalar multiplication
    Coord2D operator*(int scalar) const {
        return Coord2D(x * scalar, y * scalar);
    }

    // Dimension access
    static constexpr int dimension() {
        return 2;
    }

    // Get coordinate by index (0=x, 1=y)
    int get(int index) const {
        return index == 0 ? x : y;
    }

    // Set coordinate by index
    void set(int index, int value) {
        if (index == 0)
            x = value;
        else
            y = value;
    }
};

/**
 * 3D Coordinate structure
 * Represents a point in 3D space with integer coordinates
 */
struct Coord3D {
    int x;
    int y;
    int z;

    // Default constructor
    Coord3D() : x(0), y(0), z(0) {
    }

    // Parameterized constructor
    Coord3D(int x, int y, int z) : x(x), y(y), z(z) {
    }

    // Equality operators
    bool operator==(const Coord3D& other) const {
        return x == other.x && y == other.y && z == other.z;
    }

    bool operator!=(const Coord3D& other) const {
        return !(*this == other);
    }

    // Arithmetic operators
    Coord3D operator+(const Coord3D& other) const {
        return Coord3D(x + other.x, y + other.y, z + other.z);
    }

    Coord3D operator-(const Coord3D& other) const {
        return Coord3D(x - other.x, y - other.y, z - other.z);
    }

    Coord3D& operator+=(const Coord3D& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    Coord3D& operator-=(const Coord3D& other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    // Scalar multiplication
    Coord3D operator*(int scalar) const {
        return Coord3D(x * scalar, y * scalar, z * scalar);
    }

    // Dimension access
    static constexpr int dimension() {
        return 3;
    }

    // Get coordinate by index (0=x, 1=y, 2=z)
    int get(int index) const {
        return index == 0 ? x : (index == 1 ? y : z);
    }

    // Set coordinate by index
    void set(int index, int value) {
        if (index == 0)
            x = value;
        else if (index == 1)
            y = value;
        else
            z = value;
    }
};

/**
 * Hash functions for coordinates (needed for unordered containers)
 */
namespace std {
template <>
struct hash<Coord2D> {
    size_t operator()(const Coord2D& coord) const {
        // Use a simple but effective hash combination
        size_t h1 = std::hash<int>{}(coord.x);
        size_t h2 = std::hash<int>{}(coord.y);
        return h1 ^ (h2 << 1);
    }
};

template <>
struct hash<Coord3D> {
    size_t operator()(const Coord3D& coord) const {
        size_t h1 = std::hash<int>{}(coord.x);
        size_t h2 = std::hash<int>{}(coord.y);
        size_t h3 = std::hash<int>{}(coord.z);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};
}  // namespace std

/**
 * Coordinate concept
 * Defines the required interface for a coordinate type
 */
template <typename T>
concept Coordinate =
    requires(T coord, const T const_coord, int index, int value) {
        // Must have equality comparison
        { const_coord == const_coord } -> std::convertible_to<bool>;
        { const_coord != const_coord } -> std::convertible_to<bool>;

        // Must have arithmetic operators
        { const_coord + const_coord } -> std::convertible_to<T>;
        { const_coord - const_coord } -> std::convertible_to<T>;

        // Must have dimension access
        { T::dimension() } -> std::convertible_to<int>;
        { const_coord.get(index) } -> std::convertible_to<int>;

        // Must be hashable
        { std::hash<T>{}(const_coord) } -> std::convertible_to<size_t>;
    };

// Verify that our coordinate types satisfy the concept
static_assert(Coordinate<Coord2D>);
static_assert(Coordinate<Coord3D>);
