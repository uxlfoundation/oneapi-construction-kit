// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "cpu_hal.h"

#include <dlfcn.h>
#include <malloc.h>
#if defined(_WIN32)
#include <process.h>
#else
#include <unistd.h>
#endif

#include <cstring>
#include <iomanip>
#include <sstream>
#include <string>
#include <thread>

#include "device/device_if.h"

cpu_hal::cpu_hal(hal::hal_device_info_t *info, std::mutex &hal_lock)
    : hal::hal_device_t(info), hal_lock(hal_lock) {
  local_mem = (uint8_t *)malloc(local_mem_size);
}

cpu_hal::~cpu_hal() { free(local_mem); }

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

void cpu_hal::kernel_entry(exec_state *exec) {
  direct_kernel_fn kernel = (direct_kernel_fn)exec->kernel_entry;
  kernel((void *)exec->packed_args, exec);
}

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
  exec.barrier = [](exec_state *exec) {
    exec->hal->barrier.wait(exec->num_threads);
  };
  exec.hal = this;

  // Pack arguments.
  std::vector<uint8_t> packed_args;
  if (!pack_args(packed_args, args, num_args, elf, exec.flags)) {
    return false;
  }
  exec.packed_args = (kernel_args_ptr)packed_args.data();

  // Specialize the execution state struct for each thread.
  size_t num_threads = work_group_size;
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
    kernel_entry(&exec_for_thread[0]);
  }

  return true;
}

// Pack a value of arbitrary size into an argument buffer.
void cpu_hal::pack_arg(std::vector<uint8_t> &packed_data, const void *value,
                       size_t size, size_t align) {
  if (!align) {
    align = size;
  }
  size_t offset = packed_data.size();
  offset = (offset + align - 1) / align * align;
  size_t new_size = offset + size;
  packed_data.resize(new_size, 0);
  memcpy(&packed_data[offset], value, size);
  if (hal_debug()) {
    fprintf(stderr, "cpu_hal::pack_arg(offset=%zu, align=%zu, value=0x", offset,
            align);
    for (size_t i = offset; i < new_size; i++) {
      fprintf(stderr, "%02x", packed_data[new_size + offset - (i + 1)]);
    }
    fprintf(stderr, ")\n");
  }
}

// Pack a 32-bit value into an argument buffer.
void cpu_hal::pack_uint32_arg(std::vector<uint8_t> &packed_data, uint32_t value,
                              size_t align) {
  pack_arg(packed_data, &value, sizeof(uint32_t), align);
}

// Pack a 64-bit value into an argument buffer.
void cpu_hal::pack_uint64_arg(std::vector<uint8_t> &packed_data, uint64_t value,
                              size_t align) {
  pack_arg(packed_data, &value, sizeof(uint64_t), align);
}

// Pack a word-sized value into an argument buffer.
void cpu_hal::pack_word_arg(std::vector<uint8_t> &packed_data, uint64_t value,
                            uint32_t hal_flags) {
  size_t align = get_word_size();
  if (get_word_size() == sizeof(uint64_t)) {
    pack_uint64_arg(packed_data, value, align);
  } else {
    pack_uint32_arg(packed_data, (uint32_t)value, align);
  }
}

bool cpu_hal::pack_args(std::vector<uint8_t> &packed_data,
                        const hal::hal_arg_t *args, uint32_t num_args,
                        elf_program program, uint32_t hal_flags) {
  // Determine the area we can use to allocate local memory arguments.
  uint8_t *local_mem_start = local_mem;
  uint8_t *local_mem_end = local_mem_start + local_mem_size;

  // Translate arguments.
  for (uint32_t i = 0; i < num_args; i++) {
    const hal::hal_arg_t &arg(args[i]);
    switch (arg.kind) {
      case hal::hal_arg_address:
        if (arg.space == hal::hal_space_local) {
          pack_word_arg(packed_data, (uint64_t)local_mem_start, hal_flags);
          local_mem_start += arg.size;
          if (local_mem_start > local_mem_end) {
            return false;
          }
        } else {
          pack_word_arg(packed_data, arg.address, hal_flags);
        }
        break;
      case hal::hal_arg_value:
        pack_arg(packed_data, arg.pod_data, arg.size, arg.size);
        break;
    }
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
  if (!pattern) {
    return false;
  }
  uint8_t *pdst = (uint8_t *)dst;
  while (size >= pattern_size) {
    memcpy(pdst, pattern, pattern_size);
    size -= pattern_size;
    dst += pattern_size;
  }
  return true;
}
