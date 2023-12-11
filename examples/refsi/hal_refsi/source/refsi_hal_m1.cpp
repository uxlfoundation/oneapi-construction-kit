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

#include "refsi_hal_m1.h"

#include <string>

#include "arg_pack.h"
#include "device/device_if.h"
#include "device/dma_regs.h"
#include "elf_loader.h"
#include "refsi_command_buffer.h"
#include "riscv_encoder.h"

// Default memory area for storing kernel ELF binaries. When the RefSi device
// does not have dedicated (TCIM) memory for storing kernel exeutables, a memory
// window is set up to map this memory area to a reserved area in DMA.
// We have increased the memory size from 1 << 20 to handle kernels larger than
// 1MiB
constexpr const uint64_t REFSI_ELF_BASE = 0x10000ull;
constexpr const uint64_t REFSI_ELF_SIZE = (1 << 27) - REFSI_ELF_BASE;

refsi_m1_hal_device::refsi_m1_hal_device(refsi_device_t device,
                                         riscv::hal_device_info_riscv_t *info,
                                         std::mutex &hal_lock)
    : refsi_hal_device(device, info, hal_lock) {
  for (uint32_t i = 0; i < CTR_NUM_COUNTERS; i++) {
    host_counter_data.push_back({i, 1});
  }
  launch_kernel_addrs.resize(DIMS, 0);
}

refsi_m1_hal_device::~refsi_m1_hal_device() {
  refsi_locker locker(hal_lock);
  mem_free(rom_base, locker);
  mem_free(elf_mem_mapped_addr, locker);
  rom_base = 0;
  elf_mem_mapped_addr = 0;
  refsiShutdownDevice(device);
}

bool refsi_m1_hal_device::initialize(refsi_locker &locker) {
  machine = elf_machine::riscv_rv64;

  // Query cores and hart counts.
  refsi_device_info_t device_info;
  if (refsi_success != refsiQueryDeviceInfo(device, &device_info)) {
    return false;
  }
  num_harts_per_core = device_info.num_harts_per_core;
  num_cores = device_info.num_cores;

  if (!createWindows(locker)) {
    return false;
  }

  if (!createROM(locker)) {
    return false;
  }

  for (uint32_t i = 0; i < REFSI_NUM_PER_HART_PERF_COUNTERS; i++) {
    hart_counter_data.push_back({i, num_harts_per_core * num_cores});
  }

  return true;
}

bool refsi_m1_hal_device::createWindows(refsi_locker &locker) {
  refsi_command_buffer cb;

  // Set up a memory window for ELF executables if needed.
  auto tcimEntry = mem_map.find(TCIM);
  if (tcimEntry != mem_map.end()) {
    // The RefSi device has TCIM, no need for an ELF window.
    elf_mem_base = tcimEntry->second.start_addr;
    elf_mem_size = tcimEntry->second.size;
  } else {
    // Allocate 'ELF' memory in DRAM, to store kernel executables.
    elf_mem_base = REFSI_ELF_BASE;
    elf_mem_size = REFSI_ELF_SIZE;
    if (elf_mem_mapped_addr) {
      mem_free(elf_mem_mapped_addr, locker);
    }
    elf_mem_mapped_addr = mem_alloc(elf_mem_size, 4096, locker);
    if (!elf_mem_mapped_addr) {
      return false;
    }
    if (!createWindow(cb, 0 /* win_id */, CMP_WINDOW_MODE_SHARED, elf_mem_base,
                      elf_mem_mapped_addr, 0, elf_mem_size)) {
      return false;
    }
  }

  // Set up a memory window for per-hart storage in TCDM. When a hart accesses
  // the memory through the window, the contents are specific to that hart due
  // to each hart accessing a different area of TCDM.
  //
  // For example, if the per-hart area of TCDM starts at 0x103e0000 and the
  // memory window at 0x14000000 with a size of 0x8000, when the first hart
  // loads a value from address 0x14000000 it will be loaded from 0x103e0000 in
  // TCDM. When the second hart does the same it will be loaded from 0x103e8000
  // (0x103e0000 + 0x8000) and so on.
  auto tcdmEntry = mem_map.find(TCDM);
  if (tcdmEntry == mem_map.end()) {
    return false;
  }
  tcdm_base = tcdmEntry->second.start_addr;
  tcdm_size = tcdmEntry->second.size;
  tcdm_hart_size = 2 * (1 << 20);
  tcdm_hart_base = tcdm_base + 64 * (1 << 20);
  tcdm_hart_target = tcdm_base + tcdm_size - tcdm_hart_size;
  tcdm_hart_size_per_hart = tcdm_hart_size / num_harts_per_core;
  if (!createWindow(cb, 1 /* win_id */, CMP_WINDOW_MODE_PERT_HART,
                    tcdm_hart_base, tcdm_hart_target, tcdm_hart_size_per_hart,
                    tcdm_hart_size_per_hart)) {
    return false;
  }

  cb.addFINISH();
  return cb.run(*this, locker) == refsi_success;
}

bool refsi_m1_hal_device::createWindow(refsi_command_buffer &cb,
                                       uint32_t win_id, uint32_t mode,
                                       refsi_addr_t base, refsi_addr_t target,
                                       uint64_t scale, uint64_t size) {
  auto base_reg = (refsi_cmp_register_id)(CMP_REG_WINDOW_BASE0 + win_id);
  auto target_reg = (refsi_cmp_register_id)(CMP_REG_WINDOW_TARGET0 + win_id);
  auto mode_reg = (refsi_cmp_register_id)(CMP_REG_WINDOW_MODE0 + win_id);
  auto scale_reg = (refsi_cmp_register_id)(CMP_REG_WINDOW_SCALE0 + win_id);

  // Break down the scale into two factors, a and b.
  if (scale > (1ull << 32)) {
    return false;
  }
  uint64_t scale_a = (scale > 0) ? 1 : 0;
  uint64_t scale_b = (scale > 0) ? scale - 1 : 0;

  // Encode the mode register value.
  uint64_t mode_val =
      CMP_WINDOW_ACTIVE | (mode & 0x6) | ((size - 1) & 0xffffffff) << 32ull;

  // Add register writes to the command buffer.
  cb.addWRITE_REG64(base_reg, base);
  cb.addWRITE_REG64(target_reg, target);
  cb.addWRITE_REG64(scale_reg, (scale_a & 0x1f) | (scale_b << 32ull));
  cb.addWRITE_REG64(mode_reg, mode_val);
  return true;
}

bool refsi_m1_hal_device::createROM(refsi_locker &locker) {
  // Create a buffer in DDR that contains hard-coded funtions necessary for
  // running kernels on M1. The address of kernel_exit should be used as the
  // kernel return address to let the simulator know when the kernel has
  // finished executing. The kernel return address being a valid address also
  // enables the driver to use a breakpoint to avoid the overhead of the machine
  // trap caused by ecall.
  riscv_encoder enc;

  // Generate code for ROM hard-coded functions.
  encodeKernelExit(enc);
  for (unsigned i = 0; i < DIMS; i++) {
    launch_kernel_addrs[i] = enc.size();
    encodeLaunchKernel(enc, i + 1);
  }

  // Write the ROM in device memory.
  rom_size = enc.size();
  rom_base = mem_alloc(rom_size, sizeof(uint64_t), locker);
  if (!rom_base || !mem_write(rom_base, enc.data(), rom_size, locker)) {
    return false;
  }
  for (unsigned i = 0; i < DIMS; i++) {
    launch_kernel_addrs[i] += rom_base;
  }

  return true;
}

void refsi_m1_hal_device::encodeKernelExit(riscv_encoder &enc) {
  enc.addLI(A0, 0);
  enc.addLI(A7, 0);
  enc.addECALL();
}

void refsi_m1_hal_device::encodeLaunchKernel(riscv_encoder &enc,
                                             unsigned num_dims) {
  auto getRankOffset = [](uint32_t offset, uint32_t rank) {
    return offset + (rank * sizeof(uint64_t));
  };

  unsigned wg_offset = offsetof(exec_state_t, wg);
  unsigned group_id_offset = offsetof(wg_info_t, group_id);
  unsigned num_groups_offset = offsetof(wg_info_t, num_groups);

  // group_id[0] = instance_id
  enc.addSW(A0, A3, getRankOffset(group_id_offset, 0));
  if (num_dims > 1) {
    // group_id[1] = slice_id % num_groups[1]
    enc.addLW(T1, A3, getRankOffset(num_groups_offset, 1));
    enc.addMulInst(REMU, T2, A1, T1);
    enc.addSW(T2, A3, getRankOffset(group_id_offset, 1));
    if (num_dims > 2) {
      // group_id[2] = slice_id / num_groups[2]
      enc.addMulInst(DIVU, T2, A1, T1);
      enc.addSW(T2, A3, getRankOffset(group_id_offset, 2));
    }
  }

  // Set the packed kernel argument pointer argument.
  enc.addMV(A0, A2);

  // Compute the address to the wg_info_t scheduling struct.
  enc.addADDI(A1, A3, wg_offset);

  // Load the kernel entry point address and call it.
  enc.addLW(T1, A3, offsetof(exec_state_t, kernel_entry));
  enc.addJR(T1);
}

bool refsi_m1_hal_device::kernel_exec(hal::hal_program_t program,
                                      hal::hal_kernel_t kernel,
                                      const hal::hal_ndrange_t *nd_range,
                                      const hal::hal_arg_t *args,
                                      uint32_t num_args, uint32_t work_dim) {
  refsi_locker locker(hal_lock);
  if ((program == hal::hal_invalid_program) ||
      (kernel == hal::hal_invalid_kernel) || !nd_range ||
      (num_args > 0) && !args) {
    return false;
  }
  refsi_hal_program *refsi_program = (refsi_hal_program *)program;
  ELFProgram *elf = refsi_program->elf.get();
  auto *kernel_wrapper = reinterpret_cast<refsi_hal_kernel *>(kernel);
  if (hal_debug()) {
    fprintf(stderr,
            "refsi_hal_device::kernel_exec(kernel=0x%08lx, num_args=%d, "
            "global=<%ld:%ld:%ld>, local=<%ld:%ld:%ld>)\n",
            kernel_wrapper->symbol, num_args, nd_range->global[0],
            nd_range->global[1], nd_range->global[2], nd_range->local[0],
            nd_range->local[1], nd_range->local[2]);
  }

  // Fill the execution state and work-group info structs.
  exec_state_t exec;
  wg_info_t &wg(exec.wg);
  memset(&exec, 0, sizeof(exec_state_t));
  exec.magic = REFSI_MAGIC;
  exec.state_size = sizeof(exec_state_t);
  exec.flags = 0;

  // Prepare N-D range dimensions.
  uint64_t work_group_size = 1;
  uint32_t max_harts = num_harts_per_core;
  wg.num_dim = work_dim;
  for (int i = 0; i < DIMS; i++) {
    wg.local_size[i] = nd_range->local[i];
    work_group_size *= wg.local_size[i];
    wg.num_groups[i] = (nd_range->global[i] / wg.local_size[i]);
    if ((wg.num_groups[i] * wg.local_size[i]) != nd_range->global[i]) {
      return false;
    }
    wg.global_offset[i] = nd_range->offset[i];
  }
  wg.hal_extra = tcdm_hart_base;

  // Ensure that ELF segments will be loaded in a valid area of memory.
  hal::hal_addr_t text_end_addr = elf_mem_base + elf_mem_size;
  for (const elf_segment &segment : elf->get_segments()) {
    if ((segment.address < elf_mem_base) ||
        (segment.address >= text_end_addr)) {
      return false;
    }
    hal::hal_addr_t segment_end = segment.address + segment.memory_size;
    if ((segment_end < elf_mem_base) || (segment_end > text_end_addr)) {
      return false;
    }
  }

  // Load ELF into Spike's memory.
  RefSiMemoryWrapper mem_device(device);
  MemoryController loader_if(&mem_device);
  if (!elf->load(loader_if)) {
    return false;
  }
  exec.kernel_entry = kernel_wrapper->symbol;

  auto alignBuffer = [](std::vector<uint8_t> &buffer, uint64_t align) {
    size_t aligned_size = (buffer.size() + align - 1) / align * align;
    buffer.resize(aligned_size);
  };

  // Pack arguments.
  uint32_t kargs_offset = 0;
  std::vector<uint8_t> packed_args;
  hal::util::hal_argpack_t packer(64);
  if (!packer.build(args, num_args)) {
    return false;
  }
  packed_args.resize(packer.size());
  memcpy(packed_args.data(), packer.data(), packer.size());
  alignBuffer(packed_args, sizeof(uint64_t));

  // Pack work-group scheduling info.
  uint32_t exec_offset = packed_args.size();
  uint32_t exec_size = sizeof(exec_state_t);
  packed_args.resize(packed_args.size() + exec_size);
  memcpy(&packed_args[exec_offset], &exec, exec_size);
  alignBuffer(packed_args, sizeof(uint64_t));

  // Allocate memory for the Kernel Uniform Block.
  uint64_t kub_align = 256;
  alignBuffer(packed_args, kub_align);
  hal::hal_addr_t kub_size = packed_args.size();
  hal::hal_addr_t kub_addr = mem_alloc(kub_size, kub_align, locker);
  if (!kub_addr || !mem_write(kub_addr, packed_args.data(), kub_size, locker)) {
    return false;
  }

  // Allocate memory for performance counters. We need to allocate two sets of
  // performance counter registers, one captured before executing the kernel
  // and one after. The reported values for the counters will be the difference
  // between the two sets of values.
  hal::hal_addr_t counters_buffer_addr = hal::hal_nullptr;
  hal::hal_addr_t counters_io_addr = hal::hal_nullptr;
  uint32_t num_counters = REFSI_NUM_PER_HART_PERF_COUNTERS;
  uint32_t counters_set_size = num_counters * sizeof(uint64_t) * max_harts;
  uint32_t counters_buffer_size = counters_set_size * 2;
  if (counters_enabled) {
    counters_io_addr = mem_map[PERF_COUNTERS].start_addr;
    counters_buffer_addr =
        mem_alloc(counters_buffer_size, sizeof(uint64_t), locker);
    if (!counters_buffer_addr) {
      return false;
    }
  }

  refsi_command_buffer cb;

  // Start a 2D DMA transfer to copy scheduling info to all harts.
  uint64_t config = REFSI_DMA_2D | REFSI_DMA_STRIDE_BOTH;
  cb.addWriteDMAReg(REFSI_REG_DMASRCADDR, kub_addr + exec_offset);
  cb.addWriteDMAReg(REFSI_REG_DMADSTADDR, tcdm_hart_target);
  cb.addWriteDMAReg(REFSI_REG_DMAXFERSIZE0 + 0, exec_size);
  cb.addWriteDMAReg(REFSI_REG_DMAXFERSIZE0 + 1, num_harts_per_core);
  cb.addWriteDMAReg(REFSI_REG_DMAXFERSRCSTRIDE0 + 0,
                    0 /* Copy the same data N times */);
  cb.addWriteDMAReg(REFSI_REG_DMAXFERDSTSTRIDE0 + 0, tcdm_hart_size_per_hart);
  cb.addWriteDMAReg(REFSI_REG_DMACTRL, config | REFSI_DMA_START);
  cb.addLOAD_REG64(CMP_REG_SCRATCH, cb.getDMARegAddr(REFSI_REG_DMASTARTSEQ));

  // Wait for the DMA transfer to finish.
  cb.addSTORE_REG64(CMP_REG_SCRATCH, cb.getDMARegAddr(REFSI_REG_DMADONESEQ));

  // Flush/invalidate the caches prior to executing the kernel. This is needed
  // when a different kernel ELF has been previously executed by the simulator.
  // Otherwise the simulator's cache will likely contain instructions and data
  // from the previous ELF. Synchronising the caches is also needed after the
  // kernel finishes executing, so that global memory contains all the changes
  // made by the kernel.
  uint32_t cache_flags = CMP_CACHE_SYNC_ACC_DCACHE | CMP_CACHE_SYNC_ACC_ICACHE;
  cb.addSYNC_CACHE(cache_flags);

  uint64_t stack_top = tcdm_hart_base + tcdm_hart_size_per_hart;
  uint64_t return_addr = rom_base;
  if (!return_addr) {
    return false;
  }
  cb.addWRITE_REG64(CMP_REG_ENTRY_PT_FN, launch_kernel_addrs[work_dim - 1]);
  cb.addWRITE_REG64(CMP_REG_STACK_TOP, stack_top);
  cb.addWRITE_REG64(CMP_REG_RETURN_ADDR, return_addr);
  if (counters_enabled) {
    // Read values from performance counters before executing the kernel.
    hal::hal_addr_t dest_addr = counters_buffer_addr;
    for (uint32_t i = 0; i < max_harts; i++) {
      uint32_t unit = REFSI_UNIT_ID(REFSI_UNIT_KIND_ACC_HART, i);
      cb.addCOPY_MEM64(counters_io_addr, dest_addr, num_counters, unit);
      dest_addr += (num_counters * sizeof(uint64_t));
    }
  }
  std::vector<uint64_t> extra_args;
  extra_args.push_back(0);                        // slice_id
  extra_args.push_back(kub_addr + kargs_offset);  // kernel arguments
  extra_args.push_back(tcdm_hart_base);           // execution state
  uint64_t num_instances = wg.num_groups[0];
  uint64_t num_slices = 0;
  num_slices = (work_dim == 2) ? wg.num_groups[1] : 1;
  num_slices =
      (work_dim == 3) ? wg.num_groups[1] * wg.num_groups[2] : num_slices;
  for (uint64_t i = 0; i < num_slices; i++) {
    extra_args[0] = i;
    cb.addRUN_INSTANCES(max_harts, num_instances, extra_args);
  }
  cb.addSYNC_CACHE(cache_flags);
  if (counters_enabled) {
    // Read values from performance counters after the kernel has finished.
    hal::hal_addr_t dest_addr = counters_buffer_addr + counters_set_size;
    for (uint32_t i = 0; i < max_harts; i++) {
      uint32_t unit = REFSI_UNIT_ID(REFSI_UNIT_KIND_ACC_HART, i);
      cb.addCOPY_MEM64(counters_io_addr, dest_addr, num_counters, unit);
      dest_addr += (num_counters * sizeof(uint64_t));
    }
  }
  cb.addFINISH();

  // Execute the command buffer.
  if (refsi_success != cb.run(*this, locker)) {
    return false;
  }

  // Compute the difference between the 'before' and 'after' performance counter
  // values.
  if (counters_enabled) {
    uint64_t *counters_before = (uint64_t *)refsiGetMappedAddress(
        device, counters_buffer_addr, counters_buffer_size);
    if (counters_before) {
      uint64_t *counters_after = &counters_before[num_counters * max_harts];
      for (uint32_t j = 0; j < max_harts; j++) {
        for (uint32_t i = 0; i < num_counters; i++) {
          uint64_t delta = counters_after[i] - counters_before[i];
          hart_counter_data[i].set_value(j, delta);
        }
        counters_before += num_counters;
        counters_after += num_counters;
      }
    }
  }

  mem_free(kub_addr, locker);
  mem_free(counters_buffer_addr, locker);
  return true;
}

bool refsi_m1_hal_device::mem_copy(hal::hal_addr_t dst, hal::hal_addr_t src,
                                   hal::hal_size_t size) {
  refsi_locker locker(hal_lock);

  if (hal_debug()) {
    fprintf(stderr,
            "refsi_hal_device::mem_copy(dst=0x%08lx, src=0x%08lx, "
            "size=%ld)\n",
            dst, src, size);
  }

  refsi_command_buffer cb;

  // Start a 1D DMA transfer to copy data from one buffer to another.
  uint64_t config = REFSI_DMA_1D | REFSI_DMA_STRIDE_NONE;
  cb.addWriteDMAReg(REFSI_REG_DMASRCADDR, src);
  cb.addWriteDMAReg(REFSI_REG_DMADSTADDR, dst);
  cb.addWriteDMAReg(REFSI_REG_DMAXFERSIZE0, size);
  cb.addWriteDMAReg(REFSI_REG_DMACTRL, config | REFSI_DMA_START);
  cb.addLOAD_REG64(CMP_REG_SCRATCH, cb.getDMARegAddr(REFSI_REG_DMASTARTSEQ));

  // Wait for the DMA transfer to finish.
  cb.addSTORE_REG64(CMP_REG_SCRATCH, cb.getDMARegAddr(REFSI_REG_DMADONESEQ));

  // Execute the command buffer. Do not update the host performance counters,
  // since the data is not leaving the device.
  return refsi_success == cb.run(*this, locker);
}
