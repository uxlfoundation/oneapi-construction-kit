// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "mux/hal/memory.h"

#include "mux/utils/allocator.h"

namespace mux {
namespace hal {
memory::memory(uint64_t size, uint32_t properties, ::hal::hal_addr_t targetPtr,
               void *hostPtr)
    : targetPtr(targetPtr), hostPtr(hostPtr) {
  this->size = size;
  this->properties = properties;
  this->handle = static_cast<uint64_t>(targetPtr);
}

// muxMapMemory
cargo::expected<void *, mux_result_t> memory::map(::hal::hal_device_t *device,
                                                  uint64_t offset,
                                                  uint64_t size) {
  (void)device;
  if (hostPtr) {
    return static_cast<unsigned char *>(hostPtr) + offset;
  }
  if (cargo::success != mappedMemory.alloc(size)) {
    return cargo::make_unexpected(mux_error_out_of_memory);
  }
  mapOffset = offset;
  return mappedMemory.data();
}

// muxFlushMappedMemoryToDevice
mux_result_t memory::flushToDevice(::hal::hal_device_t *device, uint64_t offset,
                                   uint64_t size) {
  uint8_t *src = nullptr;
  if (hostPtr) {
    src = static_cast<uint8_t *>(hostPtr) + offset;
  } else {
    if (mappedMemory.empty()) {
      return mux_error_failure;
    }
    src = mappedMemory.data() + offset - mapOffset;
  }
  if (!device->mem_write(targetPtr + offset, src, size)) {
    return mux_error_failure;
  }
  return mux_success;
}

// muxFlushMappedMemoryFromDevice
mux_result_t memory::flushFromDevice(::hal::hal_device_t *device,
                                     uint64_t offset, uint64_t size) {
  uint8_t *dst = nullptr;
  if (hostPtr) {
    dst = static_cast<uint8_t *>(hostPtr) + offset;
  } else {
    if (mappedMemory.empty()) {
      return mux_error_failure;
    }
    dst = static_cast<uint8_t *>(mappedMemory.data()) + offset - mapOffset;
  }
  if (!device->mem_read(dst, targetPtr + offset, size)) {
    return mux_error_failure;
  }
  return mux_success;
}

// muxUnmapMemory
mux_result_t memory::unmap(::hal::hal_device_t *device) {
  (void)device;
  mappedMemory.clear();
  hostPtr = nullptr;
  return mux_success;
}
}  // namespace hal
}  // namespace mux
