#pragma once
#include <atlas/alloc.hpp>
#include <atlas/io/traits.hpp>
#include <atlas/string.hpp>
#include <atlas/vec.hpp>

namespace Atlas {

template <Allocator Alloc = DefaultAllocator> class Dot {
public:
  Dot(Alloc alloc = Alloc()) : alloc_(alloc) {}

  struct NodeProperties {
    String color;
    String shape;
    String style;
    String text_color;
  };

  void add_node(String node) { nodes_.push_back(Node{.name = node}); }
  void add_node(String node, NodeProperties props) {
    nodes_.push_back(Node{.name = node, .props = props});
  }

  void add_edge(const String &from, const String &to) {
    edges_.push_back(cons(from, to));
  }

  template <Io::Write<const char> W> Result<None, Io::Error> output(W &writer) {
    TRY(writer.write("digraph{"_sv.as_slice()));

    for (const auto &node : nodes_) {
      TRY(writer.write("\""_sv.as_slice()));
      TRY(writer.write(node.name.as_slice()));
      TRY(writer.write("\""_sv.as_slice()));

      bool first = true;

      if (!node.props.color.empty()) {
        TRY(writer.write("[color="_sv.as_slice()));
        TRY(writer.write(node.props.color.as_slice()));

        first = false;

        if (!node.props.shape.empty() || !node.props.style.empty() ||
            !node.props.text_color.empty()) {
          TRY(writer.write(","_sv.as_slice()));
        }
      }

      if (!node.props.text_color.empty()) {
        if (first) {
          TRY(writer.write("["_sv.as_slice()));
          first = false;
        }

        TRY(writer.write("fontcolor="_sv.as_slice()));
        TRY(writer.write(node.props.text_color.as_slice()));

        first = false;

        if (!node.props.shape.empty() || !node.props.style.empty()) {
          TRY(writer.write(","_sv.as_slice()));
        }
      }

      if (!node.props.shape.empty()) {
        if (first) {
          TRY(writer.write("["_sv.as_slice()));
          first = false;
        }
        TRY(writer.write("shape="_sv.as_slice()));
        TRY(writer.write(node.props.shape.as_slice()));

        first = false;

        if (!node.props.style.empty()) {
          TRY(writer.write(","_sv.as_slice()));
        }
      }

      if (!node.props.style.empty()) {
        if (first) {
          TRY(writer.write("["_sv.as_slice()));
          first = false;
        }
        TRY(writer.write("style="_sv.as_slice()));
        TRY(writer.write(node.props.style.as_slice()));
      }

      if (!first) {
        TRY(writer.write("]"_sv.as_slice()));
      }

      TRY(writer.write(";"_sv.as_slice()));
    }

    for (const auto &edge : edges_) {
      TRY(writer.write("\""_sv.as_slice()));
      TRY(writer.write(edge.first().as_slice()));
      TRY(writer.write("\""_sv.as_slice()));
      TRY(writer.write("->"_sv.as_slice()));
      TRY(writer.write("\""_sv.as_slice()));
      TRY(writer.write(edge.second().as_slice()));
      TRY(writer.write("\";"_sv.as_slice()));
    }

    TRY(writer.write("}"_sv.as_slice()));

    return Ok(NONE);
  }

private:
  struct Node {
    String name;
    NodeProperties props;
  };

  Vec<Node, Alloc> nodes_;
  Vec<Cons<String, String>, Alloc> edges_;

  Alloc alloc_;
};

} // namespace Atlas