// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef VK_SMALL_VECTOR_H_INCLUDED
#define VK_SMALL_VECTOR_H_INCLUDED

#include <cargo/small_vector.h>
#include <vk/allocator.h>

namespace vk {
template <class T, size_t N>
using small_vector = cargo::small_vector<T, N, cargo_allocator<T>>;
}  // namespace vk

#endif  // VK_SMALL_VECTOR_H_INCLUDED
