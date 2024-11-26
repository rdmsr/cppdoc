#pragma once
#include <cstddef>
#include <cstdint>

namespace Atlas {
template <class T, size_t N> constexpr auto array_size(T (&)[N]) { return N; }

struct None {};

constexpr inline auto NONE = None{};
} // namespace Atlas
