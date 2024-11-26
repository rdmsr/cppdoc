#pragma once
#include <concepts>

namespace Atlas {

template <typename F, typename T>
concept Predicate = requires(F func, T a) {
  { func(a) } -> std::convertible_to<bool>;
};

template <typename T, typename T1>
concept Container = requires(T a, T1 b) {
  { a.push_back(b) };
};

template <typename T>
concept Sortable = requires(T a, T b) {
  { a <=> b };
};

} // namespace Atlas
