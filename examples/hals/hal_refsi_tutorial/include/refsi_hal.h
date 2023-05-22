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

#ifndef _HAL_REFSI_REFSI_HAL_H
#define _HAL_REFSI_REFSI_HAL_H

#include <mutex>

#include "hal.h"
#include "hal_riscv.h"

using refsi_locker = std::lock_guard<std::recursive_mutex>;

class refsi_hal_device : public hal::hal_device_t {
 public:
  refsi_hal_device(riscv::hal_device_info_riscv_t *info,
                   std::recursive_mutex &hal_lock);

  // find a specific kernel function in a compiled program
  // returns `hal_invalid_kernel` if no symbol could be found
  hal::hal_kernel_t program_find_kernel(hal::hal_program_t program,
                                        const char *name) override;

  // load an ELF file into target memory
  // returns `hal_invalid_program` if the program could not be loaded
  hal::hal_program_t program_load(const void *data,
                                  hal::hal_size_t size) override;

  // execute a kernel on the target
  bool kernel_exec(hal::hal_program_t program, hal::hal_kernel_t kernel,
                   const hal::hal_ndrange_t *nd_range,
                   const hal::hal_arg_t *args, uint32_t num_args,
                   uint32_t work_dim) override;

  // unload a program from the target
  bool program_free(hal::hal_program_t program) override;

  // allocate a memory range on the target
  // will return `hal_nullptr` if the operation was unsuccessful
  hal::hal_addr_t mem_alloc(hal::hal_size_t size,
                            hal::hal_size_t alignment) override;

  // free a memory range on the target
  bool mem_free(hal::hal_addr_t addr) override;

  // read memory from the target to the host
  bool mem_read(void *dst, hal::hal_addr_t src, hal::hal_size_t size) override;

  // write host memory to the target
  bool mem_write(hal::hal_addr_t dst, const void *src,
                 hal::hal_size_t size) override;

 private:
  std::recursive_mutex &hal_lock;
  hal::hal_device_info_t *info;
};

#endif  // _HAL_REFSI_REFSI_HAL_H
