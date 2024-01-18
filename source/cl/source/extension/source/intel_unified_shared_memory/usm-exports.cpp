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

#include <extension/intel_unified_shared_memory.h>

#ifdef OCL_EXTENSION_cl_intel_unified_shared_memory
#include <cl/command_queue.h>
#include <cl/context.h>
#include <cl/device.h>
#include <cl/kernel.h>
#include <cl/mux.h>
#include <cl/program.h>
#include <cl/validate.h>
#include <tracer/tracer.h>

#include <cstring>
#include <unordered_set>

namespace {
// Class encapsulating details of user data passed as an argument to a USM
// copy or fill command. This data needs to be bound to a Mux buffer on the
// device which is then freed when the command has completed.
struct UserDataWrapper final {
  using WrapperOrMuxError = cargo::expected<UserDataWrapper *, mux_result_t>;
  // Heap allocate an instance of the class and setup members, returning the
  // initialized instance or an error on failure.
  static WrapperOrMuxError create(cl_device_id device, const size_t size,
                                  void *host_ptr = nullptr) {
    auto wrapper = new UserDataWrapper(device, size, host_ptr);
    OCL_CHECK(nullptr == wrapper,
              return cargo::make_unexpected(mux_error_out_of_memory));

    if (auto mux_error = wrapper->initalize()) {
      delete wrapper;
      return cargo::make_unexpected(mux_error);
    }
    return WrapperOrMuxError(wrapper);
  }

  // Write data from host pointer argument to device memory
  mux_result_t writeToDevice(const void *host_ptr) {
    void *writeTo = nullptr;
    auto mux_error = muxMapMemory(mux_device, mux_memory, 0, size, &writeTo);
    OCL_CHECK(mux_error, return mux_error);

    std::memcpy(writeTo, host_ptr, size);

    mux_error = muxFlushMappedMemoryToDevice(mux_device, mux_memory, 0, size);
    OCL_CHECK(mux_error, return mux_error);

    mux_error = muxUnmapMemory(mux_device, mux_memory);
    return mux_error ? mux_error : mux_success;
  }

  // Read from device memory into host pointer set on class construction
  mux_result_t readFromDevice() {
    assert(host_read_ptr != nullptr);

    void *read_from = nullptr;
    auto mux_error = muxMapMemory(mux_device, mux_memory, 0, size, &read_from);
    OCL_CHECK(mux_error, return mux_error);

    mux_error = muxFlushMappedMemoryFromDevice(mux_device, mux_memory, 0, size);
    OCL_CHECK(mux_error, return mux_error);

    std::memcpy(host_read_ptr, read_from, size);

    mux_error = muxUnmapMemory(mux_device, mux_memory);
    return mux_error ? mux_error : mux_success;
  }

  // Free device allocated memory
  ~UserDataWrapper() {
    if (mux_memory) {
      muxFreeMemory(mux_device, mux_memory, mux_allocator);
    }

    if (mux_buffer) {
      muxDestroyBuffer(mux_device, mux_buffer, mux_allocator);
    }
  }

 private:
  // Constructor setting members for allocating `size` bytes on device `device`,
  // setting an optional host side pointer to copy data into when reading the
  // buffer back from device
  UserDataWrapper(cl_device_id device, const size_t size,
                  void *host_ptr = nullptr)
      : mux_device(device->mux_device),
        mux_allocator(device->mux_allocator),
        mux_memory(nullptr),
        mux_buffer(nullptr),
        size(size),
        host_read_ptr(host_ptr) {}

  // Disable copy constructor & operator, otherwise destructor could free
  // already released Mux memory objects.
  UserDataWrapper(const UserDataWrapper &) = delete;
  UserDataWrapper &operator=(const UserDataWrapper &) = delete;

  // Allocate mappable memory on device and bind it to Mux buffer construct
  mux_result_t initalize() {
    const uint32_t memoryProperties =
        mux_memory_property_host_cached | mux_memory_property_host_visible;
    const mux_allocation_type_e allocationType = mux_allocation_type_alloc_host;
    const uint32_t alignment = 0;  // No alignment preference
    mux_result_t mux_error =
        muxAllocateMemory(mux_device, size, 1, memoryProperties, allocationType,
                          alignment, mux_allocator, &mux_memory);
    OCL_CHECK(mux_error, return mux_error);

    // Initialize the Mux objects needed by each device
    mux_error = muxCreateBuffer(mux_device, size, mux_allocator, &mux_buffer);
    OCL_CHECK(mux_error, return mux_error);

    mux_error = muxBindBufferMemory(mux_device, mux_memory, mux_buffer, 0);
    return mux_error ? mux_error : mux_success;
  }

 public:
  mux_device_t mux_device;
  mux_allocator_info_t mux_allocator;
  mux_memory_t mux_memory;
  mux_buffer_t mux_buffer;
  const size_t size;
  void *host_read_ptr;
};

// Helper function which given a USM allocation records the cl_event
// associated with copy command, and sets an output parameter for the Mux
// buffer tied to the USM allocation.
mux_result_t ExamineUSMAlloc(extension::usm::allocation_info *usm_alloc,
                             const cl_device_id queue_device,
                             const cl_event return_event,
                             mux_buffer_t &mux_buffer) {
  // Host USM allocations aren't tied to a single device, use the
  // mux_buffer_t associated with the device tied to the command queue
  cl_device_id device = usm_alloc->getDevice();
  if (nullptr == device) {
    device = queue_device;
  }

  // Set function output parameter mux_buffer_t to use in muxCommandCopyBuffer
  mux_buffer = usm_alloc->getMuxBufferForDevice(device);

  // Record the event associated with this enqueue command so that we
  // can wait on it in blocking free USM calls.
  return usm_alloc->record_event(return_event);
};

// Calculates the byte offset between a pointer and start of USM memory
// allocation
inline uint64_t getUSMOffset(const void *ptr,
                             const extension::usm::allocation_info *usm_alloc) {
  auto offset = reinterpret_cast<uintptr_t>(ptr) -
                reinterpret_cast<uintptr_t>(usm_alloc->base_ptr);
  return static_cast<uint64_t>(offset);
}
}  // namespace

CL_API_ENTRY
void *clHostMemAllocINTEL(cl_context context,
                          const cl_mem_properties_intel *properties,
                          size_t size, cl_uint alignment, cl_int *errcode_ret) {
  const tracer::TraceGuard<tracer::OpenCL> trace(__func__);

  OCL_CHECK(!context, OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_CONTEXT);
            return nullptr);

  const bool no_host_support = std::none_of(
      context->devices.cbegin(), context->devices.cend(),
      [](cl_device_id device) {
        return extension::usm::deviceSupportsHostAllocations(device);
      });

  OCL_CHECK(no_host_support,
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_OPERATION);
            return nullptr);

  auto new_usm_allocation = extension::usm::host_allocation_info::create(
      context, properties, size, alignment);

  if (!new_usm_allocation) {
    OCL_SET_IF_NOT_NULL(errcode_ret, new_usm_allocation.error());
    return nullptr;
  }

  // Lock context for pushing to list of usm allocations
  const std::lock_guard<std::mutex> context_guard(context->usm_mutex);
  if (context->usm_allocations.push_back(
          std::move(new_usm_allocation.value()))) {
    OCL_SET_IF_NOT_NULL(errcode_ret, CL_OUT_OF_HOST_MEMORY);
    return nullptr;
  }

  OCL_SET_IF_NOT_NULL(errcode_ret, CL_SUCCESS);
  return context->usm_allocations.back()->base_ptr;
}

CL_API_ENTRY
void *clDeviceMemAllocINTEL(cl_context context, cl_device_id device,
                            const cl_mem_properties_intel *properties,
                            size_t size, cl_uint alignment,
                            cl_int *errcode_ret) {
  const tracer::TraceGuard<tracer::OpenCL> trace(__func__);
  OCL_CHECK(!context, OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_CONTEXT);
            return nullptr);

  OCL_CHECK(!device || !context->hasDevice(device),
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_DEVICE);
            return nullptr);

  auto new_usm_allocation = extension::usm::device_allocation_info::create(
      context, device, properties, size, alignment);

  if (!new_usm_allocation) {
    OCL_SET_IF_NOT_NULL(errcode_ret, new_usm_allocation.error());
    return nullptr;
  }

  // Lock context for pushing to list of usm allocations
  const std::lock_guard<std::mutex> context_guard(context->usm_mutex);
  if (context->usm_allocations.push_back(
          std::move(new_usm_allocation.value()))) {
    OCL_SET_IF_NOT_NULL(errcode_ret, CL_OUT_OF_HOST_MEMORY);
    return nullptr;
  }
  OCL_SET_IF_NOT_NULL(errcode_ret, CL_SUCCESS);
  return context->usm_allocations.back()->base_ptr;
}

CL_API_ENTRY
void *clSharedMemAllocINTEL(cl_context context, cl_device_id device,
                            const cl_mem_properties_intel *properties,
                            size_t size, cl_uint alignment,
                            cl_int *errcode_ret) {
  const tracer::TraceGuard<tracer::OpenCL> trace(__func__);

  OCL_CHECK(!context, OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_CONTEXT);
            return nullptr);

  OCL_CHECK(device && !context->hasDevice(device),
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_DEVICE);
            return nullptr);

  if (device) {
    if (!extension::usm::deviceSupportsSharedAllocations(device)) {
      OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_OPERATION);
      return nullptr;
    }
  } else {
    // If no device is given, we fail if no device in the context supports
    // shared USM
    const bool no_host_support = std::none_of(
        context->devices.cbegin(), context->devices.cend(),
        [](cl_device_id device) {
          return extension::usm::deviceSupportsSharedAllocations(device);
        });

    OCL_CHECK(no_host_support,
              OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_OPERATION);
              return nullptr);
  }

  auto new_usm_allocation = extension::usm::shared_allocation_info::create(
      context, device, properties, size, alignment);

  if (!new_usm_allocation) {
    OCL_SET_IF_NOT_NULL(errcode_ret, new_usm_allocation.error());
    return nullptr;
  }

  // Lock context for pushing to list of usm allocations
  const std::lock_guard<std::mutex> context_guard(context->usm_mutex);
  if (context->usm_allocations.push_back(
          std::move(new_usm_allocation.value()))) {
    OCL_SET_IF_NOT_NULL(errcode_ret, CL_OUT_OF_HOST_MEMORY);
    return nullptr;
  }
  OCL_SET_IF_NOT_NULL(errcode_ret, CL_SUCCESS);
  return context->usm_allocations.back()->base_ptr;
}

CL_API_ENTRY
cl_int clMemFreeINTEL(cl_context context, void *ptr) {
  const tracer::TraceGuard<tracer::OpenCL> trace(__func__);

  OCL_CHECK(!context, return CL_INVALID_CONTEXT);
  OCL_CHECK(ptr == NULL, return CL_SUCCESS);

  // Lock context to ensure usm allocation iterators are valid
  const std::lock_guard<std::mutex> context_guard(context->usm_mutex);

  auto isUsmPtr =
      [ptr](const std::unique_ptr<extension::usm::allocation_info> &usm_alloc) {
        return usm_alloc->base_ptr == ptr;
      };

  auto usm_alloc_iterator =
      std::find_if(context->usm_allocations.begin(),
                   context->usm_allocations.end(), isUsmPtr);

  if (context->usm_allocations.end() != usm_alloc_iterator) {
    // Remove now empty shared pointer from list
    context->usm_allocations.erase(usm_alloc_iterator);
  }

  return CL_SUCCESS;
}

CL_API_ENTRY
cl_int clMemBlockingFreeINTEL(cl_context context, void *ptr) {
  const tracer::TraceGuard<tracer::OpenCL> trace(__func__);

  OCL_CHECK(!context, return CL_INVALID_CONTEXT);
  OCL_CHECK(ptr == NULL, return CL_SUCCESS);

  // Lock context to ensure usm allocation iterators are valid
  const std::lock_guard<std::mutex> context_guard(context->usm_mutex);

  auto isUsmPtr =
      [ptr](const std::unique_ptr<extension::usm::allocation_info> &usm_alloc) {
        return usm_alloc->base_ptr == ptr;
      };

  auto usm_alloc_iterator =
      std::find_if(context->usm_allocations.begin(),
                   context->usm_allocations.end(), isUsmPtr);

  if (context->usm_allocations.end() == usm_alloc_iterator) {
    return CL_SUCCESS;
  }

  // Implicitly flush all the queues that the events belong to
  std::unordered_set<_cl_command_queue *> flushed_queues;
  auto &events = (*usm_alloc_iterator)->queued_commands;
  for (auto &event : events) {
    auto queue = event->queue;

    // we only want to flush queues in events that are queued.
    if (event->command_status == CL_QUEUED) {
      // Don't repeatedly flush queues we've already seen
      if (flushed_queues.count(queue) == 0) {
        const std::lock_guard<std::mutex> lock(
            queue->context->getCommandQueueMutex());

        const cl_int result = queue->flush();

        if (CL_SUCCESS != result) {
          return result;
        }

        flushed_queues.insert(queue);
      }
    }
  }

  // Wait on events separately rather than entire queue to avoid deadlock on
  // queue mutex
  for (auto &event : events) {
    // If a queue has been freed by a user, then dereferencing the `queue`
    // pointer here can lead to a segfault. Avoid this by checking if the event
    // we're waiting on is in-flight, meaning the `queue` it's associated with
    // should be valid.
    if (event->command_status > CL_COMPLETE) {
      event->queue->waitForEvents(1, &event);
    }
  }

  // Remove now empty unique pointer from list
  context->usm_allocations.erase(usm_alloc_iterator);
  return CL_SUCCESS;
}

CL_API_ENTRY
cl_int clGetMemAllocInfoINTEL(cl_context context, const void *ptr,
                              cl_mem_info_intel param_name,
                              size_t param_value_size, void *param_value,
                              size_t *param_value_size_ret) {
  const tracer::TraceGuard<tracer::OpenCL> trace(__func__);

  OCL_CHECK(!context, return CL_INVALID_CONTEXT);
  const std::lock_guard<std::mutex> context_guard(context->usm_mutex);

  const extension::usm::allocation_info *const usm_alloc =
      extension::usm::findAllocation(context, ptr);

  switch (param_name) {
    case CL_MEM_ALLOC_TYPE_INTEL: {
      cl_unified_shared_memory_type_intel result = CL_MEM_TYPE_UNKNOWN_INTEL;
      if (usm_alloc) {
        result = usm_alloc->getMemoryType();
      }

      if (nullptr != param_value) {
        OCL_CHECK(param_value_size < sizeof(result), return CL_INVALID_VALUE);
        *static_cast<decltype(result) *>(param_value) = result;
      }
      OCL_SET_IF_NOT_NULL(param_value_size_ret, sizeof(result));
      break;
    }
    case CL_MEM_ALLOC_BASE_PTR_INTEL: {
      void *result = usm_alloc ? usm_alloc->base_ptr : nullptr;
      if (nullptr != param_value) {
        OCL_CHECK(param_value_size < sizeof(result), return CL_INVALID_VALUE);
        *static_cast<decltype(result) *>(param_value) = result;
      }
      OCL_SET_IF_NOT_NULL(param_value_size_ret, sizeof(result));
      break;
    }
    case CL_MEM_ALLOC_SIZE_INTEL: {
      const size_t result = usm_alloc ? usm_alloc->size : 0;
      if (nullptr != param_value) {
        OCL_CHECK(param_value_size < sizeof(result), return CL_INVALID_VALUE);
        *static_cast<std::remove_cv_t<decltype(result)> *>(param_value) =
            result;
      }
      OCL_SET_IF_NOT_NULL(param_value_size_ret, sizeof(result));
      break;
    }
    case CL_MEM_ALLOC_DEVICE_INTEL: {
      cl_device_id result = usm_alloc ? usm_alloc->getDevice() : nullptr;
      if (nullptr != param_value) {
        OCL_CHECK(param_value_size < sizeof(result), return CL_INVALID_VALUE);
        *static_cast<decltype(result) *>(param_value) = result;
      }
      OCL_SET_IF_NOT_NULL(param_value_size_ret, sizeof(result));
      break;
    }
    case CL_MEM_ALLOC_FLAGS_INTEL: {
      const cl_mem_alloc_flags_intel result =
          usm_alloc ? usm_alloc->alloc_flags : 0;
      if (nullptr != param_value) {
        OCL_CHECK(param_value_size < sizeof(result), return CL_INVALID_VALUE);
        *static_cast<std::remove_cv_t<decltype(result)> *>(param_value) =
            result;
      }
      OCL_SET_IF_NOT_NULL(param_value_size_ret, sizeof(result));
      break;
    }
    default:
      return CL_INVALID_VALUE;
  }

  return CL_SUCCESS;
}

CL_API_ENTRY
cl_int clSetKernelArgMemPointerINTEL(cl_kernel kernel, cl_uint arg_index,
                                     const void *arg_value) {
  const tracer::TraceGuard<tracer::OpenCL> trace(__func__);

  OCL_CHECK(!kernel, return CL_INVALID_KERNEL);

  OCL_CHECK(arg_index >= kernel->info->getNumArguments(),
            return CL_INVALID_ARG_INDEX);

  auto arg_type = kernel->GetArgType(arg_index);
  OCL_CHECK(!arg_type, return arg_type.error());

  OCL_CHECK(arg_type->kind != compiler::ArgumentKind::POINTER,
            return CL_INVALID_ARG_VALUE);

  OCL_CHECK(!(arg_type->address_space == compiler::AddressSpace::GLOBAL ||
              arg_type->address_space == compiler::AddressSpace::CONSTANT),
            return CL_INVALID_ARG_VALUE);

  // The cl_intel_unified_shared_memory specification has an open question on
  // whether unknown pointers should be accepted. We accept them since the SYCL
  // specification and the SYCL CTS imply this must be treated as valid.
  kernel->saved_args[arg_index] =
      _cl_kernel::argument(*arg_type, &arg_value, sizeof(void *));

  return CL_SUCCESS;
}

namespace {
// Used to implement clEnqueueMemsetINTEL and clEnqueueMemFillINTEL
cl_int MemFillImpl(cl_command_queue command_queue, void *dst_ptr,
                   const void *pattern, size_t pattern_size, size_t size,
                   cl_uint num_events_in_wait_list,
                   const cl_event *event_wait_list, cl_event *event) {
  const cl_int error = cl::validate::EventWaitList(
      num_events_in_wait_list, event_wait_list, command_queue->context, event);
  OCL_CHECK(error != CL_SUCCESS, return error);

  auto new_event = _cl_event::create(command_queue, CL_COMMAND_MEMFILL_INTEL);
  if (!new_event) {
    return new_event.error();
  }
  cl_event return_event = *new_event;

  const std::lock_guard<std::mutex> context_guard(
      command_queue->context->usm_mutex);

  // Find USM allocation from pointer
  extension::usm::allocation_info *usm_alloc =
      extension::usm::findAllocation(command_queue->context, dst_ptr);

  cl::release_guard<cl_event> event_release_guard(return_event,
                                                  cl::ref_count_type::EXTERNAL);
  {
    mux_buffer_t mux_buffer = nullptr;
    const cl_device_id device = command_queue->device;

    uint64_t offset = 0;
    std::unique_ptr<UserDataWrapper> dst_user_data;
    // TODO CA-3084 Unresolved issue in extension doc whether fill on arbitrary
    // host pointer should be allowed.
    if (usm_alloc == nullptr) {
      // Source pointer is to arbitrary user data, heap allocate a wrapper class
      // so we can use Mux memory constructs to work with it.
      auto wrapper = UserDataWrapper::create(device, size, dst_ptr);
      OCL_CHECK(!wrapper, return CL_OUT_OF_RESOURCES);
      dst_user_data.reset(*wrapper);
      mux_buffer = dst_user_data->mux_buffer;
    } else {
      // Push Mux fill buffer operation
      auto mux_error =
          ExamineUSMAlloc(usm_alloc, device, return_event, mux_buffer);
      OCL_CHECK(mux_error, return CL_OUT_OF_RESOURCES);
      offset = getUSMOffset(dst_ptr, usm_alloc);
    }

    // TODO CA-2863 Define correct return code for this situation where device
    // USM allocation device is not the same as command queue device
    OCL_CHECK(mux_buffer == nullptr, return CL_INVALID_COMMAND_QUEUE);

    const std::lock_guard<std::mutex> lock(
        command_queue->context->getCommandQueueMutex());

    auto mux_command_buffer = command_queue->getCommandBuffer(
        {event_wait_list, num_events_in_wait_list}, return_event);
    OCL_CHECK(!mux_command_buffer, return CL_OUT_OF_RESOURCES);

    auto mux_error =
        muxCommandFillBuffer(*mux_command_buffer, mux_buffer, offset, size,
                             pattern, pattern_size, 0, nullptr, nullptr);
    if (mux_error) {
      auto error = cl::getErrorFrom(mux_error);
      if (return_event) {
        return_event->complete(error);
      }
      return error;
    }

    // If the destination operand was user data, we need to manually copy
    // the destination Mux buffer back to the user supplied `dst_ptr` by
    // mapping the buffer.
    if (dst_user_data) {
      mux_error = muxCommandUserCallback(
          *mux_command_buffer,
          [](mux_queue_t, mux_command_buffer_t, void *user_data) {
            static_cast<UserDataWrapper *>(user_data)->readFromDevice();
          },
          dst_user_data.get(), 0, nullptr, nullptr);

      if (mux_error) {
        auto error = cl::getErrorFrom(mux_error);
        if (return_event) {
          return_event->complete(error);
        }
        return error;
      }
    }

    // UserDataWrapper objects used to encapsulate user pointer operands are
    // heap allocated. Free them once the command has completed.
    auto raw_dst_user_data = dst_user_data.release();
    if (auto error = command_queue->registerDispatchCallback(
            *mux_command_buffer, return_event, [raw_dst_user_data]() {
              if (raw_dst_user_data) {
                delete raw_dst_user_data;
              }
            })) {
      return error;
    }
  }

  if (nullptr != event) {
    *event = event_release_guard.dismiss();
  }

  return CL_SUCCESS;
}
}  // namespace

CL_API_ENTRY
cl_int clEnqueueMemFillINTEL(cl_command_queue command_queue, void *dst_ptr,
                             const void *pattern, size_t pattern_size,
                             size_t size, cl_uint num_events_in_wait_list,
                             const cl_event *event_wait_list, cl_event *event) {
  const tracer::TraceGuard<tracer::OpenCL> trace(__func__);

  OCL_CHECK(!command_queue, return CL_INVALID_COMMAND_QUEUE);
  OCL_CHECK(!dst_ptr, return CL_INVALID_VALUE);
  OCL_CHECK(!pattern, return CL_INVALID_VALUE);

  OCL_CHECK((pattern_size & (pattern_size - 1)) != 0, return CL_INVALID_VALUE);
  // dst_ptr must be aligned to pattern_size bytes
  OCL_CHECK(!(reinterpret_cast<uintptr_t>(dst_ptr) % pattern_size == 0),
            return CL_INVALID_VALUE);
  OCL_CHECK(size && (size % pattern_size), return CL_INVALID_VALUE);

  const size_t largest_data_type_size =
      command_queue->device->min_data_type_align_size;
  OCL_CHECK(pattern_size > largest_data_type_size, return CL_INVALID_VALUE);

  return MemFillImpl(command_queue, dst_ptr, pattern, pattern_size, size,
                     num_events_in_wait_list, event_wait_list, event);
}

// Deprecated entry-point not defined in the spec, but included in the
// extension header, therefore behaviour is inferred.
CL_API_ENTRY
cl_int clEnqueueMemsetINTEL(cl_command_queue command_queue, void *dst_ptr,
                            cl_int value, size_t size,
                            cl_uint num_events_in_wait_list,
                            const cl_event *event_wait_list, cl_event *event) {
  const tracer::TraceGuard<tracer::OpenCL> trace(__func__);

  OCL_CHECK(!command_queue, return CL_INVALID_COMMAND_QUEUE);
  OCL_CHECK(!dst_ptr, return CL_INVALID_VALUE);
  OCL_CHECK(size && (size % sizeof(cl_int)), return CL_INVALID_VALUE);

  return MemFillImpl(command_queue, dst_ptr, &value, sizeof(value), size,
                     num_events_in_wait_list, event_wait_list, event);
}

CL_API_ENTRY
cl_int clEnqueueMemcpyINTEL(cl_command_queue command_queue, cl_bool blocking,
                            void *dst_ptr, const void *src_ptr, size_t size,
                            cl_uint num_events_in_wait_list,
                            const cl_event *event_wait_list, cl_event *event) {
  const tracer::TraceGuard<tracer::OpenCL> trace(__func__);

  OCL_CHECK(!command_queue, return CL_INVALID_COMMAND_QUEUE);
  OCL_CHECK(!dst_ptr || !src_ptr, return CL_INVALID_VALUE);

  auto dst_addr = reinterpret_cast<uintptr_t>(dst_ptr);
  auto src_addr = reinterpret_cast<uintptr_t>(src_ptr);
  if (dst_addr < src_addr) {
    OCL_CHECK((dst_addr + size) > src_addr, return CL_MEM_COPY_OVERLAP);
  } else {
    OCL_CHECK((src_addr + size) > dst_addr, return CL_MEM_COPY_OVERLAP);
  }

  const cl_int error =
      cl::validate::EventWaitList(num_events_in_wait_list, event_wait_list,
                                  command_queue->context, event, blocking);
  OCL_CHECK(error != CL_SUCCESS, return error);

  auto new_event = _cl_event::create(command_queue, CL_COMMAND_MEMCPY_INTEL);
  if (!new_event) {
    return new_event.error();
  }
  cl_event return_event = *new_event;
  const std::lock_guard<std::mutex> context_guard(
      command_queue->context->usm_mutex);
  cl::release_guard<cl_event> event_release_guard(return_event,
                                                  cl::ref_count_type::EXTERNAL);
  {
    // Find destination USM allocation
    extension::usm::allocation_info *usm_dst_alloc =
        extension::usm::findAllocation(command_queue->context, dst_ptr);

    // Find source USM allocation
    extension::usm::allocation_info *usm_src_alloc =
        extension::usm::findAllocation(command_queue->context, src_ptr);

    const std::lock_guard<std::mutex> lock(
        command_queue->context->getCommandQueueMutex());

    auto mux_command_buffer = command_queue->getCommandBuffer(
        {event_wait_list, num_events_in_wait_list}, return_event);
    if (!mux_command_buffer) {
      return CL_OUT_OF_RESOURCES;
    }

    cl_device_id queue_device = command_queue->device;

    // Set details relating to source operand of copy
    mux_buffer_t mux_src_buffer = nullptr;  // Mux buffer to copy from
    uint64_t src_offset = 0;                // Offset into mux_src_buffer
    // Holds details of user data, initialized to heap memory if source operand
    // is not a USM allocation
    std::unique_ptr<UserDataWrapper> src_user_data;

    if (usm_src_alloc == nullptr) {
      // Source pointer is to arbitrary user data, heap allocate a wrapper class
      // so we can use Mux memory constructs to work with it.
      auto wrapper = UserDataWrapper::create(queue_device, size);
      OCL_CHECK(!wrapper, return CL_OUT_OF_RESOURCES);
      src_user_data.reset(*wrapper);
      mux_src_buffer = src_user_data->mux_buffer;

      // Copy the data from src_ptr to the Mux device
      auto mux_error = src_user_data->writeToDevice(src_ptr);
      OCL_CHECK(mux_error, return CL_OUT_OF_RESOURCES);
    } else {
      // Read details from source operand USM allocation into variables needed
      // to call muxCommandCopyBuffer.
      auto mux_error = ExamineUSMAlloc(usm_src_alloc, queue_device,
                                       return_event, mux_src_buffer);
      OCL_CHECK(mux_error, return CL_OUT_OF_RESOURCES);
      src_offset = getUSMOffset(src_ptr, usm_src_alloc);
    }

    // Set details relating to destination operand of copy
    mux_buffer_t mux_dst_buffer = nullptr;  // Mux buffer to copy into
    uint64_t dst_offset = 0;                // Offset into mux_dst_buffer
    // Holds details of user data, initialized to heap memory if destination
    // operand is not a USM allocation
    std::unique_ptr<UserDataWrapper> dst_user_data;
    if (usm_dst_alloc == nullptr) {
      // Destination pointer is to arbitrary user data, heap allocate a wrapper
      // class so we can use Mux memory constructs to work with it.
      auto wrapper = UserDataWrapper::create(queue_device, size, dst_ptr);
      OCL_CHECK(!wrapper, return CL_OUT_OF_RESOURCES);
      dst_user_data.reset(*wrapper);
      mux_dst_buffer = dst_user_data->mux_buffer;
    } else {
      // Read details from source operand USM allocation into variables needed
      // to call muxCommandCopyBuffer.
      auto mux_error = ExamineUSMAlloc(usm_dst_alloc, queue_device,
                                       return_event, mux_dst_buffer);
      OCL_CHECK(mux_error, return CL_OUT_OF_RESOURCES);
      dst_offset = getUSMOffset(dst_ptr, usm_dst_alloc);
    }

    auto mux_error = muxCommandCopyBuffer(
        *mux_command_buffer, mux_src_buffer, src_offset, mux_dst_buffer,
        dst_offset, size, 0, nullptr, nullptr);
    if (mux_error) {
      auto error = cl::getErrorFrom(mux_error);
      if (return_event) {
        return_event->complete(error);
      }
      return error;
    }

    // If the destination operand was user data, we need to manually copy
    // the destination Mux buffer back to the user supplied `dst_ptr` by
    // mapping the buffer.
    if (dst_user_data) {
      mux_error = muxCommandUserCallback(
          *mux_command_buffer,
          [](mux_queue_t, mux_command_buffer_t, void *user_data) {
            static_cast<UserDataWrapper *>(user_data)->readFromDevice();
          },
          dst_user_data.get(), 0, nullptr, nullptr);

      if (mux_error) {
        auto error = cl::getErrorFrom(mux_error);
        if (return_event) {
          return_event->complete(error);
        }
        return error;
      }
    }

    // UserDataWrapper objects used to encapsulate user pointer operands are
    // heap allocated. Free them once the command has completed.
    auto raw_src_user_data = src_user_data.release();
    auto raw_dst_user_data = dst_user_data.release();
    if (auto error = command_queue->registerDispatchCallback(
            *mux_command_buffer, return_event,
            [raw_src_user_data, raw_dst_user_data]() {
              if (raw_src_user_data) {
                delete raw_src_user_data;
              }

              if (raw_dst_user_data) {
                delete raw_dst_user_data;
              }
            })) {
      return error;
    }
  }

  if (blocking) {
    const cl_int ret = cl::WaitForEvents(1, &event_release_guard.get());
    if (CL_SUCCESS != ret) {
      return ret;
    }
  }

  if (nullptr != event) {
    *event = event_release_guard.dismiss();
  }

  return CL_SUCCESS;
}

CL_API_ENTRY
cl_int clEnqueueMigrateMemINTEL(cl_command_queue command_queue, const void *ptr,
                                size_t size, cl_mem_migration_flags flags,
                                cl_uint num_events_in_wait_list,
                                const cl_event *event_wait_list,
                                cl_event *event) {
  const tracer::TraceGuard<tracer::OpenCL> trace(__func__);

  OCL_CHECK(!command_queue, return CL_INVALID_COMMAND_QUEUE);
  OCL_CHECK(!ptr, return CL_INVALID_VALUE);
  OCL_CHECK(
      (flags == 0) || (flags & ~(CL_MIGRATE_MEM_OBJECT_HOST |
                                 CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED)),
      return CL_INVALID_VALUE);

  const cl_int error = cl::validate::EventWaitList(
      num_events_in_wait_list, event_wait_list, command_queue->context, event);
  OCL_CHECK(error != CL_SUCCESS, return error);

  auto new_event =
      _cl_event::create(command_queue, CL_COMMAND_MIGRATEMEM_INTEL);
  if (!new_event) {
    return new_event.error();
  }
  cl_event return_event = *new_event;

  cl::release_guard<cl_event> event_release_guard(return_event,
                                                  cl::ref_count_type::EXTERNAL);
  {
    const std::lock_guard<std::mutex> context_guard(
        command_queue->context->usm_mutex);
    const cl_context context = command_queue->context;
    extension::usm::allocation_info *const usm_alloc =
        extension::usm::findAllocation(context, ptr);
    OCL_CHECK(nullptr == usm_alloc, return CL_INVALID_VALUE);

    const intptr_t ptr_offset =
        reinterpret_cast<uintptr_t>(ptr) -
        reinterpret_cast<uintptr_t>(usm_alloc->base_ptr);
    const intptr_t bytes_till_end = usm_alloc->size - ptr_offset;
    OCL_CHECK(intptr_t(size) > bytes_till_end, return CL_INVALID_VALUE);

    const std::lock_guard<std::mutex> lock(
        command_queue->context->getCommandQueueMutex());

    auto mux_command_buffer = command_queue->getCommandBuffer(
        {event_wait_list, num_events_in_wait_list}, return_event);
    OCL_CHECK(!mux_command_buffer, return CL_OUT_OF_RESOURCES);

    auto mux_error = usm_alloc->record_event(return_event);
    OCL_CHECK(mux_error, return CL_OUT_OF_RESOURCES);
  }

  if (event) {
    *event = event_release_guard.dismiss();
  }

  return CL_SUCCESS;
}

CL_API_ENTRY
cl_int clEnqueueMemAdviseINTEL(cl_command_queue command_queue, const void *ptr,
                               size_t size, cl_mem_advice_intel advice,
                               cl_uint num_events_in_wait_list,
                               const cl_event *event_wait_list,
                               cl_event *event) {
  const tracer::TraceGuard<tracer::OpenCL> trace(__func__);

  OCL_CHECK(!command_queue, return CL_INVALID_COMMAND_QUEUE);
  OCL_CHECK(!ptr, return CL_INVALID_VALUE);
  OCL_CHECK(advice != 0, return CL_INVALID_VALUE);  // None defined yet
  OCL_CHECK(size == 0, return CL_INVALID_VALUE);    // None defined yet

  const cl_int error = cl::validate::EventWaitList(
      num_events_in_wait_list, event_wait_list, command_queue->context, event);
  OCL_CHECK(error != CL_SUCCESS, return error);

  auto new_event = _cl_event::create(command_queue, CL_COMMAND_MEMADVISE_INTEL);
  if (!new_event) {
    return new_event.error();
  }
  cl_event return_event = *new_event;

  cl::release_guard<cl_event> event_release_guard(return_event,
                                                  cl::ref_count_type::EXTERNAL);
  {
    const std::lock_guard<std::mutex> context_guard(
        command_queue->context->usm_mutex);
    const cl_context context = command_queue->context;
    extension::usm::allocation_info *const usm_alloc =
        extension::usm::findAllocation(context, ptr);
    OCL_CHECK(nullptr == usm_alloc, return CL_INVALID_VALUE);

    const std::lock_guard<std::mutex> lock(
        command_queue->context->getCommandQueueMutex());

    auto mux_command_buffer = command_queue->getCommandBuffer(
        {event_wait_list, num_events_in_wait_list}, return_event);
    OCL_CHECK(!mux_command_buffer, return CL_OUT_OF_RESOURCES);

    auto mux_error = usm_alloc->record_event(return_event);
    OCL_CHECK(mux_error, return CL_OUT_OF_RESOURCES);
  }

  if (event) {
    *event = event_release_guard.dismiss();
  }

  return CL_SUCCESS;
}
#endif  // OCL_EXTENSION_cl_intel_unified_shared_memory

void *
extension::intel_unified_shared_memory::GetExtensionFunctionAddressForPlatform(
    cl_platform_id, const char *func_name) const {
#ifndef OCL_EXTENSION_cl_intel_unified_shared_memory
  OCL_UNUSED(func_name);
  return nullptr;
#else
  if (0 == std::strcmp("clHostMemAllocINTEL", func_name)) {
    return reinterpret_cast<void *>(&clHostMemAllocINTEL);
  } else if (0 == std::strcmp("clDeviceMemAllocINTEL", func_name)) {
    return reinterpret_cast<void *>(&clDeviceMemAllocINTEL);
  } else if (0 == std::strcmp("clSharedMemAllocINTEL", func_name)) {
    return reinterpret_cast<void *>(&clSharedMemAllocINTEL);
  } else if (0 == std::strcmp("clMemFreeINTEL", func_name)) {
    return reinterpret_cast<void *>(&clMemFreeINTEL);
  } else if (0 == std::strcmp("clMemBlockingFreeINTEL", func_name)) {
    return reinterpret_cast<void *>(&clMemBlockingFreeINTEL);
  } else if (0 == std::strcmp("clGetMemAllocInfoINTEL", func_name)) {
    return reinterpret_cast<void *>(&clGetMemAllocInfoINTEL);
  } else if (0 == std::strcmp("clSetKernelArgMemPointerINTEL", func_name)) {
    return reinterpret_cast<void *>(&clSetKernelArgMemPointerINTEL);
  } else if (0 == std::strcmp("clEnqueueMemFillINTEL", func_name)) {
    return reinterpret_cast<void *>(&clEnqueueMemFillINTEL);
  } else if (0 == std::strcmp("clEnqueueMemcpyINTEL", func_name)) {
    return reinterpret_cast<void *>(&clEnqueueMemcpyINTEL);
  } else if (0 == std::strcmp("clEnqueueMigrateMemINTEL", func_name)) {
    return reinterpret_cast<void *>(&clEnqueueMigrateMemINTEL);
  } else if (0 == std::strcmp("clEnqueueMemAdviseINTEL", func_name)) {
    return reinterpret_cast<void *>(&clEnqueueMemAdviseINTEL);
  } else if (0 == std::strcmp("clEnqueueMemsetINTEL", func_name)) {
    // Deprecated
    return reinterpret_cast<void *>(&clEnqueueMemsetINTEL);
  }
  return nullptr;
#endif  // OCL_EXTENSION_cl_intel_unified_shared_memory
}
