// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief HAL base implementation of the mux_buffer_s object.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef MUX_HAL_BUFFER_H_INCLUDED
#define MUX_HAL_BUFFER_H_INCLUDED

#include "cargo/expected.h"
#include "mux/hal/memory.h"
#include "mux/utils/allocator.h"

namespace mux {
namespace hal {
struct buffer : mux_buffer_s {
  buffer(mux_memory_requirements_s memory_requirements);

  /// @see muxCreateBuffer
  template <class Buffer>
  static cargo::expected<Buffer *, mux_result_t> create(
      mux_device_t device, size_t size, mux::allocator allocator) {
    static_assert(std::is_base_of<mux::hal::buffer, Buffer>::value,
                  "template type Buffer must derive from mux::hal::buffer");
    (void)device;
    mux_memory_requirements_s memory_requirements = {
        size, /*alignment*/ 16, mux::hal::memory::HEAP_BUFFER};
    auto buffer = allocator.create<Buffer>(memory_requirements);
    if (!buffer) {
      return cargo::make_unexpected(mux_error_out_of_memory);
    }
    return buffer;
  }

  /// @see muxDestroyBuffer
  template <class Buffer>
  static void destroy(mux_device_t device, Buffer *buffer,
                      mux::allocator allocator) {
    static_assert(std::is_base_of<mux::hal::buffer, Buffer>::value,
                  "template type Buffer must derive from mux::hal::buffer");
    (void)device;
    allocator.destroy(static_cast<Buffer *>(buffer));
  }

  /// @see muxBindBufferMemory
  mux_result_t bind(mux_device_t device, mux::hal::memory *memory,
                    uint64_t offset);

  /// @brief Address of buffer on the target.
  ::hal::hal_addr_t targetPtr;
};
}  // namespace hal
}  // namespace mux

#endif  // MUX_HAL_BUFFER_H_INCLUDED
