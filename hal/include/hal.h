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

/// @file
///
/// @brief Device Hardware Abstraction Layer interface.

#ifndef HAL_H_INCLUDED
#define HAL_H_INCLUDED

#include <algorithm>
#include <memory>
#include <string>

#include "hal_types.h"

namespace hal {
/// @addtogroup hal
/// @{

/// @brief hal_device_t provides direct access to a device exposed by
/// a hal. it provides access to device memory, program loading, execution
/// and information queries.
struct hal_device_t {
  hal_device_t(hal_device_info_t *device_info) : device_info(device_info){};
  virtual ~hal_device_t(){};

  /// @brief Find a specific kernel function in a compiled program.
  ///
  /// @param program is a handle to a previously loaded program.
  /// @param name is a null terminated c-string with the kernel name to be
  /// searched for.
  ///
  /// @return Returns `hal_invalid_kernel` if no symbol could be found
  /// otherwise a kernel handle will be returned.
  virtual hal_kernel_t program_find_kernel(hal_program_t program,
                                           const char *name) = 0;

  /// @brief Load an ELF file into target memory.
  ///
  /// @param data is a pointer to the raw executable binary blob to load
  /// (i.e. ELF file data).
  /// @param size is the size of the raw executable binary provided.
  ///
  /// @return returns `hal_invalid_program` if the program could not be loaded
  /// otherwise a handle to the program.
  virtual hal_program_t program_load(const void *data, hal_size_t size) = 0;

  /// @brief Execute a kernel on the target.
  ///
  /// @param program is a handle to a previously loaded program.
  /// @param kernel is a handle to a previously found kernel.
  /// @param nd_range contains the work range to execute.
  /// @param args is a list of argument descriptors for the kernel.
  /// @param num_args is the number of argument descriptors provided.
  /// @param work_dim specifies the work dimension for execution (1, 2 or 3).
  ///
  /// @return returns `false` if the operation fails otherwise `true`.
  virtual bool kernel_exec(hal_program_t program, hal_kernel_t kernel,
                           const hal_ndrange_t *nd_range, const hal_arg_t *args,
                           uint32_t num_args, uint32_t work_dim) = 0;

  /// @brief Unload a program from the target.
  ///
  /// @param program is a handle to a previously loaded program.
  ///
  /// @return returns `false` if the operation fails otherwise `true`.
  virtual bool program_free(hal_program_t program) = 0;

  /// @brief Return target information - can be upcast based on type
  /// information.
  ///
  /// @return Returns `nullptr` if the operation fails.
  const hal_device_info_t *get_info() const { return device_info; }

  /// @brief Allocate a memory range on the target.
  ///
  /// @param size is the number of bytes requested.
  /// @param alignment is a power of two number which the allocation should be
  /// aligned to (i.e. 1, 2, 4, ...).
  ///
  /// @return Returns `hal_nullptr` if the operation was unsuccessful otherwise
  /// a device specific memory address.
  virtual hal_addr_t mem_alloc(hal_size_t size, hal_size_t alignment) = 0;

  /// @brief Copy memory between target buffers.
  ///
  /// @note It is assumed the destination and source will not overlap.
  /// @note This is a slow and default implementation.
  ///
  /// @param dst device address which is the copy destination.
  /// @param src device address which is the copy source.
  /// @param size is the total number of bytes to be transferred.
  ///
  /// @return Returns `false` if the operation fails otherwise `true`.
  virtual bool mem_copy(hal_addr_t dst, hal_addr_t src, hal_size_t size) {
    const constexpr hal_size_t max_malloc_size = 1024 * 1024;
    const std::unique_ptr<uint8_t[]> temp_ptr(new uint8_t[max_malloc_size]);
    void *temp = temp_ptr.get();

    if (temp == NULL) {
      return false;
    }

    while (size > 0) {
      const hal_size_t chunk_size = std::min<hal_size_t>(size, max_malloc_size);

      if (!mem_read(temp, src, chunk_size)) {
        return false;
      }

      if (!mem_write(dst, temp, chunk_size)) {
        return false;
      }

      dst += chunk_size;
      src += chunk_size;
      size -= chunk_size;
    }

    return true;
  }

  /// @brief Free a memory range on the target.
  ///
  /// @param addr is address of the device memory block to release.
  ///
  /// @return Returns `false` if the operation fails otherwise `true`.
  virtual bool mem_free(hal_addr_t addr) = 0;

  /// @brief Fill memory with a repeating pattern.
  ///
  /// @param dst device address which is the read destination.
  /// @param pattern host address which is the source write pattern.
  /// @param pattern_size is the number of bytes in the memory pattern.
  /// @param size is the total number of bytes to be written.
  ///
  /// @return Returns `false` if the operation fails otherwise `true`.
  virtual bool mem_fill(hal_addr_t dst, const void *pattern,
                        hal_size_t pattern_size, hal_size_t size) {
    if (!pattern) {
      return false;
    }
    while (size >= pattern_size) {
      if (!mem_write(dst, pattern, pattern_size)) {
        return false;
      }
      size -= pattern_size;
      dst += pattern_size;
    }
    return true;
  }

  /// @brief Read memory from the target to the host.
  ///
  /// @param dst host address which is the read destination.
  /// @param src device address which is the source memory location.
  /// @param size is the number of bytes to be written.
  ///
  /// @return returns `false` if the operation fails otherwise `true`.
  virtual bool mem_read(void *dst, hal_addr_t src, hal_size_t size) = 0;

  /// @brief Write host memory to the target.
  ///
  /// @param dst device address which is the write destination.
  /// @param src host address which is the source memory location.
  /// @param size is the number of bytes to be written.
  ///
  /// @return returns `false` if the operation fails otherwise `true`.
  virtual bool mem_write(hal_addr_t dst, const void *src, hal_size_t size) = 0;

  /// @brief If the counter specified has an unread value, read it out.
  /// This will implicitly mark the data as read.
  ///
  /// @param counter_id The ID of the counter to read
  /// @param out Reference to store the output value to
  /// @param index Optional index for contained values
  ///
  /// @return true if there was an unread value which was read out, otherwise
  /// false.
  virtual bool counter_read(uint32_t counter_id, uint64_t &out,
                            uint32_t index = 0) {
    (void)counter_id;
    (void)out;
    (void)index;
    return false;
  };

  /// @brief Enable or disable counter support in the HAL.
  /// Must be set to true before calling `counter_read`. If disabled the HAL
  /// should avoid the overhead of reading counter values where possible.
  ///
  /// @param enable True to enable counter support, false to disable
  virtual void counter_set_enabled(bool enable) { (void)enable; };

 private:
  /// @brief device_info is the default hal_device_info_t structure provided by
  /// the hal when this device was instanciated. it is returned by the base
  /// `get_info` function.
  hal_device_info_t *device_info;
};

/// @brief hal_t provides access to a hardware abstraction layer allowing the
/// caller to query hal and device information as well as instantiate devices.
struct hal_t {
  /// @brief Current version of the HAL API. The version number needs to be
  /// bumped any time the interface is changed.
  static constexpr uint32_t api_version = 6;

  /// @brief Return generic platform information.
  ///
  /// @return Returns a structure with information about the hal.
  virtual const hal_info_t &get_info() = 0;

  /// @brief Return generic target information.
  ///
  /// @param device_index ranges from 0 to hal_manager_info_t::num_devices.
  ///
  /// @return Returns `nullptr` if the operation fails or a pointer to the
  /// device information. The hal retains ownership of the returned pointer and
  /// it does not need to be released. The returned pointer can be upcast
  /// depending on the type information member.
  virtual const hal_device_info_t *device_get_info(uint32_t device_index) = 0;

  /// @brief Request the creation of a new hal device.
  ///
  /// @param device_index ranges from 0 to hal_info_t::num_devices.
  ///
  /// @return Returns `nullptr` if the operation fails.
  virtual hal_device_t *device_create(uint32_t device_index) = 0;

  /// @brief Destroy a device instance.
  ///
  /// @param Device is a currently valid hal_device_t object.
  ///
  /// @return Returns `false` if the operation fails otherwise `true`.
  virtual bool device_delete(hal_device_t *device) = 0;
};

/// @}
}  // namespace hal

#if defined _WIN32 || defined __CYGWIN__
#define HAL_DLL_IMPORT __declspec(dllimport)
#define HAL_DLL_EXPORT __declspec(dllexport)
#else
#define HAL_DLL_IMPORT __attribute__((visibility("default")))
#define HAL_DLL_EXPORT __attribute__((visibility("default")))
#endif
#ifdef BUILD_HAL_DLL
#define HAL_API HAL_DLL_EXPORT
#else
#define HAL_API HAL_DLL_IMPORT
#endif

extern "C" {

/// @brief Return a hal instance provided by a hal implementation.
///
/// @param api_version Returns the API version implemented by the hal.
///
/// @note A HAL implementor will supply this function.
/// @note The returned object does not need to be released by the caller.
/// @note It is the caller's responsibility to ensure the API version used by
/// the hal is compatible with the caller's.
///
/// @return returns a static instance of a hal_t object or nullptr if on error.
HAL_API hal::hal_t *get_hal(uint32_t &api_version);
}

#endif  // HAL_H_INCLUDED
