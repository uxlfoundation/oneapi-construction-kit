// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief C++ helpers for managing mux API objects.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef MUX_MUX_HPP_INCLUDED
#define MUX_MUX_HPP_INCLUDED

#include <cargo/type_traits.h>
#include <mux/mux.h>

#include <cassert>
#include <memory>

namespace mux {
/// @brief Custom `std::unique_ptr` deleter for Mux API objects.
///
/// @tparam T Type of the Mux API object.
template <class T>
struct deleter;

/// @brief Custom deleter for `mux_device_t` objects.
template <>
struct deleter<mux_device_t> {
  /// @brief Deleter state constructor.
  ///
  /// @param allocator_info Core allocator used for destruction.
  deleter(mux_allocator_info_t allocator_info)
      : allocator_info(allocator_info) {}

  /// @brief Function call operator to invoke destruction.
  ///
  /// @param device Core device to destroy.
  void operator()(mux_device_t device) {
    muxDestroyDevice(device, allocator_info);
  }

  mux_allocator_info_t allocator_info;
};

/// @brief Custom deleter for `mux_memory_t` objects.
template <>
struct deleter<mux_memory_t> {
  /// @brief Deleter state constructor.
  ///
  /// @param device Mux device used for destruction.
  /// @param allocator_info Mux allocator used for destruction.
  deleter(mux_device_t device, mux_allocator_info_t allocator_info)
      : device(device), allocator_info(allocator_info) {}

  /// @brief Function call operator to invoke destruction.
  ///
  /// @param memory Mux memory to destroy.
  void operator()(mux_memory_t memory) {
    muxFreeMemory(device, memory, allocator_info);
  }

  mux_device_t device;
  mux_allocator_info_t allocator_info;
};

/// @brief Custom deleter for `mux_buffer_t` objects.
template <>
struct deleter<mux_buffer_t> {
  /// @brief Deleter state constructor.
  ///
  /// @param device Mux device used for destruction.
  /// @param allocator_info Mux allocator used for destruction.
  deleter(mux_device_t device, mux_allocator_info_t allocator_info)
      : device(device), allocator_info(allocator_info) {}

  /// @brief Function call operator to invoke destruction.
  ///
  /// @param buffer Mux buffer to destroy.
  void operator()(mux_buffer_t buffer) {
    muxDestroyBuffer(device, buffer, allocator_info);
  }

  mux_device_t device;
  mux_allocator_info_t allocator_info;
};

/// @brief Custom deleter for `mux_image_t` objects.
template <>
struct deleter<mux_image_t> {
  /// @brief Deleter state constructor.
  ///
  /// @param device Mux device used for destruction.
  /// @param allocator_info Mux allocator used for destruction.
  deleter(mux_device_t device, mux_allocator_info_t allocator_info)
      : device(device), allocator_info(allocator_info) {}

  /// @brief Function call operator to invoke destruction.
  ///
  /// @param image Mux image to destroy.
  void operator()(mux_image_t image) {
    muxDestroyImage(device, image, allocator_info);
  }

  mux_device_t device;
  mux_allocator_info_t allocator_info;
};

/// @brief Custom deleter for `mux_fence_t` objects.
template <>
struct deleter<mux_fence_t> {
  /// @brief Deleter state constructor.
  ///
  /// @param device Mux device used for destruction.
  /// @param allocator_info Mux allocator used for destruction.
  deleter(mux_device_t device, mux_allocator_info_t allocator_info)
      : device(device), allocator_info(allocator_info) {}

  /// @brief Function call operator to invoke destruction.
  ///
  /// @param fence Mux fence to destroy.
  void operator()(mux_fence_t fence) {
    muxDestroyFence(device, fence, allocator_info);
  }

  mux_device_t device;
  mux_allocator_info_t allocator_info;
};

/// @brief Custom deleter for `mux_semaphore_t` objects.
template <>
struct deleter<mux_semaphore_t> {
  /// @brief Deleter state constructor.
  ///
  /// @param device Mux device used for destruction.
  /// @param allocator_info Mux allocator used for destruction.
  deleter(mux_device_t device, mux_allocator_info_t allocator_info)
      : device(device), allocator_info(allocator_info) {}

  /// @brief Function call operator to invoke destruction.
  ///
  /// @param semaphore Mux semaphore to destroy.
  void operator()(mux_semaphore_t semaphore) {
    muxDestroySemaphore(device, semaphore, allocator_info);
  }

  mux_device_t device;
  mux_allocator_info_t allocator_info;
};

/// @brief Custom deleter for `mux_command_buffer_t` objects.
template <>
struct deleter<mux_command_buffer_t> {
  /// @brief Deleter state constructor.
  ///
  /// @param device Mux device used for destruction.
  /// @param allocator_info Mux allocator used for destruction.
  deleter(mux_device_t device, mux_allocator_info_t allocator_info)
      : device(device), allocator_info(allocator_info) {}

  /// @brief Function call operator to invoke destruction.
  ///
  /// @param command_buffer Mux command group to destroy.
  void operator()(mux_command_buffer_t command_buffer) {
    muxDestroyCommandBuffer(device, command_buffer, allocator_info);
  }

  mux_device_t device;
  mux_allocator_info_t allocator_info;
};

/// @brief Custom deleter for `mux_executable_t` objects.
template <>
struct deleter<mux_executable_t> {
  /// @brief Deleter state constructor.
  ///
  /// @param device Mux device used for destruction.
  /// @param allocator_info Mux allocator used for destruction.
  deleter(mux_device_t device, mux_allocator_info_t allocator_info)
      : device(device), allocator_info(allocator_info) {}

  /// @brief Function call operator to invoke destruction.
  ///
  /// @param executable Mux executable to destroy.
  void operator()(mux_executable_t executable) {
    muxDestroyExecutable(device, executable, allocator_info);
  }

  mux_device_t device;
  mux_allocator_info_t allocator_info;
};

/// @brief Custom deleter for `mux_kernel_t` objects.
template <>
struct deleter<mux_kernel_t> {
  /// @brief Deleter state constructor.
  ///
  /// @param device Mux device used for destruction.
  /// @param allocator_info Mux allocator used for destruction.
  deleter(mux_device_t device, mux_allocator_info_t allocator_info)
      : device(device), allocator_info(allocator_info) {}

  /// @brief Function call operator to invoke destruction.
  ///
  /// @param kernel Mux kernel to destroy.
  void operator()(mux_kernel_t kernel) {
    muxDestroyKernel(device, kernel, allocator_info);
  }

  mux_device_t device;
  mux_allocator_info_t allocator_info;
};

/// @brief Alias of `std::unique_ptr` with customer deleter for the Mux API.
///
/// @tparam T Type of the Mux API object.
template <class T>
using unique_ptr = std::unique_ptr<cargo::remove_pointer_t<T>, mux::deleter<T>>;
}  // namespace mux

#endif  // MUX_MUX_HPP_INCLUDED
