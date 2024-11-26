#pragma once
#include "option.hpp"
#include <cstddef>

namespace Atlas {

template <typename T> struct PairingHeapNode {
  T *child;
  T *next;

  // This points to the parent node if this is the leftmost child of the parent
  T *previous;
};

template <typename T, PairingHeapNode<T> T::*N, auto Compare>
class PairingHeap {
public:
  PairingHeap() : root_(nullptr) {}

  void insert(T *elem) {
    size_++;
    root_ = meld(root_, elem);
  }

  [[nodiscard]] T *top() const { return root_; }

  Option<T *> pop() {
    if (root_ == nullptr) {
      return NONE;
    }

    T *result = root_;

    if (h(root_)->child == nullptr) {
      root_ = nullptr;
      size_--;
      return result;
    }

    root_ = merge_pairs(h(root_)->child);

    h(result)->child = nullptr;
    size_--;

    return result;
  }

  [[nodiscard]] size_t size() const { return size_; }

private:
  T *root_;
  size_t size_ = 0;

  T *meld(T *heap1, T *heap2) {
    if (heap1 == nullptr) {
      return heap2;
    } else if (heap2 == nullptr) {
      return heap1;
    }

    if (Compare(heap1, heap2)) {
      h(heap2)->next = h(heap1)->child;
      h(heap1)->child = heap2;
      h(heap2)->previous = heap1;
      return heap1;
    }

    else {
      h(heap1)->next = h(heap2)->child;
      h(heap2)->child = heap1;
      h(heap1)->previous = heap2;
      return heap2;
    }

    // This should never be reached
    return nullptr;
  }

  PairingHeapNode<T> *h(T *heap) const { return &(heap->*N); }

  T *merge_pairs(T *node) {
    if (!node || h(node)->next == nullptr) {
      return node;
    }

    T *first = node;
    T *second = h(node)->next;
    T *rest = h(second)->next;

    h(first)->next = nullptr;
    h(second)->next = nullptr;

    return meld(meld(first, second), merge_pairs(rest));
  }
};

} // namespace Atlas
