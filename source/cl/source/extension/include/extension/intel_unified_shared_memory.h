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
/// @brief Support for the `cl_intel_unified_shared_memory` extension.

#ifndef EXTENSION_INTEL_UNIFIED_SHARED_MEMORY_H_INCLUDED
#define EXTENSION_INTEL_UNIFIED_SHARED_MEMORY_H_INCLUDED

#include <CL/cl_ext.h>
#include <cargo/dynamic_array.h>
#include <cargo/expected.h>
#include <cargo/small_vector.h>
#include <extension/extension.h>
#include <mux/mux.h>

#include <mutex>

namespace extension {
/// @addtogroup cl_extension
/// @{

class intel_unified_shared_memory final : public extension {
 public:
  /// @brief Default constructor.
  intel_unified_shared_memory();

  /// @copydoc extension::extension::GetExtensionFunctionAddressForPlatform
  void *GetExtensionFunctionAddressForPlatform(
      cl_platform_id platform, const char *func_name) const override;

  /// @copydoc extension::extension::GetDeviceInfo
  cl_int GetDeviceInfo(cl_device_id device, cl_device_info param_name,
                       size_t param_value_size, void *param_value,
                       size_t *param_value_size_ret) const override;

#if (defined(CL_VERSION_3_0) || \
     defined(OCL_EXTENSION_cl_codeplay_kernel_exec_info))
  /// @copydoc extension::extension::SetKernelExecInfo
  cl_int SetKernelExecInfo(cl_kernel kernel,
                           cl_kernel_exec_info_codeplay param_name,
                           size_t param_value_size,
                           const void *param_value) const override;
#endif
};

#ifdef OCL_EXTENSION_cl_intel_unified_shared_memory
namespace usm {
/// @brief Bitfield of available USM cl_kernel_exec_info_codeplay flags
enum kernel_exec_info_flags_e {
  /// @brief CL_KERNEL_EXEC_INFO_INDIRECT_HOST_ACCESS
  kernel_exec_info_indirect_host_access = (0x1 << 0),
  /// @brief CL_KERNEL_EXEC_INFO_INDIRECT_DEVICE_ACCESS
  kernel_exec_info_indirect_device_access = (0x1 << 1),
  /// @brief CL_KERNEL_EXEC_INFO_INDIRECT_SHARED_ACCESS
  kernel_exec_info_indirect_shared_access = (0x1 << 2)
};

/// @brief Abstract class which different USM allocations types can inherit
/// from.
class allocation_info {
 protected:
  /// @brief Private constructor, use the `create()` functions instead.
  allocation_info(const cl_context context, const size_t size);

  allocation_info(const allocation_info &) = delete;

 public:
  /// @brief Destructor.
  virtual ~allocation_info();

  /// @brief Overridden by derived classes to return device associated with USM
  /// allocation, or nullptr if host allocation.
  virtual cl_device_id getDevice() const = 0;

  /// @brief Allocates memory for the USM allocation and binds it to Mux
  /// objects for supported devices.
  ///
  /// @param[in] alignment Minimum alignment in bytes for allocation
  ///
  /// @return CL_SUCCESS or an OpenCL error code on failure
  virtual cl_int allocate(uint32_t alignment) = 0;

  /// @brief Given an OpenCL device returns the Mux buffer object associated
  /// with the device for this USM allocation.
  ///
  /// @param[in] query_device Device to find a buffer for.
  ///
  /// @return A matching Mux buffer object on success, or nullptr on failure
  virtual mux_buffer_t getMuxBufferForDevice(cl_device_id device) const = 0;

  /// @brief Return the USM memory type for this allocation
  ///
  /// @return The enum variant corresponding to this memory allocation type
  virtual cl_unified_shared_memory_type_intel getMemoryType() const = 0;

  /// @brief Checks if a pointer belongs to USM allocation
  ///
  /// @param[in] ptr Pointer to host memory address.
  ///
  /// @return True if pointer in address range of allocation, false otherwise
  bool isOwnerOf(const void *ptr) const {
    const cl_uchar *start_ptr = reinterpret_cast<cl_uchar *>(base_ptr);
    const cl_uchar *end_ptr = start_ptr + size;

    return (ptr >= start_ptr && ptr < end_ptr);
  }

  /// @brief Store events associated with enqueued USM commands so that we can
  /// wait on them in the case of a blocking free call to the allocation.
  ///
  /// @param[in] event Event to cache.
  ///
  /// @return mux_success, or a Mux error code on failure.
  mux_result_t record_event(cl_event event);

  /// @brief Context the memory object belongs to.
  const cl_context context;
  /// @brief Size in bytes of the requested device allocation.
  size_t size;
  /// @brief Pointer returned by USM allocation entry points
  void *base_ptr;
  /// @brief Properties set on allocation
  cl_mem_alloc_flags_intel alloc_flags;
  /// @brief List of events associated with commands using the USM allocation.
  cargo::small_vector<cl_event, 4> queued_commands;
  /// @brief Mutex to lock when pushing to queued_commands
  std::mutex mutex;
};

/// @brief Derived class for host USM allocations
class host_allocation_info final : public allocation_info {
  /// @brief Private constructor, use the `create()` functions instead.
  host_allocation_info(const cl_context context, const size_t size);

  host_allocation_info(const host_allocation_info &) = delete;

  /// @brief Information about memory to flush passed as `user_data` argument
  /// to `muxCommandUserCallback()`
  struct flush_info_t final {
    flush_info_t(mux_memory_t mux_memory, uint64_t offset, size_t size)
        : mux_memory(mux_memory), offset(offset), size(size) {}

    mux_memory_t mux_memory;
    uint64_t offset;
    size_t size;
  };

 public:
  /// @brief Create a host-side USM allocation
  ///
  /// @param[in] context Context the allocation will belong to.
  /// @param[in] properties Properties bitfield encoding which properties to
  /// enable.
  /// @param[in] size Bytes to allocate.
  /// @param[in] alignment Minimum alignment of allocation.
  static cargo::expected<std::unique_ptr<host_allocation_info>, cl_int> create(
      cl_context context, const cl_mem_properties_intel *properties,
      const size_t size, cl_uint alignment);

  /// @brief Destructor.
  ~host_allocation_info() override;

  /// @brief Overridden by derived classes to return device associated with USM
  /// allocation, or nullptr if host allocation.
  cl_device_id getDevice() const override { return nullptr; }

  /// @brief Allocates host side memory for the USM allocation and binds
  /// it to Mux buffer objects for supported devices. Populates the members
  /// 'mux_memories' and 'mux_buffers'
  ///
  /// @param[in] alignment Minimum alignment in bytes for allocation
  ///
  /// @return CL_SUCCESS or an OpenCL error code on failure
  cl_int allocate(uint32_t alignment) override;

  /// @brief Given an OpenCL device returns the Mux buffer object associated
  /// with the device for this USM allocation.
  ///
  /// @param[in] query_device Device to find a buffer for.
  ///
  /// @return A matching Mux buffer object on success, or nullptr on failure
  mux_buffer_t getMuxBufferForDevice(cl_device_id device) const override;

  /// @brief Return the USM memory type for this allocation
  ///
  /// @return The enum variant corresponding to this memory allocation type
  cl_unified_shared_memory_type_intel getMemoryType() const override {
    return CL_MEM_TYPE_HOST_INTEL;
  }

  /// @brief Mux memory object bound to every device in the OpenCL context.
  cargo::dynamic_array<mux_memory_t> mux_memories;

  /// @brief Mux buffer object bound to every device in the OpenCL context.
  cargo::dynamic_array<mux_buffer_t> mux_buffers;
};

/// @brief Derived class for device USM allocations
class device_allocation_info final : public allocation_info {
  /// @brief Private constructor, use the `create()` functions instead.
  device_allocation_info(const cl_context context, const cl_device_id device,
                         const size_t size);

  device_allocation_info(const device_allocation_info &) = delete;

 public:
  /// @brief Create a device-side USM allocation
  ///
  /// @param[in] context Context the allocation will belong to.
  /// @param[in] device Device to allocate memory from.
  /// @param[in] properties Properties bitfield encoding which properties to
  /// enable.
  /// @param[in] size Bytes to allocate.
  /// @param[in] alignment Minimum alignment of allocation.
  static cargo::expected<std::unique_ptr<device_allocation_info>, cl_int>
  create(cl_context context, cl_device_id device,
         const cl_mem_properties_intel *properties, const size_t size,
         cl_uint alignment);

  /// @brief Destructor.
  ~device_allocation_info() override;

  /// @brief Get device used when allocation was made
  cl_device_id getDevice() const override { return device; }

  /// @brief Allocates device side memory for the USM allocation and binds
  /// it to a Mux buffer object. Sets 'mux_memory' and 'mux_buffer' members.
  ///
  /// @param[in] alignment Minimum alignment in bytes for allocation
  ///
  /// @return CL_SUCCESS or an OpenCL error code on failure
  cl_int allocate(uint32_t alignment) override;

  /// @brief Given an OpenCL device returns the Mux buffer object associated
  /// with the device for this USM allocation.
  ///
  /// @param[in] query_device Device to find a buffer for.
  ///
  /// @return A matching Mux buffer object on success, or nullptr on failure
  mux_buffer_t getMuxBufferForDevice(cl_device_id query_device) const override {
    return query_device == device ? mux_buffer : nullptr;
  };

  /// @brief Return the USM memory type for this allocation
  ///
  /// @return The enum variant corresponding to this memory allocation type
  cl_unified_shared_memory_type_intel getMemoryType() const override {
    return CL_MEM_TYPE_DEVICE_INTEL;
  }

  /// @brief OpenCL device associated with memory allocation
  const cl_device_id device;
  /// @brief Mux memory allocated on device
  mux_memory_t mux_memory;
  /// @brief Mux buffer tied to mux_memory
  mux_buffer_t mux_buffer;
};

/// @brief Derived class for shared USM allocations
class shared_allocation_info final : public allocation_info {
  /// @brief Private constructor, use the `create()` functions instead.
  shared_allocation_info(const cl_context context, const cl_device_id device,
                         const size_t size);

  shared_allocation_info(const shared_allocation_info &) = delete;

 public:
  /// @brief Create a shared USM allocation
  ///
  /// @param[in] context Context the allocation will belong to.
  /// @param[in] device Device associated with this shared allocation, may be
  /// null to not associate it with a device.
  /// @param[in] properties Properties bitfield encoding which properties to
  /// enable.
  /// @param[in] size Bytes to allocate.
  /// @param[in] alignment Minimum alignment of allocation.
  static cargo::expected<std::unique_ptr<shared_allocation_info>, cl_int>
  create(cl_context context, cl_device_id device,
         const cl_mem_properties_intel *properties, const size_t size,
         cl_uint alignment);

  /// @brief Destructor.
  ~shared_allocation_info() override;

  /// @brief Get device used when allocation was made, or nullptr if no device
  /// was given
  cl_device_id getDevice() const override { return device; }

  /// @brief Allocates memory for the USM allocation and binds it to a Mux
  /// buffer object if a device is associated with this allocation. Sets
  /// 'mux_memory' and 'mux_buffer' members if bound, otherwise they remain
  /// nullptr.
  ///
  /// @param[in] alignment Minimum alignment in bytes for allocation
  ///
  /// @return CL_SUCCESS or an OpenCL error code on failure
  cl_int allocate(uint32_t alignment) override;

  /// @brief Given an OpenCL device returns the Mux buffer object associated
  /// with the device for this USM allocation.
  ///
  /// @param[in] query_device Device to find a buffer for.
  ///
  /// @return A matching Mux buffer object on success, or nullptr on failure
  mux_buffer_t getMuxBufferForDevice(cl_device_id query_device) const override {
    if (!device) {
      return nullptr;
    }
    return query_device == device ? mux_buffer : nullptr;
  };

  /// @brief Return the USM memory type for this allocation
  ///
  /// @return The enum variant corresponding to this memory allocation type
  cl_unified_shared_memory_type_intel getMemoryType() const override {
    return CL_MEM_TYPE_SHARED_INTEL;
  }

  /// @brief OpenCL device associated with memory allocation, may be nullptr
  const cl_device_id device;
  /// @brief Mux memory allocated on device
  mux_memory_t mux_memory;
  /// @brief Mux buffer tied to mux_memory
  mux_buffer_t mux_buffer;
};

/// @brief Validates properties passed to the USM allocation entry points for
/// correctness, returning memory allocation flags so they can be stored for
/// later user queries.
///
/// @param[in] properties NULL terminated list of properties passed to
/// allocation entry points.
///
/// @return Bitfield of flags set for CL_MEM_ALLOC_FLAGS_INTEL property, or
/// an OpenCL error code if properties are malformed according to extension
/// spec.
cargo::expected<cl_mem_alloc_flags_intel, cl_int> parseProperties(
    const cl_mem_properties_intel *properties, bool is_shared);

/// @brief Finds if a pointer belongs to the memory addresses of any USM memory
/// allocations existing in the context.
///
/// @param[in] context Context containing list of USM allocations to search.
/// @param[in] ptr Pointer to find an owning USM allocation for.
///
/// @note this is not thread safe and a USM mutex should be used above it.
/// @return Pointer to matching allocation on success, or nullptr on failure.
allocation_info *findAllocation(const cl_context context, const void *ptr);

/// @brief Checks if an OpenCL device can support device USM allocations, a
/// mandatory feature of the extension specification.
///
/// @param[in] device OpenCL device to query support for.
///
/// @return True if device can support device allocations, false otherwise.
bool deviceSupportsDeviceAllocations(cl_device_id device);

/// @brief Checks if an OpenCL device can support host USM allocations, an
/// optional feature of the extension specification.
///
/// @param[in] device OpenCL device to query support for.
///
/// @return True if device can support host allocations, false otherwise.
bool deviceSupportsHostAllocations(cl_device_id device);

/// @brief Checks if an OpenCL device can support shared USM allocations, an
/// optional feature of the extension specification.
///
/// @param[in] device OpenCL device to query support for.
///
/// @return True if device can support host allocations, false otherwise.
bool deviceSupportsSharedAllocations(cl_device_id device);

/// @brief Creates an OpenCL event for use by kernel enqueue commands
/// `clEnqueueNDRangeKernel` and `clEnqueueTask`.
///
/// The created event is recorded by all USM allocations associated with the
/// kernel as arguments, and returned as an output parameter.
///
/// @param[in] queue Queue command is being pushed to
/// @param[in] kernel Kernel object used in command
/// @param[in] type Command type to create event for
/// @param[out] event Event created by function
///
/// @return CL_SUCCESS, or an OpenCL error code on failure.
cl_int createBlockingEventForKernel(cl_command_queue queue, cl_kernel kernel,
                                    const cl_command_type type,
                                    cl_event &event);
}  // namespace usm
#endif  // OCL_EXTENSION_cl_intel_unified_shared_memory
/// @}
}  // namespace extension

#endif  // EXTENSION_INTEL_UNIFIED_SHARED_MEMORY_H_INCLUDED
