#pragma once
#include "alloc.hpp"
#include "cstr.hpp"
#include "rbtree.hpp"
#include "result.hpp"
#include "string.hpp"
#include "string_view.hpp"
#include "traits.hpp"

namespace Atlas {

template <typename T> struct MapKey {};

template <typename T>
  requires(Sortable<T>)
struct MapKey<T> {
  T val;
  int operator<=>(const MapKey &other) const { return val <=> other.val; }
};

template <> struct MapKey<const char *> {
  const char *val;
  int operator<=>(const MapKey<const char *> other) const {
    return strcmp(val, other.val);
  }

  bool operator==(const MapKey<const char *> other) const {
    return strcmp(val, other.val) == 0;
  }
};

template <> struct MapKey<String> {
  String val;
  int operator<=>(const MapKey<String> &other) const {
    return strcmp(val.data(), other.val.data());
  }

  bool operator==(const MapKey<String> &other) const {
    return val == other.val;
  }
};

template <> struct MapKey<StringView> {
  StringView val;
  int operator<=>(const MapKey<StringView> &other) const {
    return strcmp(val.data(), other.val.data());
  }

  bool operator==(const MapKey<StringView> &other) const {
    return val == other.val;
  }
};

template <typename K, typename V, Allocator A = DefaultAllocator> class Map {

public:
  Map(A alloc = A()) : tree_(), alloc_(alloc) {}

  [[nodiscard]] size_t size() const { return size_; }

  [[nodiscard]] bool empty() const { return size_ == 0; }

  Result<> insert(K key, V value) {
    if (tree_.find(MapKey<K>{key})) {
      return Err(Error::Duplicate);
    }

    auto node = reinterpret_cast<MapNode *>(alloc_.allocate(sizeof(MapNode)));

    *node = MapNode{.key = {key}, .value = value};

    tree_.insert(node);

    size_++;

    return Ok(NONE);
  }

  [[nodiscard]] Option<V> get(K key) const {
    auto ret = tree_.find(MapKey<K>{key});

    if (!ret)
      return NONE;

    return ret.unwrap()->value;
  }

  Result<> remove(K key) {
    auto ret = tree_.find(MapKey<K>{key});

    if (!ret)
      return Err(Error::NotFound);

    auto node = ret.unwrap();

    tree_.remove(node);

    alloc_.deallocate(node, sizeof(MapNode));

    size_--;

    return Ok(NONE);
  }

  void clear() {
    for (auto node : tree_.iter()) {
      tree_.remove(node);
      node->~MapNode();
      alloc_.deallocate(node, sizeof(MapNode));
    }

    size_ = 0;
  }

  ~Map() { clear(); }

  [[nodiscard]] V operator[](K key) const { return get(key).unwrap(); }

  auto iter() {
    auto iterator_modifier = [](auto iter) {
      auto next_func = [iter]() mutable -> Option<Cons<K, V>> {
        auto ret = iter.next();
        if (!ret) {
          return NONE;
        }

        auto node = ret.unwrap();
        return cons(node->key.val, node->value);
      };

      return Iterator<decltype(next_func)>(next_func);
    };

    return (tree_.iter() | iterator_modifier);
  }

private:
  struct MapNode {
    MapKey<K> key;
    V value;
    RBTreeNode<MapNode> hook;
  };

  size_t size_;
  RBTree<MapNode, &MapNode::hook, MapKey<K>, &MapNode::key> tree_;
  A alloc_;
};

} // namespace Atlas