// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "refsidrv/refsi_device.h"

#include <assert.h>

#include "refsidrv/refsi_accelerator.h"
#include "refsidrv/refsi_command_processor.h"
#include "refsidrv/refsi_memory.h"

RefSiDevice::RefSiDevice(refsi_soc_family family) : family(family),
  allocator(dram_base, dram_size) {
  if (const char *val = getenv("REFSI_DEBUG")) {
    if (strcmp(val, "0") != 0) {
      debug = true;
    }
  }
}

RefSiDevice::~RefSiDevice() {}

refsi_result RefSiDevice::queryDeviceInfo(refsi_device_info_t &device_info) {
  RefSiLock lock(mutex);
  device_info.family = (refsi_device_family)getFamily();
  device_info.num_cores = num_cores;
  device_info.num_harts_per_core = num_harts_per_core;
  device_info.num_memory_map_entries = getMemoryMap().size();
  device_info.core_isa = accelerator->getISA();
  device_info.core_vlen = accelerator->getVectorLen();
  device_info.core_elen = accelerator->getVectorElemLen();
  return refsi_success;
}

RefSiAccelerator &RefSiDevice::getAccelerator() { return *accelerator; }

/// @brief Access the device's memory interface.
RefSiMemoryController &RefSiDevice::getMemory() { return *mem_ctl; }

/// @brief Access the device's memory map.
const std::vector<refsi_memory_map_entry> &RefSiDevice::getMemoryMap() const {
  return mem_ctl->getMemoryMap();
}

refsi_addr_t RefSiDevice::allocDeviceMemory(size_t size, size_t alignment,
                                            refsi_memory_map_kind kind) {
  RefSiLock lock(mutex);
  if (kind != DRAM) {
    return refsi_failure;
  }
  return allocator.alloc(size, alignment);
}

refsi_result RefSiDevice::freeDeviceMemory(refsi_addr_t phys_addr) {
  RefSiLock lock(mutex);
  allocator.free(phys_addr);
  return refsi_success;
}

void *RefSiDevice::getMappedAddress(refsi_addr_t phys_addr, size_t size) {
  RefSiLock lock(mutex);
  return mem_ctl->addr_to_mem(phys_addr, size, make_unit(unit_kind::external));
}

refsi_result RefSiDevice::flushDeviceMemory(refsi_addr_t phys_addr,
                                            size_t size) {
  // Flushing device memory is currently a no-op. getMappedAddress returns the
  // underlying buffer.
  return refsi_success;
}

refsi_result RefSiDevice::invalidateDeviceMemory(refsi_addr_t phys_addr,
                                                 size_t size) {
  // Invalidating device memory is currently a no-op. getMappedAddress returns
  // the underlying buffer.
  return refsi_success;
}

refsi_result RefSiDevice::readDeviceMemory(uint8_t *dest, refsi_addr_t addr,
                                           size_t size, uint32_t unit_id) {
  RefSiLock lock(mutex);
  return mem_ctl->load(addr, size, dest, (unit_id_t)unit_id) ?
        refsi_success : refsi_failure;
}

refsi_result RefSiDevice::writeDeviceMemory(refsi_addr_t addr,
                                            const uint8_t *source, size_t size,
                                            uint32_t unit_id) {
  RefSiLock lock(mutex);
  return mem_ctl->store(addr, size, source, (unit_id_t)unit_id) ?
        refsi_success : refsi_failure;
}
