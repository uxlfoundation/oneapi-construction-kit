// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef _REFSIDRV_COMMON_COMMON_DEVICES_H
#define _REFSIDRV_COMMON_COMMON_DEVICES_H

#include <map>
#include <string>
#include <vector>

#include "decode.h"

class slim_sim_t;

enum class elf_machine;

using unit_id_t = uint32_t;

/// @brief Identifies a RefSi execution unit by its kind.
enum class unit_kind {
  /// @brief The unit kind is not known or does not matter.
  any = 0,
  /// @brief The unit is external to the RefSi device (e.g. host).
  external = 1,
  /// @brief The unit is the command processor (CMP).
  cmp = 2,
  /// @brief The unit is a particular hart of an accelerator core.
  acc_hart = 3,
  /// @brief The unit is a particular accelerator core.
  acc_core = 4
};

/// @brief Create a new unit ID from a unit kind and unit index.
/// @param kind Unit kind to use for this unit ID.
/// @param index Unit index to use for this unit ID.
inline unit_id_t make_unit(unit_kind kind = unit_kind::any,
                           uint16_t index = 0) {
  return (unit_id_t)((((uint32_t)kind) << 24) | index);
}

/// @brief Retrieve the 'kind' part of a unit ID.
inline unit_kind get_unit_kind(unit_id_t unit_id) {
  return (unit_kind)((unit_id & 0xff000000) >> 24);
}

/// @brief Retrieve the 'index' part of a unit ID. This could be for example the
/// hart ID if the unit refers to a hart.
inline uint16_t get_unit_index(unit_id_t unit_id) {
  return (unit_id & 0xffff);
}

/// @brief Return a textual representation of the unit ID.
std::string format_unit(unit_id_t unit_id);

/// @brief Abstract interface for memory devices. Such devices can be used to
/// load data from or store data to. Some devices may also be memory-mapped,
/// allowing for a host pointer to the underlying data to be queried using the
/// @ref addr_to_mem function.
///
/// Device offsets are used to locate memory in the device. These can be seen as
/// the difference between the memory address to be accessed and the device's
/// base address (i.e. where the device is mapped in memory). For devices that
/// represent the entire platform, device offsets are effectively physical
/// memory addresses.
///
/// Functions intended to access device memory take a 'unit_id' parameter, which
/// identifies the execution unit that made the request. It can be used to
/// implement devices that have different contents for different units (e.g.
/// per-hart storage), as well as simple access control.
class MemoryDevice {
 public:
  virtual ~MemoryDevice() {}

  /// @brief Size of the memory device, in bytes, for fixed-sized devices. For
  /// variable-sized devices, zero is returned. When this function returns N,
  /// this means that device offsets of zero to N-1 are valid.
  virtual size_t mem_size() const = 0;

  /// @brief Try to retrieve a host pointer to a subset of the memory contents
  /// of the device. The pointer can be used to both read and write the memory
  /// contents of the device. May return null for special I/O devices, in which
  /// case @ref load and @ref store may be used to access memory instead.
  /// @param dev_offset Offset to the start of the memory area to map.
  /// @param size Size of the memory area to map.
  /// @param unit_id ID of the execution unit requesting the memory access.
  /// @return Host pointer for the specified area, or null.
  virtual uint8_t *addr_to_mem(reg_t dev_offset, size_t size,
                               unit_id_t unit_id) = 0;

  /// @brief Try to read data from the device.
  /// @param dev_offset Offset to the start of the memory area to read from.
  /// @param len Size of the memory area to read from.
  /// @param bytes Buffer to copy the data read from the device to.
  /// @param unit_id ID of the execution unit requesting the memory access.
  /// @return true on success and false on failure.
  virtual bool load(reg_t dev_offset, size_t len, uint8_t *bytes,
                    unit_id_t unit_id) = 0;

  /// @brief Try to write data to the device.
  /// @param dev_offset Offset to the start of the memory area to write to.
  /// @param len Size of the memory area to write to.
  /// @param bytes Data to write to the device.
  /// @param unit_id ID of the execution unit requesting the memory access.
  /// @return true on success and false on failure.
  virtual bool store(reg_t dev_offset, size_t len, const uint8_t *bytes,
                     unit_id_t unit_id) = 0;
};

/// @brief Convenience class that makes it easier to implement a memory device.
/// Most memory devices will only need to implement the @ref addr_to_mem and
/// @ref mem_size functions. The default @ref load and @ref store implementation
/// use @ref addr_to_mem to access memory.
class MemoryDeviceBase : public MemoryDevice {
 public:
  /// @brief The default of this implementation always returns null.
  uint8_t *addr_to_mem(reg_t dev_offset, size_t size,
                       unit_id_t unit_id) override;

  /// @brief The default of this implementation tries to use @ref addr_to_mem
  /// to load data from the device.
  bool load(reg_t dev_offset, size_t len, uint8_t *bytes,
            unit_id_t unit_id) override;

  /// @brief The default of this implementation tries to use @ref addr_to_mem
  /// to store data to the device.
  bool store(reg_t dev_offset, size_t len, const uint8_t *bytes,
             unit_id_t unit_id) override;
};

/// @brief Base class for devices that are composed of multiple sub-devices.
/// Sub-devices are mapped at a given (base) memory address relative to the
/// device's base address. This base address is used to query the memory
/// interface for the sub-device which 'lives' at a specified address.
class MemoryInterface : public MemoryDevice {
public:
  /// @brief Query the memory interface for a device at the given address.
  /// @param addr Address to query for a device.
  /// @param dev_offset On success, offset from the returned device's base
  /// address.
  virtual MemoryDevice *find_device(reg_t addr, reg_t &dev_offset) = 0;
};

/// @brief Utility class to help manage a set of devices under the same address
/// space. Devices can be added and removed dynamically.
class MemoryController : public MemoryInterface {
public:
  /// @brief Create a new memory controller with no sub-devices.
  MemoryController() {}

  /// @brief Create a new memory controller and map the specified root device
  /// at address zero.
  MemoryController(MemoryDevice *root_device);

  /// @brief Add a device to the memory controller at the given base address.
  /// @return true if the device was added to the memory controller, false if
  /// another device is already using the base address.
  /// @param addr Base address of the device, i.e. where it will be mapped.
  /// @param dev Device to add to map in the memory controller.
  bool add_device(reg_t addr, MemoryDevice* dev);

  /// @brief Remove (unmap) a device from the memory controller, given its exact
  /// base address.
  /// @param addr Base address of the device to remove.
  /// @return Pointer to the removed device or null if it could not be found.
  MemoryDevice *remove_device(reg_t addr);

  /// @brief Try to find a device mapped at the given address, which does not
  /// need to be the base address but can point anywhere in the device's memory
  /// region.
  /// @param addr Address to use to locate the device.
  /// @return Pair of (base address, memory device) on success.
  std::pair<reg_t, MemoryDevice*> find_device(reg_t addr);

  /// @brief Return a read-only map of the mapped devices.
  const std::map<reg_t, MemoryDevice*> & get_devices() { return devices; }

  /// @brief Try to find a device mapped at the given address, which does not
  /// need to be the base address but can point anywhere in the device's memory
  /// region.
  /// @param addr Address to use to locate the device.
  /// @param dev_offset On success, device offset between the base address and
  /// the given address (@ref ddr).
  /// @return Memory device on success or null if it could not be found.
  MemoryDevice *find_device(reg_t addr, reg_t &dev_offset) override;

  /// @brief Return zero. Memory controllers are variable-sized devices.
  size_t mem_size() const override { return 0; }

  /// @brief Try to retrieve a host pointer to a subset of the memory contents
  /// of the device mapped at the given address. This is equivalent to calling
  /// @ref find_device followed by @ref addr_to_mem on the returned device,
  /// converting the address to a device offset.
  /// @param addr Device address from which to retrieve the host pointer.
  /// @param size Size of the memory area to map.
  /// @param unit ID of the execution unit requesting the memory access.
  /// @return Host pointer for the specified area, or null.
  uint8_t *addr_to_mem(reg_t addr, size_t size, unit_id_t unit) override;

  /// @brief Try to read data from the device. This is equivalent to calling
  /// @ref find_device followed by @ref load on the returned device,
  /// converting the address to a device offset.
  /// @param addr Device address to copy data from.
  /// @param len Size of the memory area to read from.
  /// @param bytes Buffer to copy the data read from the device to - must not
  /// be nullptr.
  /// @param unit ID of the execution unit requesting the memory access.
  /// @return true on success and false on failure.
  bool load(reg_t addr, size_t len, uint8_t* bytes, unit_id_t unit) override;

  /// @brief Try to write data to the device. This is equivalent to calling
  /// @ref find_device followed by @ref store on the returned device,
  /// converting the address to a device offset.
  /// @param addr Device address to store data to.
  /// @param len Size of the memory area to write to.
  /// @param bytes Data to write to the device - must not be nullptr.
  /// @param unit ID of the execution unit requesting the memory access.
  /// @return true on success and false on failure.
  bool store(reg_t addr, size_t len, const uint8_t* bytes,
             unit_id_t unit) override;

  /// @brief Try to copy data from one area of memory to another.
  /// @param dst_addr Device address to copy data to.
  /// @param src_addr Device address to copy data from.
  /// @param len Number of bytes to copy.
  /// @param unit ID of the execution unit requesting the memory access.
  /// @return true on success and false on failure.
  bool copy(reg_t dst_addr, reg_t src_addr, size_t len, unit_id_t unit);

private:
  std::map<reg_t, MemoryDevice*> devices;
};

class RAMDevice : public MemoryDeviceBase {
 public:
  RAMDevice (size_t size);
  virtual ~RAMDevice() {
    free(data);
  }

  uint8_t *contents() { return data; }
  size_t mem_size() const override { return size; }
  uint8_t *addr_to_mem(reg_t dev_offset, size_t size,
                       unit_id_t unit_id) override;

 private:
  uint8_t *data;
  size_t size;
};

class ROMDevice : public MemoryDeviceBase {
 public:
  ROMDevice (size_t size) {
    data.resize(size);
  }

  uint8_t *contents() { return data.data(); }
  size_t mem_size() const override { return data.size(); }
  uint8_t *addr_to_mem(reg_t dev_offset, size_t size,
                       unit_id_t unit_id) override;
  bool load(reg_t dev_offset, size_t len, uint8_t *bytes,
            unit_id_t unit_id) override;
  bool store(reg_t dev_offset, size_t len, const uint8_t *bytes,
             unit_id_t unit_id) override;

 private:
  std::vector<uint8_t> data;
};

class HartLocalDevice : public MemoryDeviceBase {
public:
  HartLocalDevice(size_t size) : size(size) {}
  virtual ~HartLocalDevice();

  uint8_t *addr_to_mem(reg_t addr, size_t size, unit_id_t unit_id) override;

  uint8_t* mem_contents(unit_id_t unit_id);
  size_t mem_size() const override { return size; }

private:
  size_t size;
  std::vector<uint8_t *> hart_contents;
};

class FileDevice : public MemoryDevice {
public:
  FileDevice(const char *path);
  virtual ~FileDevice();

  bool is_open() const { return fd >= 0; }

  size_t mem_size() const override;

  bool load(reg_t addr, size_t len, uint8_t* bytes, unit_id_t unit_id) override;
  bool store(reg_t addr, size_t len, const uint8_t* bytes,
             unit_id_t unit_id) override;

private:
  int fd;
};

class BufferDevice : public MemoryDeviceBase {
public:
  BufferDevice(const void *data, size_t size) : data(data), size(size) {}

  size_t mem_size() const override { return size; }

  uint8_t *addr_to_mem(reg_t dev_offset, size_t size,
                       unit_id_t unit_id) override;

private:
  const void *data;
  size_t size;
};

#endif
