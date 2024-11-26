#pragma once
#include "string.hpp"
#include "string_view.hpp"

namespace Atlas {

/*
    MurmurHash2a, by Austin Appleby
*/
uint64_t murmur_hash(const void *key, size_t len, uint64_t seed) {
  const uint64_t m = 0xc6a4a7935bd1e995LLU;
  const int r = 47;
  uint64_t h = seed ^ (len * m);
  auto data = (const uint64_t *)key;
  const uint64_t *end = (len >> 3) + data;
  while (data != end) {
    uint64_t k = *data++;
    k *= m;
    k ^= k >> r;
    k *= m;
    h ^= k;
    h *= m;
  }
  auto data2 = (const unsigned char *)data;
  switch (len & 7) {
  case 7:
    h ^= (uint64_t)(data2[6]) << 48;
  case 6:
    h ^= (uint64_t)(data2[5]) << 40;
  case 5:
    h ^= (uint64_t)(data2[4]) << 32;
  case 4:
    h ^= (uint64_t)(data2[3]) << 24;
  case 3:
    h ^= (uint64_t)(data2[2]) << 16;
  case 2:
    h ^= (uint64_t)(data2[1]) << 8;
  case 1:
    h ^= (uint64_t)(data2[0]);
    h *= m;
  default:
    break;
  };
  h ^= h >> r;
  h *= m;
  h ^= h >> r;
  return h;
}

template <typename T> struct Hash;

template <std::integral T> struct Hash<T> {
  uint64_t operator()(T a) const { return murmur_hash(&a, sizeof(a), 0); }
};

template <> struct Hash<String> {
  uint64_t operator()(const String &s) const {
    return murmur_hash(s.data(), s.length(), 0);
  }
};

template <> struct Hash<StringView> {
  uint64_t operator()(const StringView &s) const {
    return murmur_hash(s.data(), s.length(), 0);
  }
};

template <> struct Hash<const char *> {
  uint64_t operator()(const char *s) const {
    return murmur_hash(s, strlen(s), 0);
  }
};

} // namespace Atlas