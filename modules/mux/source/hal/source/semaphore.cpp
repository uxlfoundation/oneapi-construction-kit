// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "mux/hal/semaphore.h"

#include "mux/utils/allocator.h"

namespace mux {
namespace hal {
semaphore::semaphore(mux_device_t device) : status{0} { this->device = device; }

void semaphore::signal() { status |= states::SIGNAL; }
bool semaphore::is_signalled() {
  return (status & states::SIGNAL) == states::SIGNAL;
}

void semaphore::reset() { status = 0; }

void semaphore::terminate() { status |= states::TERMINATE; }
bool semaphore::is_terminated() {
  return (status & states::TERMINATE) == states::TERMINATE;
}
}  // namespace hal
}  // namespace mux
