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

#ifndef _REFSIDRV_REFSI_DEVICE_H
#define _REFSIDRV_REFSI_DEVICE_H

#include <memory>
#include <mutex>
#include <vector>

#include "allocator.h"
#include "common_devices.h"
#include "device/dma_regs.h"
#include "devices.h"
#include "elf_loader.h"
#include "refsidrv.h"

struct RefSiAccelerator;
struct RefSiMemoryController;

using RefSiLock = std::unique_lock<std::mutex>;

/// @brief Constants used to describe the configure of a RefSi device.
enum refsi_device_constants {
  num_cores = 1,  // TODO: Multi-core support
  num_harts_per_core = 4,
  core_vlen = 512,
  core_elen = 64,
  num_memory_windows = CMP_NUM_WINDOWS,
  num_global_perf_counters = REFSI_NUM_GLOBAL_PERF_COUNTERS,
  num_per_hart_perf_counters = REFSI_NUM_PER_HART_PERF_COUNTERS
};
#define REFSI_ISA "RV64GCVZbc"

/// @brief List of memory regions accessible to a RefSi device. This includes
/// different kinds of memory such as TCIM, TCDM, DRAM as well as memory-mapped
/// regions such as DMA registers and host I/O.
enum refsi_device_memory_regions {
  /// @brief Base address of the TCDM memory region.
  tcdm_base = 0x10000000,
  /// @brief Size of the TCDM memory region, in bytes.
  tcdm_size = 4 * (1 << 20),
  /// @brief Size of the hart-specific part of the TCDM memory region, in bytes.
  tcdm_hart_size = 2 * (1 << 20),
  /// @brief Base address of the hart-specific part of the TCDM memory region.
  tcdm_hart_base = tcdm_base + tcdm_size - tcdm_hart_size,
  /// @brief Base address of the DMA registers memory region.
  dma_io_base = 0x20002000,
  /// @brief Size of the DMA registers memory region, in bytes.
  dma_io_size = REFSI_DMA_NUM_REGS * sizeof(uint64_t),
  /// @brief Base address of the performance counter registers memory region.
  perf_counters_io_base = 0x020100000,
  /// @brief Size of the performance counter registers memory region, in bytes.
  perf_counters_io_size = REFSI_NUM_PERF_COUNTERS * sizeof(uint64_t),
  /// @brief Base address of the DRAM memory region.
  dram_base = 0x40000000,
  /// @brief Size of the DRAM memory region, in bytes.
  dram_size = 2 * (1ull << 30)
};

/// @brief Lists the different RefSi SoC families.
enum class refsi_soc_family {
  /// @brief RefSi 'M' family (e.g. M1).
  m = 1,
  /// @brief RefSi 'G' family (e.g. G1).
  g = 2
};

/// @brief Represents and gives control to a virtual RefSi device. This class
/// allows device memory to be allocated, data transfers between host and device
/// memory to be performed as well as command buffers to be executed.
struct RefSiDevice {
  /// @brief Create a new device.
  RefSiDevice(refsi_soc_family family);

  /// @brief Shut down the device.
  virtual ~RefSiDevice();

  /// @brief Identifies the SoC Family for the RefSi Device.
  refsi_soc_family getFamily() const { return family; }

  /// @brief Access the device's accelerator.
  RefSiAccelerator &getAccelerator();

  /// @brief Access the device's memory interface.
  RefSiMemoryController &getMemory();

  /// @brief Return the lock used to protect device state.
  std::mutex & getLock() { return mutex; }

  /// @brief Whether debug output is enabled or not.
  bool getDebug() const { return debug; }

  /// @brief Perform device initialization.
  virtual refsi_result initialize() { return refsi_success; }

  /// @brief Query information about the device.
  /// @param device_info To be filled with information about the device.
  virtual refsi_result queryDeviceInfo(refsi_device_info_t &device_info);

  /// @brief Access the device's memory map.
  const std::vector<refsi_memory_map_entry> &getMemoryMap() const;

  // Device memory allocation.

  /// @brief Allocate device memory.
  /// @param size Size of the memory range to allocate, in bytes.
  /// @param alignment Minimum alignment for the returned physical address.
  /// @param kind Kind of memory to allocate, e.g. DRAM, scratchpad.
  refsi_addr_t allocDeviceMemory(size_t size, size_t alignment,
                                 refsi_memory_map_kind kind);

  /// @brief Free device memory allocated with refsiAllocDeviceMemory.
  /// @param phys_addr Device address to free.
  refsi_result freeDeviceMemory(refsi_addr_t phys_addr);

  // Device memory access.

  /// @brief Get a CPU-accessible pointer that maps to the given device address.
  /// Device memory is first mapped when a connection to the device is
  /// established and unmapped when the connection is closed.
  /// @param phys_addr Device address to map to a virtual address (CPU pointer).
  /// @param size Size of the memory range to access, in bytes.
  void *getMappedAddress(refsi_addr_t phys_addr, size_t size);

  /// @brief Flush any changes to device memory from the CPU cache.
  /// @param phys_addr Device address that defines the start of the memory range
  /// to flush from the CPU cache.
  /// @param size Size of the memory range to flush, in bytes.
  refsi_result flushDeviceMemory(refsi_addr_t phys_addr, size_t size);

  /// @brief Invalidate any cached device data from the CPU cache.
  /// @param phys_addr Device address that defines the start of the memory range
  /// to invalidate from the CPU cache.
  /// @param size Size of the memory range to invalidate, in bytes.
  refsi_result invalidateDeviceMemory(refsi_addr_t phys_addr, size_t size);

  /// @brief Read data from device memory.
  /// @param dest Buffer to copy read data to.
  /// @param phys_addr Device address that defines the start of the memory range
  /// to read from.
  /// @param size Size of the memory range to read, in bytes.
  /// @param unit_id UnitID of the execution unit to use when making memory
  /// requests. This is usually 'external' but hart IDs can also be used.
  refsi_result readDeviceMemory(uint8_t *dest, refsi_addr_t phys_addr,
                                size_t size, uint32_t unit_id);

  /// @brief Write data to device memory.
  /// @param device Device to write to.
  /// @param phys_addr Device address that defines the start of the memory range
  /// to write to.
  /// @param source Buffer that contains the data to write to device memory.
  /// @param size Size of the memory range to write, in bytes.
  /// @param unit_id UnitID of the execution unit to use when making memory
  /// requests. This is usually 'external' but hart IDs can also be used.
  refsi_result writeDeviceMemory(refsi_addr_t phys_addr, const uint8_t *source,
                                 size_t size, uint32_t unit_id);

 protected:
  std::mutex mutex;
  refsi_soc_family family;
  hal::allocator_t allocator;
  std::unique_ptr<RefSiAccelerator> accelerator;
  std::unique_ptr<RefSiMemoryController> mem_ctl;
  bool debug = false;
};

#endif  // _REFSIDRV_REFSI_DEVICE_H
