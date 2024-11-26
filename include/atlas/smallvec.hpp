#pragma once
#include "base.hpp"
#include "option.hpp"
#include "slice.hpp"
#include <cstddef>
#include <utility>

namespace Atlas {

template <typename T, size_t N> class SmallVec {

public:
  constexpr SmallVec() : size_(0) {}

  constexpr SmallVec(auto... val)
    requires(sizeof...(val) <= N)
      : size_(sizeof...(val)) {
    size_t i = 0;
    for (const T &value : {val...})
      new (&data_[i++]) T(value);
  }

  constexpr void push_back(T &value) {
    ENSURE(size_ < N, "SmallVec is full");

    new (&data_[size_++]) T(value);
  }

  constexpr void push_back(T &&value) {
    ENSURE(size_ < N, "SmallVec is full");
    new (&data_[size_++]) T(std::move(value));
  }

  constexpr T *begin() { return data_; }
  constexpr T *end() { return data_ + size_; }
  constexpr T *data() { return data_; }

  constexpr auto iter() { return as_slice().iter(); }

  constexpr Slice<T> as_slice() { return Slice<T>(data_, size_); }

  constexpr T &operator[](size_t index) {
    ENSURE(index < size_, "index out of bounds");
    return data_[index];
  }

  constexpr Option<T> pop() {
    if (size_ <= 0) {
      return NONE;
    }
    return data_[--size_];
  }

  [[nodiscard]] size_t size() const { return size_; }

private:
  T data_[N];
  size_t size_;
};

template <typename... T> SmallVec(T...) -> SmallVec<T..., sizeof...(T)>;

} // namespace Atlas