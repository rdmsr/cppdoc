#pragma once
#include "error.hpp"
#include <atlas/option.hpp>
#include <atlas/result.hpp>
#include <atlas/slice.hpp>

namespace Atlas::Io {

template <typename T>
concept Read = requires(T a, Slice<uint8_t> buf) {
  { a.read(buf) } -> std::same_as<Result<size_t, Io::Error>>;
};

template <typename T, typename T1 = uint8_t>
concept Write = requires(T a, const Slice<T1> buf) {
  { a.write(buf) } -> std::same_as<Result<size_t, Io::Error>>;
};

//? Maybe move this elsewhere?
enum class Whence {
  Start,
  End,
  Current,
};

struct SeekFrom {
  Whence whence_;
  uint64_t offset_;

  static constexpr SeekFrom start(uint64_t offset) {
    return SeekFrom{Whence::Start, offset};
  }

  static constexpr SeekFrom end(int64_t offset) {
    return SeekFrom{Whence::End, static_cast<uint64_t>(offset)};
  }

  static constexpr SeekFrom current(int64_t offset) {
    return SeekFrom{Whence::Current, static_cast<uint64_t>(offset)};
  }
};

template <typename T>
concept Seek = requires(T a, SeekFrom from) {
  { a.seek(from) } -> std::same_as<Result<uint64_t, Io::Error>>;
};

} // namespace Atlas::Io