#pragma once
#include "alloc.hpp"
#include "assert.hpp"
#include <memory>
#include <utility>

namespace Atlas {
template <typename T, Allocator A = DefaultAllocator> struct Box {

  constexpr Box(T *ptr, A alloc = A()) : ptr_(ptr), alloc_(alloc) {}

  constexpr Box(Box &&other)
      : ptr_(other.ptr_), alloc_(std::move(other.alloc_)) {
    other.ptr_ = nullptr;
  }

  constexpr Box(Box &other) = delete;

  constexpr ~Box() {
    if (ptr_) {
      ptr_->~T();
      alloc_.deallocate(ptr_, sizeof(T));
    }
    ptr_ = nullptr;
  }

  constexpr Box &operator=(Box &other) = delete;

  constexpr Box &operator=(Box &&other) {
    ptr_ = other.ptr;
    alloc_ = std::move(other.alloc);
    other.ptr = nullptr;
    return *this;
  }

  constexpr T &operator*() {
    ENSURE(ptr_ != nullptr, "Box was moved");
    return *ptr_;
  }
  constexpr T *operator->() {
    ENSURE(ptr_ != nullptr, "Box was moved");
    return ptr_;
  }

  constexpr T *as_pointer() {
    ENSURE(ptr_ != nullptr, "Box was moved");
    return ptr_;
  }

  constexpr static Box<T> make(T value, A alloc = A()) {
    auto obj = reinterpret_cast<T *>(alloc.allocate(sizeof(T)));
    std::construct_at(obj, value);
    return Box<T, A>(obj);
  }

private:
  T *ptr_;
  A alloc_;
};

} // namespace Atlas