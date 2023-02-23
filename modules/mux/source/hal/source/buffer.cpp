// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "mux/hal/buffer.h"

namespace mux {
namespace hal {
buffer::buffer(mux_memory_requirements_s memory_requirements)
    : targetPtr(::hal::hal_nullptr) {
  this->memory_requirements = memory_requirements;
}

mux_result_t buffer::bind(mux_device_t device, mux::hal::memory *memory,
                          uint64_t offset) {
  (void)device;
  targetPtr = memory->targetPtr + offset;
  return mux_success;
}
}  // namespace hal
}  // namespace mux
