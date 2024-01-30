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

#include "refsi_hal_g1.h"

#include <time.h>

#include <string>

#include "arg_pack.h"
#include "device/device_if.h"
#include "device/memory_map.h"
#include "elf_loader.h"
#include "refsi_g1_loader_binary.h"

// Default memory area for storing kernel ELF binaries. When the RefSi device
// does not have dedicated (TCIM) memory for storing kernel exeutables, a memory
// window is set up to map this memory area to a reserved area in DMA.
// We have increased the memory size from 1 << 20 to handle kernels larger than
// 1MiB
constexpr const uint64_t REFSI_ELF_BASE = 0x10000ull;
constexpr const uint64_t REFSI_ELF_SIZE = (1 << 27) - REFSI_ELF_BASE;

constexpr const uint64_t REFSI_MAX_HARTS = 64;

static float time_diff_in_sec(timespec &start, timespec &end) {
  return (float)(end.tv_sec - start.tv_sec) +
         ((float)((end.tv_nsec - start.tv_nsec) * 1e-6f)) * 1e-3f;
}

refsi_g1_hal_device::refsi_g1_hal_device(refsi_device_t device,
                                         riscv::hal_device_info_riscv_t *info,
                                         std::mutex &hal_lock)
    : refsi_hal_device(device, info, hal_lock) {
  local_ram_addr = REFSI_LOCAL_MEM_ADDRESS;
  local_ram_size = REFSI_LOCAL_MEM_END_ADDRESS - REFSI_LOCAL_MEM_ADDRESS;

  for (uint32_t i = 0; i < REFSI_NUM_PER_HART_PERF_COUNTERS; i++) {
    hart_counter_data.push_back({i, REFSI_MAX_HARTS});
  }

  for (uint32_t i = 0; i < CTR_NUM_COUNTERS; i++) {
    host_counter_data.push_back({i, 1});
  }
}

refsi_g1_hal_device::~refsi_g1_hal_device() {
  refsi_locker locker(hal_lock);

  refsiShutdownDevice(device);
}

bool refsi_g1_hal_device::initialize(refsi_locker &locker) {
  refsi_device_info_t device_info;
  if (refsi_success != refsiQueryDeviceInfo(device, &device_info)) {
    return false;
  }
  max_harts = device_info.num_harts_per_core;

  // Query memory map ranges.
  auto perf_counters = mem_map.find(PERF_COUNTERS);
  if (perf_counters != mem_map.end()) {
    perf_counters_addr = perf_counters->second.start_addr;
  }

  // Open the RISC-V loader binary.
  std::string isa_str(device_info.core_isa);
  if (isa_str.find("RV32") == 0) {
    machine = elf_machine::riscv_rv32;
  } else if (isa_str.find("RV64") == 0) {
    machine = elf_machine::riscv_rv64;
  } else {
    fprintf(stderr, "error: unsupported RISC-V ISA: %s\n",
            device_info.core_isa);
    return false;
  }
  if (!open_loader()) {
    return false;
  }

  return true;
}

bool refsi_g1_hal_device::open_loader() {
  // Load the refsi_g1_loader ELF binary into device memory.

  BufferDevice source(refsi_g1_loader_binary, refsi_g1_loader_binary_size);
  std::unique_ptr<ELFProgram> program(new ELFProgram);

  if (!program->read(source)) {
    fprintf(stderr, "error: could not read the loader ELF\n");
    return false;
  } else if (program->get_machine() != machine) {
    fprintf(stderr, "error: the loader ELF has an invalid architecture\n");
    return false;
  }
  RefSiMemoryWrapper wrapper(device);
  if (!program->load(wrapper)) {
    fprintf(stderr, "error: could not load the loader ELF in memory\n");
    return false;
  }
  loader.swap(program);
  return true;
}

bool refsi_g1_hal_device::kernel_exec(hal::hal_program_t program,
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
  auto *kernel_wrapper = reinterpret_cast<refsi_hal_kernel *>(kernel);
  if (hal_debug()) {
    fprintf(stderr,
            "refsi_hal_device::kernel_exec(kernel=0x%08lx, num_args=%d,"
            " global=<%ld:%ld:%ld>, local=<%ld:%ld:%ld>)\n",
            kernel_wrapper->symbol, num_args, nd_range->global[0],
            nd_range->global[1], nd_range->global[2], nd_range->local[0],
            nd_range->local[1], nd_range->local[2]);
  }

  timespec start, end;
  if (hal_debug()) {
    clock_gettime(CLOCK_MONOTONIC, &start);
  }

  RefSiMemoryWrapper wrapper(device);
  MemoryController mem_ctl(&wrapper);
  refsi_hal_program *refsi_program = (refsi_hal_program *)program;
  ELFProgram *elf = refsi_program->elf.get();
  // Load ELF into Spike's memory
  if (!elf->load(mem_ctl)) {
    return false;
  }

  uint32_t thread_mode = 0;
#if defined(HAL_REFSI_MODE_WG)
  thread_mode = REFSI_THREAD_MODE_WG;
#elif defined(HAL_REFSI_MODE_WI)
  thread_mode = REFSI_THREAD_MODE_WI;
#else
#error Either HAL_REFSI_MODE_WG or HAL_REFSI_MODE_WI needs to be defined.
#endif
  uint32_t flags = thread_mode;

  uint64_t work_group_size = 1;

  // Fill the execution state struct.
  exec_state_t exec;
  wg_info_t &wg(exec.wg);
  memset(&exec, 0, sizeof(exec_state_t));
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
  wg.hal_extra = REFSI_CONTEXT_ADDRESS;
  exec.kernel_entry = kernel_wrapper->symbol;
  exec.magic = REFSI_MAGIC;
  exec.state_size = sizeof(exec_state_t);
  exec.flags = flags;

  // Determine how many harts should be used to execute the kernel.
  size_t num_harts = (thread_mode == REFSI_THREAD_MODE_WG)
                         ? NUM_HARTS_FOR_CA_MODE
                         : work_group_size;
  if (num_harts > max_harts) {
    return false;
  }

  // Pack arguments.
  std::vector<uint8_t> packed_args;
  if (!pack_args(packed_args, args, num_args, elf, exec.flags)) {
    return false;
  }
  hal::hal_addr_t args_addr = refsiAllocDeviceMemory(device, packed_args.size(),
                                                     sizeof(uint64_t), DRAM);
  if (!args_addr ||
      (!packed_args.empty() &&
       !mem_ctl.store(args_addr, packed_args.size(), packed_args.data(),
                      make_unit(unit_kind::external)))) {
    return false;
  }
  exec.packed_args = args_addr;

  // Specialize the execution state struct for each hardware thread.
  std::vector<exec_state_t> exec_for_hart(num_harts);
  for (size_t hart_id = 0; hart_id < num_harts; hart_id++) {
    // Copy the execution state 'template' to this thread.
    uint32_t unit_id = REFSI_UNIT_ID(REFSI_UNIT_KIND_ACC_HART, hart_id);
    exec_state_t &thread_exec(exec_for_hart[hart_id]);
    memcpy(&thread_exec, &exec, sizeof(exec_state_t));
    thread_exec.thread_id = hart_id;
    if (auto error = refsiWriteDeviceMemory(device, REFSI_CONTEXT_ADDRESS,
                                            (uint8_t *)&thread_exec,
                                            sizeof(exec_state_t), unit_id)) {
      return error;
    }
  }

  // Execute the kernel.
  auto result = refsiExecuteKernel(device, kernel_wrapper->symbol, num_harts);

  // Retrieve performance counter values.
  if (counters_enabled) {
    std::vector<uint64_t> samples(REFSI_NUM_PER_HART_PERF_COUNTERS);
    size_t sample_size = samples.size() * sizeof(uint64_t);
    for (size_t i = 0; i < num_harts; i++) {
      uint32_t unit_id = REFSI_UNIT_ID(REFSI_UNIT_KIND_ACC_HART, i);
      if (refsi_success ==
          refsiReadDeviceMemory(device, (uint8_t *)samples.data(),
                                perf_counters_addr, sample_size, unit_id)) {
        for (unsigned j = 0; j < info->num_counters; j++) {
          auto &desc = info->counter_descriptions[j];
          if (std::string(desc.sub_value_name) != "hart") {
            // Only retrieve per-hart counters.
            continue;
          }
          unsigned counter_id = desc.counter_id;
          if (counter_id >= samples.size()) {
            continue;
          }
          hart_counter_data[counter_id].set_value(i, samples[counter_id]);
        }
      }
    }
  }

  refsiFreeDeviceMemory(device, args_addr);

  if (hal_debug()) {
    clock_gettime(CLOCK_MONOTONIC, &end);
    fprintf(stderr,
            "refsi_hal_device::kernel_exec finished in "
            "%0.3f s\n",
            time_diff_in_sec(start, end));
  }

  return result == refsi_success;
}

bool refsi_g1_hal_device::mem_copy(hal::hal_addr_t dst, hal::hal_addr_t src,
                                   hal::hal_size_t size) {
  refsi_locker locker(hal_lock);

  if (hal_debug()) {
    fprintf(stderr,
            "refsi_hal_device::mem_copy(dst=0x%08lx, src=0x%08lx, "
            "size=%ld)\n",
            dst, src, size);
  }

  std::vector<uint8_t> temp(size);
  if (!mem_read(temp.data(), src, size, locker)) {
    return false;
  }
  if (!mem_write(dst, temp.data(), size, locker)) {
    return false;
  }

  return true;
}
