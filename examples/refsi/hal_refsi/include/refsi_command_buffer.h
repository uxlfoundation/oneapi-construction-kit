// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#ifndef _HAL_REFSI_REFSI_COMMAND_BUFFER_H
#define _HAL_REFSI_REFSI_COMMAND_BUFFER_H

#include <vector>

#include "refsidrv/refsidrv.h"
#include "refsi_hal.h"

class refsi_hal_device;

/// @brief Utility class that can be used to generate RefSi command buffers and
/// execute them on a RefSi device.
class refsi_command_buffer {
 public:
  /// @brief Add a command to stop execution of commands in the command buffer.
  void addFINISH();

  /// @brief Add a command to store a 64-bit value to a CMP register.
  /// @param reg CMP register index to write to.
  /// @param value Immediate value to write to the register.
  void addWRITE_REG64(refsi_cmp_register_id reg, uint64_t value);

  /// @brief Add a command to store a 64-bit value to memory.
  /// @param dest_addr Memory address to write the 64-bit value to.
  /// @param value Immediate value to write to memory.
  void addSTORE_IMM64(refsi_addr_t dest_addr, uint64_t value);

  /// @brief Add a command to load a 64-bit value to a CMP register.
  /// @param reg CMP register index.
  /// @param src_addr Memory address to load a 64-bit value from.
  void addLOAD_REG64(refsi_cmp_register_id reg, uint64_t src_addr);

  /// @brief Add a command to store the contents of a CMP register to memory.
  /// @param reg CMP register index.
  /// @param dest_addr Memory address to write the CMP register value to.
  void addSTORE_REG64(refsi_cmp_register_id reg, uint64_t dest_addr);

  /// @brief Add a command to copy several 64-bit values from one memory region
  /// to another. Values are copied individually, so that addresses belonging to
  /// I/O devices can be used with this command (either as source or
  /// destination).
  /// @param src_addr Memory address to copy 64-bit values from.
  /// @param dest_addr Memory address to copy 64-bit values to.
  /// @param unit_id ID of the execution unit to use for memory requests.
  void addCOPY_MEM64(uint64_t src_addr, uint64_t dest_addr, uint32_t count,
                     uint32_t unit_id);

  /// @brief Add a command to run a kernel slice on the device.
  /// @param max_harts Maximum number of harts to use for running the kernel.
  /// @param num_instances How many times the kernel will be invoked.
  /// @param slice_id Identifies the kernel slice command as part of a batch.
  void addRUN_KERNEL_SLICE(uint32_t max_harts, uint64_t num_instances,
                           uint64_t slice_id);

  /// @brief Add a command to run multiple instances of a kernel on the device.
  /// @param max_harts Maximum number of harts to use for running the kernel.
  /// @param num_instances How many times the kernel will be invoked.
  /// @param extra_args Extra arguments to pass to the kernel entry point.
  void addRUN_INSTANCES(uint32_t max_harts, uint64_t num_instances,
                        std::vector<uint64_t> &extra_args);

  /// @brief Add a command to flush and/or invalidate caches in the SoC.
  /// @param flags Flags to control which cache(s) get invalidated/flushed.
  void addSYNC_CACHE(uint32_t flags);

  /// @brief Add a command to write an immediate value to a DMA register.
  /// @param dma_reg DMA register index.
  /// @param value Immediate value to write to the register.
  void addWriteDMAReg(uint32_t dma_reg, uint64_t value);

  /// @brief Retrieve the address of a DMA register.
  /// @param dma_reg DMA register index.
  /// @return Address of the DMA register in memory.
  refsi_addr_t getDMARegAddr(uint32_t dma_reg) const;

  /// @brief Execute the commands that have been added to the buffer.
  /// @param hal_device Device to execute the command buffer.
  /// @param locker Mutex for the HAL device.
  refsi_result run(refsi_hal_device &hal_device, refsi_locker &locker);

private:
  std::vector<uint64_t> chunks;
};

#endif  // _HAL_REFSI_REFSI_COMMAND_BUFFER_H
