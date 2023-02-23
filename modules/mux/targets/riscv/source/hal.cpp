// Copyright (C) Codeplay Software Limited. All Rights Reserved.
#include <hal.h>
#include <hal_library.h>

namespace riscv {

/// @brief Current version of the HAL API. The version number needs to be
/// bumped any time the interface is changed.
static const uint32_t expected_hal_version = 6;

// hal instances
static hal::hal_library_t hal_library;
static hal::hal_t *hal_instance;

hal::hal_t *hal_get() {
  if (hal_instance) {
    // hal has already been loaded so just return it
    return hal_instance;
  }
  static_assert(expected_hal_version == hal::hal_t::api_version,
                "Expected HAL API version for Mux target does not match hal.h");
  hal_instance =
      hal::load_hal(CA_HAL_DEFAULT_DEVICE, expected_hal_version, hal_library);
  return hal_instance;
}

void hal_unload() {
  // discard the hal_t instance
  hal_instance = nullptr;
  // unload the hal library
  hal::unload_hal(hal_library);
  hal_library = nullptr;
}

}  // namespace riscv
