#pragma once

#include <cstddef>
#include <iterator>
#include <new>
#include <type_traits>
#include <vector>

/**
 * Boost.Container small_vector–style buffer: up to InlineCapacity elements live
 * inline (no heap); larger sizes use std::vector<T>.
 *
 * Default: zero size, zero heap allocation. reserve(cap) only touches the
 * heap when cap > InlineCapacity (or after migration).
 */
template <typename T, size_t InlineCapacity = 256>
class SmallVector {
   public:
    using value_type = T;
    using size_type = size_t;
    using iterator = T*;
    using const_iterator = const T*;

   private:
    enum class Mode { Inline, Heap };

    size_type size_;
    Mode mode_;
    alignas(T) unsigned char inline_bytes_[InlineCapacity * sizeof(T)];
    std::vector<T> heap_;

    T* inline_ptr() {
        return reinterpret_cast<T*>(inline_bytes_);
    }

    const T* inline_ptr() const {
        return reinterpret_cast<const T*>(inline_bytes_);
    }

    T* data_ptr() {
        if (mode_ == Mode::Heap) {
            return heap_.data();
        }
        return inline_ptr();
    }

    const T* data_ptr() const {
        if (mode_ == Mode::Heap) {
            return heap_.data();
        }
        return inline_ptr();
    }

    void destroy_inline_elements() {
        T* p = inline_ptr();
        for (size_type i = 0; i < size_; i++) {
            p[i].~T();
        }
    }

    void migrate_inline_to_heap() {
        std::vector<T> tmp;
        tmp.reserve(size_);
        T* p = inline_ptr();
        for (size_type i = 0; i < size_; i++) {
            tmp.push_back(std::move(p[i]));
            p[i].~T();
        }
        heap_ = std::move(tmp);
        mode_ = Mode::Heap;
    }

   public:
    SmallVector() : size_(0), mode_(Mode::Inline) {
    }

    SmallVector(const SmallVector& other) : size_(0), mode_(Mode::Inline) {
        if (other.size_ == 0) {
            return;
        }
        if (other.mode_ == Mode::Heap) {
            mode_ = Mode::Heap;
            heap_ = other.heap_;
            size_ = other.size_;
            return;
        }
        const T* src = other.inline_ptr();
        for (size_type i = 0; i < other.size_; i++) {
            new (inline_ptr() + i) T(src[i]);
        }
        size_ = other.size_;
        mode_ = Mode::Inline;
    }

    SmallVector& operator=(const SmallVector& other) {
        if (this == &other) {
            return *this;
        }
        clear();
        if (other.size_ == 0) {
            return *this;
        }
        if (other.mode_ == Mode::Heap) {
            mode_ = Mode::Heap;
            heap_ = other.heap_;
            size_ = other.size_;
            return *this;
        }
        const T* src = other.inline_ptr();
        for (size_type i = 0; i < other.size_; i++) {
            new (inline_ptr() + i) T(src[i]);
        }
        size_ = other.size_;
        mode_ = Mode::Inline;
        return *this;
    }

    SmallVector(SmallVector&& other) noexcept(
        std::is_nothrow_move_constructible_v<T>)
        : size_(0), mode_(Mode::Inline) {
        if (other.mode_ == Mode::Heap) {
            heap_ = std::move(other.heap_);
            size_ = other.size_;
            mode_ = Mode::Heap;
            other.size_ = 0;
            other.mode_ = Mode::Inline;
        } else {
            size_ = other.size_;
            T* src = other.inline_ptr();
            T* dst = inline_ptr();
            for (size_type i = 0; i < size_; i++) {
                new (dst + i) T(std::move(src[i]));
                src[i].~T();
            }
            other.size_ = 0;
            mode_ = Mode::Inline;
        }
    }

    SmallVector& operator=(SmallVector&& other) noexcept(
        std::is_nothrow_move_assignable_v<T> &&
        std::is_nothrow_move_constructible_v<T>) {
        if (this == &other) {
            return *this;
        }
        clear();
        if (other.mode_ == Mode::Heap) {
            heap_ = std::move(other.heap_);
            size_ = other.size_;
            mode_ = Mode::Heap;
            other.size_ = 0;
            other.mode_ = Mode::Inline;
        } else {
            size_ = other.size_;
            T* src = other.inline_ptr();
            T* dst = inline_ptr();
            for (size_type i = 0; i < size_; i++) {
                new (dst + i) T(std::move(src[i]));
                src[i].~T();
            }
            other.size_ = 0;
            mode_ = Mode::Inline;
        }
        return *this;
    }

    ~SmallVector() {
        clear();
    }

    void assign(size_type n, const T& value) {
        clear();
        if (n == 0) {
            return;
        }
        if (n > InlineCapacity) {
            mode_ = Mode::Heap;
            heap_.assign(n, value);
            size_ = n;
        } else {
            T* p = inline_ptr();
            for (size_type i = 0; i < n; i++) {
                new (p + i) T(value);
            }
            size_ = n;
            mode_ = Mode::Inline;
        }
    }

    void reserve(size_type cap) {
        if (cap <= InlineCapacity) {
            return;
        }
        if (mode_ == Mode::Heap) {
            heap_.reserve(cap);
            return;
        }
        if (size_ == 0) {
            mode_ = Mode::Heap;
            heap_.reserve(cap);
            return;
        }
        migrate_inline_to_heap();
        heap_.reserve(cap);
    }

    void push_back(const T& value) {
        if (mode_ == Mode::Heap) {
            heap_.push_back(value);
            size_ = heap_.size();
            return;
        }
        if (size_ < InlineCapacity) {
            new (inline_ptr() + size_) T(value);
            size_++;
            return;
        }
        migrate_inline_to_heap();
        heap_.push_back(value);
        size_ = heap_.size();
    }

    template <typename InputIt>
    void insert_back(InputIt first, InputIt last) {
        const size_type add = static_cast<size_type>(std::distance(first, last));
        if (add == 0) {
            return;
        }
        if (mode_ == Mode::Heap) {
            heap_.insert(heap_.end(), first, last);
            size_ = heap_.size();
            return;
        }
        if (size_ + add <= InlineCapacity) {
            T* p = inline_ptr();
            for (InputIt it = first; it != last; ++it) {
                new (p + size_) T(*it);
                size_++;
            }
            return;
        }
        migrate_inline_to_heap();
        heap_.insert(heap_.end(), first, last);
        size_ = heap_.size();
    }

    void clear() {
        if (mode_ == Mode::Inline) {
            destroy_inline_elements();
        } else {
            heap_.clear();
        }
        size_ = 0;
        mode_ = Mode::Inline;
    }

    size_type size() const {
        return size_;
    }

    bool empty() const {
        return size_ == 0;
    }

    T& operator[](size_type i) {
        return data_ptr()[i];
    }

    const T& operator[](size_type i) const {
        return data_ptr()[i];
    }

    T* data() {
        return size_ == 0 ? nullptr : data_ptr();
    }

    const T* data() const {
        return size_ == 0 ? nullptr : data_ptr();
    }

    iterator begin() {
        return size_ == 0 ? nullptr : data_ptr();
    }

    iterator end() {
        return size_ == 0 ? nullptr : data_ptr() + size_;
    }

    const_iterator begin() const {
        return size_ == 0 ? nullptr : data_ptr();
    }

    const_iterator end() const {
        return size_ == 0 ? nullptr : data_ptr() + size_;
    }
};
