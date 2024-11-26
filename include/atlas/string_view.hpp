#pragma once
#include "cstr.hpp"
#include "slice.hpp"
#include <cstddef>

namespace Atlas {

class StringView {

public:
  constexpr StringView() = default;

  StringView(const char *str) : data_(str), length_(strlen(str)) {}

  constexpr StringView(const char *str, size_t length)
      : data_(str), length_(length) {}

  [[nodiscard]] constexpr const char *data() const { return data_; }
  [[nodiscard]] constexpr size_t length() const { return length_; }

  constexpr char operator[](size_t index) const { return data_[index]; }

  [[nodiscard]] constexpr StringView substr(size_t start, size_t length) const {
    return {data_ + start, length};
  }

  constexpr bool operator==(const StringView &other) const {
    if (length_ != other.length_) {
      return false;
    }
    for (size_t i = 0; i < length_; i++) {
      if (data_[i] != other.data_[i]) {
        return false;
      }
    }
    return true;
  }

  constexpr const char *begin() { return data_; }
  constexpr const char *end() { return data_ + length_; }

  [[nodiscard]] constexpr Slice<const char> as_slice() const {
    return {data_, length_};
  }

private:
  const char *data_;
  size_t length_;
};

constexpr StringView operator""_sv(const char *str, size_t length) {
  return {str, length};
}

} // namespace Atlas
