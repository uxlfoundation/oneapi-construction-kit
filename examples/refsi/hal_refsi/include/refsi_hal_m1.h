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

#ifndef _HAL_REFSI_REFSI_HAL_M1_H
#define _HAL_REFSI_REFSI_HAL_M1_H

#include <mutex>

#include "refsi_hal.h"

class refsi_command_buffer;
class riscv_encoder;

class refsi_m1_hal_device : public refsi_hal_device {
 public:
  refsi_m1_hal_device(refsi_device_t device,
                      riscv::hal_device_info_riscv_t *info,
                      std::mutex &hal_lock);
  virtual ~refsi_m1_hal_device();

  bool initialize(refsi_locker &locker) override;

  // execute a kernel on the target
  bool kernel_exec(hal::hal_program_t program, hal::hal_kernel_t kernel,
                   const hal::hal_ndrange_t *nd_range,
                   const hal::hal_arg_t *args, uint32_t num_args,
                   uint32_t work_dim) override;

  // copy memory between target buffers
  bool mem_copy(hal::hal_addr_t dst, hal::hal_addr_t src,
                hal::hal_size_t size) override;

 private:
  bool createWindows(refsi_locker &locker);
  bool createWindow(refsi_command_buffer &cb, uint32_t win_id, uint32_t mode,
                    refsi_addr_t base, refsi_addr_t target, uint64_t scale,
                    uint64_t size);
  bool createROM(refsi_locker &locker);
  void encodeKernelExit(riscv_encoder &enc);
  void encodeLaunchKernel(riscv_encoder &enc, unsigned num_dims);

  unsigned num_harts_per_core = 0;
  unsigned num_cores = 0;

  hal::hal_addr_t rom_base = 0;
  hal::hal_addr_t rom_size = 0;
  std::vector<hal::hal_addr_t> launch_kernel_addrs;

  hal::hal_addr_t elf_mem_base = 0;
  hal::hal_addr_t elf_mem_size = 0;
  hal::hal_addr_t elf_mem_mapped_addr = 0;

  hal::hal_addr_t tcdm_base = 0;         // Base address of TCDM.
  hal::hal_addr_t tcdm_size = 0;         // Total TCDM size.
  hal::hal_addr_t tcdm_hart_base = 0;    // Base address of hart-private window.
  hal::hal_addr_t tcdm_hart_size = 0;    // Total size of hart-private TCDM.
  hal::hal_addr_t tcdm_hart_target = 0;  // Base address of hart-private TCDM.
  hal::hal_addr_t tcdm_hart_size_per_hart = 0;  // Size of hart-private TCDM.
};

#endif  // _HAL_REFSI_REFSI_HAL_M1_H
