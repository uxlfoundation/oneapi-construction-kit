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

#include <stdio.h>
#include <vk/allocator.h>
#include <vk/error.h>

#ifdef _WIN32
#include <windows.h>
#endif

#ifdef __linux__
#include <stdlib.h>
#endif

#if defined(__linux__) || defined(__APPLE__)
#include <string.h>
#endif

namespace internal {
inline static size_t upScaleAlignment(size_t alignment) {
  if (1 == alignment) {
    return sizeof(void *);
  }

  VK_ASSERT(0 == alignment % 2, "alignment must be a power of 2!");

  // Alignment must be a power of 2 and a multiple of sizeof(void*).
  while (alignment % sizeof(void *)) {
    // Implementation inspired by
    // https://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
    alignment |= alignment >> 1;
    alignment |= alignment >> 2;
    alignment |= alignment >> 4;
    alignment |= alignment >> 8;
    alignment |= alignment >> 16;
#if defined(__LP64__) || defined(_WIN64) || defined(__x86_64__) || \
    defined(_M_X64) || defined(__ia64) || defined(_M_IA64) ||      \
    defined(__aarch64__) || defined(__powerpc64__)
    alignment |= alignment >> 32;
#endif
    ++alignment;
  }

  return alignment;
}

static void *VKAPI_CALL alloc(void *pUserData, size_t size, size_t alignment,
                              VkSystemAllocationScope allocationScope) {
  // TODO: Use these to instrument how the driver allocates memory and for what
  // purpose it is used.
  (void)pUserData;
  (void)allocationScope;

  void *pMemory = nullptr;
  alignment = internal::upScaleAlignment(alignment);

#ifdef _WIN32
  pMemory = _aligned_malloc(size, alignment);
#elif defined(__linux__) || defined(__APPLE__)
  if (posix_memalign(&pMemory, alignment, size)) {
    VK_ABORT("posix_memalign failed!");
  }
#else
#error Platform not supported!
#endif

  return pMemory;
}

static void *VKAPI_CALL realloc(void *pUserData, void *pOriginal, size_t size,
                                size_t alignment,
                                VkSystemAllocationScope allocationScope) {
  // TODO: Use these to instrument how the driver allocates memory and for what
  // purpose it is used.
  (void)pUserData;
  (void)allocationScope;

  void *pMemory = nullptr;

#ifdef _WIN32
  alignment = internal::upScaleAlignment(alignment);
  pMemory = _aligned_realloc(pOriginal, size, alignment);
#elif defined(__linux__) || defined(__APPLE__)
  // Linux does not have an aligned reallocation function, so we emulate.
  if (posix_memalign(&pMemory, alignment, size)) {
    VK_ABORT("posix_memalign failed!");
  }
  memcpy(pMemory, pOriginal, size);
  ::free(pOriginal);
#else
#error Platform not supported!
#endif

  return pMemory;
}

static void VKAPI_CALL free(void *pUserData, void *pMemory) {
  // TODO: Use this to instrument how the driver allocates memroy.
  (void)pUserData;

#ifdef _WIN32
  _aligned_free(pMemory);
#elif defined(__linux__) || defined(__APPLE__)
  ::free(pMemory);
#else
#error Platform not supported!
#endif
}

static void VKAPI_CALL internalAlloc(void *pUserData, size_t size,
                                     VkInternalAllocationType allocationType,
                                     VkSystemAllocationScope allocationScope) {
  (void)pUserData;
  (void)size;
  (void)allocationType;
  (void)allocationScope;
}

static void VKAPI_CALL internalFree(void *pUserData, size_t size,
                                    VkInternalAllocationType allocationType,
                                    VkSystemAllocationScope allocationScope) {
  (void)pUserData;
  (void)size;
  (void)allocationType;
  (void)allocationScope;
}

static VkAllocationCallbacks defaultAllocator = {
    nullptr,         &internal::alloc,         &internal::realloc,
    &internal::free, &internal::internalAlloc, &internal::internalFree};
}  // namespace internal

namespace vk {
const VkAllocationCallbacks *getDefaultAllocatorIfNull(
    const VkAllocationCallbacks *pAllocator) {
  if (pAllocator) {
    return pAllocator;
  }

  return &internal::defaultAllocator;
}
}  // namespace vk
