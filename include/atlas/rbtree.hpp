#pragma once
#include "iter.hpp"
#include <cstddef>

// This implementation of a red-black tree is mostly based on 'Introduction to
// Algorithms' by Cormen et al.

namespace Atlas {

enum class RBColor { Red, Black };

template <typename T> struct RBTreeNode {
  T *parent;

  T *left = nullptr;
  T *right = nullptr;

  RBColor color = RBColor::Black;
};

template <typename T, RBTreeNode<T> T::*N, typename U, U T::*Key>
  requires(Sortable<U>)
class RBTree {

public:
  RBTree()
      : nil_({
            .parent = nullptr,
            .left = nullptr,
            .right = nullptr,
            .color = RBColor::Black,
        }) {}

  [[nodiscard]] T *root() const { return root_; }

  [[nodiscard]] bool empty() const { return root_ == nullptr; }

  [[nodiscard]] bool is_red(T *n) { return h(n)->color == RBColor::Red; }

  [[nodiscard]] bool is_black(T *n) { return h(n)->color == RBColor::Black; }

  void insert(T *to_insert) {
    auto new_node = h(to_insert);

    auto x = root_;
    auto x_node = h(x);

    T *y = nullptr;

    while (!is_nil(x)) {
      y = x;
      if (key(to_insert) < key(x)) {
        x = x_node->left;
        x_node = h(x);
      } else {
        x = x_node->right;
        x_node = h(x);
      }
    }

    new_node->parent = y;

    if (is_nil(y)) {
      root_ = to_insert;
    } else if (key(to_insert) < key(y)) {
      h(y)->left = to_insert;
    } else {
      h(y)->right = to_insert;
    }

    new_node->left = nullptr;
    new_node->right = nullptr;

    new_node->color = RBColor::Red;

    insert_fixup(to_insert);
  }

  void remove(T *node) {
    auto n = h(node);
    auto y = n;
    auto x = node;
    auto y_orig_color = y->color;

    if (is_nil(n->left)) {
      x = n->right;
      transplant(node, n->right); // Replace n with its right child
    } else if (is_nil(n->right)) {
      x = n->left;
      transplant(node, n->left); // Replace n with its left child
    } else {
      y = h(minimum(n->right));

      y_orig_color = y->color;
      x = y->right;

      if (y != h(n->right)) {
        transplant(raw(y), y->right);
        y->right = n->right;
        h(y->right)->parent = raw(y);
      } else {
        h(x)->parent = raw(y);
      }

      transplant(node, raw(y));
      y->left = n->left;
      h(y->left)->parent = raw(y);
      y->color = n->color;
    }

    if (y_orig_color == RBColor::Black) {
      remove_fixup(x);
    }
  }

  // The minimum element of a tree is its leftmost element
  [[nodiscard]] T *minimum(T *root) const {
    auto x = root;
    auto x_node = h(x);

    while (!is_nil(x_node->left)) {
      x = x_node->left;
      x_node = h(x);
    }

    return x;
  }

  [[nodiscard]] T *maximum(T *root) const {
    auto x = root;
    auto x_node = h(x);

    while (!is_nil(x_node->right)) {
      x = x_node->right;
      x_node = h(x);
    }

    return x;
  }

  template <typename K> [[nodiscard]] Option<T *> find(K key) const {
    auto x = root_;
    auto x_node = h(x);

    while (!is_nil(x)) {
      if (key == this->key(x)) {
        return x;
      } else if (key < this->key(raw(x_node))) {
        x = x_node->left;
        x_node = h(x);
      } else {
        x = x_node->right;
        x_node = h(x);
      }
    }

    return NONE;
  }

  /// Return an iterator over the tree's contents
  /// NOTE: Iteration is done in order
  [[nodiscard]] auto iter() const {
    auto next_func = [this, current = minimum(root_)]() mutable -> Option<T *> {
      if (is_nil(current)) {
        return NONE;
      }

      auto ret = current;
      auto current_node = h(current);

      if (!is_nil(current_node->right)) {
        current = minimum(current_node->right);
      } else {
        auto y = current_node->parent;
        while (!is_nil(y) && current == h(y)->right) {
          current = y;
          y = h(y)->parent;
        }
        current = y;
      }

      return ret;
    };

    auto prev_func = [this, current = maximum(root_)]() mutable -> Option<T *> {
      if (is_nil(current)) {
        return NONE;
      }

      auto ret = current;
      auto current_node = h(current);

      if (!is_nil(current_node->left)) {
        current = maximum(current_node->left);
      } else {
        auto y = current_node->parent;
        while (!is_nil(y) && current == h(y)->left) {
          current = y;
          y = h(y)->parent;
        }
        current = y;
      }

      return ret;
    };

    return Iterator<decltype(next_func), decltype(prev_func)>(next_func,
                                                              prev_func);
  }

private:
  [[nodiscard]] RBTreeNode<T> *h(T *n) const {
    if (!n)
      return const_cast<RBTreeNode<T> *>(&nil_);
    return &(n->*N);
  }

  inline RBTreeNode<T> *parent(T *n) { return h(h(n)->parent); }
  inline RBTreeNode<T> *parent(RBTreeNode<T> *n) { return h(n->parent); }

  inline RBTreeNode<T> *grandparent(RBTreeNode<T> *n) {
    return h(h(n->parent)->parent);
  }

  // This is a bit like container_of
  inline T *raw(RBTreeNode<T> *n) const {
    auto off = (size_t)(&((T *)nullptr->*N));
    return reinterpret_cast<T *>((char *)n - off);
  }

  inline auto key(T *n) const { return *(&(n->*Key)); }

  void insert_fixup(T *to_insert) {
    auto node = h(to_insert);

    while (parent(node)->color == RBColor::Red) {

      // Is the parent of the node a left child?
      if (node->parent == grandparent(node)->left) {
        auto y = h(grandparent(node)->right);

        // If parent and uncle are both red, make them both black and
        // grandparent red, as a red node cannot have a red parent
        if (y->color == RBColor::Red) {
          parent(node)->color = RBColor::Black;

          y->color = RBColor::Black;

          grandparent(node)->color = RBColor::Red;

          to_insert = raw(grandparent(node));

          node = grandparent(node);
        } else {

          // If the parent is a right child, rotate left to make it a left child
          // and then rotate right to fix the tree
          if (to_insert == parent(node)->right) {
            to_insert = node->parent;
            node = parent(node);
            rotate_left(to_insert);
          }

          parent(node)->color = RBColor::Black;
          grandparent(node)->color = RBColor::Red;

          rotate_right(raw(grandparent(node)));
        }
      }

      // This code is symmetric to the code above
      else {
        auto y = h(grandparent(node)->left);

        if (y->color == RBColor::Red) {
          parent(node)->color = RBColor::Black;

          y->color = RBColor::Black;

          grandparent(node)->color = RBColor::Red;

          to_insert = raw(grandparent(node));

          node = grandparent(node);
        } else {
          if (to_insert == parent(node)->left) {
            to_insert = node->parent;
            node = parent(node);
            rotate_right(to_insert);
          }

          parent(node)->color = RBColor::Black;
          grandparent(node)->color = RBColor::Red;

          rotate_left(raw(grandparent(node)));
        }
      }
    }

    h(root_)->color = RBColor::Black;
  }

  void transplant(T *u, T *v) {
    auto u_node = h(u);
    auto v_node = h(v);

    if (is_nil(h(u)->parent)) {
      root_ = v;
    } else if (u == parent(u)->left) {
      parent(u)->left = v;
    } else {
      parent(u)->right = v;
    }

    v_node->parent = u_node->parent;
  }

  [[nodiscard]] bool is_nil(T *n) const { return h(n) == &nil_; }

  void rotate_left(T *n) {
    auto node = h(n);

    auto y_raw = node->right;

    auto y = h(node->right);
    node->right = y->left;

    if (!is_nil(y->left)) {
      h(y->left)->parent = n;
    }

    y->parent = node->parent;

    if (is_nil(node->parent)) {
      root_ = y_raw;
    } else if (n == parent(node)->left) {
      parent(node)->left = y_raw;
    } else {
      parent(node)->right = y_raw;
    }

    y->left = n;
    node->parent = y_raw;
  }

  void rotate_right(T *n) {
    auto node = h(n);

    auto y_raw = node->left;

    auto y = h(node->left);
    node->left = y->right;

    if (!is_nil(y->right)) {
      h(y->right)->parent = n;
    }

    y->parent = node->parent;

    if (is_nil(node->parent)) {
      root_ = y_raw;
    } else if (n == parent(node)->left) {
      parent(node)->left = y_raw;
    } else {
      parent(node)->right = y_raw;
    }

    y->right = n;
    node->parent = y_raw;
  }

  void remove_fixup(T *x) {
    while (x != root_ && h(x)->color == RBColor::Black) {
      if (x == parent(x)->left) {
        auto w = h(parent(x)->right);
        if (w->color == RBColor::Red) {
          w->color = RBColor::Black;
          parent(x)->color = RBColor::Red;
          rotate_left(h(x)->parent);
          w = h(parent(x)->right);
        }

        if (h(w->left)->color == RBColor::Black &&
            h(w->right)->color == RBColor::Black) {
          w->color = RBColor::Red;
          x = h(x)->parent;
        } else {
          if (h(w->right)->color == RBColor::Black) {
            h(w->left)->color = RBColor::Black;
            w->color = RBColor::Red;
            rotate_right(raw(w));
            w = h(parent(x)->right);
          }

          w->color = parent(x)->color;
          parent(x)->color = RBColor::Black;
          h(w->right)->color = RBColor::Black;
          rotate_left(h(x)->parent);
          x = root_;
        }
      }

      else {
        auto w = h(parent(x)->left);

        if (w->color == RBColor::Red) {
          w->color = RBColor::Black;
          parent(x)->color = RBColor::Red;
          rotate_right(h(x)->parent);
          w = h(parent(x)->left);
        }

        if (h(w->left)->color == RBColor::Black &&
            h(w->right)->color == RBColor::Black) {
          w->color = RBColor::Red;
          x = h(x)->parent;
        } else {
          if (h(w->left)->color == RBColor::Black) {
            h(w->right)->color = RBColor::Black;
            w->color = RBColor::Red;
            rotate_left(raw(w));
            w = h(parent(x)->left);
          }

          w->color = parent(x)->color;
          h(w->left)->color = RBColor::Black;
          parent(x)->color = RBColor::Black;
          rotate_right(h(x)->parent);
          x = root_;
        }
        h(x)->color = RBColor::Black;
      }
    }

    h(x)->color = RBColor::Black;
  }

  T *root_ = nullptr;
  const RBTreeNode<T> nil_;
};

} // namespace Atlas
