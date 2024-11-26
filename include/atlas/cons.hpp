#pragma once
#include <cstddef>

namespace Atlas {

template <typename A, typename B> struct Cons {
  A car;
  B cdr;

  constexpr Cons(A car, B cdr) : car(car), cdr(cdr) {}

  constexpr A first() const { return car; }
  constexpr B second() const { return cdr; }

  template <size_t I> constexpr auto &get() const {
    if constexpr (I == 0) {
      return car;
    } else if constexpr (I == 1) {
      return cdr;
    } else {
      static_assert(I < 2, "Index out of bounds");
    }
  }

  bool operator==(const Cons<A, B> &other) const = default;
};

template <typename A, typename B> constexpr Cons<A, B> cons(A a, B b) {
  return Cons<A, B>(a, b);
}

} // namespace Atlas
