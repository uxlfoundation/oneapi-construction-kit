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

#include <map>
#include <memory>
#include <mutex>
#include <vector>

#include "common_devices.h"
#include "elf_loader.h"
#include "hal.h"
#include "hal_counters.h"
#include "hal_riscv.h"
#include "refsidrv/refsidrv.h"

class ELFProgram;

struct refsi_hal_kernel {
  refsi_hal_kernel(reg_t symbol, std::string name)
      : symbol(symbol), name(std::move(name)) {}

  const reg_t symbol;
  const std::string name;
};

struct refsi_hal_program {
  refsi_hal_program(std::unique_ptr<ELFProgram> program)
      : elf(std::move(program)) {}

  refsi_hal_kernel *find_kernel(const char *name);

  std::unique_ptr<ELFProgram> elf;
  std::map<std::string, std::unique_ptr<refsi_hal_kernel>> kernels;
};

/// @brief Enumerates host-related profiling counters.
enum refsi_host_counter {
  /// @brief Counts the number of bytes written by the host to the device.
  CTR_HOST_MEM_WRITE = 0,
  /// @brief Counts the number of bytes read by the host from the device.
  CTR_HOST_MEM_READ,

  /// @brief Number of performance counters exposed by the host.
  CTR_NUM_COUNTERS = CTR_HOST_MEM_READ + 1
};

using refsi_locker = std::unique_lock<std::mutex>;

class refsi_hal_device : public hal::hal_device_t {
 public:
  refsi_hal_device(refsi_device_t device, riscv::hal_device_info_riscv_t *info,
                   std::mutex &hal_lock);
  virtual ~refsi_hal_device();

  refsi_device_t get_device() const { return device; }

  uint32_t get_word_size() const;

  /// @brief Perform device-specific initialization. This is done while holding
  /// the HAL lock.
  virtual bool initialize(refsi_locker &locker) { return true; }

  // find a specific kernel function in a compiled program
  // returns `hal_invalid_kernel` if no symbol could be found
  hal::hal_kernel_t program_find_kernel(hal::hal_program_t program,
                                        const char *name) override;

  // load an ELF file into target memory
  // returns `hal_invalid_program` if the program could not be loaded
  hal::hal_program_t program_load(const void *data,
                                  hal::hal_size_t size) override;

  // unload a program from the target
  bool program_free(hal::hal_program_t program) override;

  // allocate a memory range on the target
  // will return `hal_nullptr` if the operation was unsuccessful
  hal::hal_addr_t mem_alloc(hal::hal_size_t size,
                            hal::hal_size_t alignment) override;

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

  bool counter_read(uint32_t counter_id, uint64_t &out,
                    uint32_t index) override;

  void counter_set_enabled(bool enabled) override;

  // Concrete implementation of memory operations. The HAL lock must be held
  // when they are called.
  hal::hal_addr_t mem_alloc(hal::hal_size_t size, hal::hal_size_t alignment,
                            refsi_locker &locker);
  bool mem_free(hal::hal_addr_t addr, refsi_locker &locker);
  bool mem_read(void *dst, hal::hal_addr_t src, hal::hal_size_t size,
                refsi_locker &locker);
  bool mem_write(hal::hal_addr_t dst, const void *src, hal::hal_size_t size,
                 refsi_locker &locker);

 protected:
  bool hal_debug() const { return debug; }

  bool pack_args(std::vector<uint8_t> &packed_data, const hal::hal_arg_t *args,
                 uint32_t num_args, ELFProgram *program, uint32_t thread_mode);
  void pack_arg(std::vector<uint8_t> &packed_data, const void *value,
                size_t size, size_t align = 0);
  void pack_word_arg(std::vector<uint8_t> &packed_data, uint64_t value);
  void pack_uint32_arg(std::vector<uint8_t> &packed_data, uint32_t value,
                       size_t align = 0);
  void pack_uint64_arg(std::vector<uint8_t> &packed_data, uint64_t value,
                       size_t align = 0);

  elf_machine machine = elf_machine::unknown;
  refsi_addr_t local_ram_addr = 0;
  size_t local_ram_size = 0;
  refsi_device_t device;
  std::mutex &hal_lock;
  hal::hal_device_info_t *info = nullptr;
  std::vector<hal::util::hal_counter_value_t> hart_counter_data;
  std::vector<hal::util::hal_counter_value_t> host_counter_data;
  bool counters_enabled = false;
  bool debug = false;
  std::map<refsi_memory_map_kind, refsi_memory_map_entry> mem_map;
};

class RefSiMemoryWrapper : public MemoryDeviceBase {
 public:
  RefSiMemoryWrapper(refsi_device_t device);

  /// @brief Return zero. Memory controllers are variable-sized devices.
  size_t mem_size() const override { return 0; }

  /// @brief Try to read data from the device.
  /// @param dev_offset Offset to the start of the memory area to read from.
  /// @param len Size of the memory area to read from.
  /// @param bytes Buffer to copy the data read from the device to.
  /// @param unit_id ID of the execution unit requesting the memory access.
  /// @return true on success and false on failure.
  bool load(reg_t addr, size_t len, uint8_t *bytes, unit_id_t unit) override;

  /// @brief Try to write data to the device.
  /// @param dev_offset Offset to the start of the memory area to write to.
  /// @param len Size of the memory area to write to.
  /// @param bytes Data to write to the device.
  /// @param unit_id ID of the execution unit requesting the memory access.
  /// @return true on success and false on failure.
  bool store(reg_t addr, size_t len, const uint8_t *bytes,
             unit_id_t unit) override;

 private:
  refsi_device_t device;
};

#endif  // _HAL_REFSI_REFSI_HAL_H
