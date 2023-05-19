// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <cargo/allocator.h>
#include <cargo/error.h>

void *cargo::alloc(size_t size, size_t alignment) {
  CARGO_ASSERT(alignment, "align must not be zero!");
  if (alignment == 1) {
    // Promote alignment of 1 byte to the requirements of aligned allocators.
    alignment = sizeof(void *);
  }
  // Due to the platform specific aligned allocation constraints stated below
  // the alignment should be a power of two, also the new alignment must be a
  // multiple of alignment and also a multiple of sizeof(void*) so keep raising
  // to the power two until this holds true.
  size_t align = alignment;
  while (align % alignment || align % sizeof(void *)) {
    // See the link for rounding up to next power of 2.
    // https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
    // The initial decrement is not performed in order to continue raising to
    // the next power to two whilst the condition is not met.
    align |= align >> 1;
    align |= align >> 2;
    align |= align >> 4;
    align |= align >> 8;
    align |= align >> 16;
#if defined(__LP64__) || defined(_WIN64) || defined(__x86_64__) || \
    defined(_M_X64) || defined(__ia64) || defined(_M_IA64) ||      \
    defined(__aarch64__) || defined(__powerpc64__)
    align |= align >> 32;
#else
    // All 64-bit targets should have taken the other path of the #ifdef, so
    // lets make sure that we are a 32-bit target here.  Just using an if
    // statement instead of the #ifdef + static_assert is not possible because
    // 32-bit compilers will still warn on the 32-bit shift on a 32-bit value
    // even though it would be dead-code on 32-bit targets.
    static_assert(sizeof(void *) < 8, "Invalid alignment assumption");
#endif
    align++;
  }
  void *pointer = nullptr;
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
  // The MSVC documentation for _aligned_alloc states that the alignment must
  // be an integral power of 2.
  pointer = _aligned_malloc(size, align);
#elif defined __ANDROID__
  pointer = memalign(align, size);
#else
  // The Open Group documentation for posix_memalign states that the alignment
  // must be a multiple of sizeof(void*).
  if (posix_memalign(&pointer, align, size)) {
    return nullptr;  // GCOVR_EXCL_LINE non-deterministic? I don't know why.
  }
#endif
  return pointer;
}

void cargo::free(void *pointer) {
#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
  _aligned_free(pointer);
#else
  ::free(pointer);
#endif
}
