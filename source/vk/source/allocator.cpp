// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <vk/allocator.h>
#include <vk/error.h>

#include <stdio.h>

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
inline size_t upScaleAlignment(size_t alignment) {
  if (1 == alignment) {
    return sizeof(void*);
  }

  VK_ASSERT(0 == alignment % 2, "alignment must be a power of 2!");

  // Alignment must be a power of 2 and a multiple of sizeof(void*).
  while (alignment % sizeof(void*)) {
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

void* VKAPI_CALL alloc(void* pUserData, size_t size, size_t alignment,
                       VkSystemAllocationScope allocationScope) {
  // TODO: Use these to instrument how the driver allocates memory and for what
  // purpose it is used.
  (void)pUserData;
  (void)allocationScope;

  void* pMemory = nullptr;
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

void* VKAPI_CALL realloc(void* pUserData, void* pOriginal, size_t size,
                         size_t alignment,
                         VkSystemAllocationScope allocationScope) {
  // TODO: Use these to instrument how the driver allocates memory and for what
  // purpose it is used.
  (void)pUserData;
  (void)allocationScope;

  void* pMemory = nullptr;

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

void VKAPI_CALL free(void* pUserData, void* pMemory) {
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

void VKAPI_CALL internalAlloc(void* pUserData, size_t size,
                              VkInternalAllocationType allocationType,
                              VkSystemAllocationScope allocationScope) {
  (void)pUserData;
  (void)size;
  (void)allocationType;
  (void)allocationScope;
}

void VKAPI_CALL internalFree(void* pUserData, size_t size,
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
const VkAllocationCallbacks* getDefaultAllocatorIfNull(
    const VkAllocationCallbacks* pAllocator) {
  if (pAllocator) {
    return pAllocator;
  }

  return &internal::defaultAllocator;
}
}  // namespace vk
