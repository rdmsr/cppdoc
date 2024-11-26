#pragma once
#include "cons.hpp"
#include "option.hpp"
#include "traits.hpp"
#include <utility>

namespace Atlas {

template <typename T>
concept IterNext = requires(T a) {
  { a() } -> std::same_as<Option<T>>;
};

template <typename F, typename T>
concept IterFunc = requires(F func, T &a) {
  { func(a) };
};

template <typename F, typename T>
concept FoldFunc = requires(F func, T acc, T a) {
  { func(acc, a) } -> std::same_as<T>;
};

template <typename Next, typename Back = Next> class Iterator {
  Next next_;
  Option<Back> back_;

  using T = decltype(next_().take());

public:
  constexpr Iterator(Next next) : next_(next), back_(NONE) {}
  constexpr Iterator(Next next, Back back) : next_(next), back_(back) {}

  template <typename T> struct Iter_ {
    Option<T> current_;
    Next next_;

    constexpr Iter_(Option<T> begin, Next next)
        : current_(begin), next_(next) {}

    constexpr const T &operator*() { return *current_; }
    constexpr void operator++() { current_ = next_(); }

    constexpr bool operator!=(const Iter_ &other) {
      return current_ != other.current_;
    }
    constexpr bool operator!=(None) { return current_ != NONE; }
  };

  Iter_<T> begin() { return Iter_<T>{next_(), next_}; }
  Iter_<T> end() { return Iter_<T>{NONE, next_}; }

  constexpr auto next() { return next_(); }

  // Iterator composition
  constexpr auto operator|(auto f) { return f(*this); }

  // Most of these methods come from Rust's Iterator trait
  template <IterFunc<T> F> constexpr auto map(F f) {
    auto next_func = [that = std::move(*this), f]() mutable -> Option<T> {
      auto a = that.next_();
      if (!a) {
        return NONE;
      }
      return f(*a);
    };

    return Iterator<decltype(next_func)>(next_func);
  }

  constexpr auto enumerate() {
    size_t i = 0;
    auto next_func = [that = std::move(*this),
                      i]() mutable -> Option<Cons<size_t, T>> {
      auto a = that.next_();
      if (!a) {
        return NONE;
      }
      return cons(i++, *a);
    };

    return Iterator<decltype(next_func)>(next_func);
  }

  template <Predicate<T> F> constexpr auto filter(F f) {
    auto next_func = [that = std::move(*this), f]() mutable -> Option<T> {
      for (auto a = that.next_(); a; a = that.next_()) {
        if (f(*a)) {
          return a;
        }
      }
      return NONE;
    };

    return Iterator<decltype(next_func)>(next_func);
  }

  constexpr auto step_by(size_t step) {
    if (step == 0) {
      panic("step_by called with step 0");
    }

    auto next_func = [that = std::move(*this), step,
                      i = 0]() mutable -> Option<T> {
      for (auto a = that.next_(); a; a = that.next_()) {
        if (i++ % step == 0) {
          return a;
        }
      }
      return NONE;
    };

    return Iterator<decltype(next_func)>(next_func);
  }

  template <Predicate<T> F> constexpr bool all(F f) {
    for (auto a = next_(); a; a = next_()) {
      if (!f(*a)) {
        return false;
      }
    }
    return true;
  }

  template <Predicate<T> F> constexpr bool any(F f) {
    for (auto a = next_(); a; a = next_()) {
      if (f(*a)) {
        return true;
      }
    }
    return false;
  }

  template <Predicate<T> F> constexpr Option<T> find(F f) {
    for (auto a = next_(); a; a = next_()) {
      if (f(*a)) {
        return a;
      }
    }
    return NONE;
  }

  template <FoldFunc<T> F> constexpr T fold(T init, F f) {
    T acc = init;
    for (auto a = next_(); a; a = next_()) {
      acc = f(acc, *a);
    }
    return acc;
  }

  template <FoldFunc<T> F> constexpr Option<T> reduce(F f) {
    auto a = next_();
    if (!a) {
      return NONE;
    }
    return fold(*a, f);
  }

  template <IterFunc<T> F> constexpr auto for_each(F f) {
    for (auto a = next_(); a; a = next_()) {
      f(*a);
    }
  }

  template <Container<T> C> constexpr C collect() {
    C ret;
    for (auto a = next_(); a; a = next_()) {
      ret.push_back(a.take());
    }
    return ret;
  }

  constexpr size_t count() {
    size_t ret = 0;
    for (auto a = next_(); a; a = next_()) {
      ret++;
    }
    return ret;
  }

  constexpr Option<T> last() {
    Option<T> ret = NONE;
    for (auto a = next_(); a; a = next_()) {
      ret = a;
    }
    return ret;
  }

  constexpr Option<T> nth(size_t n) {
    for (size_t i = 0; i < n; i++) {
      (void)next_();
    }
    return next_();
  }

  constexpr auto rev() {
    if (!back_) {
      panic("rev called on iterator without a back function");
    }

    return Iterator<decltype(back_.take())>(*back_);
  }
};

// Takes C++ iterators and turns them into ours
template <typename Iter> constexpr auto make_iterator(Iter begin, Iter end) {
  using T = std::remove_cvref_t<decltype(*begin)>;
  return Iterator([begin, end]() mutable -> Option<T> {
    if (begin == end) {
      return NONE;
    }
    return *begin++;
  });
}

template <typename Iter, typename RIter>
constexpr auto make_iterator(Iter begin, Iter end, RIter rbegin, RIter rend) {
  using T = std::remove_cvref_t<decltype(*begin)>;

  return Iterator(
      // Forward iterator
      [begin, end]() mutable -> Option<T> {
        if (begin == end) {
          return NONE;
        }
        return *begin++;
      },

      // Reverse iterator
      [rbegin, rend]() mutable -> Option<T> {
        if (rbegin == rend) {
          return NONE;
        }
        return *rbegin++;
      });
}

} // namespace Atlas
