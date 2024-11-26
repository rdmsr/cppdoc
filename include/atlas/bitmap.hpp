#pragma once
#include "error.hpp"
#include "iter.hpp"
#include "result.hpp"
#include "slice.hpp"
#include <cstddef>
#include <cstdint>

namespace Atlas {

class Bitmap {
public:
  constexpr Bitmap(uint8_t *data, size_t size)
      : data_(Slice<uint8_t>{data, size}) {}
  constexpr Bitmap(Slice<uint8_t> slice) : data_(slice) {}

  [[nodiscard]] constexpr size_t size() const { return data_.size(); }
  [[nodiscard]] constexpr uint8_t *data() { return data_.data(); }
  [[nodiscard]] constexpr Slice<uint8_t> as_slice() const { return data_; }

  [[nodiscard]] constexpr Option<bool> get(size_t index) const {
    if (index >= size()) {
      return NONE;
    }

    return data_[index / 8] & (1 << (index % 8));
  }

  constexpr Result<> set(size_t index, bool value) {
    if (index >= size()) {
      return Err(Error::OutOfBounds);
    }

    if (value) {
      data_[index / 8] |= (1 << (index % 8));
    } else {
      data_[index / 8] &= ~(1 << (index % 8));
    }

    return Ok(NONE);
  }

  constexpr auto iter() {

    auto next_func = [data = data_.data(), size = data_.size(),
                      index = size_t(0)]() mutable -> Option<bool> {
      if (index >= size) {
        return NONE;
      }

      auto ret = data[index / 8] & (1 << (index % 8));

      index++;
      return ret;
    };

    auto prev_func = [data = data_.data(), size = data_.size(),
                      index = data_.size() - 1]() mutable -> Option<bool> {
      if (index == SIZE_MAX) {
        return NONE;
      }

      auto ret = data[index / 8] & (1 << (index % 8));
      index--;
      return ret;
    };

    return Iterator<decltype(next_func), decltype(prev_func)>{next_func,
                                                              prev_func};
  }

private:
  Slice<uint8_t> data_;
};

} // namespace Atlas
