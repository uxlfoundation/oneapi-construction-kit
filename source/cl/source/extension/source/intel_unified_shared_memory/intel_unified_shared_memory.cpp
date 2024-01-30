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

#include <CL/cl_ext.h>
#include <cl/context.h>
#include <cl/device.h>
#include <cl/event.h>
#include <cl/kernel.h>
#include <cl/program.h>
#include <extension/intel_unified_shared_memory.h>

namespace extension {
#ifdef OCL_EXTENSION_cl_intel_unified_shared_memory
namespace usm {
// Return whether a given alignment for a USM pointer is valid
//
// A valid alignment is a possibly 0 power of two. This function does not check
// for device support.
constexpr bool isAlignmentValid(cl_uint alignment) {
  return !alignment || (alignment & (alignment - 1)) == 0;
}

/// Return whether multiple bits are set for the given value
template <typename T>
constexpr bool areMultipleBitsSet(T value) {
  return value && (value & (value - 1)) != 0;
};

cargo::expected<cl_mem_alloc_flags_intel, cl_int> parseProperties(
    const cl_mem_properties_intel *properties, bool is_shared) {
  cl_mem_alloc_flags_intel alloc_flags = 0;
  if (properties && properties[0] != 0) {
    auto current = properties;
    cl_mem_properties_intel seen = 0;
    do {
      const cl_mem_properties_intel property = current[0];
      const cl_mem_properties_intel value = current[1];
      switch (property) {
        case CL_MEM_ALLOC_FLAGS_INTEL: {
          if (0 == (seen & CL_MEM_ALLOC_FLAGS_INTEL)) {
            constexpr auto PLACEMENT =
                CL_MEM_ALLOC_INITIAL_PLACEMENT_DEVICE_INTEL |
                CL_MEM_ALLOC_INITIAL_PLACEMENT_HOST_INTEL;

            const auto valid_flags =
                CL_MEM_ALLOC_WRITE_COMBINED_INTEL | (is_shared ? PLACEMENT : 0);

            if (value & ~valid_flags) {
              // Either an invalid flag is set, or one of the "placement" ones
              // set on a non-shared pointer
              return cargo::make_unexpected(CL_INVALID_PROPERTY);
            }

            if (areMultipleBitsSet(value & PLACEMENT)) {
              // Options are mutually exclusive - Can't have both
              return cargo::make_unexpected(CL_INVALID_PROPERTY);
            }

            seen |= property;
            alloc_flags = value;
            break;
          }
          // Fallthrough to error if we've seen property already
          [[fallthrough]];
        }
        default:
          return cargo::make_unexpected(CL_INVALID_PROPERTY);
      }
      current += 2;
    } while (current[0] != 0);
  }
  return alloc_flags;
}

allocation_info *findAllocation(const cl_context context, const void *ptr) {
  // Note this is not thread safe and the usm mutex should be locked above this.
  auto ownsUsmPtr = [ptr](const std::unique_ptr<allocation_info> &usm_alloc) {
    return usm_alloc->isOwnerOf(ptr);
  };

  auto usm_alloc_itr = std::find_if(context->usm_allocations.begin(),
                                    context->usm_allocations.end(), ownsUsmPtr);

  const bool found = usm_alloc_itr != context->usm_allocations.end();
  return found ? usm_alloc_itr->get() : nullptr;
}

bool deviceSupportsDeviceAllocations(cl_device_id device) {
  const auto device_info = device->mux_device->info;
  return device_info->allocation_capabilities &
         mux_allocation_capabilities_alloc_device;
}

bool deviceSupportsHostAllocations(cl_device_id device) {
  const auto device_info = device->mux_device->info;

  // A device requires the capability to allocate host memory, as this implies
  // a cache coherent memory architecture which does not require flushing.
  // However, for host usm allocations the device will use pre-allocated memory
  // rather than allocating itself.
  const bool can_access_host = 0 != (device_info->allocation_capabilities &
                                     mux_allocation_capabilities_cached_host);

  // Pointers from host allocations are required to have address equivalence
  // on device. Mismatching bit-widths would require extra workarounds to enable
  // this, not currently implemented.
  const bool ptr_widths_match = 0 != (device_info->address_capabilities &
#if INTPTR_MAX == INT64_MAX
                                      mux_address_capabilities_bits64);
#elif INTPTR_MAX == INT32_MAX
                                      mux_address_capabilities_bits32);
#else
#error Unsupported pointer size
#endif
  return can_access_host & ptr_widths_match;
}

bool deviceSupportsSharedAllocations(cl_device_id device) {
  // Currently, we implement shared allocations as a wrapper around host
  // allocations. Any device that supports host allocations also supports shared
  // allocations.
  return deviceSupportsHostAllocations(device);
}

cl_int createBlockingEventForKernel(cl_command_queue queue, cl_kernel kernel,
                                    const cl_command_type type,
                                    cl_event &return_event) {
  // For USM we keep a record of commands enqueued using USM allocations by
  // tracking the cl_events, this allows us to perform a blocking free by
  // waiting on there completion.
  auto new_event = _cl_event::create(queue, type);
  if (!new_event) {
    return CL_OUT_OF_RESOURCES;
  }
  return_event = *new_event;

  // USM allocations which have been set explicitly via clSetKernelExecInfo
  // to be used indirectly in the kernel.
  for (auto indirect_alloc : kernel->indirect_usm_allocs) {
    if (indirect_alloc) {
      auto mux_error = indirect_alloc->record_event(return_event);
      OCL_CHECK(mux_error, return CL_OUT_OF_RESOURCES);
    }
  }

  // If cl_kernel_exec_info flags have been set, we need to record
  // the event for all allocations which match the flag type, as they
  // could be indirectly used.
  if (kernel->kernel_exec_info_usm_flags) {
    auto context = kernel->program->context;
    // Kernel may access any host USM alloc
    const bool host_flag_set = kernel->kernel_exec_info_usm_flags &
                               kernel_exec_info_indirect_host_access;
    // Kernel may access any device USM alloc
    const bool device_flag_set = kernel->kernel_exec_info_usm_flags &
                                 kernel_exec_info_indirect_device_access;
    for (auto &usm_alloc : context->usm_allocations) {
      const bool host_alloc = nullptr == usm_alloc->getDevice();

      const bool is_indirect_alloc =
          (host_alloc && host_flag_set) || (!host_alloc && device_flag_set);
      if (is_indirect_alloc) {
        auto mux_error = usm_alloc->record_event(return_event);
        OCL_CHECK(mux_error, return CL_OUT_OF_RESOURCES);
      }
    }
  }

  return CL_SUCCESS;
}

allocation_info::allocation_info(const cl_context context, const size_t size)
    : context(context), size(size), base_ptr(nullptr), alloc_flags(0) {
  cl::retainInternal(context);
}

allocation_info::~allocation_info() {
  for (auto event : queued_commands) {
    cl::releaseInternal(event);
  }

  cl::releaseInternal(context);
}

mux_result_t allocation_info::record_event(cl_event event) {
  const std::unique_lock<std::mutex> guard(this->mutex);

  cl::retainInternal(event);
  return queued_commands.push_back(event);
}

host_allocation_info::host_allocation_info(const cl_context context,
                                           const size_t size)
    : allocation_info(context, size) {}

host_allocation_info::~host_allocation_info() {
  // Free the Mux objects we've created
  for (cl_uint index = 0; index < context->devices.size(); ++index) {
    auto device = context->devices[index];

    mux_buffer_t mux_buffer = mux_buffers[index];
    if (mux_buffer) {
      (void)muxDestroyBuffer(device->mux_device, mux_buffer,
                             device->mux_allocator);
    }

    mux_memory_t mux_memory = mux_memories[index];
    if (mux_memory) {
      muxFreeMemory(device->mux_device, mux_memory, device->mux_allocator);
    }
  }

  // Free the host side allocation
  if (base_ptr) {
    cargo::free(base_ptr);
  }
};

cargo::expected<std::unique_ptr<host_allocation_info>, cl_int>
host_allocation_info::create(cl_context context,
                             const cl_mem_properties_intel *properties,
                             size_t size, cl_uint alignment) {
  OCL_CHECK(size == 0, return cargo::make_unexpected(CL_INVALID_BUFFER_SIZE));
  OCL_CHECK(!isAlignmentValid(alignment),
            return cargo::make_unexpected(CL_INVALID_VALUE));

  cl_uint max_align = 0;
  for (auto device : context->devices) {
    const auto device_align = device->min_data_type_align_size;

    OCL_CHECK(size > device->max_mem_alloc_size,
              return cargo::make_unexpected(CL_INVALID_BUFFER_SIZE));
    OCL_CHECK(alignment > device_align,
              return cargo::make_unexpected(CL_INVALID_VALUE));

    max_align = device_align > max_align ? device_align : max_align;
  }

  if (alignment == 0) {
    alignment = max_align;
  }

  auto alloc_properties = parseProperties(properties, false);
  if (!alloc_properties) {
    return cargo::make_unexpected(alloc_properties.error());
  }

  auto usm_alloc = std::unique_ptr<host_allocation_info>(
      new (std::nothrow) host_allocation_info(context, size));
  OCL_CHECK(!usm_alloc, return cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY));

  OCL_CHECK(nullptr == usm_alloc,
            return cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY));

  const cl_int error = usm_alloc->allocate(alignment);
  OCL_CHECK(error != CL_SUCCESS, return cargo::make_unexpected(error));

  usm_alloc->alloc_flags = alloc_properties.value();

  return usm_alloc;
}

cl_int host_allocation_info::allocate(cl_uint alignment) {
  base_ptr = cargo::alloc(size, alignment);
  if (base_ptr == nullptr) {
    return CL_OUT_OF_HOST_MEMORY;
  }

  const size_t num_devices = context->devices.size();
  if (cargo::success != mux_memories.alloc(num_devices)) {
    return CL_OUT_OF_HOST_MEMORY;
  }

  if (cargo::success != mux_buffers.alloc(num_devices)) {
    return CL_OUT_OF_HOST_MEMORY;
  }

  for (cl_uint index = 0; index < num_devices; ++index) {
    cl_device_id device = context->devices[index];

    if (!deviceSupportsHostAllocations(device)) {
      // Device doesn't support host allocations
      continue;
    }

    // Initialize the Mux objects needed by each device
    if (muxCreateBuffer(device->mux_device, size, device->mux_allocator,
                        &mux_buffers[index])) {
      return CL_OUT_OF_HOST_MEMORY;
    }

    mux_result_t mux_error =
        muxCreateMemoryFromHost(device->mux_device, size, base_ptr,
                                device->mux_allocator, &mux_memories[index]);
    if (mux_error) {
      return CL_OUT_OF_RESOURCES;
    }

    const uint64_t offset = 0;
    mux_error = muxBindBufferMemory(device->mux_device, mux_memories[index],
                                    mux_buffers[index], offset);
    if (mux_error) {
      return CL_MEM_OBJECT_ALLOCATION_FAILURE;
    }
  }
  return CL_SUCCESS;
}

mux_buffer_t host_allocation_info::getMuxBufferForDevice(
    cl_device_id device) const {
  const auto device_index = context->getDeviceIndex(device);
  if (auto mux_buffer = mux_buffers.at(device_index)) {
    return *mux_buffer;
  }
  return nullptr;
}

device_allocation_info::device_allocation_info(const cl_context context,
                                               const cl_device_id device,
                                               const size_t size)
    : allocation_info(context, size),
      device(device),
      mux_memory(nullptr),
      mux_buffer(nullptr) {
  cl::retainInternal(device);
};

device_allocation_info::~device_allocation_info() {
  if (mux_buffer) {
    (void)muxDestroyBuffer(device->mux_device, mux_buffer,
                           device->mux_allocator);
  }

  if (mux_memory) {
    muxFreeMemory(device->mux_device, mux_memory, device->mux_allocator);
  }

  cl::releaseInternal(device);
};

cargo::expected<std::unique_ptr<device_allocation_info>, cl_int>
device_allocation_info::create(cl_context context, cl_device_id device,
                               const cl_mem_properties_intel *properties,
                               size_t size, cl_uint alignment) {
  OCL_CHECK(size == 0, return cargo::make_unexpected(CL_INVALID_BUFFER_SIZE));
  OCL_CHECK(!isAlignmentValid(alignment),
            return cargo::make_unexpected(CL_INVALID_VALUE));

  const auto device_align = device->mux_device->info->buffer_alignment;
  OCL_CHECK(size > device->max_mem_alloc_size,
            return cargo::make_unexpected(CL_INVALID_BUFFER_SIZE));
  OCL_CHECK(alignment > device_align,
            return cargo::make_unexpected(CL_INVALID_VALUE));

  auto alloc_properties = parseProperties(properties, false);
  if (!alloc_properties) {
    return cargo::make_unexpected(alloc_properties.error());
  }

  if (alignment == 0) {
    alignment = device_align;
  }

  auto usm_alloc = std::unique_ptr<device_allocation_info>(
      new (std::nothrow) device_allocation_info(context, device, size));
  OCL_CHECK(!usm_alloc, return cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY));

  const cl_int error = usm_alloc->allocate(alignment);
  OCL_CHECK(error != CL_SUCCESS, return cargo::make_unexpected(error));

  usm_alloc->alloc_flags = alloc_properties.value();

  return usm_alloc;
}

cl_int device_allocation_info::allocate(cl_uint alignment) {
  // Allocation device local memory
  const uint32_t heap = 1;
  mux_result_t mux_error = muxAllocateMemory(
      device->mux_device, size, heap, mux_memory_property_device_local,
      mux_allocation_type_alloc_device, alignment, device->mux_allocator,
      &mux_memory);
  if (mux_error) {
    return CL_OUT_OF_RESOURCES;
  }

  mux_error = muxCreateBuffer(device->mux_device, size, device->mux_allocator,
                              &mux_buffer);
  if (mux_error) {
    return CL_OUT_OF_RESOURCES;
  }

  mux_error =
      muxBindBufferMemory(device->mux_device, mux_memory, mux_buffer, 0);
  if (mux_error) {
    return CL_OUT_OF_RESOURCES;
  }

#if INTPTR_MAX == INT64_MAX
  base_ptr = reinterpret_cast<void *>(mux_memory->handle);
#elif INTPTR_MAX == INT32_MAX
  assert((device->mux_device->info->address_capabilities &
          mux_address_capabilities_bits32) != 0 &&
         "32-bit host with 64-bit device not supported");
  base_ptr =
      reinterpret_cast<void *>(static_cast<uint32_t>(mux_memory->handle));
#else
#error Unsupported pointer size
#endif
  OCL_CHECK(nullptr == base_ptr, return CL_OUT_OF_RESOURCES);

  return CL_SUCCESS;
}

shared_allocation_info::shared_allocation_info(const cl_context context,
                                               const cl_device_id device,
                                               const size_t size)
    : allocation_info(context, size),
      device(device),
      mux_memory(nullptr),
      mux_buffer(nullptr) {
  if (device) {
    cl::retainInternal(device);
  }
};

shared_allocation_info::~shared_allocation_info() {
  if (mux_buffer) {
    assert(device);
    (void)muxDestroyBuffer(device->mux_device, mux_buffer,
                           device->mux_allocator);
  }

  if (mux_memory) {
    assert(device);
    muxFreeMemory(device->mux_device, mux_memory, device->mux_allocator);
  }

  // Free the host side allocation
  if (base_ptr) {
    cargo::free(base_ptr);
  }

  if (device) {
    cl::releaseInternal(device);
  }
};

cargo::expected<std::unique_ptr<shared_allocation_info>, cl_int>
shared_allocation_info::create(cl_context context, cl_device_id device,
                               const cl_mem_properties_intel *properties,
                               size_t size, cl_uint alignment) {
  OCL_CHECK(size == 0, return cargo::make_unexpected(CL_INVALID_BUFFER_SIZE));

  if (device) {
    OCL_CHECK(size > device->max_mem_alloc_size,
              return cargo::make_unexpected(CL_INVALID_BUFFER_SIZE));
    OCL_CHECK(alignment > device->mux_device->info->buffer_alignment,
              return cargo::make_unexpected(CL_INVALID_VALUE));
  } else {
    // If no device is given, all devices that support shared USM must meet
    // the max_alloc_size and alignment requirements
    for (auto device : context->devices) {
      if (deviceSupportsSharedAllocations(device)) {
        const auto device_align = device->mux_device->info->buffer_alignment;
        OCL_CHECK(size > device->max_mem_alloc_size,
                  return cargo::make_unexpected(CL_INVALID_BUFFER_SIZE));
        OCL_CHECK(alignment > device_align,
                  return cargo::make_unexpected(CL_INVALID_VALUE));
      }
    }
  }

  OCL_CHECK(!isAlignmentValid(alignment),
            return cargo::make_unexpected(CL_INVALID_VALUE));

  if (alignment == 0) {
    if (device) {
      alignment = device->mux_device->info->buffer_alignment;
    } else {
      // Default alignment with no device is the highest alignment of all other
      // devices that support shared allocations (must be at least one,
      // otherwise this function is undefined)
      cl_uint max_align = 2;
      for (auto device : context->devices) {
        if (deviceSupportsSharedAllocations(device)) {
          const auto device_align = device->min_data_type_align_size;
          max_align = std::max(device_align, max_align);
        }
      }
      alignment = max_align;
    }
  }

  auto alloc_properties = parseProperties(properties, true);
  if (!alloc_properties) {
    return cargo::make_unexpected(alloc_properties.error());
  }

  auto usm_alloc = std::unique_ptr<shared_allocation_info>(
      new (std::nothrow) shared_allocation_info(context, device, size));
  OCL_CHECK(!usm_alloc, return cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY));

  // Decide whether to allocate initially on device or not
  // NOTE: The specification says these are only hints, so we ignore them for
  // now
  bool prefers_host = true;
  if (*alloc_properties & CL_MEM_ALLOC_INITIAL_PLACEMENT_DEVICE_INTEL) {
    prefers_host = false;
  }
  (void)prefers_host;

  const cl_int error = usm_alloc->allocate(alignment);
  OCL_CHECK(error != CL_SUCCESS, return cargo::make_unexpected(error));

  usm_alloc->alloc_flags = alloc_properties.value();

  return usm_alloc;
}

cl_int shared_allocation_info::allocate(cl_uint alignment) {
  base_ptr = cargo::alloc(size, alignment);
  if (base_ptr == nullptr) {
    return CL_OUT_OF_HOST_MEMORY;
  }

  if (device) {
    if (!deviceSupportsSharedAllocations(device)) {
      return CL_INVALID_OPERATION;
    }

    // Initialize the Mux objects needed by each device
    if (muxCreateBuffer(device->mux_device, size, device->mux_allocator,
                        &mux_buffer)) {
      return CL_OUT_OF_HOST_MEMORY;
    }

    mux_result_t mux_error = muxCreateMemoryFromHost(
        device->mux_device, size, base_ptr, device->mux_allocator, &mux_memory);
    if (mux_error) {
      return CL_OUT_OF_RESOURCES;
    }

    const uint64_t offset = 0;
    mux_error =
        muxBindBufferMemory(device->mux_device, mux_memory, mux_buffer, offset);
    if (mux_error) {
      return CL_MEM_OBJECT_ALLOCATION_FAILURE;
    }
  }

  return CL_SUCCESS;
}

}  // namespace usm
#endif  // OCL_EXTENSION_cl_intel_unified_shared_memory

intel_unified_shared_memory::intel_unified_shared_memory()
    : extension("cl_intel_unified_shared_memory",
#ifdef OCL_EXTENSION_cl_intel_unified_shared_memory
                usage_category::DEVICE
#else
                usage_category::DISABLED
#endif
                    // Version "R" of spec
                    CA_CL_EXT_VERSION(0, 18, 0)) {
}

cl_int intel_unified_shared_memory::GetDeviceInfo(
    cl_device_id device, cl_device_info param_name, size_t param_value_size,
    void *param_value, size_t *param_value_size_ret) const {
#ifndef OCL_EXTENSION_cl_intel_unified_shared_memory
  return extension::GetDeviceInfo(device, param_name, param_value_size,
                                  param_value, param_value_size_ret);
#else
  // Device allocations are required to support the extension, rather than an
  // optional capability. Therefore, if the extension is enabled in the build
  // but a device doesn't have this capability, then return CL_INVALID_DEVICE
  // to our CL extension mechanism so it knows not to include the extension
  // when queried by the user for CL_DEVICE_EXTENSIONS.
  if (!usm::deviceSupportsDeviceAllocations(device)) {
    return CL_INVALID_DEVICE;
  }

  cl_device_unified_shared_memory_capabilities_intel result = 0;

  switch (param_name) {
    case CL_DEVICE_HOST_MEM_CAPABILITIES_INTEL:
      if (!usm::deviceSupportsHostAllocations(device)) {
        break;
      }
      [[fallthrough]];
    case CL_DEVICE_DEVICE_MEM_CAPABILITIES_INTEL:
      result = CL_UNIFIED_SHARED_MEMORY_ACCESS_INTEL;
      break;
    case CL_DEVICE_SINGLE_DEVICE_SHARED_MEM_CAPABILITIES_INTEL:
      if (!usm::deviceSupportsSharedAllocations(device)) {
        break;
      }
      result = CL_UNIFIED_SHARED_MEMORY_ACCESS_INTEL;
      break;
    case CL_DEVICE_CROSS_DEVICE_SHARED_MEM_CAPABILITIES_INTEL:
      // Not supported yet
      result = 0;
      break;
    case CL_DEVICE_SHARED_SYSTEM_MEM_CAPABILITIES_INTEL:
      break;
    default:
      // Use default implementation that uses the name set in the constructor
      // as the name usage specifies.
      return extension::GetDeviceInfo(device, param_name, param_value_size,
                                      param_value, param_value_size_ret);
  }

  const size_t type_size =
      sizeof(cl_device_unified_shared_memory_capabilities_intel);
  if (nullptr != param_value) {
    OCL_CHECK(param_value_size < type_size, return CL_INVALID_VALUE);
    *static_cast<cl_device_unified_shared_memory_capabilities_intel *>(
        param_value) = result;
  }
  OCL_SET_IF_NOT_NULL(param_value_size_ret, type_size);
  return CL_SUCCESS;
#endif  // OCL_EXTENSION_cl_intel_unified_shared_memory
}

#if (defined(CL_VERSION_3_0) || \
     defined(OCL_EXTENSION_cl_codeplay_kernel_exec_info))
cl_int intel_unified_shared_memory::SetKernelExecInfo(
    cl_kernel kernel, cl_kernel_exec_info_codeplay param_name,
    size_t param_value_size, const void *param_value) const {
#ifdef OCL_EXTENSION_cl_intel_unified_shared_memory
  switch (param_name) {
    case CL_KERNEL_EXEC_INFO_USM_PTRS_INTEL: {
      OCL_CHECK(!param_value, return CL_INVALID_VALUE);
      OCL_CHECK(
          (param_value_size == 0) || (param_value_size % sizeof(void *) != 0),
          return CL_INVALID_VALUE);

      // Set the _cl_kernel list of indirect USM allocations
      void *const *usm_pointers = static_cast<void *const *>(param_value);
      const size_t num_pointers = param_value_size / sizeof(void *);
      auto &indirect_allocs = kernel->indirect_usm_allocs;
      if (indirect_allocs.alloc(num_pointers) != cargo::success) {
        return CL_OUT_OF_HOST_MEMORY;
      }

      const cl_context context = kernel->program->context;
      const std::lock_guard<std::mutex> context_guard(context->usm_mutex);
      for (size_t i = 0; i < num_pointers; i++) {
        indirect_allocs[i] = usm::findAllocation(context, usm_pointers[i]);
      }

      return CL_SUCCESS;
    }

#define INDIRECT_ACCESS_FLAG_CASE(arg_type, type)                            \
  case arg_type: {                                                           \
    OCL_CHECK(!param_value, return CL_INVALID_VALUE);                        \
    OCL_CHECK(param_value_size != sizeof(cl_bool), return CL_INVALID_VALUE); \
                                                                             \
    const cl_bool flag_set = *(static_cast<const cl_bool *>(param_value));   \
    if (flag_set) {                                                          \
      kernel->kernel_exec_info_usm_flags |= type;                            \
    } else {                                                                 \
      kernel->kernel_exec_info_usm_flags &= ~type;                           \
    }                                                                        \
    return CL_SUCCESS;                                                       \
  }

      INDIRECT_ACCESS_FLAG_CASE(CL_KERNEL_EXEC_INFO_INDIRECT_HOST_ACCESS_INTEL,
                                usm::kernel_exec_info_indirect_host_access);
      INDIRECT_ACCESS_FLAG_CASE(
          CL_KERNEL_EXEC_INFO_INDIRECT_DEVICE_ACCESS_INTEL,
          usm::kernel_exec_info_indirect_device_access);
      INDIRECT_ACCESS_FLAG_CASE(
          CL_KERNEL_EXEC_INFO_INDIRECT_SHARED_ACCESS_INTEL,
          usm::kernel_exec_info_indirect_shared_access);
#undef INDIRECT_ACCESS_FLAG_CASE

    default:
      break;
  }
#endif  // OCL_EXTENSION_cl_intel_unified_shared_memory
  return extension::SetKernelExecInfo(kernel, param_name, param_value_size,
                                      param_value);
}
#endif  // CL_VERSION_3_0 || OCL_EXTENSION_cl_intel_unified_shared_memory
}  // namespace extension
