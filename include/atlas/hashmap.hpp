#pragma once
#include "alloc.hpp"
#include "hash.hpp"
#include "result.hpp"

namespace Atlas {

/// An open addressing hash map
template <typename K, typename V, Allocator A = DefaultAllocator,
          typename H = Hash<K>>
class HashMap {

public:
  HashMap(A alloc = A(), H hasher = H()) : alloc_(alloc), hasher_(hasher) {
    size_ = 0;
    capacity_ = 8;
    buckets_ = reinterpret_cast<Option<Bucket> *>(
        alloc_.allocate(capacity_ * sizeof(Option<Bucket>)));
    for (size_t i = 0; i < capacity_; i++) {
      buckets_[i] = NONE;
    }
  }

  HashMap(size_t capacity, A alloc = A(), H hasher = H())
      : size_(0), capacity_(capacity), alloc_(alloc), hasher_(hasher) {
    buckets_ = reinterpret_cast<Option<Bucket> *>(
        alloc_.allocate(capacity_ * sizeof(Option<Bucket>)));
    for (size_t i = 0; i < capacity; i++) {
      buckets_[i] = NONE;
    }
  }

  ~HashMap() {
    for (size_t i = 0; i < capacity_; i++) {
      buckets_[i].~Option();
    }
    alloc_.deallocate(buckets_, capacity_ * sizeof(Option<Bucket>));
  }

  [[nodiscard]] size_t size() const { return size_; }

  [[nodiscard]] bool empty() const { return size_ == 0; }

  Result<> insert(K key, V value) {
    // We need to resize the map, so rehash all the elements
    if (size_ == capacity_) {
      capacity_ *= 2;
      auto new_buckets = reinterpret_cast<Option<Bucket> *>(
          alloc_.allocate(capacity_ * sizeof(Option<Bucket>)));

      for (size_t i = 0; i < capacity_; i++) {
        new_buckets[i] = NONE;
      }

      for (size_t i = 0; i < size_; i++) {
        if (buckets_[i].is_some()) {
          size_t index = index_for_hash(hasher_(buckets_[i]->key));

          while (new_buckets[index].is_some()) {
            index = (index + 1) % capacity_;
          }

          new_buckets[index] = buckets_[i];
        }

        buckets_[i].~Option();
      }

      alloc_.deallocate(buckets_, size_);

      buckets_ = new_buckets;
    }

    // Do linear probing: find the next empty slot
    size_t index = index_for_hash(hasher_(key));

    if (buckets_[index].is_some() && buckets_[index]->key == key) {
      return Err(Error::Duplicate);
    }

    while (buckets_[index].is_some()) {
      if (buckets_[index]->key == key) {
        return Err(Error::Duplicate);
      }

      index = (index + 1) % capacity_;
    }

    buckets_[index] = Bucket{key, value};
    size_++;

    return Ok(NONE);
  }

  [[nodiscard]] Option<V> get(K key) const {
    size_t index = index_for_hash(hasher_(key));
    size_t mark = index;

    while (buckets_[index].is_some() && buckets_[index].unwrap().key != key) {
      index = (index + 1) % capacity_;

      // We've looped around and haven't found the key, so return NONE
      if (index == mark) {
        return NONE;
      }
    }

    if (buckets_[index].is_none()) {
      return NONE;
    }

    return buckets_[index].unwrap().value;
  }

  Result<> remove(K key) {
    size_t index = index_for_hash(hasher_(key));
    size_t mark = index;

    while (buckets_[index].is_some() && buckets_[index].unwrap().key != key) {
      index = (index + 1) % capacity_;

      if (index == mark) {
        return Err(Error::NotFound);
      }
    }

    if (buckets_[index].is_none()) {
      return Err(Error::NotFound);
    }

    buckets_[index] = NONE;
    size_--;

    return Ok(NONE);
  }

  void clear() {
    for (size_t i = 0; i < capacity_; i++) {
      buckets_[i] = NONE;
    }
    size_ = 0;
  }

  [[nodiscard]] V operator[](K key) const { return get(key).unwrap(); }

private:
  struct Bucket {
    K key;
    V value;
  };

  size_t size_;
  size_t capacity_;
  A alloc_;
  H hasher_;

  Option<Bucket> *buckets_;

  [[nodiscard]] size_t index_for_hash(uint64_t hash) const {
    return hash % capacity_;
  }
};

} // namespace Atlas
