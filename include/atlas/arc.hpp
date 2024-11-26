#pragma once
#include "alloc.hpp"
#include "assert.hpp"
#include <atomic>
#include <cstddef>
#include <memory>

namespace Atlas {
template <typename T, Allocator A = DefaultAllocator> class Arc {
public:
  constexpr explicit Arc(T *ptr, A alloc = A()) : ptr_(ptr), alloc_(alloc) {
    refcount_data_ = 1;
    refcount_ = &refcount_data_;
  }

  constexpr Arc(Arc &other)
      : ptr_(other.ptr_), alloc_(other.alloc_), refcount_(other.refcount_) {
    if (ptr_) {
      ++(*refcount_);
    }
  }

  constexpr Arc(Arc &&other) noexcept
      : ptr_(other.ptr_), alloc_(other.alloc_), refcount_(other.refcount_) {
    ASSERT(*refcount_ > 0);

    other.ptr_ = nullptr;
    other.refcount_ = nullptr;
  }

  constexpr ~Arc() {
    if (ptr_ && --(*refcount_) == 0) {
      ptr_->~T();
      alloc_.deallocate(ptr_, sizeof(T));
    }
  }

  constexpr Arc &operator=(Arc const &other) {
    if (this == &other)
      return *this;

    ptr_ = other.ptr_;
    alloc_ = other.alloc_;
    refcount_ = other.refcount_;

    if (ptr_) {
      ++(*refcount_);
    }

    return *this;
  }

  constexpr Arc &operator=(Arc &&other) noexcept {
    ptr_ = other.ptr_;
    refcount_ = other.refcount_;
    alloc_ = other.alloc_;

    other.ptr_ = nullptr;

    return *this;
  }

  [[nodiscard]] constexpr size_t ref() const {
    ENSURE(refcount_ != nullptr, "Pointer to refcount is null");
    return *(refcount_);
  }

  constexpr T *as_pointer() const { return ptr_; }

  constexpr T *operator->() const { return ptr_; }
  constexpr T &operator*() const { return *ptr_; }

  constexpr explicit operator bool() const { return ptr_ != nullptr; }

  constexpr static Arc<T> make(T value, A alloc = A()) {
    auto obj = reinterpret_cast<T *>(alloc.allocate(sizeof(T)));
    std::construct_at(obj, value);
    return Arc<T>(obj, alloc);
  }

private:
  T *ptr_ = nullptr;
  A alloc_;
  std::atomic<size_t> refcount_data_;
  std::atomic<size_t> *refcount_ = nullptr;
};
} // namespace Atlas
