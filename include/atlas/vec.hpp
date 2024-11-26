#pragma once
#include "alloc.hpp"
#include "slice.hpp"

namespace Atlas {

template <typename T, Allocator A = DefaultAllocator> class Vec {

public:
  Vec(A alloc = A())
      : data_(nullptr), size_(0), capacity_(0), alloc_(std::move(alloc)) {}

  Vec(size_t size, A alloc = A()) : capacity_(size), alloc_(std::move(alloc)) {
    data_ = reinterpret_cast<T *>(alloc_.allocate(size * sizeof(T)));
    size_ = size;
    for (size_t i = 0; i < size; i++)
      new (&data_[i]) T();
  }

  Vec(const Vec &other) : size_(other.size_), alloc_(other.alloc_) {
    data_ = reinterpret_cast<T *>(alloc_.allocate(sizeof(T) * other.size_));
    for (size_t i = 0; i < size_; i++)
      new (&data_[i]) T(other.data_[i]);
  }

  Vec(Vec &&other) : alloc_(other.alloc_) {
    std::swap(data_, other.data_);
    std::swap(size_, other.size_);
    std::swap(capacity_, other.capacity_);
    std::swap(alloc_, other.alloc_);
  }

  Vec(std::initializer_list<T> list, A alloc = A())
      : size_(list.size()), capacity_(list.size()), alloc_(std::move(alloc)) {
    data_ = reinterpret_cast<T *>(alloc_.allocate(size_ * sizeof(T)));
    size_t i = 0;
    for (const T &value : list)
      new (&data_[i++]) T(value);
  }

  Vec &operator=(Vec other) {
    std::swap(data_, other.data_);
    std::swap(size_, other.size_);
    std::swap(capacity_, other.capacity_);
    std::swap(alloc_, other.alloc_);
    return *this;
  }

  void push_back(const T &value) {
    if (size_ == capacity_) {
      reserve(capacity_ == 0 ? 1 : capacity_ * 2);
    }

    new (&data_[size_++]) T(value);
  }

  void reserve(size_t new_capacity) {
    if (new_capacity <= capacity_)
      return;

    T *new_data =
        reinterpret_cast<T *>(alloc_.allocate(new_capacity * sizeof(T)));
    for (size_t i = 0; i < size_; i++)
      new (&new_data[i]) T(std::move(data_[i]));

    for (size_t i = 0; i < size_; i++)
      data_[i].~T();

    alloc_.deallocate(data_, size_ * sizeof(T));

    capacity_ = new_capacity;
    data_ = new_data;
  }

  Option<T> pop() {
    if (size_ == 0) {
      return NONE;
    }

    T value = std::move(data_[size_ - 1]);
    data_[--size_].~T();
    return value;
  }

  T *begin() { return data_; }
  T *end() { return data_ + size_; }
  T *data() { return data_; }

  constexpr T &operator[](size_t index) {
    ENSURE(index < size_, "index out of bounds");
    return data_[index];
  }
  constexpr const T &operator[](size_t index) const {
    ENSURE(index < size_, "index out of bounds");
    return data_[index];
  }

  [[nodiscard]] size_t size() const { return size_; }
  [[nodiscard]] size_t capacity() const { return capacity_; }

  Slice<T> as_slice() const { return Slice<T>(data_, size_); }

  auto iter() const { return as_slice().iter(); }

  void clear() {
    for (size_t i = 0; i < size_; i++)
      data_[i].~T();
    size_ = 0;
  }

  ~Vec() {
    if (!data_)
      return;

    for (size_t i = 0; i < size_; i++)
      data_[i].~T();

    alloc_.deallocate(data_, capacity_ * sizeof(T));
  }

private:
  T *data_;
  size_t size_;
  size_t capacity_;
  A alloc_;
};

} // namespace Atlas
