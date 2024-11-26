#pragma once
#include "assert.hpp"
#include "iter.hpp"
#include <cstddef>

namespace Atlas {

template <typename T> class Slice {
public:
  constexpr Slice() = default;

  constexpr Slice(T *data, size_t size) : data_(data), size_(size) {}

  constexpr T *data() const { return data_; }
  [[nodiscard]] constexpr size_t size() const { return size_; }

  constexpr T *begin() const { return data_; }
  constexpr T *end() const { return data_ + size_; }

  // Implements trait AsSlice
  constexpr Slice<T> as_slice() const { return *this; }

  constexpr T &operator[](size_t index) const {
    ENSURE(index < size_, "index out of bounds");
    return data_[index];
  }

  constexpr Option<Slice<T>> sub_slice(size_t start, size_t end) const {
    if (start > size_ || end > size_ || start > end) [[unlikely]] {
      return NONE;
    }

    return Slice<T>(data_ + start, end - start);
  }

  constexpr auto iter() {
    // FIXME: Is capturing by value here the right thing to do?
    // This is needed for e.g. array's iter() because we use slice to iterate
    return Iterator(
        // Forward iterator
        [*this, index = size_ - size_]() mutable -> Option<T> {
          if (index < size_) {
            return data_[index++];
          }
          return NONE;
        },

        // Reverse Iterator
        [data = data_, index = size_]() mutable -> Option<T> {
          if (index > 0) {
            return data[--index];
          }
          return NONE;
        }

    );
  }

  constexpr int operator<=>(const Slice<T> &other) const {
    if (size_ < other.size_) {
      return -1;
    } else if (size_ > other.size_) {
      return 1;
    }

    for (size_t i = 0; i < size_; i++) {
      if (data_[i] < other.data_[i]) {
        return -1;
      } else if (data_[i] > other.data_[i]) {
        return 1;
      }
    }

    return 0;
  }

  constexpr bool operator==(const Slice<T> &other) const {
    if (size_ != other.size_) {
      return false;
    }

    for (size_t i = 0; i < size_; i++) {
      if (data_[i] != other.data_[i]) {
        return false;
      }
    }

    return true;
  }

private:
  T *data_ = nullptr;
  size_t size_ = 0;
};

template <typename T, typename T1>
concept AsSlice = requires(T a) {
  { a.as_slice() } -> std::same_as<Slice<T1>>;
};

} // namespace Atlas
