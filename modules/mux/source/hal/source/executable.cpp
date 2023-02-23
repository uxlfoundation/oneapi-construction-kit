// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "mux/hal/executable.h"

namespace mux {
namespace hal {
executable::executable(mux::hal::device *device,
                       mux::dynamic_array<uint8_t> &&object_code)
    : object_code(std::move(object_code)) {
  this->device = device;
}
}  // namespace hal
}  // namespace mux
