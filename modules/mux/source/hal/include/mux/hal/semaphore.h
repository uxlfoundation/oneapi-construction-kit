// Copyright (C) Codeplay Software Limited. All Rights Reserved.

/// @file
///
/// @brief HAL base implementation of the mux_semaphore_s object.
///
/// @copyright Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef MUX_HAL_SEMAPHORE_H_INCLUDED
#define MUX_HAL_SEMAPHORE_H_INCLUDED

#include <atomic>
#include <type_traits>

#include "cargo/expected.h"
#include "hal.h"
#include "mux/mux.h"
#include "mux/utils/allocator.h"
#include "mux/utils/small_vector.h"

namespace mux {
namespace hal {
struct semaphore : mux_semaphore_s {
  semaphore(mux_device_t device);

  /// @see muxCreateSemaphore
  template <class Semaphore>
  static cargo::expected<Semaphore *, mux_result_t> create(
      mux_device_t device, mux::allocator allocator) {
    static_assert(
        std::is_base_of<mux::hal::semaphore, Semaphore>::value,
        "template type Semaphore must derive from mux::hal::semaphore");
    const auto semaphore = allocator.create<Semaphore>(device);
    if (!semaphore) {
      return cargo::make_unexpected(mux_error_out_of_memory);
    }
    return semaphore;
  }

  /// @see muxDestroySemaphore
  template <class Semaphore>
  static void destroy(mux_device_t device, Semaphore *semaphore,
                      mux::allocator allocator) {
    static_assert(
        std::is_base_of<mux::hal::semaphore, Semaphore>::value,
        "template type Semaphore must derive from mux::hal::semaphore");
    (void)device;
    allocator.destroy(semaphore);
  }

  /// @see muxResetSemaphore
  void reset();

  void signal();
  bool is_signalled();

  void terminate();
  bool is_terminated();

 private:
  enum states : uint32_t {
    SIGNAL = 0x1,
    TERMINATE = 0x80000000,
  };
  std::atomic_uint32_t status;
};
}  // namespace hal
}  // namespace mux

#endif  // MUX_HAL_SEMAPHORE_H_INCLUDED
