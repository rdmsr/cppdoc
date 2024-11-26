#pragma once
#include "base.hpp"
#include "error.hpp"
#include "panic.hpp"
#include <utility>

namespace Atlas {

template <typename T> struct Ok {

public:
  constexpr Ok(T &&value) : value(std::move(value)) {}
  constexpr Ok(const T &value) : value(value) {}

  T value;
};

template <typename T> struct Err {

public:
  constexpr Err(T &&value) : value(std::move(value)) {}
  constexpr Err(const T &value) : value(value) {}

  T value;
};

template <typename T = None, typename E = Error> class [[nodiscard]] Result {

public:
  constexpr Result(Ok<T> &&value)
      : variant_(std::move(value)), has_value_(true) {}
  constexpr Result(const Ok<T> &value) : variant_(value), has_value_(true) {}

  constexpr Result(Result<T, E> &&other)
      : variant_(std::move(other.variant_)), has_value_(other.has_value_) {}

  constexpr Result(Result<T, E> &other)
      : variant_(other.variant_), has_value_(other.has_value_) {}

  constexpr Result(Err<E> error)
      : variant_({.error = error}), has_value_(false) {}

  constexpr explicit operator bool() const { return has_value_; }
  [[nodiscard]] constexpr bool is_ok() const { return has_value_; }

  constexpr const T &
  unwrap(const char *msg = "Called 'unwrap' on an error value") const {
    if (!has_value_) [[unlikely]] {
      panic(msg);
    }

    return variant_.value.value;
  }

  constexpr const T unwrap_or(T const other) const {
    if (has_value_) {
      return variant_.value.value;
    }

    return other;
  }

  constexpr const E &error() {
    if (has_value_) {
      panic("Called 'error' on an ok value");
    }

    return variant_.error.value;
  }

private:
  union {
    Ok<T> value;
    Err<E> error;
  } variant_;

  bool has_value_;
};

#define TRY(X)                                                                 \
  ({                                                                           \
    auto __ret = (X);                                                          \
    if (!__ret.is_ok())                                                        \
      return Err(__ret.error());                                               \
    __ret.unwrap();                                                            \
  })

} // namespace Atlas
