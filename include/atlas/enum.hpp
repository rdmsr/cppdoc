#pragma once

#include "option.hpp"
#include <type_traits>

namespace Atlas {

template <typename T>
concept Enum = std::is_enum_v<T>;

template <typename T>
concept BoundedEnum = Enum<T> && requires {
  { T::First };
  { T::Last };
};

template <Enum T>
constexpr inline Option<T> enum_cast(std::underlying_type_t<T> value, T first,
                                     T last) {
  if (value < static_cast<std::underlying_type_t<T>>(first) ||
      value > static_cast<std::underlying_type_t<T>>(last)) {
    return NONE;
  }
  return static_cast<T>(value);
}

template <BoundedEnum T>
constexpr inline Option<T> enum_cast(std::underlying_type_t<T> value) {
  return enum_cast(value, T::First, T::Last);
}

} // namespace Atlas