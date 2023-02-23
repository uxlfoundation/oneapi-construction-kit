// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief 
///
/// @copyright
/// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef CARGO_MEMORY_H_INCLUDED
#define CARGO_MEMORY_H_INCLUDED

#include <memory>

namespace cargo {

/// @brief Reimplementation of the C++17 std::uninitialized_move
template <class InputIterator, class OutputIterator>
InputIterator uninitialized_move(InputIterator first, InputIterator last,
                                 OutputIterator dst_first) {
  for (; first != last; ++first, ++dst_first) {
    new (static_cast<void *>(std::addressof(*dst_first)))
        typename std::iterator_traits<InputIterator>::value_type(
            std::move(*first));
  }
  return first;
}

}  // namespace cargo

#endif  // CARGO_MEMORY_H_INCLUDED
