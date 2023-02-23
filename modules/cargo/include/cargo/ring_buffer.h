// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Fixed sized ring buffer.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef CARGO_RING_BUFFER_H_INCLUDED
#define CARGO_RING_BUFFER_H_INCLUDED

#include <cargo/attributes.h>
#include <cargo/error.h>

#include <algorithm>
#include <array>
#include <limits>

namespace cargo {
/// @addtogroup cargo
/// @{

/// @brief Fixed sized ring buffer implementation.
///
/// @tparam T Element type of the ring buffer.
/// @tparam N The number of elements in the array, N must be greater than one, a
/// power of two, and less than std::numeric_limits<uint32_t>::max()!
template <class T, uint32_t N>
class ring_buffer final {
 public:
  using value_type = T;
  using size_type = uint32_t;
  using const_reference = const value_type&;

  static_assert(1 < N, "N must be greater than one!");
  static_assert((0 == (N & (N - 1))), "N must be a power of two!");
  static_assert((N < std::numeric_limits<size_type>::max()),
                "N must be less than std::numeric_limits<uint32_t>::max()!");

  ring_buffer(const ring_buffer&) = delete;
  ring_buffer& operator=(const ring_buffer& other) = delete;
  ring_buffer() = default;

  /// @brief Add an item to the ring buffer.
  /// @param t The item to add.
  /// @return Returns `cargo::overflow` if ring buffer is full,
  /// `cargo::success` otherwise.
  CARGO_NODISCARD cargo::result enqueue(value_type&& t) {
    // Check if our indices match (which means we're either empty or full)
    if (enqueue_index == dequeue_index) {
      if (empty) {
        empty = false;
      } else {
        return cargo::result::overflow;
      }
    }

    const size_type next_index = (enqueue_index + 1) & mask;

    // store the item into our ring buffer
    payload[enqueue_index] = std::move(t);

    // move the enqueue index on
    enqueue_index = next_index;

    return cargo::result::success;
  }

  /// @brief Add an item to the ring buffer.
  /// @param t The item to add.
  /// @return Returns `cargo::out_of_bounds` if ring buffer is full,
  /// `cargo::success` otherwise.
  CARGO_NODISCARD cargo::result enqueue(const_reference t) {
    // Check if our indices match (which means we're either empty or full)
    if (enqueue_index == dequeue_index) {
      if (empty) {
        empty = false;
      } else {
        return cargo::result::out_of_bounds;
      }
    }

    const size_type next_index = (enqueue_index + 1) & mask;

    // copy the item into our ring buffer
    payload[enqueue_index] = t;

    // move the enqueue index on
    enqueue_index = next_index;

    return cargo::result::success;
  }

  /// @brief Remove an item from the ring buffer.
  /// @return Returns an item if succeeded, otherwise
  /// cargo::result::out_of_bounds.
  CARGO_NODISCARD cargo::error_or<value_type> dequeue() {
    // if we're empty, bail out
    if (empty) {
      return cargo::result::out_of_bounds;
    }

    const size_type old_index = dequeue_index;

    // move the dequeue index on
    dequeue_index = (dequeue_index + 1) & mask;

    // if we've emptied the ring buffer, remember so!
    empty = (dequeue_index == enqueue_index);

    return std::move(payload[old_index]);
  }

 private:
  static const uint32_t mask = N - 1;

  /// @brief The data backing the ring buffer.
  std::array<value_type, N> payload;

  /// @brief The index where we can enqueue new items into the ring buffer.
  size_type enqueue_index = 0;

  /// @brief The index where we can dequeue old items from the ring buffer.
  size_type dequeue_index = 0;

  /// @brief Whether the ring buffer is empty or not.
  /// This allows us to differentiate the (enqueue_index == dequque_index) case.
  bool empty = true;
};

/// @}
}  // cargo

#endif  // CARGO_RING_BUFFER_H_INCLUDED
