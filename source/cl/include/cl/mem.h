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

/// @file
///
/// @brief Definition of the OpenCL memory object.

#ifndef CL_MEM_H_INCLUDED
#define CL_MEM_H_INCLUDED

#include <cargo/array_view.h>
#include <cargo/dynamic_array.h>
#include <cargo/small_vector.h>
#include <cl/base.h>
#include <cl/config.h>
#include <cl/context.h>
#include <cl/limits.h>
#include <mux/mux.h>

#include <unordered_map>

#include "CL/cl.h"

namespace cl {
/// @addtogroup cl
/// @{

/// @brief Memory object destructor callback function pointer definition.
///
/// @param memobj Memory object the callback belongs to.
/// @param user_data User data to pass to the callback function.
using pfn_notify_mem_t = void(CL_CALLBACK *)(cl_mem memobj, void *user_data);

/// @}
}  // namespace cl

/// @addtogroup cl
/// @{

/// @brief Definitions of OpenCL memory object API.
///
/// The `cl_mem` type is a handle to a "Memory Object" (as described in (Section
/// 3.5 of the OpenCL 1.1 Spec). `cl_mem` is a number (like a file handler for
/// Linux) that is reserved for the use as a "memory identifier" (the API/driver
/// stores information about your memory under this number so that it knows what
/// it holds/how big it is etc). Essentially are inputs and outputs for OpenCL
/// kernels, and are returned from OpenCL API calls in host code such as
/// `clCreateBuffer`.
struct _cl_mem : public cl::base<_cl_mem> {
  /// @brief Memory object constructor.
  ///
  /// @param[in] context Context the memory object belongs to.
  /// @param[in] flags Memory allocation flags.
  /// @param[in] size Size in bytes of the requested device allocation.
  /// @param[in] type Type of the memory object.
  /// @param[in] optional_parent Optional parent memory object of this sub
  /// buffer.
  /// @param[in] host_ptr Pointer to optionally user provided host memory.
  /// @param[in] ref_count_init_type Reference counting type, internal/external.
  /// @param[in] mux_memories Mux device memory objects.
  _cl_mem(const cl_context context, const cl_mem_flags flags, const size_t size,
          const cl_mem_object_type type, cl_mem optional_parent, void *host_ptr,
          const cl::ref_count_type ref_count_init_type,
          cargo::dynamic_array<mux_memory_t> &&mux_memories);

  /// @brief Destructor.
  ~_cl_mem();

  /// @brief Register a destructor callback function to the memory object.
  ///
  /// @param[in] pfn_notify Destructor callback function pointer.
  /// @param[in] user_data User data to be passed to the callback function.
  ///
  /// @return Return true on success, false otherwise.
  bool registerCallback(cl::pfn_notify_mem_t pfn_notify, void *user_data);

  /// @brief Allocate device memory is **required**.
  ///
  /// Before calling this function the `::_cl_mem` object will not have *any*
  /// physical device memory associated with it. After this function succeeds
  /// there *will* physical device memory but inherited `::_cl_mem_buffer` or
  /// `::_cl_mem_image` **will not** be bound to this memory.
  ///
  /// @param[in] mux_device Mux device to allocate memory on.
  /// @param[in] supported_heaps Supported heaps to allocate from.
  /// @param[in] mux_allocator Allocator used for memory management.
  /// @param[out] out_memory The allocated device memory.
  ///
  /// @return Returns `CL_SUCCESS` on success, or
  /// `CL_MEM_OBJECT_ALLOCATION_FAILURE` on failure.
  cl_int allocateMemory(mux_device_t mux_device, uint32_t supported_heaps,
                        mux_allocator_info_t mux_allocator,
                        mux_memory_t *out_memory);

  /// @brief Push a map command to the queue.
  ///
  /// @param[in] command_queue OpenCL command queue to enqueue on.
  /// @param[out] mappedPointer Pointer that must be filled with a pointer to
  /// the mapped region when this function returns. returned to the user. Can be
  /// a nullptr if an event is not necessary or requested for this command.
  /// @param[in] offset Offset in bytes into the memory to map.
  /// @param[in] size Size in bytes of the memory to map.
  /// @param[in] read Perform a read during mapping.
  /// @param[in] write Perform a write during mapping.
  /// @param[in] invalidate Perform a invalidating write during mapping.
  /// @param[in] event_wait_list List of events to wait for.
  /// @param[out] return_event Event associated with this command that will be
  cl_int pushMapMemory(cl_command_queue command_queue, void **mappedPointer,
                       size_t offset, size_t size, bool read, bool write,
                       bool invalidate,
                       cargo::array_view<const cl_event> event_wait_list,
                       cl_event return_event);

  /// @brief Checks whether a mapping will overlap any already existing mapping.
  bool overlaps(size_t offset, size_t size);

  /// @brief Context the memory object belongs to.
  const cl_context context;
  /// @brief Memory allocation flags.
  const cl_mem_flags flags;
  /// @brief Size in bytes of the requested device allocation.
  size_t size;
  /// @brief Type of the memory object.
  const cl_mem_object_type type;
  /// @brief Optional parent memory object of this sub buffer.
  const cl_mem optional_parent;
  // @brief Pointer to optionally user provided host memory.
  void *host_ptr;

  /// @brief List of mux memory objects, the physical device memory allocation.
  cargo::dynamic_array<mux_memory_t> mux_memories;

  // TODO: redmine(7053) _cl_event makes use of a callback container which
  // contains the function pointer and the user data pointer which allows it to
  // store these in a single buffer. Should there be a utility callback
  // container?
  /// @brief Buffer of registered memory object destructor callbacks.
  cargo::small_vector<cl::pfn_notify_mem_t, 4> callbacks;
  /// @brief Buffer of user data pointers for callbacks.
  cargo::small_vector<void *, 4> callback_datas;

  /// @brief Mutex to lock access to the map count, the mapped base pointer, and
  /// the active write mappings.
  std::mutex mutex;

  /// @brief Count of the times this memory object has been mapped this count
  /// includes mapping on sub-buffers, and will be 0 for sub-buffers.
  cl_uint mapCount;

  /// @brief Base pointer for mappings associated with this `cl_mem`, for
  /// sub-buffers this will always be a nullptr, the map_base_pointer of the
  /// parent should be used.
  void *map_base_pointer;

  /// @brief Struct representing a mapping
  struct mapping final {
    /// @brief Absolute offset of the mapping in the buffer.
    cl_uint offset;

    /// @brief Size of the mapping.
    cl_uint size;

    /// @brief Flag indicating whether this mapping is currently "active".
    ///
    /// Active mappings have yet to be unmapped via clEnqueueUnmapMemObject,
    /// meaning that subsequent calls to clEnqueueMapBuffer need to check for
    /// overlap.
    ///
    /// This flag is required because in the sequence of commands
    /// write_map->unmap->write_map, overlapping regions may be
    /// valid, but since we don't actually dispatch any mux commands until there
    /// is a flush, we need to keep track of whether a write region will be
    /// mapped or unmapped in order to do error checking.
    ///
    /// Because we currently only support in order queues we can toggle this
    /// flag in the clEnqueueMapBuffer/clEnqueueUnmapMemObject entry points,
    /// rather than in the callbacks passed to muxCommandUserCallback and the
    /// ordering of maps/unmaps will be correct.
    bool is_active = true;
  };

#if defined(CL_VERSION_3_0)
  /// @brief Array of the buffers associated properties and their respective
  /// values.
  cargo::dynamic_array<cl_mem_properties> properties;
#endif

  /// @brief Map storing all the active write mappings on this `cl_mem`, this
  /// includes write mappings on sub-buffers and will be empty on sub-buffers.
  std::unordered_map<void *, mapping> write_mappings;
  /// @brief Device which owns the most up to date version of the data.
  cl_device_id device_owner;
#if defined(CL_VERSION_3_0)
  /// @brief CL_FALSE if no devices in the context associated with memobj
  /// support Shared Virtual Memory.
  cl_bool uses_svm_pointer;
#endif
};

/// @}

namespace cl {
/// @addtogroup cl
/// @{

/// @brief Increment the memory object references count.
///
/// @param[in] memobj Memory object to increment reference count on.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clRetainMemObject.html
CL_API_ENTRY cl_int CL_API_CALL RetainMemObject(cl_mem memobj);

/// @brief Decrement the memory objects reference count.
///
/// @param[in] memobj Memory object to decrement reference count on.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clReleaseMemObject.html
CL_API_ENTRY cl_int CL_API_CALL ReleaseMemObject(cl_mem memobj);

/// @brief Register a callback to notify when the memory object is destroyed.
///
/// @param[in] memobj Memory object to register callback on.
/// @param[in] pfn_notify Callback function to invoke when the destructor is
/// triggered.
/// @param[in] user_data User data to be passed to the callback function.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clSetMemObjectDestructorCallback.html
CL_API_ENTRY cl_int CL_API_CALL SetMemObjectDestructorCallback(
    cl_mem memobj, cl::pfn_notify_mem_t pfn_notify, void *user_data);

/// @brief Query the memory object for information.
///
/// @param[in] memobj Memory object to query for information.
/// @param[in] param_name Type of information to query.
/// @param[in] param_value_size Size in bytes of @a param_value storage.
/// @param[in] param_value Pointer to value to store query in.
/// @param[out] param_value_size_ret Return size in bytes the query requires if
/// not null.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clGetMemObjectInfo.html
CL_API_ENTRY cl_int CL_API_CALL GetMemObjectInfo(cl_mem memobj,
                                                 cl_mem_info param_name,
                                                 size_t param_value_size,
                                                 void *param_value,
                                                 size_t *param_value_size_ret);

/// @brief Enqueue a memory object unmap to the queue.
///
/// @param[in] command_queue Command queue to enqueue the unmap on.
/// @param[in] memobj Memory object to unmap.
/// @param[in] mapped_ptr Pointer to beginning of mapped memory.
/// @param[in] num_events_in_wait_list Number of events in list to wait for.
/// @param[in] event_wait_list List of events to wait for.
/// @param[out] event Return event if not null.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clEnqueueUnmapMemObject.html
CL_API_ENTRY cl_int CL_API_CALL
EnqueueUnmapMemObject(cl_command_queue command_queue, cl_mem memobj,
                      void *mapped_ptr, cl_uint num_events_in_wait_list,
                      const cl_event *event_wait_list, cl_event *event);

/// @brief Enqueue a memory object migration on the queue.
///
/// @param[in] queue Command queue to enqueue the migration on.
/// @param[in] num_mem_objects Number of memory objects in list to migrate.
/// @param[in] mem_objects List of memory objects to migrate.
/// @param[in] flags Type of migration, bit-field.
/// @param[in] num_events Number of events in list to wait for.
/// @param[in] events List of events to wait for.
/// @param[out] event Return event in not null.
///
/// @return Return error code.
///
/// @see
/// http://www.khronos.org/registry/cl/sdk/1.2/docs/man/xhtml/clEnqueueMigrateMemObjects.html
CL_API_ENTRY cl_int CL_API_CALL EnqueueMigrateMemObjects(
    cl_command_queue queue, cl_uint num_mem_objects, const cl_mem *mem_objects,
    cl_mem_migration_flags flags, cl_uint num_events, const cl_event *events,
    cl_event *event);

/// @}
}  // namespace cl

#endif  // CL_MEM_H_INCLUDED
