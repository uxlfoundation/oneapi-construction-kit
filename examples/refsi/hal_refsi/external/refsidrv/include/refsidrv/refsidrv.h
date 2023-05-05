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

#ifndef _REFSIDRV_REFSIDRV_H
#define _REFSIDRV_REFSIDRV_H

#include <stddef.h>
#include <stdint.h>

struct RefSiDevice;

typedef uint64_t refsi_addr_t;
typedef struct RefSiDevice *refsi_device_t;

#if defined _WIN32 || defined __CYGWIN__
#define REFSI_DLL_IMPORT __declspec(dllimport)
#define REFSI_DLL_EXPORT __declspec(dllexport)
#else
#define REFSI_DLL_IMPORT __attribute__((visibility("default")))
#define REFSI_DLL_EXPORT __attribute__((visibility("default")))
#endif

#ifdef BUILD_REFSI_DLL
#define REFSI_API REFSI_DLL_EXPORT
#else
#define REFSI_API REFSI_DLL_IMPORT
#endif

#if defined(__cplusplus)
extern "C" {
#endif

/// @brief Return value from a RefSi driver call.
enum refsi_result {
  refsi_success = 0,
  refsi_failure,
  refsi_invalid_device,
  refsi_device_closed,
  refsi_not_supported
};

enum { refsi_nullptr = 0 };

/// @brief Represents the kind of RefSi device to control.
enum refsi_device_family {
  REFSI_DEFAULT = 0,
  REFSI_M = 1,
  REFSI_G = 2
};

// Driver start-up and teardown.

/// @brief Initialize the driver. No other driver function can be called prior
/// to calling this function.
REFSI_API refsi_result refsiInitialize();

/// @brief Terminate the driver. No driver function other than refsiInitialize
/// can be called after calling this function.
REFSI_API refsi_result refsiTerminate();

// Device management.

/// @brief Open the device. This establishes a connection with the device and
/// ensures that it has been successfully started. Device memory functions, as
/// well as command buffer execution functions, can only be called after this
/// function has been called.
/// @param family Type of RefSi device to open a connection to.
REFSI_API refsi_device_t refsiOpenDevice(refsi_device_family family);

/// @brief Shut down the device. Any pointer returned by refsiGetMappedAddress
/// can no longer be used after this function is called.
REFSI_API refsi_result refsiShutdownDevice(refsi_device_t device);

// Device queries.

/// @brief Provides information about the device.
typedef struct refsi_device_info {
  /// @brief Kind of device.
  refsi_device_family family;
  /// @brief Number of accelerator cores contained within the device.
  unsigned num_cores;
  /// @brief Number of hardware threads contained in each accelerator core.
  unsigned num_harts_per_core;
  /// @brief Number of entries in the device's memory map.
  unsigned num_memory_map_entries;
  /// @brief String that describes the ISA exposed by the cores.
  const char *core_isa;
  /// @brief Width of the cores' vector registers, in bits.
  unsigned core_vlen;
  /// @brief Maximum width of an element in a vector register, in bits.
  unsigned core_elen;
} refsi_device_info_t;

/// @brief Query information about the device.
/// @param device Device to query information for.
/// @param device_info To be filled with information about the device.
REFSI_API refsi_result refsiQueryDeviceInfo(refsi_device_t device,
                                            refsi_device_info_t *device_info);

/// @brief Enumerates entries in the device's memory map.
enum refsi_memory_map_kind {
  /// @brief The kind of memory for this memory map entry is unknown.
  UNKNOWN = 0,
  /// @brief Refers to the area of memory where the device's dedicated memory is
  /// mapped. DRAM is shared between all device cores.
  DRAM = 1,
  /// @brief Refers to the area of memory where the device's entire
  /// tightly-coupled instruction memory is mapped, for all cores.
  TCIM = 2,
  /// @brief Refers to the area of memory where the device's entire
  /// tightly-coupled data memory is mapped, for all cores.
  TCDM = 3,
  /// @brief Refers to the area of memory where each core's tightly-coupled data
  /// memory is mapped. This range has the same address for all cores, however
  /// each core will see different contents when accessing it.
  TCDM_PRIVATE = 4,
  /// @brief Refers to the area of memory where Kernel DMA registers are mapped
  /// for all hardware threads.
  KERNEL_DMA = 5,
  /// @brief Refers to the area of memory where a hardware thread's Kernel DMA
  /// registers are mapped. This range has the same address for all hardware
  /// threads, however each hart will see different contents when accessing it.
  KERNEL_DMA_PRIVATE = 6,
  /// @brief Refers to the area of memory where Performance Counter registers
  /// are mapped. This is divided into a per-hardware-thread area and a global
  /// area shared between all units in the RefSi device..
  PERF_COUNTERS = 7,
};

/// @brief Represents an entry in the device's memory map.
struct refsi_memory_map_entry {
  /// @brief Kind of memory this memory range refers to.
  refsi_memory_map_kind kind;
  /// @brief Starting address of the memory range in device memory.
  refsi_addr_t start_addr;
  /// @brief Size of the memory range in device memory, in bytes.
  size_t size;
};

/// @brief Query an entry in the device's memory map.
/// @param device Device to query memory map info for.
/// @param index Index of the entry to query.
/// @param entry To be filled with information about the memory map entry.
REFSI_API refsi_result refsiQueryDeviceMemoryMap(refsi_device_t device,
                                                 size_t index,
                                                 refsi_memory_map_entry *entry);

// Device memory allocation.

/// @brief Allocate device memory.
/// @param device Device to allocate memory on.
/// @param size Size of the memory range to allocate, in bytes.
/// @param alignment Minimum alignment for the returned physical address.
/// @param kind Kind of memory to allocate, e.g. DRAM, scratchpad.
REFSI_API refsi_addr_t refsiAllocDeviceMemory(refsi_device_t device,
                                              size_t size, size_t alignment,
                                              refsi_memory_map_kind kind);

/// @brief Free device memory allocated with refsiAllocDeviceMemory.
/// @param device Device to free memory from.
/// @param phys_addr Device address to free.
REFSI_API refsi_result refsiFreeDeviceMemory(refsi_device_t device,
                                             refsi_addr_t phys_addr);

// Device memory access.

/// @brief Get a CPU-accessible pointer that maps to the given device address.
/// Device memory is first mapped when a connection to the device is established
/// and unmapped when the connection is closed.
/// @param device Device to retrieve a mapped pointer for.
/// @param phys_addr Device address to map to a virtual address (CPU pointer).
/// @param size Size of the memory range to access, in bytes.
REFSI_API void *refsiGetMappedAddress(refsi_device_t device,
                                      refsi_addr_t phys_addr, size_t size);

/// @brief Flush any changes to device memory from the CPU cache.
/// @param device Device to flush data changes to.
/// @param phys_addr Device address that defines the start of the memory range
/// to flush from the CPU cache.
/// @param size Size of the memory range to flush, in bytes.
REFSI_API refsi_result refsiFlushDeviceMemory(refsi_device_t device,
                                              refsi_addr_t phys_addr,
                                              size_t size);

/// @brief Invalidate any cached device data from the CPU cache.
/// @param device Device to flush data changes from.
/// @param phys_addr Device address that defines the start of the memory range
/// to invalidate from the CPU cache.
/// @param size Size of the memory range to invalidate, in bytes.
REFSI_API refsi_result refsiInvalidateDeviceMemory(refsi_device_t device,
                                                   refsi_addr_t phys_addr,
                                                   size_t size);

/// @brief Read data from device memory.
/// @param device Device to read from.
/// @param dest Buffer to copy read data to.
/// @param phys_addr Device address that defines the start of the memory range
/// to read from.
/// @param size Size of the memory range to read, in bytes.
/// @param unit_id UnitID of the execution unit to use when making memory
/// requests. This is usually 'external' but hart IDs can also be used.
REFSI_API refsi_result refsiReadDeviceMemory(refsi_device_t device,
                                             uint8_t *dest,
                                             refsi_addr_t phys_addr,
                                             size_t size, uint32_t unit_id);

/// @brief Write data to device memory.
/// @param device Device to write to.
/// @param phys_addr Device address that defines the start of the memory range
/// to write to.
/// @param source Buffer that contains the data to write to device memory.
/// @param size Size of the memory range to write, in bytes.
/// @param unit_id UnitID of the execution unit to use when making memory
/// requests. This is usually 'external' but hart IDs can also be used.
REFSI_API refsi_result refsiWriteDeviceMemory(refsi_device_t device,
                                              refsi_addr_t phys_addr,
                                              const uint8_t *source,
                                              size_t size, uint32_t unit_id);

// Device execution.

/// @brief Asynchronously execute a series of commands on the device.
/// @param device Device to execute a command buffer on.
/// @param cb_addr Address of the command buffer in device memory.
/// @param size Size of the command buffer, in bytes.
REFSI_API refsi_result refsiExecuteCommandBuffer(refsi_device_t device,
                                                 refsi_addr_t cb_addr,
                                                 size_t size);

/// @brief Wait for all previously enqueued command buffers to be finished.
/// @param device Device to wait for.
REFSI_API void refsiWaitForDeviceIdle(refsi_device_t device);

/// @brief Synchronously execute a kernel on the device. Only supported on RefSi
/// G1 devices.
REFSI_API refsi_result refsiExecuteKernel(refsi_device_t device,
                                          refsi_addr_t entry_fn_addr,
                                          uint32_t num_harts);

/// @brief Identifies a command that can be executed by the command processor.
enum refsi_cmp_command_id {
  CMP_NOP = 0,
  CMP_FINISH = 1,
  CMP_WRITE_REG64 = 2,
  CMP_LOAD_REG64 = 3,
  CMP_STORE_REG64 = 4,
  CMP_STORE_IMM64 = 5,
  CMP_COPY_MEM64 = 6,
  CMP_RUN_KERNEL_SLICE = 7,
  CMP_RUN_INSTANCES = 8,
  CMP_SYNC_CACHE = 9
};

/// @brief Try to decode a CMP command header.
/// @param opcode Populated with the command's opcode.
/// @param chunk_count Populated with the command's chunk count.
/// @param inline_chunk Populated with the command's inline chunk.
REFSI_API refsi_result refsiDecodeCMPCommand(uint64_t header,
                                             refsi_cmp_command_id *opcode,
                                             uint32_t *chunk_count,
                                             uint32_t *inline_chunk);

/// @brief Encode a CMP command header.
/// @param opcode Command's opcode to encode.
/// @param chunk_count Command's chunk count to encode.
/// @param inline_chunk Command's inline chunk to encode.
REFSI_API uint64_t refsiEncodeCMPCommand(refsi_cmp_command_id opcode,
                                         uint32_t chunk_count,
                                         uint32_t inline_chunk);

#define CMP_NUM_WINDOWS 8

#define REFSI_NUM_GLOBAL_PERF_COUNTERS      32
#define REFSI_NUM_PER_HART_PERF_COUNTERS    32
#define REFSI_NUM_PERF_COUNTERS    (REFSI_NUM_GLOBAL_PERF_COUNTERS + \
                                    REFSI_NUM_PER_HART_PERF_COUNTERS)

/// @brief Identifies a command processor register.
enum refsi_cmp_register_id {
  CMP_REG_SCRATCH = 0,
  CMP_REG_ENTRY_PT_FN = 1,
  CMP_REG_KUB_DESC = 2,
  CMP_REG_KARGS_INFO = 3,
  CMP_REG_TSD_INFO = 4,
  CMP_REG_STACK_TOP = 5,
  CMP_REG_RETURN_ADDR = 6,
  CMP_REG_WINDOW_BASE0 = 8,
  CMP_REG_WINDOW_BASEn = CMP_REG_WINDOW_BASE0 + CMP_NUM_WINDOWS - 1,
  CMP_REG_WINDOW_TARGET0 = CMP_REG_WINDOW_BASEn + 1,
  CMP_REG_WINDOW_TARGETn = CMP_REG_WINDOW_TARGET0 + CMP_NUM_WINDOWS - 1,
  CMP_REG_WINDOW_MODE0 = CMP_REG_WINDOW_TARGETn + 1,
  CMP_REG_WINDOW_MODEn = CMP_REG_WINDOW_MODE0 + CMP_NUM_WINDOWS - 1,
  CMP_REG_WINDOW_SCALE0 = CMP_REG_WINDOW_MODEn + 1,
  CMP_REG_WINDOW_SCALEn = CMP_REG_WINDOW_SCALE0 + CMP_NUM_WINDOWS - 1,
  CMP_NUM_REGS = CMP_REG_WINDOW_SCALEn + 1
};

// Extract the ENTRY_POINT_ADDR field from the CMP_REG_ENTRY_PT_FN register.
#define CMP_GET_ENTRY_POINT_ADDR(reg) ((reg) & 0xffffffffull)

// Extract the KUB_ADDR field from the CMP_REG_KUB_DESC register.
#define CMP_GET_KUB_ADDR(reg) ((reg) & 0xffffffffffffull)

// Extract the KUB_SIZE field from the CMP_REG_KUB_DESC register.
#define CMP_GET_KUB_SIZE(reg) ((reg) >> 48ull)

// Extract the KARGS_OFFSET field from the CMP_REG_KARGS_INFO register.
#define CMP_GET_KARGS_OFFSET(reg) (((reg) >> 16ull) & 0xffffffull)

// Extract the KARGS_SIZE field from the CMP_REG_KARGS_INFO register.
#define CMP_GET_KARGS_SIZE(reg) ((reg) >> 40ull)

// Extract the TSD_OFFSET field from the CMP_REG_TSD_INFO register.
#define CMP_GET_TSD_OFFSET(reg) (((reg) >> 16ull) & 0xffffffull)

// Extract the TSD_SIZE field from the CMP_REG_TSD_INFO register.
#define CMP_GET_TSD_SIZE(reg) ((reg) >> 40ull)

// Extract the ACTIVE field from the CMP_REG_WINDOW_MODEx register.
#define CMP_GET_WINDOW_ACTIVE(reg) ((reg) & 0x1)

// Extract the MODE field from the CMP_REG_WINDOW_MODEx register.
#define CMP_GET_WINDOW_MODE(reg) ((reg) & 0x6)

// Extract the SIZE field from the CMP_REG_WINDOW_MODEx register.
#define CMP_GET_WINDOW_SIZE(reg) (((reg) >> 32ull) + 1)

// Extract the SCALE_A field from the CMP_REG_WINDOW_SCALEx register.
#define CMP_GET_WINDOW_SCALE_A(reg) ((reg) & 0x1f)

// Extract the SCALE_B field from the CMP_REG_WINDOW_SCALEx register.
#define CMP_GET_WINDOW_SCALE_B(reg) ((reg) >> 32ull)

#define CMP_WINDOW_ACTIVE             1
#define CMP_WINDOW_MODE_SHARED        0
#define CMP_WINDOW_MODE_PERT_HART     2

#define CMP_CACHE_SYNC_ACC_DCACHE     1
#define CMP_CACHE_SYNC_ACC_ICACHE     2

/// @brief Identifies a RefSi performance counter.
enum refsi_perf_counter_id {
  REFSI_PERF_CNTR_CYCLE = 0,
  REFSI_PERF_CNTR_RETIRED_INSN = 2,
  REFSI_PERF_CNTR_READ_BYTE_INSN = 3,
  REFSI_PERF_CNTR_READ_SHORT_INSN = 4,
  REFSI_PERF_CNTR_READ_WORD_INSN = 5,
  REFSI_PERF_CNTR_READ_DOUBLE_INSN = 6,
  REFSI_PERF_CNTR_READ_QUAD_INSN = 7,
  REFSI_PERF_CNTR_READ_INSN = 8,
  REFSI_PERF_CNTR_WRITE_BYTE_INSN = 9,
  REFSI_PERF_CNTR_WRITE_SHORT_INSN = 10,
  REFSI_PERF_CNTR_WRITE_WORD_INSN = 11,
  REFSI_PERF_CNTR_WRITE_DOUBLE_INSN = 12,
  REFSI_PERF_CNTR_WRITE_QUAD_INSN = 13,
  REFSI_PERF_CNTR_WRITE_INSN = 14,
  REFSI_PERF_CNTR_INT_INSN = 15,
  REFSI_PERF_CNTR_FLOAT_INSN = 16,
  REFSI_PERF_CNTR_BRANCH_INSN = 17,
};

// Create a new unit ID from a unit kind and unit index.
#define REFSI_UNIT_ID(kind, index) ((((kind) & 0xff) << 24) | (index))

// Retrieve the unit kind from a unit ID.
#define REFSI_GET_UNIT_KIND(unit) (((unit) & 0xff000000) >> 24)

// Retrieve the unit index from a unit ID.
#define REFSI_GET_UNIT_INDEX(unit) ((unit_id) & 0xffff)

/// @brief Identifies a RefSi execution unit by its kind.
enum refsi_unit_kind {
  /// @brief The unit kind is not known or does not matter.
  REFSI_UNIT_KIND_ANY = 0,
  /// @brief The unit is external to the RefSi device (e.g. host).
  REFSI_UNIT_KIND_EXTERNAL = 1,
  /// @brief The unit is the command processor (CMP).
  REFSI_UNIT_KIND_CMP = 2,
  /// @brief The unit is a particular hart of an accelerator core.
  REFSI_UNIT_KIND_ACC_HART = 3,
  /// @brief The unit is a particular accelerator core.
  REFSI_UNIT_KIND_ACC_CORE = 4,
};

#if defined(__cplusplus)
}
#endif

#endif  // _REFSIDRV_REFSIDRV_H
