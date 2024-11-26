#pragma once
#include <atlas/array.hpp>
#include <bit>
#include <cstddef>
#include <cstdint>

namespace Atlas::Io {

enum class Endianness {
  Big,
  Little,
};

template <typename T, Endianness E> constexpr T to_endian(T value) {
  if constexpr (E == Endianness::Big) {
    if constexpr (std::endian::native == std::endian::little) {
      T result = 0;
      for (size_t i = 0; i < sizeof(T); i++) {
        result |= static_cast<T>(static_cast<uint8_t>(value >> (i * 8)))
                  << ((sizeof(T) - i - 1) * 8);
      }
      return result;
    } else {
      return value;
    }
  } else {
    if constexpr (std::endian::native == std::endian::big) {
      T result = 0;
      for (size_t i = 0; i < sizeof(T); i++) {
        result |= static_cast<T>(static_cast<uint8_t>(value >> (i * 8)))
                  << ((sizeof(T) - i - 1) * 8);
      }
      return result;
    } else {
      return value;
    }
  }
}

template <typename T, Endianness E>
constexpr T bytes_to(Array<uint8_t, sizeof(T)> a) {
  T result = 0;
  for (size_t i = 0; i < sizeof(T); i++) {
    result |= static_cast<T>(a[i]) << (i * 8);
  }
  return to_endian<T, E>(result);
}

template <typename T, Endianness E>
constexpr Array<uint8_t, sizeof(T)> to_bytes(T value) {
  T result = to_endian<T, E>(value);
  Array<uint8_t, sizeof(T)> a{};
  for (size_t i = 0; i < sizeof(T); i++) {
    a[i] = static_cast<uint8_t>(result >> (i * 8));
  }
  return a;
}

static_assert(bytes_to<uint16_t, Endianness::Big>({0x12, 0x34}) == 0x1234);
static_assert(bytes_to<uint16_t, Endianness::Little>({0x34, 0x12}) == 0x1234);

static_assert(to_bytes<uint16_t, Endianness::Little>(0x1234) ==
              Array<uint8_t, 2>{0x34, 0x12});

static_assert(to_bytes<uint16_t, Endianness::Big>(0x1234) ==
              Array<uint8_t, 2>{0x12, 0x34});

} // namespace Atlas::Io