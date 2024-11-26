#pragma once
#include <concepts>
#include <cstddef>

namespace Atlas {

template <typename Alloc>
concept Allocator = requires(Alloc a, size_t size) {
  { a.allocate(size) } -> std::same_as<void *>;
  { a.deallocate(a.allocate(size), size) };
};

struct DefaultAllocator {
  static void *allocate(const size_t count) {
    return (void *)(new char[count]);
  }

  static void deallocate(void *ptr, const size_t count) {
    (void)count;
    delete[] (char *)ptr;
  }
};

} // namespace Atlas
