// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/codeplaysoftware/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "refsi_command_buffer.h"
#include "refsi_hal.h"
#include "device/dma_regs.h"

refsi_result refsi_command_buffer::run(refsi_hal_device &hal_device,
                                       refsi_locker &locker) {
  // Write the command buffer to device memory.
  size_t cb_size = chunks.size() * sizeof(uint64_t);
  hal::hal_addr_t cb_addr = hal_device.mem_alloc(cb_size, sizeof(uint64_t),
                                                 locker);
  if (!cb_addr || !hal_device.mem_write(cb_addr, chunks.data(), cb_size,
                                        locker)) {
    return refsi_failure;
  }

  // Execute the command buffer and wait for its completion.
  if (refsi_result result = refsiExecuteCommandBuffer(hal_device.get_device(),
                                                      cb_addr, cb_size)) {
    hal_device.mem_free(cb_addr, locker);
    return result;
  }
  refsiWaitForDeviceIdle(hal_device.get_device());
  hal_device.mem_free(cb_addr, locker);
  return refsi_success;
}

void refsi_command_buffer::addFINISH() {
  chunks.push_back(refsiEncodeCMPCommand(CMP_FINISH, 0, 0));
}

void refsi_command_buffer::addWRITE_REG64(refsi_cmp_register_id reg,
                                          uint64_t value) {
  chunks.push_back(refsiEncodeCMPCommand(CMP_WRITE_REG64, 1, reg));
  chunks.push_back(value);
}

void refsi_command_buffer::addSTORE_IMM64(refsi_addr_t dest_addr,
                                          uint64_t value) {
  chunks.push_back(refsiEncodeCMPCommand(CMP_STORE_IMM64, 1,
                                         (uint32_t)dest_addr));
  chunks.push_back(value);
}

void refsi_command_buffer::addLOAD_REG64(refsi_cmp_register_id reg,
                                         uint64_t src_addr) {
  chunks.push_back(refsiEncodeCMPCommand(CMP_LOAD_REG64, 1, reg));
  chunks.push_back(src_addr);
}

void refsi_command_buffer::addSTORE_REG64(refsi_cmp_register_id reg,
                                          uint64_t dest_addr) {
  chunks.push_back(refsiEncodeCMPCommand(CMP_STORE_REG64, 1, reg));
  chunks.push_back(dest_addr);
}

void refsi_command_buffer::addCOPY_MEM64(uint64_t src_addr, uint64_t dest_addr,
                                         uint32_t count, uint32_t unit_id) {
  chunks.push_back(refsiEncodeCMPCommand(CMP_COPY_MEM64, 3, count));
  chunks.push_back(src_addr);
  chunks.push_back(dest_addr);
  chunks.push_back(unit_id);
}

void refsi_command_buffer::addRUN_KERNEL_SLICE(uint32_t max_harts,
                                               uint64_t num_instances,
                                               uint64_t slice_id) {
  uint32_t inline_chunk = (max_harts & 0xff);
  chunks.push_back(
      refsiEncodeCMPCommand(CMP_RUN_KERNEL_SLICE, 2, inline_chunk));
  chunks.push_back(num_instances);
  chunks.push_back(slice_id);
}

void refsi_command_buffer::addRUN_INSTANCES(uint32_t max_harts,
                                            uint64_t num_instances,
                                            std::vector<uint64_t> &extra_args) {
  const uint32_t max_extra_args = 7;
  uint32_t num_extra_args = std::min((uint32_t)extra_args.size(),
                                     max_extra_args);
  uint32_t inline_chunk = (max_harts & 0xff) | (num_extra_args << 8);
  chunks.push_back(
      refsiEncodeCMPCommand(CMP_RUN_INSTANCES, 1 + num_extra_args,
                            inline_chunk));
  chunks.push_back(num_instances);
  for (uint32_t i = 0; i < num_extra_args; i++) {
    chunks.push_back(extra_args[i]);
  }
}

void refsi_command_buffer::addSYNC_CACHE(uint32_t flags) {
  uint32_t inline_chunk = flags;
  chunks.push_back(refsiEncodeCMPCommand(CMP_SYNC_CACHE, 0, inline_chunk));
}

refsi_addr_t refsi_command_buffer::getDMARegAddr(uint32_t dma_reg) const {
  return REFSI_DMA_REG_ADDR(REFSI_DMA_IO_ADDRESS, dma_reg);
}

void refsi_command_buffer::addWriteDMAReg(uint32_t dma_reg, uint64_t value) {
  addSTORE_IMM64(getDMARegAddr(dma_reg), value);
}
