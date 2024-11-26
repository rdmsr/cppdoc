#pragma once
#include "base.hpp"
#include "panic.hpp"
#include <memory>
#include <utility>

namespace Atlas {

template <typename T> class [[nodiscard]] Option {

public:
  constexpr Option() : present_(false), none_() {}

  constexpr Option(None) : present_(false), none_() {}
  constexpr Option(T &value) : present_(true), value_(value) {}

  /**
   * Constructs an option type
   * @param value Hi
   */
  constexpr Option(T &&value) : present_(true) {
    std::construct_at(&value_, std::forward<T>(value));
  }

  constexpr Option(const Option &other) : present_(other.present_) {
    if (present_) {
      std::construct_at(&value_, other.value_);
    }
  }

  constexpr Option(Option &&other) : present_(other.present_) {
    if (present_) {
      std::construct_at(&value_, other.take());
    }
  }

  constexpr Option &operator=(Option &&other) {
    clear();

    present_ = other.present_;

    if (present_) {
      std::construct_at(&value_, other.take());
    }
    return *this;
  }

  constexpr Option &operator=(Option &other) {
    clear();

    present_ = other.present_;

    if (present_) {
      std::construct_at(&value_, other.value_);
    }

    return *this;
  }

  constexpr explicit operator bool() const { return present_; }
  [[nodiscard]] constexpr bool is_some() const { return present_; }
  [[nodiscard]] constexpr bool is_none() const { return !present_; }

  constexpr T const &operator*() const {
    if (!present_) [[unlikely]]
      panic("Tried unwrapping 'None' using *");

    return value_;
  }

  constexpr T const *operator->() const {
    if (!present_) [[unlikely]]
      panic("Tried unwrapping 'None' using ->");

    return &value_;
  }

  constexpr bool operator==(const Option<T> &other) const {
    if (present_ != other.present_) {
      return false;
    }

    if (present_) {
      return value_ == other.value_;
    }

    return true;
  }

  constexpr T &unwrap(const char *msg = "Tried unwrapping 'None'") {
    if (!present_) [[unlikely]] {
      panic(msg);
    }

    return value_;
  }

  constexpr const T unwrap_or(T const default_value) const {
    if (present_) {
      return value_;
    }

    return default_value;
  }

  constexpr T take() {
    if (!present_) [[unlikely]] {
      panic("Tried unwrapping 'None'");
    }

    auto tmp = std::move(value_);
    clear();
    return tmp;
  }

  constexpr void clear() {
    if (present_) {
      value_.~T();
    }

    present_ = false;
  }

  constexpr ~Option() { clear(); }

private:
  bool present_;
  union {
    None none_;
    T value_;
  };
};

#define TRYE(VAL, ERRORT)                                                      \
  ({                                                                           \
    auto __ret = (VAL);                                                        \
    if (!__ret)                                                                \
      return Err(ERRORT);                                                      \
    __ret.unwrap();                                                            \
  })

} // namespace Atlas
