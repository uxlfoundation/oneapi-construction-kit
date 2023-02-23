// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief
///
/// @copyright
/// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef MUX_UTILS_SMALL_VECTOR_H_INCLUDES
#define MUX_UTILS_SMALL_VECTOR_H_INCLUDES

#include <cargo/small_vector.h>
#include <mux/utils/allocator.h>

namespace mux {
/// @brief Specialization of `cargo::small_vector` using a Mux allocator.
///
/// ```cpp
/// void example(mux_allocator_info_t allocator_info) {
///   mux::small_vector<int, 8> ints(allocator_info);
///   // use as normal...
/// }
/// ```
///
/// @see `cargo::small_vector<T, N, A>` documentation.
///
/// @tparam T Type of contained elements.
/// @tparam N Capacity of the embedded storage.
template <class T, size_t N>
using small_vector = cargo::small_vector<T, N, cargo_allocator<T>>;
}  // namespace mux

#endif  // MUX_UTILS_SMALL_VECTOR_H_INCLUDES
