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

#include "cpu_hal.h"

#include <arg_pack.h>
#include <dlfcn.h>
#include <malloc.h>
#if defined(_WIN32)
#include <process.h>
#else
#include <unistd.h>
#endif

#include <hal_riscv.h>
#include <stdlib.h>

#include <cstring>
#include <iomanip>
#include <sstream>
#include <string>
#include <thread>

#include "device/device_if.h"

hal::hal_device_info_t &cpu_hal::setup_cpu_hal_device_info() {
  const uint64_t global_ram_size = 256 << 20;
  const uint64_t global_mem_max_over_allocation = 16 << 20;
  const uint64_t local_ram_size = 8 << 20;
  static riscv::hal_device_info_riscv_t hal_device_info;
  hal_device_info.type = hal::hal_device_type_riscv;
  hal_device_info.word_size = sizeof(uintptr_t) * 8;
  hal_device_info.target_name = "ock cpu";
  hal_device_info.global_memory_avail =
      global_ram_size - global_mem_max_over_allocation;
  hal_device_info.shared_local_memory_size = local_ram_size;
  hal_device_info.should_link = true;
  hal_device_info.link_shared = true;
  hal_device_info.should_vectorize = false;
  // TODO: This is slightly arbitrary and based on the "host" target
  hal_device_info.preferred_vector_width = 128 / (8 * sizeof(uint8_t));
  hal_device_info.supports_fp16 = false;
  hal_device_info.supports_doubles = true;
#if HAL_CPU_MODE == HAL_CPU_WG_MODE
  hal_device_info.max_workgroup_size = 1024;
#elif HAL_CPU_MODE == HAL_CPU_WI_MODE
  hal_device_info.max_workgroup_size = 16;
#else
#error HAL_CPU_MODE must be HAL_CPU_WG_MODE or HAL_CPU_WI_MODE.
#endif
  hal_device_info.is_little_endian = true;
  hal_device_info.linker_script = "";

  // Currently only has meaning for risc-v. These are slightly arbitrary
  // and should be more programmatical.
  hal_device_info.vlen = 0;
  hal_device_info.extensions = riscv::rv_extension_G;
  hal_device_info.abi = hal_device_info.word_size == 64 ? riscv::rv_abi_LP64D
                                                        : riscv::rv_abi_ILP32D;
  return hal_device_info;
}

cpu_hal::cpu_hal(hal::hal_device_info_t *info, std::mutex &hal_lock)
    : hal::hal_device_t(info), hal_lock(hal_lock) {
#if HAL_CPU_MODE == HAL_CPU_WI_MODE
  // Align local memory to 128 bytes as that is the largest data type
  // in OpenCL and we need any values placed in local memory to fit that
  // alignment.
  local_mem = (uint8_t *)std::aligned_alloc(128, local_mem_size);
#endif
  if (const char *val = getenv("CA_HAL_DEBUG")) {
    if (strcmp(val, "0") != 0) {
      debug = true;
    }
  }
#if HAL_CPU_MODE == HAL_CPU_WG_MODE
  wg_num_threads = std::thread::hardware_concurrency();
  const char *env = std::getenv("CA_CPU_HAL_NUM_THREADS");
  // Allow overriding by no more than the hardware_concurrency() value
  if (nullptr != env) {
    if (const unsigned int t = std::atoi(env)) {
      wg_num_threads = std::min(t, wg_num_threads);
    }
  }
#endif
}

#if HAL_CPU_MODE == HAL_CPU_WI_MODE
cpu_hal::~cpu_hal() { std::free(local_mem); }
#endif

hal::hal_kernel_t cpu_hal::program_find_kernel(hal::hal_program_t program,
                                               const char *name) {
  std::lock_guard<std::mutex> locker(hal_lock);
  if (program == hal::hal_invalid_program) {
    return hal::hal_invalid_kernel;
  }

  hal::hal_kernel_t kernel = (hal::hal_kernel_t)dlsym((void *)program, name);
  if (hal_debug()) {
    fprintf(stderr,
            "cpu_hal::program_find_kernel(name='%s') -> "
            "0x%08lx\n",
            name, kernel);
  }
  return kernel;
}

// See http://www.cse.yorku.ca/~oz/hash.html
static uint32_t djb2_hash(const uint8_t *data, size_t size) {
  uint32_t h = 5381;
  for (size_t i = 0; i < size; i++) {
    h = ((h << 5) + h) + data[i];
  }
  return h;
}

// Generate a path to a temporary file based on the contents of a program
// executable. A hash function is used to limit collisions when program_load is
// executed concurrently by multiple processes.
static std::string get_temp_file_for_program(const void *data,
                                             hal::hal_size_t size) {
  uint32_t h = djb2_hash((uint8_t *)data, size);
#if defined(_WIN32)
  int pid = _getpid();
#else
  pid_t pid = getpid();
#endif
  std::stringstream ss;
  ss << "/tmp/kernel_" << std::hex << h << "_" << std::dec << pid << ".elf";
  return ss.str();
}

hal::hal_program_t cpu_hal::program_load(const void *data,
                                         hal::hal_size_t size) {
  std::lock_guard<std::mutex> locker(hal_lock);
  std::string kernel_path = get_temp_file_for_program(data, size);
  FILE *f = fopen(kernel_path.c_str(), "wb");
  if (!f) {
    return hal::hal_invalid_program;
  }
  fwrite(data, 1, size, f);
  fclose(f);
  elf_program elf = dlopen(kernel_path.c_str(), RTLD_LAZY);
  if (!elf) {
    fprintf(stderr, "Error : dlopen failed '%s'\n", dlerror());
    remove(kernel_path.c_str());
    return hal::hal_invalid_program;
  }
  hal::hal_program_t program = (hal::hal_program_t)elf;
  binary_files[program] = kernel_path;
  return program;
}

bool cpu_hal::program_free(hal::hal_program_t program) {
  if (program == hal::hal_invalid_program) {
    return false;
  }
  dlclose((void *)program);

  // Remove the program's binary from the disk.
  auto it = binary_files.find(program);
  if (it != binary_files.end()) {
    remove(it->second.c_str());
    binary_files.erase(it);
  }

  return true;
}

// Pauses the current thread until all threads have encountered the barrier.
void cpu_barrier::wait(int num_threads) {
  std::unique_lock<std::mutex> locker(mutex);
  // Each barrier event in the execution of the kernel is given a sequence
  // number, which is used to determine when it has been passed.
  int current_id = sequence_id;
  if (threads_entered == 0) {
    // The first thread is responsible for waiting until all other threads have
    // entered the barrier. The barrier is 'closed'.
    threads_entered++;
    while (threads_entered < num_threads) {
      entry.wait(locker);
    }
    // Once all threads have entered the barrier, update the sequence to 'open'
    // the barrier and wake up the other threads.
    sequence_id = current_id + 1;
    threads_entered = 0;
    exit.notify_all();
  } else {
    // Notify the first thread that one more thread has entered the barrier.
    threads_entered++;
    entry.notify_one();
    // Wait for the barrier to be opened.
    while (current_id == sequence_id) {
      exit.wait(locker);
    }
  }
}

#if HAL_CPU_MODE == HAL_CPU_WI_MODE
void cpu_hal::kernel_entry(exec_state *exec) {
  direct_kernel_fn kernel = (direct_kernel_fn)exec->kernel_entry;
  kernel(exec->packed_args, exec);
}
#else
void cpu_hal::kernel_entry(exec_state *exec) {
  direct_kernel_fn kernel = (direct_kernel_fn)exec->kernel_entry;

  // Simple split across x axis (first )by dividing the num groups by the
  // threads and rounding up. Some threads may get zero due to the x dimension
  // being smaller that the total threads or the total size not fitting the
  // number of threads exactly

  // First work out the ideal number of groups per thread by rounding up to the
  // number of threads. We may later reduce the size on a thread basis due to a
  // poor fit.
  uint32_t num_groups_per_thread =
      (exec->wg.num_groups[0] + exec->num_threads - 1) / exec->num_threads;

  // Calculate the start x per thread
  uint32_t group_x_start = num_groups_per_thread * exec->thread_id;

  // Calculate the minumum of the next thread's group start and the max groups,
  // so we know when to stop. This means some thread's may get less work items
  // in order to fit the work group size.
  uint32_t group_x_next = std::min(group_x_start + num_groups_per_thread,
                                   (uint32_t)(exec->wg.num_groups[0]));

  for (uint32_t wg_z = 0; wg_z < exec->wg.num_groups[2]; wg_z++) {
    exec->wg.group_id[2] = wg_z;
    for (uint32_t wg_y = 0; wg_y < exec->wg.num_groups[1]; wg_y++) {
      exec->wg.group_id[1] = wg_y;
      for (uint32_t wg_x = group_x_start; wg_x < group_x_next; wg_x++) {
        exec->wg.group_id[0] = wg_x;
        kernel(exec->packed_args, exec);
      }
    }
  }
}
#endif

bool cpu_hal::kernel_exec(hal::hal_program_t program, hal::hal_kernel_t kernel,
                          const hal::hal_ndrange_t *nd_range,
                          const hal::hal_arg_t *args, uint32_t num_args,
                          uint32_t work_dim) {
  std::lock_guard<std::mutex> locker(hal_lock);
  if (hal_debug()) {
    fprintf(stderr,
            "cpu_hal::kernel_exec(kernel=0x%08lx, num_args=%d, "
            "global=<%ld:%ld:%ld>, local=<%ld:%ld:%ld>)\n",
            kernel, num_args, nd_range->global[0], nd_range->global[1],
            nd_range->global[2], nd_range->local[0], nd_range->local[1],
            nd_range->local[2]);
  }
  if ((program == hal::hal_invalid_program) ||
      (kernel == hal::hal_invalid_kernel) || !nd_range ||
      (num_args > 0) && !args) {
    return false;
  }
  uint32_t flags = 0;
  elf_program elf = (elf_program)program;
  uint64_t work_group_size = 1;

  // Fill the execution state struct.
  exec_state_t exec;
  wg_info_t &wg(exec.wg);
  memset(&exec, 0, sizeof(exec_state_t));
  wg.num_dim = work_dim;
  for (int i = 0; i < DIMS; i++) {
    wg.local_size[i] = nd_range->local[i];
    work_group_size *= wg.local_size[i];
    wg.num_groups[i] = nd_range->global[i] / wg.local_size[i];
    if ((wg.num_groups[i] * wg.local_size[i]) != nd_range->global[i]) {
      return false;
    }
    wg.global_offset[i] = nd_range->offset[i];
  }
  exec.kernel_entry = (entry_point_fn)kernel;
  exec.flags = flags;

  hal::util::hal_argpack_t pack(get_word_size() * 8);
#if HAL_CPU_MODE == HAL_CPU_WI_MODE
  // If work item mode pack needs to be updated to know where local_mem is
  // and we need a thread barrier (for clik)
  pack.setWorkItemMode(reinterpret_cast<uintptr_t>(local_mem), local_mem_size);
  exec.barrier = [](exec_state *exec) {
    exec->hal->barrier.wait(exec->num_threads);
  };
#endif
  if (!pack.build(args, num_args)) {
    return false;
  }
  exec.hal = this;

  exec.packed_args = (kernel_args_ptr)pack.data();

  // Specialize the execution state struct for each thread.
#if HAL_CPU_MODE == HAL_CPU_WG_MODE
  size_t num_threads = wg_num_threads;
#elif HAL_CPU_MODE == HAL_CPU_WI_MODE
  size_t num_threads = work_group_size;
#else
#error HAL_CPU_MODE must be HAL_CPU_MODE_WG or HAL_CPU_MODE_WI.
#endif

  std::vector<exec_state_t> exec_for_thread(num_threads);
  exec.num_threads = num_threads;
  for (size_t thread_id = 0; thread_id < num_threads; thread_id++) {
    // Copy the execution state 'template' to this thread.
    exec_state_t &thread_exec(exec_for_thread[thread_id]);
    memcpy(&thread_exec, &exec, sizeof(exec_state_t));
    thread_exec.thread_id = thread_id;
  }

  // Execute the kernel on all threads.
  if (num_threads > 1) {
    std::vector<std::thread *> threads(num_threads);
    for (unsigned i = 0; i < num_threads; i++) {
      threads[i] =
          new std::thread(&cpu_hal::kernel_entry, this, &exec_for_thread[i]);
    }
    for (std::thread *t : threads) {
      t->join();
      delete t;
    }
  } else {
    cpu_hal::kernel_entry(&exec_for_thread[0]);
  }

  return true;
}

hal::hal_size_t cpu_hal::mem_avail() { return 0; }

hal::hal_addr_t cpu_hal::mem_alloc(hal::hal_size_t size,
                                   hal::hal_size_t alignment) {
  std::lock_guard<std::mutex> locker(hal_lock);
  hal::hal_addr_t alloc_addr = (hal::hal_addr_t)memalign(alignment, size);
  if (hal_debug()) {
    fprintf(stderr,
            "cpu_hal::mem_alloc(size=%ld, align=%ld) -> "
            "0x%08lx\n",
            size, alignment, alloc_addr);
  }
  return alloc_addr;
}

bool cpu_hal::mem_free(hal::hal_addr_t addr) {
  std::lock_guard<std::mutex> locker(hal_lock);
  if (hal_debug()) {
    fprintf(stderr, "cpu_hal::mem_free(address=0x%08lx)\n", addr);
  }
  free((uint8_t *)addr);
  return addr != hal::hal_nullptr;
}

bool cpu_hal::mem_copy(hal::hal_addr_t dst, hal::hal_addr_t src,
                       hal::hal_size_t size) {
  std::lock_guard<std::mutex> locker(hal_lock);
  if (hal_debug()) {
    fprintf(stderr,
            "cpu_hal::mem_copy(dst=0x%08lx, src=0x%08lx, "
            "size=%ld)\n",
            dst, src, size);
  }
  memcpy((uint8_t *)dst, (uint8_t *)src, size);
  return true;
}

bool cpu_hal::mem_read(void *dst, hal::hal_addr_t src, hal::hal_size_t size) {
  std::lock_guard<std::mutex> locker(hal_lock);
  if (hal_debug()) {
    fprintf(stderr, "cpu_hal::mem_read(src=0x%08lx, size=%ld)\n", src, size);
  }
  if (!dst) {
    return false;
  }
  memcpy(dst, (uint8_t *)src, size);
  return true;
}

bool cpu_hal::mem_write(hal::hal_addr_t dst, const void *src,
                        hal::hal_size_t size) {
  std::lock_guard<std::mutex> locker(hal_lock);
  if (hal_debug()) {
    fprintf(stderr, "cpu_hal::mem_write(dst=0x%08lx, size=%ld)\n", dst, size);
  }
  if (!src) {
    return false;
  }
  memcpy((uint8_t *)dst, src, size);
  return true;
}

bool cpu_hal::mem_fill(hal::hal_addr_t dst, const void *pattern,
                       hal::hal_size_t pattern_size, hal::hal_size_t size) {
  if (hal_debug()) {
    fprintf(stderr,
            "cpu_hal::mem_fill(dst=0x%08lx, pattern=%p pattern_size=%ld "
            "size=%ld)\n",
            dst, pattern, pattern_size, size);
  }
  if (!pattern) {
    return false;
  }
  uint8_t *pdst = (uint8_t *)dst;
  while (size >= pattern_size) {
    memcpy(pdst, pattern, pattern_size);
    size -= pattern_size;
    pdst += pattern_size;
  }
  return true;
}
