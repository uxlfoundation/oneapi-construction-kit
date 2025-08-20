// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef _HAL_REFSI_REFSI_HAL_G1_H
#define _HAL_REFSI_REFSI_HAL_G1_H

#include <memory>
#include <mutex>
#include <vector>

#include "elf_loader.h"
#include "refsi_hal.h"

class refsi_command_buffer;

class refsi_g1_hal_device : public refsi_hal_device {
 public:
  refsi_g1_hal_device(refsi_device_t device,
                      riscv::hal_device_info_riscv_t *info,
                      std::mutex &hal_lock);
  virtual ~refsi_g1_hal_device();

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
  bool open_loader();

  std::unique_ptr<ELFProgram> loader;
  refsi_addr_t perf_counters_addr = 0;
  size_t max_harts = 0;
};

#endif  // _HAL_REFSI_REFSI_HAL_G1_H
