// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "riscv/buffer.h"

#include "riscv/device.h"
#include "riscv/memory.h"
#include "riscv/riscv.h"

mux_result_t riscvCreateBuffer(mux_device_t device, size_t size,
                               mux_allocator_info_t allocator_info,
                               mux_buffer_t *out_buffer) {
  auto buffer = riscv::buffer_s::create<riscv::buffer_s>(
      static_cast<riscv::device_s *>(device), size, allocator_info);
  if (!buffer) {
    return buffer.error();
  }
  *out_buffer = buffer.value();
  return mux_success;
}

void riscvDestroyBuffer(mux_device_t device, mux_buffer_t buffer,
                        mux_allocator_info_t allocator_info) {
  riscv::buffer_s::destroy(static_cast<riscv::device_s *>(device),
                           static_cast<riscv::buffer_s *>(buffer),
                           allocator_info);
}

mux_result_t riscvBindBufferMemory(mux_device_t device, mux_memory_t memory,
                                   mux_buffer_t buffer, uint64_t offset) {
  return static_cast<riscv::buffer_s *>(buffer)->bind(
      static_cast<riscv::device_s *>(device),
      static_cast<riscv::memory_s *>(memory), offset);
}
