#pragma once
#include "panic.hpp"
#include "slice.hpp"
#include <cstddef>
#include <initializer_list>

namespace Atlas {

template <typename T, size_t N> class Array {

public:
  constexpr Array(std::initializer_list<T> list) {
    if (list.size() > N) [[unlikely]] {
      panic("Array initializer list size mismatch");
    }

    size_t i = 0;
    for (const T &v : list) {
      data_[i++] = v;
    }
  }

  constexpr T &operator[](size_t i) {
    if (i >= N) [[unlikely]] {
      panic("Array index out of bounds");
    }

    return data_[i];
  }

  constexpr const T &operator[](size_t i) const {
    if (i >= N) [[unlikely]] {
      panic("Array index out of bounds");
    }
    return data_[i];
  }

  [[nodiscard]] constexpr size_t size() const { return N; }

  constexpr T const *data() const { return data_; }
  constexpr T *data() { return data_; }

  [[nodiscard]] constexpr Slice<T> as_slice() { return Slice<T>(data_, N); }

  constexpr T *begin() { return data_; }
  constexpr T *end() { return data_ + N; }

  constexpr auto iter() { return as_slice().iter(); }

  constexpr bool operator==(const Array<T, N> &other) const {
    for (size_t i = 0; i < N; i++) {
      if (data_[i] != other.data_[i]) {
        return false;
      }
    }

    return true;
  }

private:
  T data_[N];
};

} // namespace Atlas
