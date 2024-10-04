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

#ifndef _CLIK_RUNTIME_CPU_HAL_H
#define _CLIK_RUNTIME_CPU_HAL_H

#include <condition_variable>
#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include "hal.h"

#define HAL_CPU_WI_MODE 1
#define HAL_CPU_WG_MODE 2

using elf_program = void *;
struct exec_state;

struct cpu_barrier {
  std::mutex mutex;
  std::condition_variable entry;
  int threads_entered = 0;
  std::condition_variable exit;
  int sequence_id = 0;

  void wait(int num_threads);
};

class cpu_hal final : public hal::hal_device_t {
 public:
  cpu_hal(hal::hal_device_info_t *info, std::mutex &hal_lock);
#if HAL_CPU_MODE == HAL_CPU_WI_MODE
  ~cpu_hal();
#endif

  // Set up the hal device info - this is done as a static so that other classes
  // such as hal_client can set up the desired information directly
  static hal::hal_device_info_t &setup_cpu_hal_device_info();

  size_t get_word_size() const { return sizeof(uintptr_t); }

  // find a specific kernel function in a compiled program
  // returns `hal_invalid_kernel` if no symbol could be found
  hal::hal_kernel_t program_find_kernel(hal::hal_program_t program,
                                        const char *name) override;

  // load an ELF file into target memory
  // returns `hal_invalid_program` if the program could not be loaded
  hal::hal_program_t program_load(const void *data, hal::hal_size_t size);

  // execute a kernel on the target
  bool kernel_exec(hal::hal_program_t program, hal::hal_kernel_t kernel,
                   const hal::hal_ndrange_t *nd_range,
                   const hal::hal_arg_t *args, uint32_t num_args,
                   uint32_t work_dim) override;

  // unload a program from the target
  bool program_free(hal::hal_program_t program);

  // return available target memory estimate
  hal::hal_size_t mem_avail();

  // allocate a memory range on the target
  // will return `hal_nullptr` if the operation was unsuccessful
  hal::hal_addr_t mem_alloc(hal::hal_size_t size,
                            hal::hal_size_t alignment) override;

  // copy memory between target buffers
  bool mem_copy(hal::hal_addr_t dst, hal::hal_addr_t src,
                hal::hal_size_t size) override;

  // free a memory range on the target
  bool mem_free(hal::hal_addr_t addr) override;

  // fill memory with a pattern
  bool mem_fill(hal::hal_addr_t dst, const void *pattern,
                hal::hal_size_t pattern_size, hal::hal_size_t size) override;

  // read memory from the target to the host
  bool mem_read(void *dst, hal::hal_addr_t src, hal::hal_size_t size) override;

  // write host memory to the target
  bool mem_write(hal::hal_addr_t dst, const void *src,
                 hal::hal_size_t size) override;

 private:
  void kernel_entry(exec_state *state);

  bool hal_debug() const { return debug; }

  std::mutex &hal_lock;
  hal::hal_device_info_t *info;
  bool debug = false;
#if HAL_CPU_MODE == HAL_CPU_WI_MODE
  uint32_t local_mem_size = 8 << 20;
  uint8_t *local_mem = nullptr;
  cpu_barrier barrier;
#endif
  std::map<hal::hal_program_t, std::string> binary_files;

  // Default number of threads for wg mode
  unsigned int wg_num_threads;
};

#endif
