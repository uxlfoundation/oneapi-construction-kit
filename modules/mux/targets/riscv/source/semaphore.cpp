// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <mux/utils/allocator.h>
#include <riscv/riscv.h>
#include <riscv/semaphore.h>

mux_result_t riscvCreateSemaphore(mux_device_t device,
                                  mux_allocator_info_t allocator_info,
                                  mux_semaphore_t *out_semaphore) {
  mux::allocator allocator(allocator_info);
  auto semaphore =
      riscv::semaphore_s::create<riscv::semaphore_s>(device, allocator);
  if (!semaphore) {
    return semaphore.error();
  }
  *out_semaphore = semaphore.value();
  return mux_success;
}

void riscvDestroySemaphore(mux_device_t device, mux_semaphore_t semaphore,
                           mux_allocator_info_t allocator_info) {
  mux::allocator allocator(allocator_info);
  riscv::semaphore_s::destroy(
      device, static_cast<riscv::semaphore_s *>(semaphore), allocator);
}

mux_result_t riscvResetSemaphore(mux_semaphore_t semaphore) {
  static_cast<riscv::semaphore_s *>(semaphore)->reset();
  return mux_success;
}
