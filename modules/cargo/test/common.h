// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Shared machinery for testing cargo.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef CARGO_COMMON_H_INCLUDED
#define CARGO_COMMON_H_INCLUDED

#include <utility>

struct copyable_t {
  copyable_t(int value) : value(value) {}
  copyable_t(const copyable_t &other) { value = other.value; }
  copyable_t(copyable_t &&) = delete;
  ~copyable_t() {}
  copyable_t &operator=(const copyable_t &other) {
    value = other.value;
    return *this;
  }
  int get() const { return value; }
 private:
  int value;
};

struct movable_t {
  movable_t(int value) : value(value) {}
  movable_t(const movable_t &) = delete;
  movable_t(movable_t &&other) { value = other.value; }
  ~movable_t() {}
  movable_t &operator=(movable_t &&other) {
    value = std::move(other.value);
    return *this;
  }
  int get() const { return value; }

 private:
  int value;
};

#endif  // CARGO_COMMON_H_INCLUDED
