#pragma once
#include "assert.hpp"
#include "base.hpp"
#include "cstr.hpp"
#include "option.hpp"
#include "string_view.hpp"

namespace Atlas {

class String {

public:
  String() : data_(sso_), length_(0) { sso_[0] = '\0'; }

  String(const char *str) : length_(strlen(str)) {
    if (length_ > SSO_CAPACITY) {
      data_ = new char[length_ + 1];
    } else {
      data_ = sso_;
    }
    memcpy(data_, str, length_);
    data_[length_] = '\0';
  }

  String(const String &other) : data_(other.data_), length_(other.length_) {
    if (length_ > SSO_CAPACITY) {
      data_ = new char[length_ + 1];
      memcpy(data_, other.data_, length_);
      data_[length_] = '\0';
    } else {
      data_ = sso_;
      memcpy(data_, other.data_, length_);
      data_[length_] = '\0';
    }
  }

  String(String &&other) : data_(other.data_), length_(other.length_) {
    if (length_ > SSO_CAPACITY) {
      data_ = new char[length_ + 1];
      memcpy(data_, other.data_, length_);
      data_[length_] = '\0';
      delete[] other.data_;
    } else {
      data_ = sso_;
      memcpy(data_, other.data_, length_);
      data_[length_] = '\0';
    }

    other.data_ = nullptr;
    other.length_ = 0;
  }

  String(StringView view) : length_(view.length()) {
    if (length_ > SSO_CAPACITY) {
      data_ = new char[length_ + 1];
    } else {
      data_ = sso_;
    }
    memcpy(data_, view.data(), length_);

    data_[length_] = '\0';
  }

  ~String() {
    if (data_ != sso_) {
      delete[] data_;
    }
  }

  [[nodiscard]] bool operator==(const String &other) const {
    return StringView(data_, length_) == StringView(other.data_, other.length_);
  }

  [[nodiscard]] bool operator==(const char *str) const {
    return StringView(data_, length_) == StringView(str);
  }

  [[nodiscard]] bool operator==(const StringView &view) const {
    return StringView(data_, length_) == view;
  }

  [[nodiscard]] size_t length() const { return length_; }
  [[nodiscard]] bool empty() const { return length_ == 0; }

  void resize(size_t new_size) {
    if (new_size > length_) {
      if (new_size > SSO_CAPACITY) {
        char *new_data = new char[new_size + 1];
        memcpy(new_data, data_, length_);
        new_data[new_size] = '\0';
        if (data_ != sso_) {
          delete[] data_;
        }
        data_ = new_data;
      } else {
        memcpy(sso_, data_, length_);
        sso_[new_size] = '\0';
        data_ = sso_;
      }
    } else {
      data_[new_size] = '\0';
    }
    length_ = new_size;
  }

  [[nodiscard]] const char *data() const { return data_; }
  [[nodiscard]] StringView view() const { return StringView(data_, length_); }

  [[nodiscard]] constexpr Slice<const char> as_slice() const {
    return {data_, length_};
  }

  [[nodiscard]] char &operator[](size_t index) {
    ENSURE(index < length_, "index out of bounds");
    return data_[index];
  }

  [[nodiscard]] Option<char> at(size_t index) {
    if (index >= length_) {
      return NONE;
    }
    return data_[index];
  }

private:
  static constexpr size_t SSO_CAPACITY = 16;

  char *data_;
  size_t length_;

  union {
    size_t capacity_;

    // Small String Optimization (SSO)
    char sso_[SSO_CAPACITY];
  };
};

} // namespace Atlas
