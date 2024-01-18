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

#include <cl/buffer.h>
#include <cl/command_queue.h>
#include <cl/mem.h>
#include <cl/mux.h>
#include <cl/platform.h>
#include <cl/validate.h>
#include <mux/utils/helpers.h>
#include <tracer/tracer.h>

#include <memory>

_cl_mem::_cl_mem(const cl_context context, const cl_mem_flags flags,
                 const size_t size, const cl_mem_object_type type,
                 cl_mem optional_parent, void *host_ptr,
                 const cl::ref_count_type ref_count_init_type,
                 cargo::dynamic_array<mux_memory_t> &&mux_memories)
    : base<_cl_mem>(ref_count_init_type),
      context(context),
      flags(flags),
      size(size),
      type(type),
      optional_parent(optional_parent),
      host_ptr(host_ptr),
      mux_memories(std::move(mux_memories)),
      callbacks(),
      callback_datas(),
      mapCount(0),
      map_base_pointer(nullptr),
      device_owner(nullptr)
#if defined(CL_VERSION_3_0)
      ,
      uses_svm_pointer(CL_FALSE)
#endif
{
  // target_mem is set later as its creation might require multiple steps and
  // therefore cannot happen inside an initialization list.
  OCL_ASSERT(nullptr != context, "context must be valid.");

  cl::retainInternal(context);

  if (nullptr != optional_parent) {
    OCL_ASSERT(context == optional_parent->context, "Context mismatch.");
    cl::retainInternal(optional_parent);
  }
}

_cl_mem::~_cl_mem() {
  for (int32_t i = callbacks.size() - 1; i >= 0; i--) {
    (callbacks[i])(this, callback_datas[i]);
  }

  if (optional_parent) {
    cl::releaseInternal(optional_parent);
  } else {
    for (cl_uint index = 0; index < context->devices.size(); ++index) {
      auto device = context->devices[index];
      muxFreeMemory(device->mux_device, mux_memories[index],
                    device->mux_allocator);
    }
  }

  cl::releaseInternal(context);
}

bool _cl_mem::registerCallback(cl::pfn_notify_mem_t pfn_notify,
                               void *user_data) {
  if (callbacks.push_back(pfn_notify)) {
    return false;
  }
  if (callback_datas.push_back(user_data)) {
    return false;
  }
  return true;
}

cl_int _cl_mem::allocateMemory(mux_device_t mux_device,
                               uint32_t supported_heaps,
                               mux_allocator_info_t mux_allocator,
                               mux_memory_t *out_memory) {
  const auto device_alloc_caps = mux_device->info->allocation_capabilities;

  // Device supports host coherent memory and user wants to use a host side
  // pointer. If the pointer alignment is compatible with the device, then
  // create cl_mem object from this pre-allocated memory.
  if ((CL_MEM_USE_HOST_PTR & flags) &&
      (device_alloc_caps & mux_allocation_capabilities_coherent_host)) {
    const uintptr_t host_ptr_uint = reinterpret_cast<uintptr_t>(host_ptr);
    if (0 == (host_ptr_uint % mux_device->info->buffer_alignment)) {
      const mux_result_t error = muxCreateMemoryFromHost(
          mux_device, size, host_ptr, mux_allocator, out_memory);
      return error ? CL_MEM_OBJECT_ALLOCATION_FAILURE : CL_SUCCESS;
    }
  }

  // Set the core allocation type based on if the user is requesting host
  // accessible memory, and the core device supports this capability.
  const bool device_access_host_mem =
      device_alloc_caps & (mux_allocation_capabilities_coherent_host |
                           mux_allocation_capabilities_cached_host);
  const bool try_alloc_host =
      (CL_MEM_ALLOC_HOST_PTR | CL_MEM_USE_HOST_PTR) & flags;

  mux_allocation_type_e allocationType;
  if (device_access_host_mem && try_alloc_host) {
    allocationType = mux_allocation_type_alloc_host;
  } else {
    OCL_ASSERT(device_alloc_caps & mux_allocation_capabilities_alloc_device,
               "device doesn't have device memory allocation capability");
    allocationType = mux_allocation_type_alloc_device;
  }

  // Default to host visible memory to enable mapping, unless flags are set to
  // forbid host access.
  const bool host_ptr_flag_set =
      (CL_MEM_COPY_HOST_PTR | CL_MEM_USE_HOST_PTR | CL_MEM_ALLOC_HOST_PTR) &
      flags;
  uint32_t memoryProperties;
  if (!host_ptr_flag_set && (CL_MEM_HOST_NO_ACCESS & flags)) {
    memoryProperties = mux_memory_property_device_local;
  } else {
    memoryProperties = mux_memory_property_host_visible;
  }

  const uint32_t heap = mux::findFirstSupportedHeap(supported_heaps);
  const uint32_t alignment = 0;  // No alignment preference
  mux_result_t error =
      muxAllocateMemory(mux_device, size, heap, memoryProperties,
                        allocationType, alignment, mux_allocator, out_memory);
  OCL_CHECK(error, return CL_MEM_OBJECT_ALLOCATION_FAILURE);

  if (CL_MEM_USE_HOST_PTR & flags) {
    // We couldn't use the original user provided host pointer to create device
    // memory. Instead map the newly allocated memory and copy data over from
    // the host pointer.
    error = muxMapMemory(mux_device, *out_memory, 0, size, &map_base_pointer);
    OCL_CHECK(error, return CL_MEM_OBJECT_ALLOCATION_FAILURE);

    std::memcpy(map_base_pointer, host_ptr, size);

    error = muxFlushMappedMemoryToDevice(mux_device, *out_memory, 0, size);
    OCL_CHECK(error, return CL_MEM_OBJECT_ALLOCATION_FAILURE);

    error = muxUnmapMemory(mux_device, *out_memory);
    OCL_CHECK(error, return CL_MEM_OBJECT_ALLOCATION_FAILURE);
  }
  return CL_SUCCESS;
}

cl_int _cl_mem::pushMapMemory(cl_command_queue command_queue,
                              void **mappedPointer, size_t offset, size_t size,
                              bool read, bool write, bool invalidate,
                              cargo::array_view<const cl_event> event_wait_list,
                              cl_event return_event) {
  cl::retainInternal(this);
  cl::release_guard<cl_mem> mem_release_guard(this,
                                              cl::ref_count_type::INTERNAL);

  cl_mem mem_to_map = this;
  // if this is a sub-buffer we want to map the memory in the parent so that
  // all other sub-buffers can use the same mapping.
  if (nullptr != optional_parent) {
    mem_to_map = optional_parent;
  }

  // get the mapping's absolute offset
  if (CL_MEM_OBJECT_BUFFER == type) {
    auto buffer = static_cast<cl_mem_buffer>(this);
    // add sub-buffer offset
    offset += buffer->offset;
  }

  // get the index of the device the command queue executes on
  auto device_index =
      command_queue->context->getDeviceIndex(command_queue->device);

  {
    // lock the cl_mem while we get a first mapping
    const std::lock_guard<std::mutex> lock(mem_to_map->mutex);

    // if we have no current mapping map the whole memory chunk in the parent
    if (0 == mem_to_map->mapCount) {
      mux_device_t device = command_queue->device->mux_device;
      mux_memory_t memory = mem_to_map->mux_memories[device_index];

      // we map in read to prevent unmap from modifying the buffer on the
      // device if that's not required.
      auto mux_error = muxMapMemory(device, memory, 0, memory->size,
                                    &mem_to_map->map_base_pointer);
      if (mux_error) {
        return CL_MAP_FAILURE;
      }
    }

    // increment the map count of the parent
    mem_to_map->mapCount++;

    // If the memory object was created with CL_MEM_USE_HOST_PTR set then the
    // pointer value returned must be derived from the user provided host_ptr.
    if (CL_MEM_USE_HOST_PTR & mem_to_map->flags) {
      *mappedPointer = static_cast<char *>(mem_to_map->host_ptr) + offset;
    } else {
      *mappedPointer =
          static_cast<char *>(mem_to_map->map_base_pointer) + offset;
    }

    // We need to record any writes at the point of calling the API in order to
    // check for overlapping buffer regions, if we wait til the user callback
    // below is actually executed before doing this we have no way of reporting
    // an error for overlapping regions.
    if (write || invalidate) {
      mem_to_map->write_mappings.insert(std::pair<void *, _cl_mem::mapping>(
          *mappedPointer, {static_cast<cl_uint>(offset),
                           static_cast<cl_uint>(size), /* is_active */ true}));
    }
  }

  const std::lock_guard<std::mutex> lock(
      command_queue->context->getCommandQueueMutex());

  auto mux_command_buffer =
      command_queue->getCommandBuffer(event_wait_list, return_event);
  if (!mux_command_buffer) {
    return CL_OUT_OF_RESOURCES;
  }

  // mapping state to pass to the user callback
  struct mapping_state_t {
    mapping_state_t(cl_mem mem, size_t offset, size_t size, void *ptr,
                    cl_uint device_index)
        : mem(mem->optional_parent ? mem->optional_parent : mem),
          offset(offset),
          size(size),
          ptr(ptr),
          device_index(device_index) {}

    void flushMemoryFromDevice() {
      mux_device_t device = mem->context->devices[device_index]->mux_device;
      mux_memory_t memory = mem->mux_memories[device_index];
      const mux_result_t error =
          muxFlushMappedMemoryFromDevice(device, memory, offset, size);
      OCL_ASSERT(mux_success == error,
                 "muxFlushMappedMemoryFromDevice failed!");
      OCL_UNUSED(error);

      if (CL_MEM_USE_HOST_PTR & mem->flags) {
        // Copy data from `map_base_pointer` containing our cache of the data.
        // to `host_ptr` user has access to.
        std::memcpy(static_cast<char *>(mem->host_ptr) + offset,
                    static_cast<char *>(mem->map_base_pointer) + offset, size);
      }
    }

    cl_mem mem;
    size_t offset;
    size_t size;
    void *ptr;
    cl_uint device_index;
  } *mapping =
      new mapping_state_t(this, offset, size, *mappedPointer, device_index);

  // assert the preconditions of the following callback selection logic
  OCL_ASSERT(read || write || invalidate,
             "mapping must always be one of read, write, or invalidate");
  OCL_ASSERT(!(read && invalidate), "mapping must not be read and invalidate");
  OCL_ASSERT(!(write && invalidate), "mapping must not be read and invalidate");

  // select actions the mapping callback should perform based on the flags
  void (*callback)(mux_queue_t, mux_command_buffer_t, void *const) = nullptr;
  if (write) {
    callback = [](mux_queue_t, mux_command_buffer_t, void *const user_data) {
      auto mapping = static_cast<mapping_state_t *>(user_data);
      const std::lock_guard<std::mutex> lock(mapping->mem->mutex);
      mapping->flushMemoryFromDevice();
    };
  } else if (read) {
    callback = [](mux_queue_t, mux_command_buffer_t, void *const user_data) {
      auto mapping = static_cast<mapping_state_t *>(user_data);
      const std::lock_guard<std::mutex> lock(mapping->mem->mutex);
      mapping->flushMemoryFromDevice();
    };
  } else if (invalidate) {
    callback = [](mux_queue_t, mux_command_buffer_t, void *const user_data) {
      auto mapping = static_cast<mapping_state_t *>(user_data);
      const std::lock_guard<std::mutex> lock(mapping->mem->mutex);
    };
  }
  OCL_ASSERT(callback, "failed to set user callback for mapping");

  // then add the mapping callback to the command buffer
  auto mux_error = muxCommandUserCallback(*mux_command_buffer, callback,
                                          mapping, 0, nullptr, nullptr);
  if (mux_error) {
    return cl::getErrorFrom(mux_error);
  }

  // don't release the mem just now
  mem_release_guard.dismiss();

  return command_queue->registerDispatchCallback(
      *mux_command_buffer, return_event, [this, mapping]() {
        cl::releaseInternal(this);
        delete mapping;
      });
}

bool _cl_mem::overlaps(size_t offset, size_t size) {
  // Check each mapping and see if it overlaps with this one.
  for (const auto &write_mapping : write_mappings) {
    const auto &mapping = write_mapping.second;

    // We only need to check "active" mappings, since unactive mappings will
    // have been unmapped by the time this check is performed so there is no
    // risk of overlapping.
    if (!mapping.is_active) {
      continue;
    }

    // There are two ways the maps can overlap:
    // 1. The range of the new map starts within this write map.
    if (offset >= mapping.offset && offset < mapping.offset + mapping.size) {
      return true;
    }

    // 2. It starts before this write but is large enough that it overlaps.
    if (offset < mapping.offset && offset + size > mapping.offset) {
      return true;
    }
  }
  return false;
}

CL_API_ENTRY cl_int CL_API_CALL cl::RetainMemObject(cl_mem memobj) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clRetainMemObject");
  OCL_CHECK(!memobj, return CL_INVALID_MEM_OBJECT);

  return cl::retainExternal(memobj);
}

CL_API_ENTRY cl_int CL_API_CALL cl::ReleaseMemObject(cl_mem memobj) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clReleaseMemObject");
  OCL_CHECK(!memobj, return CL_INVALID_MEM_OBJECT);

  return cl::releaseExternal(memobj);
}

CL_API_ENTRY cl_int CL_API_CALL cl::SetMemObjectDestructorCallback(
    cl_mem memobj,
    void(CL_CALLBACK *pfn_notify)(cl_mem memobj, void *user_data),
    void *user_data) {
  const tracer::TraceGuard<tracer::OpenCL> guard(
      "clSetMemObjectDestructorCallback");
  OCL_CHECK(!memobj, return CL_INVALID_MEM_OBJECT);

  if (!(memobj->registerCallback(pfn_notify, user_data))) {
    return CL_OUT_OF_HOST_MEMORY;
  }
  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL cl::GetMemObjectInfo(
    cl_mem memobj, cl_mem_info param_name, size_t param_value_size,
    void *param_value, size_t *param_value_size_ret) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clGetMemObjectInfo");
  OCL_CHECK(!memobj, return CL_INVALID_MEM_OBJECT);

#define MEM_OBJECT_INFO_CASE(NAME, TYPE, VALUE)                   \
  case NAME: {                                                    \
    OCL_CHECK(param_value &&param_value_size < sizeof(TYPE),      \
              return CL_INVALID_VALUE);                           \
    OCL_SET_IF_NOT_NULL(param_value_size_ret, sizeof(TYPE));      \
    OCL_SET_IF_NOT_NULL(static_cast<TYPE *>(param_value), VALUE); \
  } break

  switch (param_name) {
    MEM_OBJECT_INFO_CASE(CL_MEM_TYPE, cl_mem_object_type, memobj->type);
    MEM_OBJECT_INFO_CASE(CL_MEM_FLAGS, cl_mem_flags, memobj->flags);
    MEM_OBJECT_INFO_CASE(CL_MEM_SIZE, size_t, memobj->size);
    MEM_OBJECT_INFO_CASE(
        CL_MEM_HOST_PTR, void *,
        cl::validate::IsInBitSet(memobj->flags, CL_MEM_USE_HOST_PTR)
            ? memobj->host_ptr
            : nullptr);
    MEM_OBJECT_INFO_CASE(CL_MEM_MAP_COUNT, cl_uint, memobj->mapCount);
    MEM_OBJECT_INFO_CASE(CL_MEM_REFERENCE_COUNT, cl_uint,
                         memobj->refCountExternal());
    MEM_OBJECT_INFO_CASE(CL_MEM_CONTEXT, cl_context, memobj->context);

    case CL_MEM_ASSOCIATED_MEMOBJECT: {
      OCL_CHECK(param_value && param_value_size < sizeof(cl_mem),
                return CL_INVALID_VALUE);
      OCL_SET_IF_NOT_NULL(param_value_size_ret, sizeof(cl_mem));

      if (CL_MEM_OBJECT_BUFFER == memobj->type) {
        OCL_SET_IF_NOT_NULL(static_cast<cl_mem *>(param_value),
                            memobj->optional_parent);
      } else {
        OCL_SET_IF_NOT_NULL(static_cast<cl_mem *>(param_value), nullptr);
      }
    } break;

    case CL_MEM_OFFSET: {
      OCL_CHECK(param_value && param_value_size < sizeof(size_t),
                return CL_INVALID_VALUE);
      OCL_SET_IF_NOT_NULL(param_value_size_ret, sizeof(size_t));

      if (CL_MEM_OBJECT_BUFFER == memobj->type &&
          nullptr != memobj->optional_parent) {
        OCL_SET_IF_NOT_NULL(static_cast<size_t *>(param_value),
                            static_cast<_cl_mem_buffer *>(memobj)->offset);
      } else {
        OCL_SET_IF_NOT_NULL(static_cast<size_t *>(param_value), size_t(0));
      }
    } break;
#if defined(CL_VERSION_3_0)
    case CL_MEM_PROPERTIES: {
      const size_t size = sizeof(cl_mem_properties) * memobj->properties.size();
      OCL_CHECK(param_value && param_value_size < size,
                return CL_INVALID_VALUE);
      OCL_SET_IF_NOT_NULL(param_value_size_ret, size);
      if (param_value) {
        auto *value = static_cast<cl_mem_properties *>(param_value);
        std::copy(memobj->properties.begin(), memobj->properties.end(), value);
      }
    } break;
      MEM_OBJECT_INFO_CASE(CL_MEM_USES_SVM_POINTER,
                           decltype(memobj->uses_svm_pointer),
                           memobj->uses_svm_pointer);
#endif
    default: {
      return extension::GetMemObjectInfo(memobj, param_name, param_value_size,
                                         param_value, param_value_size_ret);
    }
  }

#undef MEM_OBJECT_INFO_CASE

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL
cl::EnqueueUnmapMemObject(cl_command_queue command_queue, cl_mem memobj,
                          void *mapped_ptr, cl_uint num_events_in_wait_list,
                          const cl_event *event_wait_list, cl_event *event) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clEnqueueUnmapMemObject");
  OCL_CHECK(!command_queue, return CL_INVALID_COMMAND_QUEUE);
  OCL_CHECK(!memobj, return CL_INVALID_MEM_OBJECT);
  OCL_CHECK(!mapped_ptr, return CL_INVALID_VALUE);
  _cl_context *context = command_queue->context;
  OCL_CHECK(context != memobj->context, return CL_INVALID_CONTEXT);

  const cl_int error = cl::validate::EventWaitList(
      num_events_in_wait_list, event_wait_list, command_queue->context, event);
  OCL_CHECK(error != CL_SUCCESS, return error);
  cl_mem parent =
      (nullptr != memobj->optional_parent) ? memobj->optional_parent : memobj;

  char *base_pointer = static_cast<char *>(parent->map_base_pointer);
  if (CL_MEM_USE_HOST_PTR & parent->flags) {
    // When CL_MEM_USE_HOST_PTR is set then our map entry point returns a
    // pointer value derived from the user provided host_ptr.
    base_pointer = static_cast<char *>(parent->host_ptr);
  }

  // memobj hasn't been previously mapped
  OCL_CHECK(!base_pointer, return CL_INVALID_VALUE);

  OCL_CHECK((reinterpret_cast<char *>(mapped_ptr) < base_pointer) ||
                (reinterpret_cast<char *>(mapped_ptr) >=
                 (base_pointer + parent->size)),
            return CL_INVALID_VALUE);

  cl_event return_event = nullptr;
  if (nullptr != event) {
    auto new_event =
        _cl_event::create(command_queue, CL_COMMAND_UNMAP_MEM_OBJECT);
    if (!new_event) {
      return new_event.error();
    }
    return_event = *new_event;
    *event = return_event;
  }

  cl::retainInternal(memobj);
  cl::release_guard<cl_mem> mem_release_guard(memobj,
                                              cl::ref_count_type::INTERNAL);

  // If this is a write command then set it to the inactive state, meaning
  // subsequent map commands can overlap this region.
  auto it = memobj->write_mappings.find(mapped_ptr);
  if (it != std::end(memobj->write_mappings) && it->second.is_active) {
    it->second.is_active = false;
  }

  const std::lock_guard<std::mutex> lock(
      command_queue->context->getCommandQueueMutex());

  auto mux_command_buffer = command_queue->getCommandBuffer(
      {event_wait_list, num_events_in_wait_list}, return_event);
  if (!mux_command_buffer) {
    return CL_OUT_OF_RESOURCES;
  }

  struct unmap_info_t {
    unmap_info_t(cl_mem mem, void *ptr, cl_uint device_index)
        : mem(mem), ptr(ptr), device_index(device_index) {}

    cl_mem mem;
    void *ptr;
    cl_uint device_index;
  } *unmap_info = new unmap_info_t(
      memobj, mapped_ptr,
      command_queue->context->getDeviceIndex(command_queue->device));

  auto mux_error = muxCommandUserCallback(
      *mux_command_buffer,
      [](mux_queue_t, mux_command_buffer_t, void *const user_data) {
        auto unmap_info = static_cast<const unmap_info_t *>(user_data);
        auto mem = unmap_info->mem;

        // if this is a sub-buffer the memory is mapped in the parent
        if (nullptr != unmap_info->mem->optional_parent) {
          mem = mem->optional_parent;
        }

        // get the device and the memory
        auto mux_device =
            mem->context->devices[unmap_info->device_index]->mux_device;
        mux_memory_t mux_memory = mem->mux_memories[unmap_info->device_index];

        {
          const std::lock_guard<std::mutex> lock(mem->mutex);

          auto it = mem->write_mappings.find(unmap_info->ptr);
          // if this is a write mapping we need to flush the memory region to
          // the device
          if (mem->write_mappings.end() != it) {
            _cl_mem::mapping const &map = it->second;

            if (CL_MEM_USE_HOST_PTR & mem->flags) {
              // Copy data from `host_ptr` user has accessed/modified to our
              // cache of the data in `map_base_pointer`.
              std::memcpy(
                  static_cast<char *>(mem->map_base_pointer) + map.offset,
                  static_cast<char *>(mem->host_ptr) + map.offset, map.size);
            }

            // flush the memory region back to the device if required
            auto mux_error = muxFlushMappedMemoryToDevice(
                mux_device, mux_memory, map.offset, map.size);
            OCL_ASSERT(!mux_error, "muxFlushMappedMemoryToDevice failed!");
            OCL_UNUSED(mux_error);

            // then remove the mapping from the saved ones
            mem->write_mappings.erase(unmap_info->ptr);
          }

          if (1 == mem->mapCount) {
            // if we're the last mapping we can actually unmap the memory
            auto mux_error = muxUnmapMemory(mux_device, mux_memory);
            OCL_ASSERT(!mux_error, "muxUnmapMemory failed!");
            OCL_UNUSED(mux_error);
            mem->map_base_pointer = nullptr;
          }

          // decrement the mapCount
          mem->mapCount--;

          delete unmap_info;
        }
      },
      unmap_info, 0, nullptr, nullptr);
  if (mux_error) {
    return cl::getErrorFrom(mux_error);
  }

  mem_release_guard.dismiss();
  return command_queue->registerDispatchCallback(
      *mux_command_buffer, return_event,
      [memobj]() { cl::releaseInternal(memobj); });
}

CL_API_ENTRY cl_int CL_API_CALL cl::EnqueueMigrateMemObjects(
    cl_command_queue queue, cl_uint num_mem_objects, const cl_mem *mem_objects,
    cl_mem_migration_flags flags, cl_uint num_events, const cl_event *events,
    cl_event *event) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clEnqueueMigrateMemObjects");
  OCL_CHECK(!queue, return CL_INVALID_COMMAND_QUEUE);

  OCL_CHECK((0 == num_mem_objects) && mem_objects, return CL_INVALID_VALUE);
  OCL_CHECK((0 < num_mem_objects) && !mem_objects, return CL_INVALID_VALUE);

  for (cl_uint i = 0; i < num_mem_objects; i++) {
    OCL_CHECK(!(mem_objects[i]), return CL_INVALID_MEM_OBJECT);
    OCL_CHECK(queue->context != mem_objects[i]->context,
              return CL_INVALID_CONTEXT);
  }

  OCL_CHECK(
      ~(CL_MIGRATE_MEM_OBJECT_HOST | CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED) &
          flags,
      return CL_INVALID_VALUE);

  const cl_int error =
      cl::validate::EventWaitList(num_events, events, queue->context, event);
  OCL_CHECK(error != CL_SUCCESS, return error);

  // We only really support one device type, with mapped main memory, so
  // migration costs us nothing!

  if (event) {
    const cl_int errorcode =
        cl::EnqueueMarkerWithWaitList(queue, num_events, events, event);
    OCL_CHECK(CL_SUCCESS != errorcode, return errorcode);
    // Make sure that if an event was provided it has a correct command type
    // set.
    static_cast<_cl_event *>(*event)->command_type =
        CL_COMMAND_MIGRATE_MEM_OBJECTS;
  }

  return CL_SUCCESS;
}
