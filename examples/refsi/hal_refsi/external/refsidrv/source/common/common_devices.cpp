// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include "common_devices.h"
#include "device/host_io_regs.h"
#include "elf_loader.h"

#include <stdexcept>
#include <sstream>
#include <fcntl.h>
#include <unistd.h>

std::string format_unit(unit_id_t unit_id) {
  switch (get_unit_kind(unit_id)) {
  case unit_kind::any:
    return "any";
  case unit_kind::external:
    return "external";
  case unit_kind::cmp:
    return "cmp";
  case unit_kind::acc_hart:
    return "hart:" + std::to_string(get_unit_index(unit_id));
  case unit_kind::acc_core:
    return "core:" + std::to_string(get_unit_index(unit_id));
  default:
    break;
  }

  std::stringstream ss;
  ss << "0x" << std::hex << unit_id;
  return ss.str();
}

uint8_t *MemoryDeviceBase::addr_to_mem(reg_t dev_offset, size_t size,
                                   unit_id_t unit_id) {
  return nullptr;
}

bool MemoryDeviceBase::load(reg_t dev_offset, size_t len, uint8_t *bytes,
                            unit_id_t unit_id) {
  if (mem_size() > 0 && (dev_offset + len > mem_size())) {
    return false;
  } else if (uint8_t *contents = addr_to_mem(dev_offset, len, unit_id)) {
    memcpy(bytes, contents, len);
    return true;
  }
  return false;
}

bool MemoryDeviceBase::store(reg_t dev_offset, size_t len, const uint8_t *bytes,
                             unit_id_t unit_id) {
  if (mem_size() > 0 && (dev_offset + len > mem_size())) {
    return false;
  } else if (uint8_t *contents = addr_to_mem(dev_offset, len, unit_id)) {
    memcpy(contents, bytes, len);
    return true;
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////

RAMDevice::RAMDevice(size_t size) : size(size) {
  data = (uint8_t *)calloc(1, size);
  if (size && !data) {
    throw std::runtime_error("couldn't allocate " + std::to_string(size) +
                             " bytes of target memory");
  }
}

uint8_t *RAMDevice::addr_to_mem(reg_t dev_offset, size_t size,
                                unit_id_t unit_id) {
  if ((dev_offset + size) <= mem_size()) {
    return contents() + dev_offset;
  }
  return nullptr;
}

////////////////////////////////////////////////////////////////////////////////

uint8_t *ROMDevice::addr_to_mem(reg_t dev_offset, size_t size,
                                unit_id_t unit_id) {
  // Only the host has write access to ROM.
  if (get_unit_kind(unit_id) == unit_kind::external) {
    if ((dev_offset + size) <= mem_size()) {
      return contents() + dev_offset;
    }
  }
  return nullptr;
}

bool ROMDevice::load(reg_t dev_offset, size_t len, uint8_t *bytes,
                     unit_id_t unit_id) {
  if ((dev_offset + len) <= mem_size()) {
    memcpy(bytes, contents() + dev_offset, len);
    return true;
  }
  return false;
}

bool ROMDevice::store(reg_t dev_offset, size_t len, const uint8_t *bytes,
                      unit_id_t unit_id) {
  return false;
}

////////////////////////////////////////////////////////////////////////////////

HartLocalDevice::~HartLocalDevice() {
  for (uint8_t *contents : hart_contents) {
    delete [] contents;
  }
  hart_contents.clear();
}

uint8_t * HartLocalDevice::mem_contents(unit_id_t unit_id) {
  if (get_unit_kind(unit_id) != unit_kind::acc_hart) {
    return nullptr;
  }
  uint16_t hart_idx = get_unit_index(unit_id);
  if (hart_idx >= hart_contents.size()) {
    hart_contents.resize(hart_idx + 1, nullptr);
  }
  uint8_t* &contents(hart_contents[hart_idx]);
  if (!contents) {
    contents = new uint8_t[size];
    memset(contents, 0, size);
  }
  return contents;
}

uint8_t *HartLocalDevice::addr_to_mem(reg_t addr, size_t size,
                                          unit_id_t unit_id) {
  uint8_t *contents = mem_contents(unit_id);
  if (!contents || ((addr + size) > mem_size())) {
    return nullptr;
  }
  return &contents[addr];
}

////////////////////////////////////////////////////////////////////////////////

FileDevice::FileDevice(const char *path) {
  fd = open(path, O_RDONLY);
}

FileDevice::~FileDevice() {
  if (is_open()) {
    close(fd);
    fd = -1;
  }
}

size_t FileDevice::mem_size() const {
  if (!is_open()) {
    return 0;
  }
  off_t size = lseek(fd, 0, SEEK_END);
  lseek(fd, 0, SEEK_SET);
  return size;
}

bool FileDevice::load(reg_t addr, size_t len, uint8_t *bytes,
                         unit_id_t unit_id) {
  if (!is_open()) {
    return false;
  }
  return pread(fd, bytes, len, addr) == len;
}

bool FileDevice::store(reg_t addr, size_t len, const uint8_t *bytes,
                          unit_id_t unit_id) {
  return false;
}

////////////////////////////////////////////////////////////////////////////////

uint8_t *BufferDevice::addr_to_mem(reg_t dev_offset, size_t size,
                                   unit_id_t unit_id) {
  if ((dev_offset + size) > mem_size()) {
    return nullptr;
  }
  return (uint8_t *)data + dev_offset;
}

////////////////////////////////////////////////////////////////////////////////

MemoryController::MemoryController(MemoryDevice*root_device) {
  add_device(0, root_device);
}

bool MemoryController::add_device(reg_t addr, MemoryDevice* device) {
  // Make sure the base address is not already mapped to another device.
  reg_t dev_offset = 0;
  if (MemoryDevice *prev_device = find_device(addr, dev_offset)) {
    if (dev_offset < prev_device->mem_size()) {
      return false;
    }
  }

  // Searching devices via lower_bound/upper_bound
  // implicitly relies on the underlying std::map
  // container to sort the keys and provide ordered
  // iteration over this sort, which it does. (python's
  // SortedDict is a good analogy)
  devices[addr] = device;
  return true;
}

MemoryDevice *MemoryController::remove_device(reg_t addr) {
  auto it = devices.find(addr);
  if (it == devices.end()) {
    return nullptr;
  }
  MemoryDevice *device = it->second;
  devices.erase(it);
  return device;
}

std::pair<reg_t, MemoryDevice*> MemoryController::find_device(reg_t addr) {
  // Find the device with the base address closest to but
  // less than addr (price-is-right search)
  auto it = devices.upper_bound(addr);
  if (devices.empty() || it == devices.begin()) {
    // Either the bus is empty, or there weren't
    // any items with a base address <= addr
    return std::make_pair((reg_t)0, (MemoryDevice *)NULL);
  }
  // Found at least one item with base address <= addr
  // The iterator points to the device after this, so
  // go back by one item.
  it--;
  return std::make_pair(it->first, it->second);
}

MemoryDevice *MemoryController::find_device(reg_t addr, reg_t &dev_offset) {
  auto pair = find_device(addr);
  dev_offset = addr - pair.first;
  return pair.second;
}

uint8_t * MemoryController::addr_to_mem(reg_t addr, size_t size,
                                        unit_id_t unit) {
  reg_t dev_offset = 0;
  if (MemoryDevice *device = find_device(addr, dev_offset)) {
    return device->addr_to_mem(dev_offset, size, unit);
  }
  return nullptr;
}

bool MemoryController::load(reg_t addr, size_t len, uint8_t *bytes,
                            unit_id_t unit) {
  reg_t dev_offset = 0;
  if (MemoryDevice *device = find_device(addr, dev_offset)) {
    if (const uint8_t *mem_contents = device->addr_to_mem(dev_offset, len,
                                                          unit)) {
      memcpy(bytes, mem_contents, len);
      return true;
    }
    return device->load(dev_offset, len, bytes, unit);
  }
  return false;
}

bool MemoryController::store(reg_t addr, size_t len, const uint8_t *bytes,
                             unit_id_t unit) {
  reg_t dev_offset = 0;
  if (MemoryDevice *device = find_device(addr, dev_offset)) {
    if (uint8_t *mem_contents = device->addr_to_mem(dev_offset, len, unit)) {
      memcpy(mem_contents, bytes, len);
      return true;
    }
    return device->store(dev_offset, len, bytes, unit);
  }
  return false;
}

bool MemoryController::copy(reg_t dst_addr, reg_t src_addr, size_t len,
                            unit_id_t unit) {
  uint8_t *src_contents = addr_to_mem(src_addr, len, unit);
  uint8_t *dst_contents = addr_to_mem(dst_addr, len, unit);
  if (!src_contents || !dst_contents) {
    // Copy is only supported with 'real' memory like RAM or ROM buffers.
    return false;
  }
  memcpy(dst_contents, src_contents, len);
  return true;
}
