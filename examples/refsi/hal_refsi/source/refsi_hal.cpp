// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "refsi_hal.h"

#include <cassert>
#include <string>

#include "device/device_if.h"
#include "device/memory_map.h"

refsi_hal_device::refsi_hal_device(refsi_device_t device,
                                   riscv::hal_device_info_riscv_t *info,
                                   std::mutex &hal_lock)
    : hal::hal_device_t(info), device(device), hal_lock(hal_lock), info(info) {
  debug = false;
  if (const char *val = getenv("CA_HAL_DEBUG")) {
    if (strcmp(val, "0") != 0) {
      debug = true;
    }
  }

  // Query memory map ranges.
  refsi_device_info_t device_info;
  refsiQueryDeviceInfo(device, &device_info);
  for (unsigned i = 0; i < device_info.num_memory_map_entries; i++) {
    refsi_memory_map_entry entry;
    if ((refsi_success == refsiQueryDeviceMemoryMap(device, i, &entry)) &&
        (mem_map.find(entry.kind) == mem_map.end())) {
      mem_map[entry.kind] = entry;
    }
  }
}

refsi_hal_device::~refsi_hal_device() {}

refsi_hal_kernel *refsi_hal_program::find_kernel(const char *name) {
  std::string name_str(name);
  auto it = kernels.find(name_str);
  if (it != kernels.end()) {
    return it->second.get();
  }
  hal::hal_kernel_t kernel_addr = elf->find_symbol(name);
  if (kernel_addr == hal::hal_nullptr) {
    return nullptr;
  }

  refsi_hal_kernel *kernel = new refsi_hal_kernel(kernel_addr, name_str);
  std::unique_ptr<refsi_hal_kernel> kernel_wrapper(kernel);
  kernels[name_str] = std::move(kernel_wrapper);
  return kernel;
}

hal::hal_kernel_t refsi_hal_device::program_find_kernel(
    hal::hal_program_t program, const char *name) {
  refsi_locker locker(hal_lock);
  if (program == hal::hal_invalid_program) {
    return hal::hal_invalid_kernel;
  }
  refsi_hal_program *refsi_program = (refsi_hal_program *)program;
  refsi_hal_kernel *kernel_wrapper = refsi_program->find_kernel(name);
  if (hal_debug()) {
    hal::hal_addr_t kernel_addr =
        kernel_wrapper ? kernel_wrapper->symbol : hal::hal_nullptr;
    fprintf(stderr,
            "refsi_hal_device::program_find_kernel(name='%s') -> "
            "0x%08lx\n",
            name, kernel_addr);
  }
  return reinterpret_cast<hal::hal_kernel_t>(kernel_wrapper);
}

hal::hal_program_t refsi_hal_device::program_load(const void *data,
                                                  hal::hal_size_t size) {
  refsi_locker locker(hal_lock);
  BufferDevice elf_data(data, size);
  std::unique_ptr<ELFProgram> new_program(new ELFProgram());
  if (!new_program->read(elf_data)) {
    return hal::hal_invalid_program;
  } else if (new_program->get_machine() != machine) {
    return hal::hal_invalid_program;
  }
  auto *refsi_program = new refsi_hal_program(std::move(new_program));
  return (hal::hal_program_t)refsi_program;
}

bool refsi_hal_device::program_free(hal::hal_program_t program) {
  refsi_locker locker(hal_lock);
  if (program == hal::hal_invalid_program) {
    return false;
  }
  auto *refsi_program = (refsi_hal_program *)program;
  delete refsi_program;
  return true;
}

bool refsi_hal_device::counter_read(uint32_t counter_id, uint64_t &out,
                                    uint32_t index) {
  refsi_locker locker(hal_lock);

  // Handle RefSi per-hart counters.
  if (counter_id < REFSI_NUM_PER_HART_PERF_COUNTERS) {
    if (hart_counter_data[counter_id].has_value(index)) {
      out = hart_counter_data[counter_id].get_value(index);
      hart_counter_data[counter_id].clear_value(index);
      return true;
    }
  }
  counter_id -= REFSI_NUM_PER_HART_PERF_COUNTERS;

  // TODO: Handle RefSi global counters when there are such counters.
  if (counter_id < REFSI_NUM_GLOBAL_PERF_COUNTERS) {
    return false;
  }
  counter_id -= REFSI_NUM_GLOBAL_PERF_COUNTERS;

  // Handle host counters.
  if (counter_id < CTR_NUM_COUNTERS) {
    if (host_counter_data[counter_id].has_value(index)) {
      out = host_counter_data[counter_id].get_value(index);
      host_counter_data[counter_id].clear_value(index);
      return true;
    }
  }
  counter_id -= CTR_NUM_COUNTERS;

  return false;
}

void refsi_hal_device::counter_set_enabled(bool enabled) {
  refsi_locker locker(hal_lock);
  if (mem_map.find(PERF_COUNTERS) != mem_map.end()) {
    counters_enabled = enabled;
  }
}

// http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
static uint64_t round_up_pot(uint64_t v) {
  v--;
  v |= v >> 1;
  v |= v >> 2;
  v |= v >> 4;
  v |= v >> 8;
  v |= v >> 16;
  v |= v >> 32;
  v++;
  return v;
}

// Returns the next integer that is greater than or equal to value and is a
// multiple of align. align must be non-zero.
static uint64_t align_to(uint64_t v, uint64_t align) {
  assert(align != 0u && "align can't be 0.");
  return (v + align - 1) / align * align;
}

uint32_t refsi_hal_device::get_word_size() const {
  switch (machine) {
    default:
    case elf_machine::riscv_rv64:
      return sizeof(uint64_t);
    case elf_machine::riscv_rv32:
      return sizeof(uint32_t);
  }
}

bool refsi_hal_device::pack_args(std::vector<uint8_t> &packed_data,
                                 const hal::hal_arg_t *args, uint32_t num_args,
                                 ELFProgram *program, uint32_t thread_mode) {
  // Determine the area we can use to allocate local memory arguments.
  uint64_t local_mem_start = local_ram_addr;
  uint64_t local_mem_end = local_mem_start + local_ram_size;
  for (const elf_segment &segment : program->get_segments()) {
    if (segment.address >= local_mem_start && segment.address < local_mem_end) {
      uint64_t segment_end = segment.address + segment.memory_size;
      local_mem_start = std::max(local_mem_start, segment_end);
    }
  }

  // Translate arguments.
  for (uint32_t i = 0; i < num_args; i++) {
    const hal::hal_arg_t &arg(args[i]);
    uint64_t align = 0;
    switch (arg.kind) {
      case hal::hal_arg_address:
        if (arg.space == hal::hal_space_local) {
          if (thread_mode == REFSI_THREAD_MODE_WG) {
            pack_word_arg(packed_data, arg.size);
          } else {
            // Align the start of the local memory buffer to a correctly
            // aligned address for the pointee type, to satisfy OpenCL-like
            // programming models. Since we don't know the pointee type, we
            // assume the max alignment supported by these programming models:
            // sizeof(long16) -> 128 bytes.
            local_mem_start = align_to(local_mem_start, 128);
            pack_word_arg(packed_data, local_mem_start);
            local_mem_start += arg.size;
            if (local_mem_start > local_mem_end) {
              return false;
            }
          }
        } else {
          pack_word_arg(packed_data, arg.address);
        }
        break;
      case hal::hal_arg_value:
        // Unconditionally align packed argument values to the next power of
        // two. This contract must be met by any client of the HAL.
        align = round_up_pot(arg.size);
        pack_arg(packed_data, arg.pod_data, arg.size, align);
        break;
    }
  }
  return true;
}
// Pack a value of arbitrary size into an argument buffer.
void refsi_hal_device::pack_arg(std::vector<uint8_t> &packed_data,
                                const void *value, size_t size, size_t align) {
  if (!align) {
    align = size;
  }
  size_t offset = align_to(packed_data.size(), align);
  size_t new_size = offset + size;
  packed_data.resize(new_size, 0);
  memcpy(&packed_data[offset], value, size);
  if (hal_debug()) {
    fprintf(stderr,
            "refsi_hal_device::pack_arg(offset=%zu, align=%zu, "
            "value=0x",
            offset, align);
    for (size_t i = offset; i < new_size; i++) {
      fprintf(stderr, "%02x", packed_data[new_size + offset - (i + 1)]);
    }
    fprintf(stderr, ")\n");
  }
}

// Pack a 32-bit value into an argument buffer.
void refsi_hal_device::pack_uint32_arg(std::vector<uint8_t> &packed_data,
                                       uint32_t value, size_t align) {
  pack_arg(packed_data, &value, sizeof(uint32_t), align);
}

// Pack a 64-bit value into an argument buffer.
void refsi_hal_device::pack_uint64_arg(std::vector<uint8_t> &packed_data,
                                       uint64_t value, size_t align) {
  pack_arg(packed_data, &value, sizeof(uint64_t), align);
}

// Pack a word-sized value into an argument buffer.
void refsi_hal_device::pack_word_arg(std::vector<uint8_t> &packed_data,
                                     uint64_t value) {
  size_t align = get_word_size();
  if (get_word_size() == sizeof(uint64_t)) {
    pack_uint64_arg(packed_data, value, align);
  } else {
    pack_uint32_arg(packed_data, (uint32_t)value, align);
  }
}

hal::hal_addr_t refsi_hal_device::mem_alloc(hal::hal_size_t size,
                                            hal::hal_size_t alignment) {
  refsi_locker locker(hal_lock);
  hal::hal_addr_t alloc_addr = mem_alloc(size, alignment, locker);
  if (hal_debug()) {
    fprintf(stderr,
            "refsi_hal_device::mem_alloc(size=%ld, align=%ld) -> "
            "0x%08lx\n",
            size, alignment, alloc_addr);
  }
  return alloc_addr;
}

bool refsi_hal_device::mem_free(hal::hal_addr_t addr) {
  refsi_locker locker(hal_lock);
  if (hal_debug()) {
    fprintf(stderr, "refsi_hal_device::mem_free(address=0x%08lx)\n", addr);
  }
  return mem_free(addr, locker);
}

bool refsi_hal_device::mem_read(void *dst, hal::hal_addr_t src,
                                hal::hal_size_t size) {
  refsi_locker locker(hal_lock);
  if (hal_debug()) {
    fprintf(stderr, "refsi_hal_device::mem_read(src=0x%08lx, size=%ld)\n", src,
            size);
  }
  return mem_read(dst, src, size, locker);
}

bool refsi_hal_device::mem_write(hal::hal_addr_t dst, const void *src,
                                 hal::hal_size_t size) {
  refsi_locker locker(hal_lock);
  if (hal_debug()) {
    fprintf(stderr, "refsi_hal_device::mem_write(dst=0x%08lx, size=%ld)\n", dst,
            size);
  }
  return mem_write(dst, src, size, locker);
}

bool refsi_hal_device::mem_fill(hal::hal_addr_t dst, const void *pattern,
                                hal::hal_size_t pattern_size,
                                hal::hal_size_t size) {
  if (!pattern) {
    return false;
  }

  refsi_locker locker(hal_lock);
  const size_t max_chunk_size = 4096;
  std::vector<uint8_t> chunk;
  while (chunk.size() < size && chunk.size() < max_chunk_size) {
    chunk.insert(chunk.end(), (const uint8_t *)pattern,
                 (const uint8_t *)pattern + pattern_size);
  }
  while (size >= pattern_size) {
    size_t to_write = std::min(size, chunk.size());
    if (!mem_write(dst, chunk.data(), to_write, locker)) {
      return false;
    }
    size -= to_write;
    dst += to_write;
  }

  // We need to overwrite the HOST_MEM_WRITE counter, which has been repeatedly
  // set to the chunk size.
  // TODO: Remove once performance counters are accumulative.
  if (counters_enabled) {
    host_counter_data[CTR_HOST_MEM_WRITE].set_value(0, size);
  }
  return true;
}

hal::hal_addr_t refsi_hal_device::mem_alloc(hal::hal_size_t size,
                                            hal::hal_size_t alignment,
                                            refsi_locker &locker) {
  return refsiAllocDeviceMemory(device, size, alignment, DRAM);
}

bool refsi_hal_device::mem_free(hal::hal_addr_t addr, refsi_locker &locker) {
  return refsiFreeDeviceMemory(device, addr) == refsi_success;
}

bool refsi_hal_device::mem_read(void *dst, hal::hal_addr_t src,
                                hal::hal_size_t size, refsi_locker &locker) {
  if (!dst) {
    return false;
  }

  uint32_t unit_id = REFSI_UNIT_ID(REFSI_UNIT_KIND_EXTERNAL, 0);
  if (refsiReadDeviceMemory(device, (uint8_t *)dst, src, size, unit_id) !=
      refsi_success) {
    return false;
  }

  if (counters_enabled) {
    host_counter_data[CTR_HOST_MEM_READ].set_value(0, size);
  }
  return true;
}

bool refsi_hal_device::mem_write(hal::hal_addr_t dst, const void *src,
                                 hal::hal_size_t size, refsi_locker &locker) {
  if (!src) {
    return false;
  }

  uint32_t unit_id = REFSI_UNIT_ID(REFSI_UNIT_KIND_EXTERNAL, 0);
  if (refsiWriteDeviceMemory(device, dst, (const uint8_t *)src, size,
                             unit_id) != refsi_success) {
    return false;
  }

  if (counters_enabled) {
    host_counter_data[CTR_HOST_MEM_WRITE].set_value(0, size);
  }
  return true;
}

RefSiMemoryWrapper::RefSiMemoryWrapper(refsi_device_t device)
    : device(device) {}

bool RefSiMemoryWrapper::load(reg_t addr, size_t len, uint8_t *bytes,
                              unit_id_t unit) {
  uint32_t unit_id = (uint32_t)unit;
  return refsiReadDeviceMemory(device, bytes, addr, len, unit_id) ==
         refsi_success;
}

bool RefSiMemoryWrapper::store(reg_t addr, size_t len, const uint8_t *bytes,
                               unit_id_t unit) {
  uint32_t unit_id = (uint32_t)unit;
  return refsiWriteDeviceMemory(device, addr, bytes, len, unit_id) ==
         refsi_success;
}
