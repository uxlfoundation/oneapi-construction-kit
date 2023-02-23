// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief A vector of fixed capacity.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef CARGO_FIXED_VECTOR_H_INCLUDED
#define CARGO_FIXED_VECTOR_H_INCLUDED

#include <cargo/small_vector.h>

namespace cargo {

/// @brief A vector with fixed capacity storage.
///
/// Using the `cargo::nullacator` "allocator" means no free store allocations
/// can be made, this results in a vector of fixed capacity which is the size
/// of `cargo::small_vector`'s small buffer optimization. `cargo::fixed_vector`
/// is therefore a vector with the storage guarantees of `std::array`.
///
/// @tparam T Type of the contained elements.
/// @tparam N Number of elements at maximum capacity.
template <class T, size_t N>
using fixed_vector = cargo::small_vector<T, N, cargo::nullacator<T>>;

}  // namespace cargo

#endif  // CARGO_FIXED_VECTOR_H_INCLUDED
