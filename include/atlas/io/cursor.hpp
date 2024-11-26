#pragma once
#include "endian.hpp"
#include "traits.hpp"
#include <atlas/array.hpp>
#include <atlas/result.hpp>
#include <atlas/slice.hpp>
#include <cstdint>

namespace Atlas::Io {

template <typename T>
  requires(AsSlice<T, uint8_t>)
class Cursor {
public:
  constexpr Cursor(T &data) : data_(data.as_slice()) {}

  constexpr Result<uint64_t, Io::Error> seek(SeekFrom from) {
    auto size = data_.size();
    uint64_t new_pos = 0;

    switch (from.whence_) {
    case Whence::Start:
      new_pos = from.offset_;
      break;

    case Whence::End:
      new_pos = size + static_cast<int64_t>(from.offset_);
      break;

    case Whence::Current:
      new_pos = pos_ + static_cast<int64_t>(from.offset_);
      break;
    }

    if (static_cast<ssize_t>(new_pos) < 0 || new_pos > size) {
      return Err(Error::OutOfBounds);
    }

    pos_ = new_pos;

    return Ok(pos_);
  }

  constexpr Result<size_t, Io::Error> read(Slice<uint8_t> &&buf) {
    auto size = data_.size();
    auto buf_size = buf.size();

    if (pos_ >= size) {
      return Err(Error::EndOfFile);
    }

    size_t bytes_to_read = buf_size;
    if (pos_ + buf_size > size) {
      bytes_to_read = size - pos_;
    }

    for (size_t i = 0; i < bytes_to_read; i++) {
      buf[i] = data_[pos_ + i];
    }

    pos_ += bytes_to_read;

    return Ok(bytes_to_read);
  }

  template <typename U, Endianness E> constexpr Result<U, Io::Error> read() {
    Array<uint8_t, sizeof(U)> buf{};
    TRY(read(buf.as_slice()));

    return Ok(bytes_to<U, E>(buf));
  }

  template <typename U, Endianness E>
  constexpr Result<size_t, Io::Error> write(U value) {
    auto arr = to_bytes<U, E>(value);
    return write(arr.as_slice());
  }

  constexpr Result<size_t, Io::Error> write(const Slice<uint8_t> &&buf) {
    auto size = data_.size();
    auto buf_size = buf.size();

    if (pos_ >= size) {
      return Ok(0ul);
    }

    size_t bytes_to_write = buf_size;
    if (pos_ + buf_size > size) {
      bytes_to_write = size - pos_;
    }

    for (size_t i = 0; i < bytes_to_write; i++) {
      data_[pos_ + i] = buf[i];
    }

    pos_ += bytes_to_write;

    return Ok(bytes_to_write);
  }

  [[nodiscard]] constexpr uint64_t position() const { return pos_; }
  constexpr void set_position(uint64_t n) { pos_ = n; }

  constexpr Slice<uint8_t> slice(uint64_t offset, size_t size) {
    return data_.sub_slice(offset, offset + size).unwrap();
  }

  static constexpr Cursor<T> make(T data) { return Cursor<T>(data); }

private:
  Slice<uint8_t> data_;
  uint64_t pos_{};
};

} // namespace Atlas::Io