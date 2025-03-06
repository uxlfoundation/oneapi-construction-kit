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

#ifndef VK_ALLOCATOR_H_INCLUDED
#define VK_ALLOCATOR_H_INCLUDED

#include <mux/mux.h>
#include <vulkan/vulkan.h>

#include <new>
#include <utility>

namespace vk {
/// @brief Get the drivers default allocator if null.
///
/// @param pAllocator Allocation callbacks.
///
/// @return Return the default allocator if pAllocator is null, pAllocator
/// otherwise.
const VkAllocationCallbacks *getDefaultAllocatorIfNull(
    const VkAllocationCallbacks *pAllocator);

/// @brief Allocator wrapping VkAllocationCallbacks and providing C++ object
/// creation and destruction.
class allocator final {
 public:
  /// @brief Constructor.
  ///
  /// @param pAllocator Vulkan allocator callbacks.
  allocator(const VkAllocationCallbacks *pAllocator)
      : pAllocator(vk::getDefaultAllocatorIfNull(pAllocator)),
        muxAllocator(
            {[](void *user_data, size_t size, size_t alignment) -> void * {
               (void)alignment;
               VkAllocationCallbacks *allocator =
                   reinterpret_cast<VkAllocationCallbacks *>(user_data);
               return allocator->pfnAllocation(
                   allocator->pUserData, size, alignment,
                   VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE);
             },
             [](void *user_data, void *pointer) {
               VkAllocationCallbacks *allocator =
                   reinterpret_cast<VkAllocationCallbacks *>(user_data);
               allocator->pfnFree(allocator->pUserData, pointer);
             },
             const_cast<VkAllocationCallbacks *>(
                 vk::getDefaultAllocatorIfNull(pAllocator))}) {}

  /// @brief Copy constructor
  ///
  /// @param other Allocator to copy
  allocator(const allocator &other)
      : pAllocator(other.pAllocator),
        muxAllocator(
            {[](void *user_data, size_t size, size_t alignment) -> void * {
               (void)alignment;
               VkAllocationCallbacks *allocator =
                   reinterpret_cast<VkAllocationCallbacks *>(user_data);
               return allocator->pfnAllocation(
                   allocator->pUserData, size, alignment,
                   VK_SYSTEM_ALLOCATION_SCOPE_INSTANCE);
             },
             [](void *user_data, void *pointer) {
               VkAllocationCallbacks *allocator =
                   reinterpret_cast<VkAllocationCallbacks *>(user_data);
               allocator->pfnFree(allocator->pUserData, pointer);
             },
             const_cast<VkAllocationCallbacks *>(pAllocator)}) {}

  /// @brief Allocate untyped memory block.
  ///
  /// @tparam Alignment Alignment in bytes of the allocation, default is
  /// sizeof(void*).
  /// @param size Size in bytes of the allocation.
  /// @param allocationScope Scope of the allocation.
  ///
  /// @return Return void pointer to allocated memory.
  template <size_t Alignment = sizeof(void *)>
  void *alloc(size_t size, VkSystemAllocationScope allocationScope) const {
    return pAllocator->pfnAllocation(pAllocator->pUserData, size, Alignment,
                                     allocationScope);
  }

  /// @brief Allocate untyped memory block.
  ///
  /// @param size Size in bytes of the allocation.
  /// @param allocationScope Scope of the allocation.
  /// @param alignment Alignment in bytes of the allocation, default is
  /// sizeof(void*).
  ///
  /// @return Return void pointer to allocated memory.
  void *alloc(size_t size, size_t alignment,
              VkSystemAllocationScope allocationScope) const {
    return pAllocator->pfnAllocation(pAllocator->pUserData, size, alignment,
                                     allocationScope);
  }

  /// @brief Reallocate untyped memory block.
  ///
  /// @tparam Alignment Alignment in bytes of the allocation, default is
  /// sizeof(void*).
  /// @param pOriginal Memory to be reallocated.
  /// @param size Size in bytes of the allocation.
  /// @param allocationScope Scope of the allocation.
  ///
  /// @return Return void pointer to allocated memory.
  template <size_t Alignment = sizeof(void *)>
  void *realloc(void *pOriginal, size_t size,
                VkSystemAllocationScope allocationScope) const {
    return pAllocator->pfnReallocation(pAllocator->pUserData, pOriginal, size,
                                       Alignment, allocationScope);
  }

  /// @brief Free allocated untyped memory block.
  ///
  /// @param pMemory Memory to be freed.
  void free(void *pMemory) const {
    pAllocator->pfnFree(pAllocator->pUserData, pMemory);
  }

  /// @brief Notify the application of an internal allocation.
  ///
  /// @param size Size in bytes of the allocation.
  /// @param allocationType Type of the allocation.
  /// @param allocationScope Scope of the allocation.
  void internalAlloc(size_t size, VkInternalAllocationType allocationType,
                     VkSystemAllocationScope allocationScope) const {
    pAllocator->pfnInternalAllocation(pAllocator->pUserData, size,
                                      allocationType, allocationScope);
  }

  /// @brief Notify the application of an internal free.
  ///
  /// @param size Size in bytes of the allocation.
  /// @param allocationType Type of the allocation.
  /// @param allocationScope Scope of the allocation.
  void internalFree(size_t size, VkInternalAllocationType allocationType,
                    VkSystemAllocationScope allocationScope) const {
    pAllocator->pfnInternalFree(pAllocator->pUserData, size, allocationType,
                                allocationScope);
  }

  /// @brief Allocate and construct a C++ object.
  ///
  /// @tparam T Type of the object.
  /// @tparam Alignment Alignment in bytes of the object.
  /// @tparam Args Constructor argument types parameter pack.
  /// @param allocationScope Scope of the allocation.
  /// @param args Constructor argument values parameter pack.
  ///
  /// @return Return point to constructed object.
  template <typename T, size_t Alignment = alignof(T), typename... Args>
  T *create(VkSystemAllocationScope allocationScope, Args... args) const {
    void *object = alloc(sizeof(T), allocationScope);
    if (!object) {
      return nullptr;
    }
    return new (object) T(std::forward<Args>(args)...);
  }

  /// @brief Deconstruct and free a C++ object.
  ///
  /// @tparam T Type of then object.
  /// @param object Object to be destructed and freed.
  template <typename T>
  void destroy(T *object) const {
    object->~T();
    free(object);
  }

  /// @brief Access the pointer to the allocation callbacks.
  ///
  /// @return Returns a pointer to the allocation callbacks structure.
  const VkAllocationCallbacks *getCallbacks() { return pAllocator; }

  /// @brief Access this allocator's instance of `mux_allocator_info_t`
  ///
  /// @return This allocator's instance of `mux_allocator_info_t`
  mux_allocator_info_t getMuxAllocator() const { return muxAllocator; }

 private:
  /// @brief Pointer to Vulkan allocation callbacks.
  const VkAllocationCallbacks *pAllocator;

  /// @brief Pointer to mux allocator info struct
  mux_allocator_info_t muxAllocator;
};

/// @brief Allocator used to specialize `cargo` containers for VK.
///
/// @tparam T Type of the object to be allocated.
template <class T>
class cargo_allocator {
 public:
  using value_type = T;
  using size_type = size_t;
  using pointer = value_type *;

  /// @brief Constructor.
  ///
  /// @param pAllocator Pointer to users Vulkan allocation callbacks.
  /// @param allocationScope Scope of allocations to use.
  cargo_allocator(const VkAllocationCallbacks *pAllocator,
                  VkSystemAllocationScope allocationScope)
      : pAllocator(pAllocator), allocationScope(allocationScope) {}

  /// @brief Allocate space for an array of uninitialized objects.
  ///
  /// @param count Number of elements in the array of objects.
  ///
  /// @return Returns a pointer to the beginning to the array of objects on
  /// success, null pointer otherwise.
  pointer alloc(size_type count = 1) {
    return static_cast<pointer>(pAllocator->pfnAllocation(
        pAllocator->pUserData, sizeof(value_type) * count, alignof(value_type),
        allocationScope));
  }

  /// @brief Deallocate a previously allocated array of objects.
  ///
  /// @param allocation Allocation to deallocate.
  void free(pointer allocation) {
    pAllocator->pfnFree(pAllocator->pUserData, static_cast<void *>(allocation));
  }

  /// @brief Allocate space for and construct an object.
  ///
  /// @tparam Args Variadic constructor argument types.
  /// @param args Variadic constructor arguments to be forwarded.
  ///
  /// @return Returns a pointer to the created object on success, null pointer
  /// otherwise.
  template <class... Args>
  pointer create(Args &&...args) {
    auto object = this->alloc();
    if (object) {
      new (object) value_type(std::forward<Args>(args)...);
    }
    return object;
  }

  /// @brief Destruct and free a previously created object.
  ///
  /// @param object Object to destroy.
  void destroy(pointer object) {
    object->~value_type();
    this->free(object);
  }

  /// @brief Get the underlaying Vulkan allocation callbacks.
  ///
  /// @return Returns a pointer to the allocation callbacks.
  const VkAllocationCallbacks *getAllocationCallbacks() const {
    return pAllocator;
  }

  /// @brief Get the Vulkan allocation scope.
  ///
  /// @return Returns the allocation scope.
  VkSystemAllocationScope getAllocationScope() const { return allocationScope; }

 private:
  /// @brief Pointer to Vulkan allocation callbacks.
  const VkAllocationCallbacks *pAllocator;
  /// @brief Scope of the allocation.
  VkSystemAllocationScope allocationScope;
};
}  // namespace vk

#endif  // VK_ALLOCATOR_H_INCLUDED
