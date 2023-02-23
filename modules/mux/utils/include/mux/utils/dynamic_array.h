// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief Mux allocator specialization of cargo::dynamic_array.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef MUX_UTILS_DYNAMIC_ARRAY_H_INCLUDED
#define MUX_UTILS_DYNAMIC_ARRAY_H_INCLUDED

#include "cargo/dynamic_array.h"
#include "mux/utils/allocator.h"

namespace mux {
/// @brief Specialization of `cargo::dynamic_array` using a Mux allocator.
///
/// ```cpp
/// void example(mux_allocator_info_t allocator_info) {
///   mux::dynamic_array<int> ints(allocator_info);
///   // use as normal...
/// }
/// ```
///
/// @tparam T Type of contained elements.
template <class T>
using dynamic_array = cargo::dynamic_array<T, cargo_allocator<T>>;
}  // namespace mux

#endif  // MUX_UTILS_DYNAMIC_ARRAY_H_INCLUDED
