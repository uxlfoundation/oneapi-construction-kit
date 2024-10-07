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

#include <UnitVK.h>
#include <stdio.h>
#include <string.h>

// GCC lambdas are only convertible to function pointers of the specified
// calling convention, and require the correct calling convention to be
// specified. MSVC lambdas are convertible to function pointers of any calling
// convention, and neither require nor allow the calling convention to be
// specified.
#ifdef __GNUC__
#define VK_LAMBDA_CALLBACK VKAPI_PTR
#else
#define VK_LAMBDA_CALLBACK
#endif

#ifdef _WIN32
#include <windows.h>
#elif defined(__linux__) || defined(__APPLE__)
#include <stdlib.h>
#else
#error Platform not supported!
#endif

namespace uvk {
static size_t upScaleAlignment(size_t alignment) {
  if (1 == alignment) {
    return sizeof(void *);
  }

  size_t scaledAlignment = alignment;
  if (1 == alignment) {
    return sizeof(void *);
  }

#ifdef _WIN32
  // Alignment must be a power of 2.
  while (scaledAlignment % alignment) {
#elif defined(__linux__) || defined(__APPLE__)
  // Alignment must be a power of 2 and a multiple of sizeof(void*).
  while (scaledAlignment % alignment || scaledAlignment % sizeof(void *)) {
#endif
    scaledAlignment |= scaledAlignment >> 1;
    scaledAlignment |= scaledAlignment >> 2;
    scaledAlignment |= scaledAlignment >> 4;
    scaledAlignment |= scaledAlignment >> 8;
    scaledAlignment |= scaledAlignment >> 16;
#if defined(_WIN64) || defined(__x86_64__)
    scaledAlignment |= scaledAlignment >> 32;
#endif
    ++scaledAlignment;
  }

  return scaledAlignment;
}  // namespace uvk

void *VKAPI_CALL alloc(void *pUserData, size_t size, size_t alignment,
                       VkSystemAllocationScope allocationScope) {
  // TODO: Use these to intrument how the driver allocates memory and for what
  // purpose it is used.
  (void)pUserData;
  (void)allocationScope;

  void *pMemory = nullptr;
  alignment = uvk::upScaleAlignment(alignment);

#ifdef _WIN32
  pMemory = _aligned_malloc(size, alignment);
#elif defined(__linux__) || defined(__APPLE__)
  if (posix_memalign(&pMemory, alignment, size)) {
    (void)fprintf(stderr, "posix_memalign failed!\n");
    abort();
  }
#endif

  return pMemory;
}

void *VKAPI_CALL realloc(void *pUserData, void *pOriginal, size_t size,
                         size_t alignment,
                         VkSystemAllocationScope allocationScope) {
  // TODO: Use these to intrument how the driver allocates memory and for what
  // purpose it is used.
  (void)pUserData;
  (void)allocationScope;

  void *pMemory = nullptr;

#ifdef _WIN32
  alignment = uvk::upScaleAlignment(alignment);
  pMemory = _aligned_realloc(pOriginal, size, alignment);
#elif defined(__linux__) || defined(__APPLE__)
  // Linux does not have an aligned reallocation function, so we emulate.
  pMemory = uvk::alloc(pUserData, size, alignment, allocationScope);
  memcpy(pMemory, pOriginal, size);
  uvk::free(pUserData, pOriginal);
#endif

  return pMemory;
}

void VKAPI_CALL free(void *pUserData, void *pMemory) {
  // TODO: Use this to instrument how the driver allocates memroy.
  (void)pUserData;

#ifdef _WIN32
  // Check if we allocated with malloc.
  _aligned_free(pMemory);
#elif defined(__linux__) || defined(__APPLE__)
  ::free(pMemory);
#endif
}

void VKAPI_CALL allocNotify(void *pUserData, size_t size,
                            VkInternalAllocationType allocationType,
                            VkSystemAllocationScope allocationScope) {
  // TODO: Track driver internal allocations.
  (void)pUserData;
  (void)size;
  (void)allocationType;
  (void)allocationScope;
}

void VKAPI_CALL freeNotify(void *pUserData, size_t size,
                           VkInternalAllocationType allocationType,
                           VkSystemAllocationScope allocationScope) {
  // TODO: Track driver internal frees.
  (void)pUserData;
  (void)size;
  (void)allocationType;
  (void)allocationScope;
}

static void *VKAPI_CALL oneUseAlloc(void *pUserData, size_t size,
                                    size_t alignment,
                                    VkSystemAllocationScope allocationScope) {
  // TODO: Use these to intrument how the driver allocates memory and for what
  // purpose it is used.
  (void)allocationScope;

  bool *used = reinterpret_cast<bool *>(pUserData);
  void *pMemory = nullptr;

  if (!*used) {
    alignment = uvk::upScaleAlignment(alignment);

#ifdef _WIN32
    pMemory = _aligned_malloc(size, alignment);
#elif defined(__linux__) || defined(__APPLE__)
    if (posix_memalign(&pMemory, alignment, size)) {
      (void)fprintf(stderr, "posix_memalign failed!\n");
      abort();
    }
#endif
    *used = true;
  } else {
    pMemory = nullptr;
  }
  return pMemory;
}

static const VkAllocationCallbacks allocationCallbacks = {
    nullptr,    &uvk::alloc,       &uvk::realloc,
    &uvk::free, &uvk::allocNotify, &uvk::freeNotify};

const VkAllocationCallbacks *defaultAllocator() {
  return &uvk::allocationCallbacks;
}

static const VkAllocationCallbacks nullAllocationCallBacks = {
    nullptr,
    [](void *, size_t, size_t, VkSystemAllocationScope)
        VK_LAMBDA_CALLBACK -> void * {
      return nullptr;
    },
    [](void *, void *, size_t, size_t, VkSystemAllocationScope)
        VK_LAMBDA_CALLBACK -> void * {
      return nullptr;
    },
    [](void *, void *) VK_LAMBDA_CALLBACK {},
    [](void *, size_t, VkInternalAllocationType, VkSystemAllocationScope)
        VK_LAMBDA_CALLBACK {
    },
    [](void *, size_t, VkInternalAllocationType, VkSystemAllocationScope)
        VK_LAMBDA_CALLBACK {
    }};

static VkAllocationCallbacks oneUseAllocationCallbacks = {
    nullptr,    &uvk::oneUseAlloc, &uvk::realloc,
    &uvk::free, &uvk::allocNotify, &uvk::freeNotify};

const VkAllocationCallbacks *nullAllocator() {
  return &uvk::nullAllocationCallBacks;
}

const VkAllocationCallbacks *oneUseAllocator(bool *used) {
  oneUseAllocationCallbacks.pUserData = used;
  return &uvk::oneUseAllocationCallbacks;
}

Result::Result(VkResult resultCode) : resultCode((int)resultCode) {}

std::string Result::description() const {
#define RESULT_CODE_CASE(RESULT_CODE) \
  case RESULT_CODE: {                 \
    return #RESULT_CODE;              \
  } break

  switch (resultCode) {
    RESULT_CODE_CASE(VK_SUCCESS);
    RESULT_CODE_CASE(VK_NOT_READY);
    RESULT_CODE_CASE(VK_TIMEOUT);
    RESULT_CODE_CASE(VK_EVENT_SET);
    RESULT_CODE_CASE(VK_EVENT_RESET);
    RESULT_CODE_CASE(VK_INCOMPLETE);
    RESULT_CODE_CASE(VK_ERROR_OUT_OF_HOST_MEMORY);
    RESULT_CODE_CASE(VK_ERROR_OUT_OF_DEVICE_MEMORY);
    RESULT_CODE_CASE(VK_ERROR_INITIALIZATION_FAILED);
    RESULT_CODE_CASE(VK_ERROR_DEVICE_LOST);
    RESULT_CODE_CASE(VK_ERROR_MEMORY_MAP_FAILED);
    RESULT_CODE_CASE(VK_ERROR_LAYER_NOT_PRESENT);
    RESULT_CODE_CASE(VK_ERROR_EXTENSION_NOT_PRESENT);
    RESULT_CODE_CASE(VK_ERROR_FEATURE_NOT_PRESENT);
    RESULT_CODE_CASE(VK_ERROR_INCOMPATIBLE_DRIVER);
    RESULT_CODE_CASE(VK_ERROR_TOO_MANY_OBJECTS);
    RESULT_CODE_CASE(VK_ERROR_FORMAT_NOT_SUPPORTED);
    default: {
      static std::stringstream ret_code_unknown;
      ret_code_unknown.str("");
      ret_code_unknown << "Unknown result code: ";
      ret_code_unknown << resultCode;
      return ret_code_unknown.str();
    }
  }
#undef RESULT_CODE_CASE
}

}  // namespace uvk
