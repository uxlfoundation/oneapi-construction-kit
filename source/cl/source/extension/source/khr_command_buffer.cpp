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

#include <cargo/array_view.h>
#include <cargo/expected.h>
#include <cl/buffer.h>
#include <cl/context.h>
#include <cl/device.h>
#include <cl/event.h>
#include <cl/image.h>
#include <cl/kernel.h>
#include <cl/printf.h>
#include <cl/program.h>
#include <cl/validate.h>
#include <extension/khr_command_buffer.h>
#include <tracer/tracer.h>

#include <algorithm>

extension::khr_command_buffer::khr_command_buffer()
    : extension(
          "cl_khr_command_buffer",
#ifdef OCL_EXTENSION_cl_khr_command_buffer
          /* There is discussion in the working group of having the command
             buffer extension support multiple queues which would mean multiple
             devices and make this a platform extension. For now, since we only
             support a single queue this will be a DEVICE extension */
          usage_category::DEVICE
#else
          usage_category::DISABLED
#endif
              CA_CL_EXT_VERSION(0, 1, 0)) {
}

void *extension::khr_command_buffer::GetExtensionFunctionAddressForPlatform(
    cl_platform_id, const char *func_name) const {
#ifndef OCL_EXTENSION_cl_khr_command_buffer
  OCL_UNUSED(func_name);
  return nullptr;
#else
  if (0 == std::strcmp("clCreateCommandBufferKHR", func_name)) {
    return reinterpret_cast<void *>(&clCreateCommandBufferKHR);
  } else if (0 == std::strcmp("clReleaseCommandBufferKHR", func_name)) {
    return reinterpret_cast<void *>(&clReleaseCommandBufferKHR);
  } else if (0 == std::strcmp("clRetainCommandBufferKHR", func_name)) {
    return reinterpret_cast<void *>(&clRetainCommandBufferKHR);
  } else if (0 == std::strcmp("clFinalizeCommandBufferKHR", func_name)) {
    return reinterpret_cast<void *>(&clFinalizeCommandBufferKHR);
  } else if (0 == std::strcmp("clEnqueueCommandBufferKHR", func_name)) {
    return reinterpret_cast<void *>(&clEnqueueCommandBufferKHR);
  } else if (0 == std::strcmp("clCommandBarrierWithWaitListKHR", func_name)) {
    return reinterpret_cast<void *>(&clCommandBarrierWithWaitListKHR);
  } else if (0 == std::strcmp("clCommandCopyBufferKHR", func_name)) {
    return reinterpret_cast<void *>(&clCommandCopyBufferKHR);
  } else if (0 == std::strcmp("clCommandCopyBufferRectKHR", func_name)) {
    return reinterpret_cast<void *>(&clCommandCopyBufferRectKHR);
  } else if (0 == std::strcmp("clCommandCopyBufferToImageKHR", func_name)) {
    return reinterpret_cast<void *>(&clCommandCopyBufferToImageKHR);
  } else if (0 == std::strcmp("clCommandCopyImageKHR", func_name)) {
    return reinterpret_cast<void *>(&clCommandCopyImageKHR);
  } else if (0 == std::strcmp("clCommandCopyImageToBufferKHR", func_name)) {
    return reinterpret_cast<void *>(&clCommandCopyImageToBufferKHR);
  } else if (0 == std::strcmp("clCommandFillBufferKHR", func_name)) {
    return reinterpret_cast<void *>(&clCommandFillBufferKHR);
  } else if (0 == std::strcmp("clCommandFillImageKHR", func_name)) {
    return reinterpret_cast<void *>(&clCommandFillImageKHR);
  } else if (0 == std::strcmp("clCommandNDRangeKernelKHR", func_name)) {
    return reinterpret_cast<void *>(&clCommandNDRangeKernelKHR);
  } else if (0 == std::strcmp("clGetCommandBufferInfoKHR", func_name)) {
    return reinterpret_cast<void *>(&clGetCommandBufferInfoKHR);
  }
  return nullptr;
#endif
}

cl_int extension::khr_command_buffer::GetDeviceInfo(
    cl_device_id device, cl_device_info param_name, size_t param_value_size,
    void *param_value, size_t *param_value_size_ret) const {
#ifndef OCL_EXTENSION_cl_khr_command_buffer
  return extension::GetDeviceInfo(device, param_name, param_value_size,
                                  param_value, param_value_size_ret);
#else
  cl_device_command_buffer_capabilities_khr result = 0;
  switch (param_name) {
    case CL_DEVICE_COMMAND_BUFFER_CAPABILITIES_KHR: {
      result = CL_COMMAND_BUFFER_CAPABILITY_KERNEL_PRINTF_KHR;

      const auto device_info = device->mux_device->info;
      if (device_info->can_clone_command_buffers) {
        result |= CL_COMMAND_BUFFER_CAPABILITY_SIMULTANEOUS_USE_KHR;
      }
    } break;
    case CL_DEVICE_COMMAND_BUFFER_REQUIRED_QUEUE_PROPERTIES_KHR:
      // We don't have any required properties for a queue to run
      // command-buffers on
      break;
    default:
      // Use default implementation that uses the name set in the constructor
      // as the name usage specifies.
      return extension::GetDeviceInfo(device, param_name, param_value_size,
                                      param_value, param_value_size_ret);
  }

  constexpr size_t type_size = sizeof(result);
  if (nullptr != param_value) {
    OCL_CHECK(param_value_size < type_size, return CL_INVALID_VALUE);
    *static_cast<cl_device_command_buffer_capabilities_khr *>(param_value) =
        result;
  }
  OCL_SET_IF_NOT_NULL(param_value_size_ret, type_size);
  return CL_SUCCESS;
#endif
}

#ifdef OCL_EXTENSION_cl_khr_command_buffer
_cl_mutable_command_khr::_cl_mutable_command_khr(cl_uint id, cl_kernel kernel)
    : id(id), kernel(kernel) {}

_cl_mutable_command_khr::~_cl_mutable_command_khr() {
  cl::releaseInternal(kernel);
}

cargo::expected<std::unique_ptr<_cl_mutable_command_khr>, cl_int>
_cl_mutable_command_khr::create(cl_uint id, cl_kernel kernel) {
  auto mutable_command = std::unique_ptr<_cl_mutable_command_khr>(
      new (std::nothrow) _cl_mutable_command_khr(id, kernel));
  if (nullptr == mutable_command) {
    return cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY);
  }

  if (auto error = cl::retainInternal(kernel)) {
    return cargo::make_unexpected(error);
  }

  return mutable_command;
}

_cl_command_buffer_khr::_cl_command_buffer_khr(cl_command_queue queue)
    : base(cl::ref_count_type::EXTERNAL),
      next_command_index(0u),
      flags(0),
      is_finalized(false),
      command_queue(queue),
      execution_refcount(0u) {
  cl::retainInternal(command_queue);
}

_cl_command_buffer_khr::~_cl_command_buffer_khr() {
  // Errors in this function can't be communicated to the caller, but this
  // destruction needs to happen here since this is thread safe and will only
  // get called by the last reference on the command buffer.

  // Release any buffers which acquired a reference via calls to
  // clEnqueueCopyBufferKHR.
  for (cl_mem mem : mems) {
    cl::releaseInternal(mem);
  }
  // Destroy any specialized kernels which acquired a reference via calls to
  // clEnqueueNDRangeKHR.
  for (const auto &entry : mux_kernels) {
    if (entry.first) {
      muxDestroyExecutable(mux_command_buffer->device, entry.first,
                           command_queue->device->mux_allocator);
    }
    if (entry.second) {
      muxDestroyKernel(mux_command_buffer->device, entry.second,
                       command_queue->device->mux_allocator);
    }
  }
  // Release any kernels which acquired a reference via calls to
  // clCommandEnqueueNDRangeKHR.
  for (cl_kernel kernel : kernels) {
    cl::releaseInternal(kernel);
  }

  // Destroy and release the underlying command buffer.
  muxDestroyCommandBuffer(mux_command_buffer->device, mux_command_buffer,
                          command_queue->device->mux_allocator);

  cl::releaseInternal(command_queue);
}

cargo::expected<std::unique_ptr<_cl_command_buffer_khr>, cl_int>
_cl_command_buffer_khr::create(
    cl_command_queue command_queue,
    const cl_command_buffer_properties_khr *properties) {
  // Ideally we'd use make_unique here, but the fact that this class has a
  // private constructor makes that tricky.
  auto command_buffer = std::unique_ptr<_cl_command_buffer_khr>(
      new (std::nothrow) _cl_command_buffer_khr(command_queue));
  if (command_buffer == nullptr) {
    return cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY);
  }

  // Create the command buffer in the underlying mux_command_buffer_t.
  if (auto mux_error =
          muxCreateCommandBuffer(command_queue->device->mux_device, nullptr,
                                 command_queue->device->mux_allocator,
                                 &command_buffer->mux_command_buffer)) {
    return cargo::make_unexpected(cl::getErrorFrom(mux_error));
  }

  // Make sure passed properties are valid and store for later querying.
  if (properties && properties[0] != 0) {
    cl_command_buffer_properties_khr seen = 0;

    auto current = properties;
    do {
      cl_command_buffer_properties_khr property = current[0];
      switch (property) {
        case CL_COMMAND_BUFFER_FLAGS_KHR:
          if (0 == (seen & CL_COMMAND_BUFFER_FLAGS_KHR)) {
            const cl_command_buffer_flags_khr flags_mask =
#ifdef OCL_EXTENSION_cl_khr_command_buffer_mutable_dispatch
                CL_COMMAND_BUFFER_SIMULTANEOUS_USE_KHR |
                CL_COMMAND_BUFFER_MUTABLE_KHR;
#else
                CL_COMMAND_BUFFER_SIMULTANEOUS_USE_KHR;
#endif

            const cl_command_buffer_flags_khr value = current[1];

            // Check flag is defined by the specification
            if (value & ~flags_mask) {
              return cargo::make_unexpected(CL_INVALID_VALUE);
            }

            // simultaneous-use is not possible on all devices, support for
            // cloning command-buffers is required.
            const auto device_info = command_queue->device->mux_device->info;
            if (!device_info->can_clone_command_buffers &&
                (value & CL_COMMAND_BUFFER_SIMULTANEOUS_USE_KHR)) {
              return cargo::make_unexpected(CL_INVALID_PROPERTY);
            }

            command_buffer->flags = value;
            seen |= property;
            break;
          }
          [[fallthrough]];
        default:
          return cargo::make_unexpected(CL_INVALID_VALUE);
      }
      current += 2;
    } while (current[0] != 0);

    if (command_buffer->properties_list.assign(properties, current + 1)) {
      return cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY);
    }
  }

  return command_buffer;
}

cl_command_buffer_state_khr _cl_command_buffer_khr::getState() const {
  if (!is_finalized) {
    return CL_COMMAND_BUFFER_STATE_RECORDING_KHR;
  }
  return execution_refcount > 0 ? CL_COMMAND_BUFFER_STATE_PENDING_KHR
                                : CL_COMMAND_BUFFER_STATE_EXECUTABLE_KHR;
}

cl_int _cl_command_buffer_khr::finalize() {
  std::lock_guard<std::mutex> guard(mutex);

  if (is_finalized) {
    return CL_INVALID_OPERATION;
  }

  if (muxFinalizeCommandBuffer(mux_command_buffer)) {
    return CL_INVALID_COMMAND_BUFFER_KHR;
  }
  is_finalized = true;
  return CL_SUCCESS;
}

cl_int _cl_command_buffer_khr::retain(cl_kernel kernel) {
  if (cargo::success != kernels.push_back(kernel)) {
    return CL_OUT_OF_HOST_MEMORY;
  }
  cl::retainInternal(kernel);
  return CL_SUCCESS;
}

cl_int _cl_command_buffer_khr::storeKernel(mux_executable_t executable,
                                           mux_kernel_t kernel) {
  if (cargo::success != mux_kernels.push_back({executable, kernel})) {
    return CL_OUT_OF_HOST_MEMORY;
  }
  return CL_SUCCESS;
}

cl_int _cl_command_buffer_khr::retain(cl_mem mem_obj) {
  // No need to store a reference to the memory object if we already have it.
  if (std::find(std::begin(mems), std::end(mems), mem_obj) != std::end(mems)) {
    return CL_SUCCESS;
  }
  if (cargo::success != mems.push_back(mem_obj)) {
    return CL_OUT_OF_HOST_MEMORY;
  }
  cl::retainInternal(mem_obj);
  return CL_SUCCESS;
}

cargo::expected<cargo::small_vector<mux_sync_point_t, 4>, cl_int>
_cl_command_buffer_khr::convertWaitList(
    const cargo::array_view<const cl_sync_point_khr> &cl_wait_list) {
  cargo::small_vector<mux_sync_point_t, 4> command_wait_list;
  for (auto index : cl_wait_list) {
    auto mux_sync_point = mux_sync_points.at(index);
    if (mux_sync_point.error()) {
      return cargo::make_unexpected(CL_INVALID_SYNC_POINT_WAIT_LIST_KHR);
    }
    if (command_wait_list.push_back(*mux_sync_point)) {
      return cargo::make_unexpected(CL_OUT_OF_HOST_MEMORY);
    }
  }
  return command_wait_list;
}

cl_int _cl_command_buffer_khr::commandBarrierWithWaitList(
    cargo::array_view<const cl_sync_point_khr> &cl_wait_list,
    cl_sync_point_khr *cl_sync_point) {
  std::lock_guard<std::mutex> guard(mutex);

  auto command_wait_list = convertWaitList(cl_wait_list);
  if (!command_wait_list) {
    return command_wait_list.error();
  }
  const auto wait_list_length = command_wait_list->size();

  // TODO CA-4383 - Barriers are a no-op operation for in-order
  // command-buffers. However, to get a sync-point out of the Mux API we need to
  // use a command recording entry-point, so a no-op callback command is used
  // here as a workaround. CA-4383 is to remove this workaround by introducing
  // a muxCommandBarrier() command to match better for an out-of-order
  // command-buffer future.
  mux_sync_point_t mux_sync_point = nullptr;
  mux_sync_point_t *out_sync_point = cl_sync_point ? &mux_sync_point : nullptr;
  auto mux_error = muxCommandUserCallback(
      mux_command_buffer, [](mux_queue_t, mux_command_buffer_t, void *const) {},
      nullptr, wait_list_length,
      wait_list_length ? command_wait_list->data() : nullptr, out_sync_point);

  if (mux_error) {
    return cl::getErrorFrom(mux_error);
  }

  if (out_sync_point) {
    if (mux_sync_points.push_back(mux_sync_point)) {
      return CL_OUT_OF_HOST_MEMORY;
    }
    *cl_sync_point = mux_sync_points.size() - 1;
  }
  // Increment the command counter.
  ++next_command_index;
  return CL_SUCCESS;
}

cl_int _cl_command_buffer_khr::commandCopyBuffer(
    cl_mem src_buffer, cl_mem dst_buffer, size_t src_offset, size_t dst_offset,
    size_t size, cargo::array_view<const cl_sync_point_khr> &cl_wait_list,
    cl_sync_point_khr *cl_sync_point) {
  std::lock_guard<std::mutex> guard(mutex);

  // Add references to the buffers.
  if (auto error = retain(src_buffer)) {
    return error;
  }
  if (auto error = retain(dst_buffer)) {
    return error;
  }

  // Look up the device index and get the buffers. Since we only have one queue
  // for one device at the moment we can just get the index from the queue.
  const auto device_index = command_queue->getDeviceIndex();
  auto mux_src_buffer =
      static_cast<_cl_mem_buffer *>(src_buffer)->mux_buffers[device_index];
  auto mux_dst_buffer =
      static_cast<_cl_mem_buffer *>(dst_buffer)->mux_buffers[device_index];

  auto command_wait_list = convertWaitList(cl_wait_list);
  if (!command_wait_list) {
    return command_wait_list.error();
  }
  const auto wait_list_length = command_wait_list->size();

  mux_sync_point_t mux_sync_point = nullptr;
  mux_sync_point_t *out_sync_point = cl_sync_point ? &mux_sync_point : nullptr;
  auto mux_error = muxCommandCopyBuffer(
      mux_command_buffer, mux_src_buffer, src_offset, mux_dst_buffer,
      dst_offset, size, wait_list_length,
      wait_list_length ? command_wait_list->data() : nullptr, out_sync_point);

  if (mux_error) {
    return cl::getErrorFrom(mux_error);
  }

  if (out_sync_point) {
    if (mux_sync_points.push_back(mux_sync_point)) {
      return CL_OUT_OF_HOST_MEMORY;
    }
    *cl_sync_point = mux_sync_points.size() - 1;
  }

  // Increment the command counter.
  ++next_command_index;
  return CL_SUCCESS;
}

cl_int _cl_command_buffer_khr::commandCopyImage(
    cl_mem src_image, cl_mem dst_image, const size_t *src_origin,
    const size_t *dst_origin, const size_t *region,
    cargo::array_view<const cl_sync_point_khr> &cl_wait_list,
    cl_sync_point_khr *cl_sync_point) {
  std::lock_guard<std::mutex> guard(mutex);

  // Add references to the images.
  if (auto error = retain(src_image)) {
    return error;
  }
  if (auto error = retain(dst_image)) {
    return error;
  }

  // Look up the device index and get the images. Since we only have one queue
  // for one device at the moment we can just get the index from the queue.
  const auto device_index = command_queue->getDeviceIndex();
  auto mux_src_image =
      static_cast<cl_mem_image>(src_image)->mux_images[device_index];
  auto mux_dst_image =
      static_cast<cl_mem_image>(dst_image)->mux_images[device_index];

  auto command_wait_list = convertWaitList(cl_wait_list);
  if (!command_wait_list) {
    return command_wait_list.error();
  }
  const auto wait_list_length = command_wait_list->size();

  mux_sync_point_t mux_sync_point = nullptr;
  mux_sync_point_t *out_sync_point = cl_sync_point ? &mux_sync_point : nullptr;
  auto mux_error = muxCommandCopyImage(
      mux_command_buffer, mux_src_image, mux_dst_image,
      {static_cast<uint32_t>(src_origin[0]),
       static_cast<uint32_t>(src_origin[1]),
       static_cast<uint32_t>(src_origin[2])},
      {static_cast<uint32_t>(dst_origin[0]),
       static_cast<uint32_t>(dst_origin[1]),
       static_cast<uint32_t>(dst_origin[2])},
      {static_cast<uint32_t>(region[0]), static_cast<uint32_t>(region[1]),
       static_cast<uint32_t>(region[2])},
      wait_list_length, wait_list_length ? command_wait_list->data() : nullptr,
      out_sync_point);

  if (mux_error) {
    return cl::getErrorFrom(mux_error);
  }

  if (out_sync_point) {
    if (mux_sync_points.push_back(mux_sync_point)) {
      return CL_OUT_OF_HOST_MEMORY;
    }
    *cl_sync_point = mux_sync_points.size() - 1;
  }

  // Increment the command counter.
  ++next_command_index;
  return CL_SUCCESS;
}

cl_int _cl_command_buffer_khr::commandCopyBufferRect(
    cl_mem src_buffer, cl_mem dst_buffer, const size_t *src_origin,
    const size_t *dst_origin, const size_t *region, size_t src_row_pitch,
    size_t src_slice_pitch, size_t dst_row_pitch, size_t dst_slice_pitch,
    cargo::array_view<const cl_sync_point_khr> &cl_wait_list,
    cl_sync_point_khr *cl_sync_point) {
  std::lock_guard<std::mutex> guard(mutex);

  // Add a references to the buffers.
  if (auto error = retain(src_buffer)) {
    return error;
  }
  if (auto error = retain(dst_buffer)) {
    return error;
  }

  // Call into the mux target to add the command to the user command buffer.
  const auto device_index = command_queue->getDeviceIndex();
  auto mux_src_buffer =
      static_cast<cl_mem_buffer>(src_buffer)->mux_buffers[device_index];
  auto mux_dst_buffer =
      static_cast<cl_mem_buffer>(dst_buffer)->mux_buffers[device_index];

  mux_buffer_region_info_t r_info{
      {region[0], region[1], region[2]},
      {src_origin[0], src_origin[1], src_origin[2]},
      {dst_origin[0], dst_origin[1], dst_origin[2]},
      {src_row_pitch, src_slice_pitch},
      {dst_row_pitch, dst_slice_pitch},
  };

  auto command_wait_list = convertWaitList(cl_wait_list);
  if (!command_wait_list) {
    return command_wait_list.error();
  }
  const auto wait_list_length = command_wait_list->size();

  mux_sync_point_t mux_sync_point = nullptr;
  mux_sync_point_t *out_sync_point = cl_sync_point ? &mux_sync_point : nullptr;
  mux_result_t mux_error = muxCommandCopyBufferRegions(
      mux_command_buffer, mux_src_buffer, mux_dst_buffer, &r_info, 1,
      wait_list_length, wait_list_length ? command_wait_list->data() : nullptr,
      out_sync_point);
  if (mux_error) {
    return cl::getErrorFrom(mux_error);
  }

  if (out_sync_point) {
    if (mux_sync_points.push_back(mux_sync_point)) {
      return CL_OUT_OF_HOST_MEMORY;
    }
    *cl_sync_point = mux_sync_points.size() - 1;
  }

  // Synchronize the buffer.
  // TODO: Does dst_buffer need to be synchronized as well?
  // This TODO is identical to the one in `clEnqueueCopyBufferRect` defined in
  // cl/buffer.cpp.
  if (auto error =
          static_cast<cl_mem_buffer>(src_buffer)->synchronize(command_queue)) {
    return error;
  }

  // Increment the command counter.
  ++next_command_index;
  return CL_SUCCESS;
}

cl_int _cl_command_buffer_khr::commandFillBuffer(
    cl_mem buffer, const void *pattern, size_t pattern_size, size_t offset,
    size_t size, cargo::array_view<const cl_sync_point_khr> &cl_wait_list,
    cl_sync_point_khr *cl_sync_point) {
  std::lock_guard<std::mutex> guard(mutex);

  // Add a reference to the buffer.
  if (auto error = retain(buffer)) {
    return error;
  }

  // Call into the mux target to add the command to the user command buffer.
  const auto device_index = command_queue->getDeviceIndex();
  auto mux_buffer =
      static_cast<cl_mem_buffer>(buffer)->mux_buffers[device_index];

  auto command_wait_list = convertWaitList(cl_wait_list);
  if (!command_wait_list) {
    return command_wait_list.error();
  }
  const auto wait_list_length = command_wait_list->size();

  mux_sync_point_t mux_sync_point = nullptr;
  mux_sync_point_t *out_sync_point = cl_sync_point ? &mux_sync_point : nullptr;

  auto mux_error = muxCommandFillBuffer(
      mux_command_buffer, mux_buffer, offset, size, pattern, pattern_size,
      wait_list_length, wait_list_length ? command_wait_list->data() : nullptr,
      out_sync_point);
  if (mux_error) {
    return cl::getErrorFrom(mux_error);
  }

  if (out_sync_point) {
    if (mux_sync_points.push_back(mux_sync_point)) {
      return CL_OUT_OF_HOST_MEMORY;
    }
    *cl_sync_point = mux_sync_points.size() - 1;
  }

  // Synchronize the buffer.
  if (auto error =
          static_cast<cl_mem_buffer>(buffer)->synchronize(command_queue)) {
    return error;
  }

  // Increment the command counter.
  ++next_command_index;
  return CL_SUCCESS;
}

cl_int _cl_command_buffer_khr::commandFillImage(
    cl_mem image, const void *fill_color, const size_t *origin,
    const size_t *region,
    cargo::array_view<const cl_sync_point_khr> &cl_wait_list,
    cl_sync_point_khr *cl_sync_point) {
  std::lock_guard<std::mutex> guard(mutex);

  // Add a reference to the image.
  if (auto error = retain(image)) {
    return error;
  }

  // Call into the mux target to add the command to the user command buffer.
  const auto device_index = command_queue->getDeviceIndex();
  auto mux_image = static_cast<cl_mem_image>(image)->mux_images[device_index];

  auto command_wait_list = convertWaitList(cl_wait_list);
  if (!command_wait_list) {
    return command_wait_list.error();
  }
  const auto wait_list_length = command_wait_list->size();

  mux_sync_point_t mux_sync_point = nullptr;
  mux_sync_point_t *out_sync_point = cl_sync_point ? &mux_sync_point : nullptr;
  auto mux_error = muxCommandFillImage(
      mux_command_buffer, mux_image, fill_color, sizeof(float) * 4,
      {static_cast<uint32_t>(origin[0]), static_cast<uint32_t>(origin[1]),
       static_cast<uint32_t>(origin[2])},
      {static_cast<uint32_t>(region[0]), static_cast<uint32_t>(region[1]),
       static_cast<uint32_t>(region[2])},
      wait_list_length, wait_list_length ? command_wait_list->data() : nullptr,
      out_sync_point);

  if (mux_error) {
    return cl::getErrorFrom(mux_error);
  }

  if (out_sync_point) {
    if (mux_sync_points.push_back(mux_sync_point)) {
      return CL_OUT_OF_HOST_MEMORY;
    }
    *cl_sync_point = mux_sync_points.size() - 1;
  }

  // Increment the command counter.
  ++next_command_index;
  return CL_SUCCESS;
}

cl_int _cl_command_buffer_khr::commandCopyBufferToImage(
    cl_mem src_buffer, cl_mem dst_image, size_t src_offset,
    const size_t *dst_origin, const size_t *region,
    cargo::array_view<const cl_sync_point_khr> &cl_wait_list,
    cl_sync_point_khr *cl_sync_point) {
  std::lock_guard<std::mutex> guard(mutex);

  // Add references to the memory objects.
  if (auto error = retain(src_buffer)) {
    return error;
  }
  if (auto error = retain(dst_image)) {
    return error;
  }

  // Look up the device index and get the images. Since we only have one queue
  // for one device at the moment we can just get the index from the queue.
  const auto device_index = command_queue->getDeviceIndex();
  auto mux_src_buffer =
      static_cast<cl_mem_buffer>(src_buffer)->mux_buffers[device_index];
  auto mux_dst_image =
      static_cast<cl_mem_image>(dst_image)->mux_images[device_index];

  auto command_wait_list = convertWaitList(cl_wait_list);
  if (!command_wait_list) {
    return command_wait_list.error();
  }
  const auto wait_list_length = command_wait_list->size();

  mux_sync_point_t mux_sync_point = nullptr;
  mux_sync_point_t *out_sync_point = cl_sync_point ? &mux_sync_point : nullptr;
  auto mux_error = muxCommandCopyBufferToImage(
      mux_command_buffer, mux_src_buffer, mux_dst_image, src_offset,
      {static_cast<uint32_t>(dst_origin[0]),
       static_cast<uint32_t>(dst_origin[1]),
       static_cast<uint32_t>(dst_origin[2])},
      {static_cast<uint32_t>(region[0]), static_cast<uint32_t>(region[1]),
       static_cast<uint32_t>(region[2])},
      wait_list_length, wait_list_length ? command_wait_list->data() : nullptr,
      out_sync_point);

  if (mux_error) {
    return cl::getErrorFrom(mux_error);
  }

  if (out_sync_point) {
    if (mux_sync_points.push_back(mux_sync_point)) {
      return CL_OUT_OF_HOST_MEMORY;
    }
    *cl_sync_point = mux_sync_points.size() - 1;
  }

  // Increment the command counter.
  ++next_command_index;
  return CL_SUCCESS;
}

cl_int _cl_command_buffer_khr::commandCopyImageToBuffer(
    cl_mem src_image, cl_mem dst_buffer, const size_t *src_origin,
    const size_t *region, size_t dst_offset,
    cargo::array_view<const cl_sync_point_khr> &cl_wait_list,
    cl_sync_point_khr *cl_sync_point) {
  std::lock_guard<std::mutex> guard(mutex);

  // Add references to the memory objects.
  if (auto error = retain(src_image)) {
    return error;
  }
  if (auto error = retain(dst_buffer)) {
    return error;
  }

  // Look up the device index and get the images. Since we only have one queue
  // for one device at the moment we can just get the index from the queue.
  const auto device_index = command_queue->getDeviceIndex();
  auto mux_src_image =
      static_cast<cl_mem_image>(src_image)->mux_images[device_index];
  auto mux_dst_buffer =
      static_cast<cl_mem_buffer>(dst_buffer)->mux_buffers[device_index];

  auto command_wait_list = convertWaitList(cl_wait_list);
  if (!command_wait_list) {
    return command_wait_list.error();
  }
  const auto wait_list_length = command_wait_list->size();

  mux_sync_point_t mux_sync_point = nullptr;
  mux_sync_point_t *out_sync_point = cl_sync_point ? &mux_sync_point : nullptr;
  auto mux_error = muxCommandCopyImageToBuffer(
      mux_command_buffer, mux_src_image, mux_dst_buffer,
      {static_cast<uint32_t>(src_origin[0]),
       static_cast<uint32_t>(src_origin[1]),
       static_cast<uint32_t>(src_origin[2])},
      dst_offset,
      {static_cast<uint32_t>(region[0]), static_cast<uint32_t>(region[1]),
       static_cast<uint32_t>(region[2])},
      wait_list_length, wait_list_length ? command_wait_list->data() : nullptr,
      out_sync_point);

  if (mux_error) {
    return cl::getErrorFrom(mux_error);
  }

  if (out_sync_point) {
    if (mux_sync_points.push_back(mux_sync_point)) {
      return CL_OUT_OF_HOST_MEMORY;
    }
    *cl_sync_point = mux_sync_points.size() - 1;
  }

  // Increment the command counter.
  ++next_command_index;
  return CL_SUCCESS;
}

cl_int _cl_command_buffer_khr::commandNDRangeKernel(
    cl_kernel kernel, cl_uint work_dim, const size_t *global_work_offset,
    const size_t *global_work_size, const size_t *local_work_size,
    cargo::array_view<const cl_sync_point_khr> &cl_wait_list,
    cl_sync_point_khr *cl_sync_point, cl_mutable_command_khr *mutable_handle) {
  std::lock_guard<std::mutex> guard(mutex);

  // Check the required work group size (if it exists).
  if (auto error = kernel->checkReqdWorkGroupSize(work_dim, local_work_size)) {
    return error;
  }

  // Check the local and global work sizes are correct.
  if (auto error = kernel->checkWorkSizes(command_queue->device, work_dim,
                                          global_work_offset, global_work_size,
                                          local_work_size)) {
    return error;
  }

  // If the user didn't pass a local size and the kernel doesn't require one
  // then pick one based on the device.
  std::array<size_t, cl::max::WORK_ITEM_DIM> final_local_work_size{1, 1, 1};
  if (local_work_size) {
    std::copy_n(local_work_size, work_dim, std::begin(final_local_work_size));
  } else {
    final_local_work_size = kernel->getDefaultLocalSize(
        command_queue->device, global_work_size, work_dim);
  }

  // If the user passed a NULL pointer as the global offset then this means that
  // the offset is {0,0,0}.
  std::array<size_t, cl::max::WORK_ITEM_DIM> final_global_offset{0, 0, 0};
  if (global_work_offset) {
    std::copy_n(global_work_offset, work_dim, std::begin(final_global_offset));
  }

  // The user must pass a global size but here we also initialize the global
  // sizes for the unused dimensions so callers don't have to keep checking the
  // work dimensions.
  std::array<size_t, cl::max::WORK_ITEM_DIM> final_global_size{1, 1, 1};
  std::copy_n(global_work_size, work_dim, std::begin(final_global_size));

  // Check the current kernel arguments are valid.
  if (auto error = kernel->checkKernelArgs()) {
    return error;
  }

  // Associate the kernel to the command buffer, it'll get released when the
  // command buffer is destroyed.
  if (retain(kernel)) {
    return CL_OUT_OF_HOST_MEMORY;
  }

  std::unique_ptr<mux_descriptor_info_t[]> descriptor_info_storage;
  cl_device_id device = command_queue->device;

  // create the printf buffer argument if necessary
  mux_buffer_t printf_buffer = nullptr;
  mux_memory_t printf_memory = nullptr;
  size_t num_groups = 0;
  size_t buffer_group_size = 0;

  auto &device_program = kernel->program->programs[device];
  if (device_program.printf_calls.size() != 0) {
    cl_int err = createPrintfBuffer(
        device, final_local_work_size, final_global_size, num_groups,
        buffer_group_size, printf_memory, printf_buffer);
    if (err) {
      return err;
    }
  }

  const cl_uint device_index = kernel->program->context->getDeviceIndex(device);
  mux_ndrange_options_t mux_execution_options =
      kernel->createKernelExecutionOptions(
          device, device_index, work_dim, final_local_work_size,
          final_global_offset, final_global_size, printf_buffer,
          descriptor_info_storage);

  mux_result_t mux_error;
  mux_kernel_t mux_kernel;

  if (kernel->device_kernel_map[device]->supportsDeferredCompilation()) {
    auto result = kernel->device_kernel_map[device]->createSpecializedKernel(
        mux_execution_options);
    if (!result.has_value()) {
      if (printf_buffer) {
        muxDestroyBuffer(device->mux_device, printf_buffer,
                         device->mux_allocator);
      }
      if (printf_memory) {
        muxFreeMemory(device->mux_device, printf_memory, device->mux_allocator);
      }

      return cl::getErrorFrom(result.error());
    }

    mux_kernel = result->mux_kernel.get();
    if (storeKernel(result->mux_executable.release(),
                    result->mux_kernel.release())) {
      return CL_OUT_OF_HOST_MEMORY;
    }
  } else {
    // Execute the kernel loaded from the pre-compiled binary instead.
    mux_kernel = kernel->device_kernel_map[device]->getPrecompiledKernel();
  }

  auto command_wait_list = convertWaitList(cl_wait_list);
  if (!command_wait_list) {
    return command_wait_list.error();
  }
  const auto wait_list_length = command_wait_list->size();

  mux_sync_point_t mux_sync_point = nullptr;
  mux_sync_point_t *out_sync_point = cl_sync_point ? &mux_sync_point : nullptr;
  mux_error = muxCommandNDRange(
      mux_command_buffer, mux_kernel, mux_execution_options, wait_list_length,
      wait_list_length ? command_wait_list->data() : nullptr, out_sync_point);
  if (mux_success != mux_error) {
    if (printf_buffer) {
      muxDestroyBuffer(device->mux_device, printf_buffer,
                       device->mux_allocator);
    }
    if (printf_memory) {
      muxFreeMemory(device->mux_device, printf_memory, device->mux_allocator);
    }

    auto error = cl::getErrorFrom(mux_error);
    return error;
  }

  if (out_sync_point) {
    if (mux_sync_points.push_back(mux_sync_point)) {
      return CL_OUT_OF_HOST_MEMORY;
    }
    *cl_sync_point = mux_sync_points.size() - 1;
  }

  // enqueue a user callback that reads the printf buffer and print the data
  // out.
  if (device_program.printf_calls.size() != 0) {
    std::unique_ptr<printf_info_t> printf_info(new printf_info_t{
        device, printf_memory, printf_buffer, buffer_group_size,
        std::vector<uint32_t>(num_groups, 0), device_program.printf_calls});

    mux_error = createPrintfCallback(mux_command_buffer, printf_info);
    OCL_ASSERT(mux_success == mux_error, "muxCommand failed!");

    // Destroy printf buffers on command-buffer destruction
    if (cargo::success != printf_buffers.emplace_back(std::move(printf_info))) {
      return CL_OUT_OF_HOST_MEMORY;
    }
  }

  auto retain = [this](cl_mem mem) { return this->retain(mem); };

  if (auto error = kernel->retainMems(command_queue, retain)) {
    return error;
  }

  if (mutable_handle) {
    auto mutable_command =
        _cl_mutable_command_khr::create(next_command_index, kernel);
    if (!mutable_command) {
      return mutable_command.error();
    }
    if (command_handles.push_back(std::move(*mutable_command))) {
      return CL_OUT_OF_HOST_MEMORY;
    }
    *mutable_handle = command_handles.back().get();
  }

  // Increment the command counter.
  ++next_command_index;

  return CL_SUCCESS;
}

namespace {
cargo::expected<mux_descriptor_info_s, cl_int> createArgumentDescriptor(
    const cl_mutable_dispatch_arg_khr &arg, cl_kernel kernel,
    cl_device_id device) {
  const auto arg_index = arg.arg_index;
  const void *arg_value = arg.arg_value;
  const auto arg_size = arg.arg_size;
  auto arg_type = kernel->GetArgType(arg_index);
  OCL_CHECK(!arg_type, return cargo::make_unexpected(arg_type.error()));

  // Construct a descriptor from this data.
  mux_descriptor_info_s descriptor;
  switch (arg_type->kind) {
    default:
      return cargo::make_unexpected(CL_INVALID_MUTABLE_COMMAND_KHR);

    // TODO CA-4144 - Support updating image arguments,
    // hardcoded to return an error for now
    case compiler::ArgumentKind::IMAGE2D:
    case compiler::ArgumentKind::IMAGE3D:
    case compiler::ArgumentKind::IMAGE2D_ARRAY:
    case compiler::ArgumentKind::IMAGE1D:
    case compiler::ArgumentKind::IMAGE1D_ARRAY:
    case compiler::ArgumentKind::IMAGE1D_BUFFER:
    case compiler::ArgumentKind::SAMPLER:
      return cargo::make_unexpected(CL_INVALID_ARG_VALUE);

    case compiler::ArgumentKind::POINTER: {
      if (arg_type->address_space == compiler::AddressSpace::GLOBAL ||
          arg_type->address_space == compiler::AddressSpace::CONSTANT) {
        OCL_CHECK(sizeof(cl_mem) != arg_size,
                  return cargo::make_unexpected(CL_INVALID_ARG_SIZE));
        // Check if the argument is nullptr or is a pointer to a nullptr.
        if (nullptr == arg_value ||
            (nullptr == *static_cast<const cl_mem *>(arg_value))) {
          descriptor.type =
              mux_descriptor_info_type_e::mux_descriptor_info_type_null_buffer;
        } else {
          cl_mem mem = *static_cast<const cl_mem *>(arg_value);
          OCL_CHECK(CL_MEM_OBJECT_BUFFER != mem->type,
                    return cargo::make_unexpected(CL_INVALID_ARG_VALUE));

          auto *buffer = static_cast<const cl_mem_buffer *>(arg_value);
          // Look up the device id from the queue associated to the
          // command buffer.
          const auto device_index =
              kernel->program->context->getDeviceIndex(device);
          auto *mux_buffer = (*buffer)->mux_buffers[device_index];

          descriptor.type =
              mux_descriptor_info_type_e::mux_descriptor_info_type_buffer;
          descriptor.buffer_descriptor =
              mux_descriptor_info_buffer_s{mux_buffer, 0};
        }
      } else if (arg_type->address_space == compiler::AddressSpace::LOCAL) {
        OCL_CHECK(nullptr != arg_value,
                  return cargo::make_unexpected(CL_INVALID_VALUE));
        OCL_CHECK(arg_size == 0,
                  return cargo::make_unexpected(CL_INVALID_ARG_SIZE));
        descriptor.type = mux_descriptor_info_type_e::
            mux_descriptor_info_type_shared_local_buffer;
        descriptor.shared_local_buffer_descriptor =
            mux_descriptor_info_shared_local_buffer_s{arg_size};
      } else {
        // Unhandled address space.
        return cargo::make_unexpected(CL_INVALID_MUTABLE_COMMAND_KHR);
      }
    } break;

      // For POD arguments we need to consider all types.
    case compiler::ArgumentKind::INT8:
    case compiler::ArgumentKind::INT8_2:
    case compiler::ArgumentKind::INT8_3:
    case compiler::ArgumentKind::INT8_4:
    case compiler::ArgumentKind::INT8_8:
    case compiler::ArgumentKind::INT8_16:
    case compiler::ArgumentKind::INT16:
    case compiler::ArgumentKind::INT16_2:
    case compiler::ArgumentKind::INT16_3:
    case compiler::ArgumentKind::INT16_4:
    case compiler::ArgumentKind::INT16_8:
    case compiler::ArgumentKind::INT16_16:
    case compiler::ArgumentKind::INT32:
    case compiler::ArgumentKind::INT32_2:
    case compiler::ArgumentKind::INT32_3:
    case compiler::ArgumentKind::INT32_4:
    case compiler::ArgumentKind::INT32_8:
    case compiler::ArgumentKind::INT32_16:
    case compiler::ArgumentKind::INT64:
    case compiler::ArgumentKind::INT64_2:
    case compiler::ArgumentKind::INT64_3:
    case compiler::ArgumentKind::INT64_4:
    case compiler::ArgumentKind::INT64_8:
    case compiler::ArgumentKind::INT64_16:
    case compiler::ArgumentKind::HALF:
    case compiler::ArgumentKind::HALF_2:
    case compiler::ArgumentKind::HALF_3:
    case compiler::ArgumentKind::HALF_4:
    case compiler::ArgumentKind::HALF_8:
    case compiler::ArgumentKind::HALF_16:
    case compiler::ArgumentKind::FLOAT:
    case compiler::ArgumentKind::FLOAT_2:
    case compiler::ArgumentKind::FLOAT_3:
    case compiler::ArgumentKind::FLOAT_4:
    case compiler::ArgumentKind::FLOAT_8:
    case compiler::ArgumentKind::FLOAT_16:
    case compiler::ArgumentKind::DOUBLE:
    case compiler::ArgumentKind::DOUBLE_2:
    case compiler::ArgumentKind::DOUBLE_3:
    case compiler::ArgumentKind::DOUBLE_4:
    case compiler::ArgumentKind::DOUBLE_8:
    case compiler::ArgumentKind::DOUBLE_16:
    case compiler::ArgumentKind::STRUCTBYVAL: {
      const auto struct_arg = kernel->saved_args[arg_index];
      const size_t original_size = struct_arg.value.size;
      OCL_CHECK(original_size != arg_size,
                return cargo::make_unexpected(CL_INVALID_ARG_SIZE));

      descriptor.type =
          mux_descriptor_info_type_e::mux_descriptor_info_type_plain_old_data;
      descriptor.plain_old_data_descriptor =
          mux_descriptor_info_plain_old_data_s{arg_value, arg_size};
    } break;
  }
  return descriptor;
}
}  // anonymous namespace

[[nodiscard]] cl_int _cl_command_buffer_khr::updateCommandBuffer(
    const cl_mutable_base_config_khr &mutable_config) {
  std::lock_guard<std::mutex> guard(mutex);
  const cl_device_id device = command_queue->device;

  cargo::array_view<const cl_mutable_dispatch_config_khr>
      mutable_dispatch_configs(mutable_config.mutable_dispatch_list,
                               mutable_config.num_mutable_dispatch);

  // Verify struct configures kernel arguments and return error if malformed
  for (const auto &config : mutable_dispatch_configs) {
    OCL_CHECK(config.type != CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR,
              return CL_INVALID_VALUE);

    OCL_CHECK(!config.command, return CL_INVALID_MUTABLE_COMMAND_KHR);
    OCL_CHECK(config.command->command_buffer != this,
              return CL_INVALID_MUTABLE_COMMAND_KHR);

    OCL_CHECK(!config.arg_list ^ !config.num_args, return CL_INVALID_VALUE);
    OCL_CHECK(!config.arg_svm_list ^ !config.num_svm_args,
              return CL_INVALID_VALUE);

    OCL_CHECK(
        !(config.command->updatable_fields & CL_MUTABLE_DISPATCH_ARGUMENTS_KHR),
        return CL_INVALID_OPERATION);
  }

  for (auto config : mutable_dispatch_configs) {
    unsigned update_index = 0;
    const unsigned num_args = config.num_args + config.num_svm_args;
    UpdateInfo update_info;
    if (update_info.descriptors.alloc(num_args)) {
      return CL_OUT_OF_HOST_MEMORY;
    }

    if (update_info.indices.alloc(num_args)) {
      return CL_OUT_OF_HOST_MEMORY;
    }

    if (update_info.pointers.alloc(config.num_svm_args)) {
      return CL_OUT_OF_HOST_MEMORY;
    }

    const auto mutable_command = config.command;
    update_info.id = mutable_command->id;
    cargo::array_view<const cl_mutable_dispatch_arg_khr> args(config.arg_list,
                                                              config.num_args);

    for (unsigned i = 0; i < config.num_args; ++i) {
      auto arg = args[i];
      update_info.indices[update_index] = arg.arg_index;
      auto descriptor =
          createArgumentDescriptor(arg, mutable_command->kernel, device);
      if (!descriptor) {
        return descriptor.error();
      }

      // Add the new descriptors to the storage.
      update_info.descriptors[update_index] = descriptor.value();
      update_index++;
    }

#ifdef OCL_EXTENSION_cl_intel_unified_shared_memory
    cargo::array_view<const cl_mutable_dispatch_arg_khr> svm_args(
        config.arg_svm_list, config.num_svm_args);

    for (unsigned i = 0; i < config.num_svm_args; ++i) {
      // Unpack the argument.
      const auto arg = svm_args[i];
      const auto arg_index = arg.arg_index;
      update_info.indices[update_index] = arg_index;
      const void *arg_value = arg.arg_value;
      auto arg_type = mutable_command->kernel->GetArgType(arg_index);
      OCL_CHECK(!arg_type, return arg_type.error());

      OCL_CHECK(arg_type->kind != compiler::ArgumentKind::POINTER,
                return CL_INVALID_MUTABLE_COMMAND_KHR);
      OCL_CHECK(!(arg_type->address_space == compiler::AddressSpace::GLOBAL ||
                  arg_type->address_space == compiler::AddressSpace::CONSTANT),
                return CL_INVALID_ARG_VALUE);

      // Construct Descriptor
      mux_descriptor_info_s descriptor;
      descriptor.type =
          mux_descriptor_info_type_e::mux_descriptor_info_type_plain_old_data;
      descriptor.plain_old_data_descriptor.data = &update_info.pointers[i];
      descriptor.plain_old_data_descriptor.length =
          sizeof update_info.pointers[i];
      update_info.pointers[i] = arg_value;
      update_info.descriptors[update_index] = descriptor;
      update_index++;
    }
#endif  // OCL_EXTENSION_cl_intel_unified_shared_memory
    if (updates.push_back(std::move(update_info))) {
      return CL_OUT_OF_HOST_MEMORY;
    }
  }
  return CL_SUCCESS;
}

bool _cl_command_buffer_khr::isQueueCompatible(
    const cl_command_queue queue) const {
  if (nullptr == queue) {
    return false;
  }

  if (queue->properties != command_queue->properties) {
    return false;
  }

  if (queue->device != command_queue->device) {
    return false;
  }

  if (queue->context != command_queue->context) {
    return false;
  }

  return true;
}

CL_API_ENTRY cl_command_buffer_khr CL_API_CALL clCreateCommandBufferKHR(
    cl_uint num_queues, const cl_command_queue *queues,
    const cl_command_buffer_properties_khr *properties, cl_int *errcode_ret) {
  tracer::TraceGuard<tracer::OpenCL> guard("clCreateCommandBufferKHR");

  OCL_CHECK(!queues, OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
            return nullptr);

  OCL_CHECK(num_queues != 1, OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
            return nullptr);

  auto queue = queues[0];
  OCL_CHECK(!queue, OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_COMMAND_QUEUE);
            return nullptr);

  // TODO: Handle event profiling via clGetEventProfilingInfo for command
  // buffers enqueue via clEnqueueCommandBufferKHR (see CA-3322).

  auto command_buffer = _cl_command_buffer_khr::create(queue, properties);
  if (!command_buffer) {
    OCL_SET_IF_NOT_NULL(errcode_ret, command_buffer.error());
    return nullptr;
  }

  OCL_SET_IF_NOT_NULL(errcode_ret, CL_SUCCESS);
  return command_buffer->release();
}

CL_API_ENTRY cl_int CL_API_CALL
clReleaseCommandBufferKHR(cl_command_buffer_khr command_buffer) {
  tracer::TraceGuard<tracer::OpenCL> guard("clReleaseCommandBufferKHR");
  OCL_CHECK(!command_buffer, return CL_INVALID_COMMAND_BUFFER_KHR);

  return cl::releaseExternal(command_buffer);
}

CL_API_ENTRY cl_int CL_API_CALL
clRetainCommandBufferKHR(cl_command_buffer_khr command_buffer) {
  tracer::TraceGuard<tracer::OpenCL> guard("clRetainCommandBufferKHR");
  OCL_CHECK(!command_buffer, return CL_INVALID_COMMAND_BUFFER_KHR);

  return cl::retainExternal(command_buffer);
}

CL_API_ENTRY cl_int CL_API_CALL
clFinalizeCommandBufferKHR(cl_command_buffer_khr command_buffer) {
  tracer::TraceGuard<tracer::OpenCL> guard("clFinalizeCommandBufferKHR");
  OCL_CHECK(!command_buffer, return CL_INVALID_COMMAND_BUFFER_KHR);

  return command_buffer->finalize();
}

CL_API_ENTRY cl_int CL_API_CALL clEnqueueCommandBufferKHR(
    cl_uint num_queues, cl_command_queue *queues,
    cl_command_buffer_khr command_buffer, cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list, cl_event *return_event) {
  tracer::TraceGuard<tracer::OpenCL> guard("clEnqueueCommandBufferKHR");

  // Verify queue arguments.
  OCL_CHECK(!queues ^ !num_queues, return CL_INVALID_COMMAND_QUEUE);
  // We currently only support one queue associated with the command buffer.
  OCL_CHECK(num_queues > 0 && num_queues != 1, return CL_INVALID_COMMAND_QUEUE);
  OCL_CHECK(queues && !command_buffer->isQueueCompatible(*queues),
            return CL_INVALID_COMMAND_QUEUE);

  OCL_CHECK(!command_buffer, return CL_INVALID_COMMAND_BUFFER_KHR);
  OCL_CHECK(!command_buffer->is_finalized, return CL_INVALID_OPERATION);

  // Command-buffer property needs to be set to support re-enqueue while
  // a previous instance is in flight
  if (!command_buffer->supportsSimultaneousUse() &&
      command_buffer->execution_refcount > 0) {
    return CL_INVALID_OPERATION;
  }

  // Substitute the queue if the user passed one.
  auto queue = queues ? *queues : command_buffer->command_queue;

  return queue->enqueueCommandBuffer(command_buffer, num_events_in_wait_list,
                                     event_wait_list, return_event);
}

CL_API_ENTRY cl_int CL_API_CALL clCommandBarrierWithWaitListKHR(
    cl_command_buffer_khr command_buffer, cl_command_queue command_queue,
    cl_uint num_sync_points_in_wait_list,
    const cl_sync_point_khr *sync_point_wait_list,
    cl_sync_point_khr *sync_point, cl_mutable_command_khr *mutable_handle) {
  tracer::TraceGuard<tracer::OpenCL> guard("clCommandBarrierWithWaitListKHR");

  OCL_CHECK(!command_buffer, return CL_INVALID_COMMAND_BUFFER_KHR);
  OCL_CHECK(command_buffer->is_finalized, return CL_INVALID_OPERATION);
  OCL_CHECK(command_queue, return CL_INVALID_COMMAND_QUEUE);
  OCL_CHECK(mutable_handle, return CL_INVALID_VALUE);
  OCL_CHECK(sync_point_wait_list && 0 == num_sync_points_in_wait_list,
            return CL_INVALID_SYNC_POINT_WAIT_LIST_KHR);
  OCL_CHECK(!sync_point_wait_list && num_sync_points_in_wait_list,
            return CL_INVALID_SYNC_POINT_WAIT_LIST_KHR);

  cargo::array_view<const cl_sync_point_khr> wait_list(
      sync_point_wait_list, num_sync_points_in_wait_list);

  // TODO: Handle mutable_handle (see CA-3347).
  (void)mutable_handle;

  return command_buffer->commandBarrierWithWaitList(wait_list, sync_point);
}

CL_API_ENTRY cl_int CL_API_CALL clCommandCopyBufferKHR(
    cl_command_buffer_khr command_buffer, cl_command_queue command_queue,
    cl_mem src_buffer, cl_mem dst_buffer, size_t src_offset, size_t dst_offset,
    size_t size, cl_uint num_sync_points_in_wait_list,
    const cl_sync_point_khr *sync_point_wait_list,
    cl_sync_point_khr *sync_point, cl_mutable_command_khr *mutable_handle) {
  tracer::TraceGuard<tracer::OpenCL> guard("clCommandCopyBufferKHR");

  OCL_CHECK(!command_buffer, return CL_INVALID_COMMAND_BUFFER_KHR);
  OCL_CHECK(command_buffer->is_finalized, return CL_INVALID_OPERATION);
  OCL_CHECK(command_queue, return CL_INVALID_COMMAND_QUEUE);
  OCL_CHECK(mutable_handle, return CL_INVALID_VALUE);
  OCL_CHECK(sync_point_wait_list && 0 == num_sync_points_in_wait_list,
            return CL_INVALID_SYNC_POINT_WAIT_LIST_KHR);
  OCL_CHECK(!sync_point_wait_list && num_sync_points_in_wait_list,
            return CL_INVALID_SYNC_POINT_WAIT_LIST_KHR);

  cl_int error = cl::validate::CopyBufferArguments(
      command_buffer->command_queue, src_buffer, dst_buffer, src_offset,
      dst_offset, size);
  OCL_CHECK(error, return error);

  cargo::array_view<const cl_sync_point_khr> wait_list(
      sync_point_wait_list, num_sync_points_in_wait_list);

  // TODO: Handle mutable_handle (see CA-3347).
  (void)mutable_handle;

  return command_buffer->commandCopyBuffer(src_buffer, dst_buffer, src_offset,
                                           dst_offset, size, wait_list,
                                           sync_point);
}

CL_API_ENTRY cl_int CL_API_CALL clCommandCopyBufferRectKHR(
    cl_command_buffer_khr command_buffer, cl_command_queue command_queue,
    cl_mem src_buffer, cl_mem dst_buffer, const size_t *src_origin,
    const size_t *dst_origin, const size_t *region, size_t src_row_pitch,
    size_t src_slice_pitch, size_t dst_row_pitch, size_t dst_slice_pitch,
    cl_uint num_sync_points_in_wait_list,
    const cl_sync_point_khr *sync_point_wait_list,
    cl_sync_point_khr *sync_point, cl_mutable_command_khr *mutable_handle) {
  tracer::TraceGuard<tracer::OpenCL> guard("clCommandCopyBufferRectKHR");

  OCL_CHECK(!command_buffer, return CL_INVALID_COMMAND_BUFFER_KHR);
  OCL_CHECK(command_buffer->is_finalized, return CL_INVALID_OPERATION);
  OCL_CHECK(command_queue, return CL_INVALID_COMMAND_QUEUE);
  OCL_CHECK(mutable_handle, return CL_INVALID_VALUE);
  OCL_CHECK(sync_point_wait_list && 0 == num_sync_points_in_wait_list,
            return CL_INVALID_SYNC_POINT_WAIT_LIST_KHR);
  OCL_CHECK(!sync_point_wait_list && num_sync_points_in_wait_list,
            return CL_INVALID_SYNC_POINT_WAIT_LIST_KHR);

  // The OpenCL spec has the following special cases for the pitches.
  if (src_row_pitch == 0) {
    src_row_pitch = region[0];
  }
  if (dst_row_pitch == 0) {
    dst_row_pitch = region[0];
  }
  if (src_slice_pitch == 0) {
    src_slice_pitch = region[1] * src_row_pitch;
  }
  if (dst_slice_pitch == 0) {
    dst_slice_pitch = region[1] * dst_row_pitch;
  }

  cl_int error = cl::validate::CopyBufferRectArguments(
      command_buffer->command_queue, src_buffer, dst_buffer, src_origin,
      dst_origin, region, src_row_pitch, src_slice_pitch, dst_row_pitch,
      dst_slice_pitch);
  OCL_CHECK(error, return error);

  cargo::array_view<const cl_sync_point_khr> wait_list(
      sync_point_wait_list, num_sync_points_in_wait_list);

  // TODO: Handle mutable_handle (see CA-3347).
  (void)mutable_handle;

  return command_buffer->commandCopyBufferRect(
      src_buffer, dst_buffer, src_origin, dst_origin, region, src_row_pitch,
      src_slice_pitch, dst_row_pitch, dst_slice_pitch, wait_list, sync_point);
}

CL_API_ENTRY cl_int CL_API_CALL clCommandCopyBufferToImageKHR(
    cl_command_buffer_khr command_buffer, cl_command_queue command_queue,
    cl_mem src_buffer, cl_mem dst_image, size_t src_offset,
    const size_t *dst_origin, const size_t *region,
    cl_uint num_sync_points_in_wait_list,
    const cl_sync_point_khr *sync_point_wait_list,
    cl_sync_point_khr *sync_point, cl_mutable_command_khr *mutable_handle) {
  tracer::TraceGuard<tracer::OpenCL> guard("clCommandCopyBufferToImageKHR");

  OCL_CHECK(!command_buffer, return CL_INVALID_COMMAND_BUFFER_KHR);
  OCL_CHECK(command_buffer->is_finalized, return CL_INVALID_OPERATION);
  OCL_CHECK(command_queue, return CL_INVALID_COMMAND_QUEUE);
  OCL_CHECK(mutable_handle, return CL_INVALID_VALUE);
  OCL_CHECK(sync_point_wait_list && 0 == num_sync_points_in_wait_list,
            return CL_INVALID_SYNC_POINT_WAIT_LIST_KHR);
  OCL_CHECK(!sync_point_wait_list && num_sync_points_in_wait_list,
            return CL_INVALID_SYNC_POINT_WAIT_LIST_KHR);

  cl_int error = cl::validate::CopyBufferToImageArguments(
      command_buffer->command_queue, src_buffer, dst_image, src_offset,
      dst_origin, region);
  OCL_CHECK(error, return error);

  cargo::array_view<const cl_sync_point_khr> wait_list(
      sync_point_wait_list, num_sync_points_in_wait_list);

  // TODO: Handle mutable_handle (see CA-3347).
  (void)mutable_handle;

  return command_buffer->commandCopyBufferToImage(
      src_buffer, dst_image, src_offset, dst_origin, region, wait_list,
      sync_point);
}

CL_API_ENTRY cl_int CL_API_CALL clCommandCopyImageKHR(
    cl_command_buffer_khr command_buffer, cl_command_queue command_queue,
    cl_mem src_image, cl_mem dst_image, const size_t *src_origin,
    const size_t *dst_origin, const size_t *region,
    cl_uint num_sync_points_in_wait_list,
    const cl_sync_point_khr *sync_point_wait_list,
    cl_sync_point_khr *sync_point, cl_mutable_command_khr *mutable_handle) {
  tracer::TraceGuard<tracer::OpenCL> guard("clCommandCopyImageKHR");

  OCL_CHECK(!command_buffer, return CL_INVALID_COMMAND_BUFFER_KHR);
  OCL_CHECK(command_buffer->is_finalized, return CL_INVALID_OPERATION);
  OCL_CHECK(command_queue, return CL_INVALID_COMMAND_QUEUE);
  OCL_CHECK(mutable_handle, return CL_INVALID_VALUE);
  OCL_CHECK(sync_point_wait_list && 0 == num_sync_points_in_wait_list,
            return CL_INVALID_SYNC_POINT_WAIT_LIST_KHR);
  OCL_CHECK(!sync_point_wait_list && num_sync_points_in_wait_list,
            return CL_INVALID_SYNC_POINT_WAIT_LIST_KHR);

  cl_int error = cl::validate::CopyImageArguments(
      command_buffer->command_queue, src_image, dst_image, src_origin,
      dst_origin, region);
  OCL_CHECK(error, return error);

  cargo::array_view<const cl_sync_point_khr> wait_list(
      sync_point_wait_list, num_sync_points_in_wait_list);

  // TODO: Handle mutable_handle (see CA-3347).
  (void)mutable_handle;

  return command_buffer->commandCopyImage(src_image, dst_image, src_origin,
                                          dst_origin, region, wait_list,
                                          sync_point);
}

CL_API_ENTRY cl_int CL_API_CALL clCommandCopyImageToBufferKHR(
    cl_command_buffer_khr command_buffer, cl_command_queue command_queue,
    cl_mem src_image, cl_mem dst_buffer, const size_t *src_origin,
    const size_t *region, size_t dst_offset,
    cl_uint num_sync_points_in_wait_list,
    const cl_sync_point_khr *sync_point_wait_list,
    cl_sync_point_khr *sync_point, cl_mutable_command_khr *mutable_handle) {
  tracer::TraceGuard<tracer::OpenCL> guard("clCommandCopyImageToBufferKHR");

  OCL_CHECK(!command_buffer, return CL_INVALID_COMMAND_BUFFER_KHR);
  OCL_CHECK(command_buffer->is_finalized, return CL_INVALID_OPERATION);
  OCL_CHECK(command_queue, return CL_INVALID_COMMAND_QUEUE);
  OCL_CHECK(mutable_handle, return CL_INVALID_VALUE);
  OCL_CHECK(sync_point_wait_list && 0 == num_sync_points_in_wait_list,
            return CL_INVALID_SYNC_POINT_WAIT_LIST_KHR);
  OCL_CHECK(!sync_point_wait_list && num_sync_points_in_wait_list,
            return CL_INVALID_SYNC_POINT_WAIT_LIST_KHR);

  cl_int error = cl::validate::CopyImageToBufferArguments(
      command_buffer->command_queue, src_image, dst_buffer, src_origin, region,
      dst_offset);
  OCL_CHECK(error, return error);

  cargo::array_view<const cl_sync_point_khr> wait_list(
      sync_point_wait_list, num_sync_points_in_wait_list);

  // TODO: Handle mutable_handle (see CA-3347).
  (void)mutable_handle;

  return command_buffer->commandCopyImageToBuffer(
      src_image, dst_buffer, src_origin, region, dst_offset, wait_list,
      sync_point);
}

CL_API_ENTRY cl_int CL_API_CALL clCommandFillBufferKHR(
    cl_command_buffer_khr command_buffer, cl_command_queue command_queue,
    cl_mem buffer, const void *pattern, size_t pattern_size, size_t offset,
    size_t size, cl_uint num_sync_points_in_wait_list,
    const cl_sync_point_khr *sync_point_wait_list,
    cl_sync_point_khr *sync_point, cl_mutable_command_khr *mutable_handle) {
  tracer::TraceGuard<tracer::OpenCL> guard("clCommandFillBufferKHR");

  OCL_CHECK(!command_buffer, return CL_INVALID_COMMAND_BUFFER_KHR);
  OCL_CHECK(command_buffer->is_finalized, return CL_INVALID_OPERATION);
  OCL_CHECK(command_queue, return CL_INVALID_COMMAND_QUEUE);
  OCL_CHECK(mutable_handle, return CL_INVALID_VALUE);
  OCL_CHECK(sync_point_wait_list && 0 == num_sync_points_in_wait_list,
            return CL_INVALID_SYNC_POINT_WAIT_LIST_KHR);
  OCL_CHECK(!sync_point_wait_list && num_sync_points_in_wait_list,
            return CL_INVALID_SYNC_POINT_WAIT_LIST_KHR);

  cl_int error =
      cl::validate::FillBufferArguments(command_buffer->command_queue, buffer,
                                        pattern, pattern_size, offset, size);
  OCL_CHECK(error, return error);

  cargo::array_view<const cl_sync_point_khr> wait_list(
      sync_point_wait_list, num_sync_points_in_wait_list);

  // TODO: Handle mutable_handle (see CA-3347).
  (void)mutable_handle;

  return command_buffer->commandFillBuffer(buffer, pattern, pattern_size,
                                           offset, size, wait_list, sync_point);
}

CL_API_ENTRY cl_int CL_API_CALL clCommandFillImageKHR(
    cl_command_buffer_khr command_buffer, cl_command_queue command_queue,
    cl_mem image, const void *fill_color, const size_t *origin,
    const size_t *region, cl_uint num_sync_points_in_wait_list,
    const cl_sync_point_khr *sync_point_wait_list,
    cl_sync_point_khr *sync_point, cl_mutable_command_khr *mutable_handle) {
  tracer::TraceGuard<tracer::OpenCL> guard("clCommandFillImageKHR");

  OCL_CHECK(!command_buffer, return CL_INVALID_COMMAND_BUFFER_KHR);
  OCL_CHECK(command_buffer->is_finalized, return CL_INVALID_OPERATION);
  OCL_CHECK(command_queue, return CL_INVALID_COMMAND_QUEUE);
  OCL_CHECK(mutable_handle, return CL_INVALID_VALUE);
  OCL_CHECK(sync_point_wait_list && 0 == num_sync_points_in_wait_list,
            return CL_INVALID_SYNC_POINT_WAIT_LIST_KHR);
  OCL_CHECK(!sync_point_wait_list && num_sync_points_in_wait_list,
            return CL_INVALID_SYNC_POINT_WAIT_LIST_KHR);

  cl_int error = cl::validate::FillImageArguments(
      command_buffer->command_queue, image, fill_color, origin, region);
  OCL_CHECK(error, return error);

  cargo::array_view<const cl_sync_point_khr> wait_list(
      sync_point_wait_list, num_sync_points_in_wait_list);

  // TODO: Handle mutable_handle (see CA-3347).
  (void)mutable_handle;

  return command_buffer->commandFillImage(image, fill_color, origin, region,
                                          wait_list, sync_point);
}

CL_API_ENTRY cl_int CL_API_CALL clCommandNDRangeKernelKHR(
    cl_command_buffer_khr command_buffer, cl_command_queue command_queue,
    const cl_ndrange_kernel_command_properties_khr *properties,
    cl_kernel kernel, cl_uint work_dim, const size_t *global_work_offset,
    const size_t *global_work_size, const size_t *local_work_size,
    cl_uint num_sync_points_in_wait_list,
    const cl_sync_point_khr *sync_point_wait_list,
    cl_sync_point_khr *sync_point, cl_mutable_command_khr *mutable_handle) {
  tracer::TraceGuard<tracer::OpenCL> guard("clCommandNDRangeKernelKHR");

  OCL_CHECK(!command_buffer, return CL_INVALID_COMMAND_BUFFER_KHR);
  OCL_CHECK(command_buffer->is_finalized, return CL_INVALID_OPERATION);
  OCL_CHECK(command_queue, return CL_INVALID_COMMAND_QUEUE);
  OCL_CHECK(!kernel, return CL_INVALID_KERNEL);
  OCL_CHECK(command_buffer->command_queue->context != kernel->program->context,
            return CL_INVALID_CONTEXT);
  OCL_CHECK(sync_point_wait_list && 0 == num_sync_points_in_wait_list,
            return CL_INVALID_SYNC_POINT_WAIT_LIST_KHR);
  OCL_CHECK(!sync_point_wait_list && num_sync_points_in_wait_list,
            return CL_INVALID_SYNC_POINT_WAIT_LIST_KHR);

  cargo::small_vector<cl_ndrange_kernel_command_properties_khr, 3>
      properties_list;

  // CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR defaults to values supported by
  // the device in CL_DEVICE_MUTABLE_DISPATCH_CAPABILITIES_KHR
  cl_mutable_dispatch_fields_khr updatable_fields =
      CL_MUTABLE_DISPATCH_ARGUMENTS_KHR;

  // Make sure passed properties are valid and store for later querying.
  if (properties && properties[0] != 0) {
#ifndef OCL_EXTENSION_cl_khr_command_buffer_mutable_dispatch
    // In base extension the only properties accepted are empty properties
    return CL_INVALID_VALUE;
#endif

    cl_ndrange_kernel_command_properties_khr seen = 0;
    auto current = properties;
    do {
      cl_command_buffer_properties_khr property = current[0];
      switch (property) {
        case CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR:
          if (0 == (seen & CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR)) {
            updatable_fields = current[1];
            if (updatable_fields) {
              // CL_MUTABLE_DISPATCH_EXEC_INFO_KHR is the largest defined value
              // in the cl_mutable_dispatch_fields_khr bitfield
              constexpr cl_mutable_dispatch_fields_khr bitfield_overflow =
                  CL_MUTABLE_DISPATCH_EXEC_INFO_KHR << 1;
              if (updatable_fields >= bitfield_overflow) {
                return CL_INVALID_VALUE;
              }
              // We only support argument update
              if (updatable_fields != CL_MUTABLE_DISPATCH_ARGUMENTS_KHR) {
                return CL_INVALID_OPERATION;
              }
            }
            seen |= property;
            break;
          }
          [[fallthrough]];
        default:
          return CL_INVALID_VALUE;
      }
      current += 2;
    } while (current[0] != 0);

    if (properties_list.assign(properties, current + 1)) {
      return CL_OUT_OF_HOST_MEMORY;
    }
  }

#ifndef OCL_EXTENSION_cl_khr_command_buffer_mutable_dispatch
  if (mutable_handle) {
    return CL_INVALID_VALUE;
  }
#endif

  cargo::array_view<const cl_sync_point_khr> wait_list(
      sync_point_wait_list, num_sync_points_in_wait_list);

  const cl_int err = command_buffer->commandNDRangeKernel(
      kernel, work_dim, global_work_offset, global_work_size, local_work_size,
      wait_list, sync_point, mutable_handle);

  if (err) {
    return err;
  }

  // Set information that can be queried by clGetMutableCommandInfo()
  if (mutable_handle) {
    cl_mutable_command_khr handle = *mutable_handle;
    handle->command_buffer = command_buffer;
    handle->properties_list = std::move(properties_list);
    handle->updatable_fields = updatable_fields;
    handle->work_dim = work_dim;

    std::copy_n(global_work_size, work_dim, std::begin(handle->global_size));

    if (global_work_offset) {
      std::copy_n(global_work_offset, work_dim,
                  std::begin(handle->work_offset));
    }

    if (local_work_size) {
      std::copy_n(local_work_size, work_dim, std::begin(handle->local_size));
    }
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL clGetCommandBufferInfoKHR(
    cl_command_buffer_khr command_buffer, cl_command_buffer_info_khr param_name,
    size_t param_value_size, void *param_value, size_t *param_value_size_ret) {
  tracer::TraceGuard<tracer::OpenCL> guard("clGetCommandBufferInfoKHR");

  OCL_CHECK(!command_buffer, return CL_INVALID_COMMAND_BUFFER_KHR);

#define COMMAND_BUFFER_INFO_CASE(TYPE, SIZE_RET, POINTER_TYPE, VALUE)   \
  case TYPE: {                                                          \
    OCL_SET_IF_NOT_NULL(param_value_size_ret, SIZE_RET);                \
    OCL_CHECK(param_value && (param_value_size < SIZE_RET),             \
              return CL_INVALID_VALUE);                                 \
    OCL_SET_IF_NOT_NULL(static_cast<POINTER_TYPE>(param_value), VALUE); \
  } break

  switch (param_name) {
    COMMAND_BUFFER_INFO_CASE(CL_COMMAND_BUFFER_NUM_QUEUES_KHR, sizeof(cl_uint),
                             cl_uint *, 1);
    COMMAND_BUFFER_INFO_CASE(CL_COMMAND_BUFFER_QUEUES_KHR,
                             sizeof(_cl_command_queue *), _cl_command_queue **,
                             command_buffer->command_queue);
    COMMAND_BUFFER_INFO_CASE(CL_COMMAND_BUFFER_REFERENCE_COUNT_KHR,
                             sizeof(cl_uint), cl_uint *,
                             command_buffer->refCountExternal());
    COMMAND_BUFFER_INFO_CASE(
        CL_COMMAND_BUFFER_STATE_KHR, sizeof(cl_command_buffer_state_khr),
        cl_command_buffer_state_khr *, command_buffer->getState());
    case CL_COMMAND_BUFFER_PROPERTIES_ARRAY_KHR: {
      const auto &properties_list = command_buffer->properties_list;
      OCL_SET_IF_NOT_NULL(
          param_value_size_ret,
          sizeof(cl_command_buffer_properties_khr) * properties_list.size());
      OCL_CHECK(param_value && param_value_size <
                                   sizeof(cl_command_buffer_properties_khr) *
                                       properties_list.size(),
                return CL_INVALID_VALUE);
      if (param_value) {
        std::copy(std::begin(properties_list), std::end(properties_list),
                  static_cast<cl_bitfield *>(param_value));
      }
    } break;
    default:
      return CL_INVALID_VALUE;
  }
#undef COMMAND_BUFFER_INFO_CASE

  return CL_SUCCESS;
}

namespace cl {
template <>
cl_int invalid<cl_command_buffer_khr>() {
  return CL_INVALID_COMMAND_BUFFER_KHR;
}
}  // namespace cl
#endif  // OCL_EXTENSION_cl_khr_command_buffer
