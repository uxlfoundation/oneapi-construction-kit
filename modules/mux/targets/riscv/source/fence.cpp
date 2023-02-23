// Copyright (C) Codeplay Software Limited. All Rights Reserved.
#include <riscv/fence.h>

#include "mux/mux.h"
#include "riscv/riscv.h"

mux_result_t riscvCreateFence(mux_device_t device,
                              mux_allocator_info_t allocator_info,
                              mux_fence_t *out_fence) {
  mux::allocator allocator(allocator_info);
  auto fence = riscv::fence_s::create<riscv::fence_s>(device, allocator);
  if (!fence) {
    return mux_error_out_of_memory;
  }
  *out_fence = *fence;
  return mux_success;
}

void riscvDestroyFence(mux_device_t device, mux_fence_t fence,
                       mux_allocator_info_t allocator_info) {
  riscv::fence_s::destroy(device, static_cast<riscv::fence_s *>(fence),
                          mux::allocator(allocator_info));
}

mux_result_t riscvResetFence(mux_fence_t fence) {
  static_cast<riscv::fence_s *>(fence)->reset();
  return mux_success;
}
