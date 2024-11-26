#pragma once
#include "error.hpp"
#include "iter.hpp"
#include "result.hpp"

#include <cstddef>

namespace Atlas {

template <typename T> struct ListNode {
  T *prev = nullptr;
  T *next = nullptr;
};

template <typename T, ListNode<T> T::*N> class List {
public:
  List() : tail_(nullptr), head_(nullptr) {}

  [[nodiscard]] T *tail() const { return tail_; }
  [[nodiscard]] T *head() const { return head_; }

  Result<> insert_head(T *elem) {
    if (!elem) {
      return Err(Error::InvalidParameters);
    }

    ListNode<T> *node = &(elem->*N);

    if (node->next || node->prev) {
      return Err(Error::InvalidParameters);
    }

    node->prev = nullptr;
    node->next = head_;

    if (head_) {
      ListNode<T> *head_node = &(head_->*N);
      head_node->prev = elem;
    }

    head_ = elem;

    if (!tail_) {
      tail_ = head_;
    }
    length_++;

    return Ok(NONE);
  }

  Result<> insert_tail(T *elem) {
    if (!tail_ || !head_) {
      TRY(insert_head(elem));
    } else {
      ListNode<T> *node = &(elem->*N);
      ListNode<T> *tail_node = &(tail_->*N);

      if (!elem)
        return Err(Error::InvalidParameters);

      if (node->next || node->prev)
        return Err(Error::InvalidParameters);

      node->prev = tail_;
      node->next = nullptr;

      tail_node->next = elem;

      tail_ = elem;
      length_++;
    }

    return Ok(NONE);
  }

  Result<> insert_before(T *elem, T *before) {
    ListNode<T> *node = &(elem->*N);
    ListNode<T> *before_node = &(before->*N);

    if (!before || !elem)
      return Err(Error::InvalidParameters);

    if (!before_node->prev && !before_node->next && head_ != before) {
      return Err(Error::InvalidParameters);
    }

    node->next = before;
    node->prev = before_node->prev;

    if (node->prev) {
      ListNode<T> *prev_node = &(node->prev->*N);
      prev_node->next = elem;
    }

    before_node->prev = elem;

    return Ok(NONE);
  }

  Result<> insert_after(T *elem, T *after) {
    ListNode<T> *node = &(elem->*N);
    ListNode<T> *after_node = &(after->*N);

    if (!after || !elem)
      return Err(Error::InvalidParameters);

    node->prev = after;
    node->next = after_node->next;
    after_node->next = node;

    if (node->next) {
      ListNode<T> *next_node = &(node->next->*N);
      next_node->prev = elem;
    }

    return Ok(NONE);
  }

  Result<T *, Error> remove_tail() { return remove(tail_); }
  Result<T *, Error> remove_head() { return remove(head_); }

  Result<T *, Error> remove(T *elem) {
    if (!elem)
      return Err(Error::InvalidParameters);

    auto node = &(elem->*N);

    auto next = node->next;
    auto prev = node->prev;

    if (elem == head_) {
      head_ = next;
    }
    if (elem == tail_) {
      tail_ = prev;
    }

    if (prev) {
      auto prev_node = &(prev->*N);
      prev_node->next = next;
    }

    if (next) {
      auto next_node = &(next->*N);
      next_node->prev = prev;
    }

    node->next = nullptr;
    node->prev = nullptr;

    length_--;

    return Ok(elem);
  }

  auto iter() {
    auto next = [this, current = head_]() mutable -> Option<T *> {
      if (!current) {
        return NONE;
      }
      auto node = &(current->*N);
      current = node->next;

      return current;
    };

    return Iterator<decltype(next)>(next);
  }

  [[nodiscard]] size_t length() const { return length_; }

  void reset() {
    head_ = nullptr;
    tail_ = nullptr;
    length_ = 0;
  }

private:
  T *tail_ = nullptr;
  T *head_ = nullptr;
  size_t length_ = 0;
};
} // namespace Atlas