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

#include <CL/cl.h>
#include <cargo/attributes.h>
#include <cargo/small_vector.h>
#include <cl/buffer.h>
#include <cl/command_queue.h>
#include <cl/context.h>
#include <cl/event.h>
#include <cl/image.h>
#include <cl/mux.h>
#include <cl/platform.h>
#include <cl/validate.h>
#include <mux/mux.h>
#include <tracer/tracer.h>

// TODO: redmine(6543) ComputeAorta must not depend on host images, only
// host-side targets should include libimg/host.h. However, there should be a
// verify/fixup header and a library just for image-related verification and
// argument fixups.
#include <libimg/host.h>
#include <libimg/validate.h>

#include <cstring>
#include <memory>

_cl_mem_image::_cl_mem_image(_cl_context *context, cl_mem_flags validated_flags,
                             const cl_image_format *image_format_,
                             const cl_image_desc *image_desc_, void *host_ptr,
                             cl_mem optional_parent,
                             cargo::dynamic_array<mux_memory_t> &&mux_memories,
                             cargo::dynamic_array<mux_image_t> &&mux_images)
    : _cl_mem(context, validated_flags, 0, image_desc_->image_type,
              optional_parent, host_ptr, cl::ref_count_type::EXTERNAL,
              std::move(mux_memories)),
      image_format(*image_format_),
      image_desc(*image_desc_),
      mux_images(std::move(mux_images)) {
  if (0 == image_desc.image_row_pitch) {
    const size_t element_size = libimg::HostGetPixelSize(image_format);
    image_desc.image_row_pitch = image_desc.image_width * element_size;
  }
  if (0 == image_desc.image_slice_pitch) {
    switch (image_desc.image_type) {
      case CL_MEM_OBJECT_IMAGE3D:
      case CL_MEM_OBJECT_IMAGE2D_ARRAY:
        image_desc.image_slice_pitch =
            image_desc.image_row_pitch * image_desc.image_height;
        break;
      case CL_MEM_OBJECT_IMAGE2D:
      case CL_MEM_OBJECT_IMAGE1D:
      case CL_MEM_OBJECT_IMAGE1D_BUFFER:
        break;
      case CL_MEM_OBJECT_IMAGE1D_ARRAY:
        image_desc.image_slice_pitch = image_desc.image_row_pitch;
        break;
    }
  }

  // Calculate raw data size required for image and update _cl_mem::size.
  switch (image_desc.image_type) {
    case CL_MEM_OBJECT_IMAGE1D:  // Fallthrough
    case CL_MEM_OBJECT_IMAGE1D_BUFFER:
      size = image_desc.image_row_pitch;
      break;
    case CL_MEM_OBJECT_IMAGE2D:
      size = image_desc.image_row_pitch * image_desc.image_height;
      break;
    case CL_MEM_OBJECT_IMAGE3D:
      size = image_desc.image_slice_pitch * image_desc.image_depth;
      break;
    case CL_MEM_OBJECT_IMAGE1D_ARRAY:
      size = image_desc.image_row_pitch * image_desc.image_array_size;
      break;
    case CL_MEM_OBJECT_IMAGE2D_ARRAY:
      size = image_desc.image_slice_pitch * image_desc.image_array_size;
      break;
  }
}

_cl_mem_image::~_cl_mem_image() {
  for (size_t index = 0; index < mux_images.size(); ++index) {
    auto device = context->devices[index];
    muxDestroyImage(device->mux_device, mux_images[index],
                    device->mux_allocator);
  }
}

CL_API_ENTRY cl_mem CL_API_CALL cl::CreateImage(
    cl_context context, cl_mem_flags flags, const cl_image_format *image_format,
    const cl_image_desc *image_desc, void *host_ptr, cl_int *errcode_ret) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clCreateImage");
  OCL_CHECK(!context, OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_CONTEXT);
            return nullptr);
  cl_int error = cl::validate::ImageSupportForAnyDevice(context);
  OCL_CHECK(error, OCL_SET_IF_NOT_NULL(errcode_ret, error); return nullptr);

  error = cl::validate::MemFlags(flags, host_ptr);
  OCL_CHECK(error, OCL_SET_IF_NOT_NULL(errcode_ret, error); return nullptr);

  // TODO: Ensure that image1d_buffer inherits flags from its buffer if
  //       required. Ensure that no host_ptr is accepted.

  OCL_CHECK(!image_format, OCL_SET_IF_NOT_NULL(
                               errcode_ret, CL_INVALID_IMAGE_FORMAT_DESCRIPTOR);
            return nullptr);

  error = libimg::ValidateImageFormat(*image_format);
  OCL_CHECK(error, OCL_SET_IF_NOT_NULL(errcode_ret, error); return nullptr);

  OCL_CHECK(!image_desc,
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_IMAGE_DESCRIPTOR);
            return nullptr);
  if (CL_MEM_OBJECT_IMAGE1D_BUFFER == image_desc->image_type) {
    OCL_CHECK(!image_desc->buffer,
              OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_IMAGE_DESCRIPTOR);
              return nullptr);
  }

  for (auto device : context->devices) {
    error = libimg::ValidateImageSize(
        *image_desc, device->image2d_max_width, device->image2d_max_height,
        device->image3d_max_width, device->image3d_max_height,
        device->image3d_max_depth, device->image_max_array_size,
        device->image_max_buffer_size);
    OCL_CHECK(error, OCL_SET_IF_NOT_NULL(errcode_ret, error); return nullptr);
  }

  // We need to track the internal reference count for the parent cl_mem when
  // the image is a CL_MEM_OBJECT_IMAGE1D_BUFFER.
  cl_mem optional_parent = nullptr;

  // Translate image descriptor to core expected values.
  const uint32_t width = image_desc->image_width;
  uint32_t height = image_desc->image_height;
  uint32_t depth = image_desc->image_depth;
  uint32_t arrayLayers = 0;
  mux_image_type_e imageType;
  switch (image_desc->image_type) {
    default:  // NOTE: Already validated, case will never be hit.
    case CL_MEM_OBJECT_IMAGE1D_ARRAY:
      arrayLayers = image_desc->image_array_size;
      [[fallthrough]];
    case CL_MEM_OBJECT_IMAGE1D:
    case CL_MEM_OBJECT_IMAGE1D_BUFFER:
      height = 1;
      depth = 1;
      imageType = mux_image_type_1d;
      optional_parent = image_desc->buffer;
      break;
    case CL_MEM_OBJECT_IMAGE2D_ARRAY:
      arrayLayers = image_desc->image_array_size;
      [[fallthrough]];
    case CL_MEM_OBJECT_IMAGE2D:
      depth = 1;
      imageType = mux_image_type_2d;
      break;
    case CL_MEM_OBJECT_IMAGE3D:
      imageType = mux_image_type_3d;
      break;
  }

  // NOTE: Construct mux_image_format_e value out of cl_channel_order and
  // cl_channel_type values, see documentation.
  const mux_image_format_e imageFormat = static_cast<mux_image_format_e>(
      image_format->image_channel_order |
      (image_format->image_channel_data_type << 16));

  cargo::dynamic_array<mux_memory_t> mux_memories;
  cargo::dynamic_array<mux_image_t> mux_images;
  if (cargo::success != mux_memories.alloc(context->devices.size()) ||
      cargo::success != mux_images.alloc(context->devices.size())) {
    OCL_SET_IF_NOT_NULL(errcode_ret, CL_OUT_OF_HOST_MEMORY);
    return nullptr;
  }

  for (cl_uint index = 0; index < context->devices.size(); ++index) {
    auto device = context->devices[index];
    auto mux_error = muxCreateImage(
        device->mux_device, imageType, imageFormat, width, height, depth,
        arrayLayers, image_desc->image_row_pitch, image_desc->image_slice_pitch,
        device->mux_allocator, &mux_images[index]);
    OCL_CHECK(mux_error, OCL_SET_IF_NOT_NULL(errcode_ret,
                                             CL_MEM_OBJECT_ALLOCATION_FAILURE);
              return nullptr);
  }

  std::unique_ptr<_cl_mem_image> image(new (std::nothrow) _cl_mem_image(
      context, flags, image_format, image_desc, host_ptr, optional_parent,
      std::move(mux_memories), std::move(mux_images)));
  if (!image) {
    OCL_SET_IF_NOT_NULL(errcode_ret, CL_OUT_OF_HOST_MEMORY);
    return nullptr;
  }

  for (cl_uint index = 0; index < context->devices.size(); ++index) {
    auto device = context->devices[index];
    auto mux_image = image->mux_images[index];

    OCL_CHECK(context->devices[index]->max_mem_alloc_size <
                  mux_image->memory_requirements.size,
              OCL_SET_IF_NOT_NULL(errcode_ret, CL_OUT_OF_RESOURCES);
              return nullptr);

    image->size = mux_image->memory_requirements.size;

    uint64_t offset;
    if (CL_MEM_OBJECT_IMAGE1D_BUFFER == image_desc->image_type) {
      cl_mem_buffer buffer = static_cast<cl_mem_buffer>(image_desc->buffer);
      image->mux_memories[index] = buffer->mux_memories[index];
      // TODO: Can you actually create an image 1D buffer from a sub-buffer?
      offset = buffer->offset;
    } else {
      const cl_int error = image->allocateMemory(
          device->mux_device, mux_image->memory_requirements.supported_heaps,
          device->mux_allocator, &image->mux_memories[index]);
      OCL_CHECK(error, OCL_SET_IF_NOT_NULL(errcode_ret, error); return nullptr);
      offset = 0;
    }

    auto mux_memory = image->mux_memories[index];
    auto mux_error =
        muxBindImageMemory(device->mux_device, mux_memory, mux_image, offset);
    OCL_CHECK(mux_error, OCL_SET_IF_NOT_NULL(errcode_ret,
                                             CL_MEM_OBJECT_ALLOCATION_FAILURE);
              return nullptr);

    if (CL_MEM_COPY_HOST_PTR & flags) {
      void *data;
      mux_error = muxMapMemory(device->mux_device, mux_memory, offset,
                               mux_image->memory_requirements.size, &data);
      OCL_CHECK(mux_error, OCL_SET_IF_NOT_NULL(
                               errcode_ret, CL_MEM_OBJECT_ALLOCATION_FAILURE);
                return nullptr);

      memcpy(data, host_ptr, mux_image->memory_requirements.size);

      const mux_result_t error =
          muxFlushMappedMemoryToDevice(device->mux_device, mux_memory, offset,
                                       mux_image->memory_requirements.size);
      if (mux_success != error ||
          (mux_success != muxUnmapMemory(device->mux_device, mux_memory))) {
        OCL_SET_IF_NOT_NULL(errcode_ret, CL_MEM_OBJECT_ALLOCATION_FAILURE);
        return nullptr;
      }
    }
  }

  OCL_SET_IF_NOT_NULL(errcode_ret, CL_SUCCESS);
  return image.release();
}

CL_API_ENTRY cl_int CL_API_CALL cl::GetSupportedImageFormats(
    cl_context context, cl_mem_flags flags, cl_mem_object_type image_type,
    cl_uint num_entries, cl_image_format *image_formats,
    cl_uint *num_image_formats) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clGetSupportedImageFormats");
  (void)image_type;
  OCL_CHECK(!context, return CL_INVALID_CONTEXT);
  OCL_CHECK(!num_entries && image_formats, return CL_INVALID_VALUE);
  OCL_CHECK(cl::validate::IsInBitSet(flags, CL_MEM_READ_WRITE) &&
                cl::validate::IsInBitSet(flags, CL_MEM_READ_ONLY),
            return CL_INVALID_VALUE);
  OCL_CHECK(cl::validate::IsInBitSet(flags, CL_MEM_READ_WRITE) &&
                cl::validate::IsInBitSet(flags, CL_MEM_WRITE_ONLY),
            return CL_INVALID_VALUE);
  OCL_CHECK(cl::validate::IsInBitSet(flags, CL_MEM_USE_HOST_PTR) &&
                cl::validate::IsInBitSet(flags, CL_MEM_ALLOC_HOST_PTR),
            return CL_INVALID_VALUE);
  OCL_CHECK(cl::validate::IsInBitSet(flags, CL_MEM_COPY_HOST_PTR) &&
                cl::validate::IsInBitSet(flags, CL_MEM_USE_HOST_PTR),
            return CL_INVALID_VALUE);
  OCL_CHECK(cl::validate::IsInBitSet(flags, CL_MEM_HOST_WRITE_ONLY) &&
                cl::validate::IsInBitSet(flags, CL_MEM_HOST_READ_ONLY),
            return CL_INVALID_VALUE);
  OCL_CHECK(cl::validate::IsInBitSet(flags, CL_MEM_HOST_NO_ACCESS) &&
                cl::validate::IsInBitSet(flags, CL_MEM_HOST_WRITE_ONLY),
            return CL_INVALID_VALUE);
  OCL_CHECK(cl::validate::IsInBitSet(flags, CL_MEM_HOST_NO_ACCESS) &&
                cl::validate::IsInBitSet(flags, CL_MEM_HOST_READ_ONLY),
            return CL_INVALID_VALUE);

  mux_image_type_e imageType;
  switch (image_type) {
    default:  // Already validated, case will never be hit.
    case CL_MEM_OBJECT_IMAGE1D:
    case CL_MEM_OBJECT_IMAGE1D_BUFFER:
    case CL_MEM_OBJECT_IMAGE1D_ARRAY:
      imageType = mux_image_type_1d;
      break;
    case CL_MEM_OBJECT_IMAGE2D:
      imageType = mux_image_type_2d;
      break;
    case CL_MEM_OBJECT_IMAGE2D_ARRAY:
    case CL_MEM_OBJECT_IMAGE3D:
      imageType = mux_image_type_3d;
      break;
  }

  mux_allocation_type_e allocationType;
  if (flags & CL_MEM_USE_HOST_PTR) {
    allocationType = mux_allocation_type_alloc_host;
  } else if (flags & CL_MEM_ALLOC_HOST_PTR) {
    allocationType = mux_allocation_type_alloc_host;
  } else {
    allocationType = mux_allocation_type_alloc_device;
  }

  cargo::small_vector<mux_image_format_e, 128> muxImageFormats;
  cargo::small_vector<cl_image_format, 128> supportedImageFormats;

  for (auto device : context->devices) {
    mux_device_t muxDevice = device->mux_device;

    uint32_t imageFormatCount;
    mux_result_t muxError = muxGetSupportedImageFormats(
        muxDevice, imageType, allocationType, 0, nullptr, &imageFormatCount);
    if (muxError || 0 == imageFormatCount) {
      continue;
    }

    muxImageFormats.clear();
    if (cargo::success != muxImageFormats.resize(imageFormatCount)) {
      return CL_OUT_OF_HOST_MEMORY;
    }

    muxError = muxGetSupportedImageFormats(muxDevice, imageType, allocationType,
                                           imageFormatCount,
                                           muxImageFormats.data(), nullptr);
    if (muxError) {
      continue;
    }

    // Loop over the new image formats so we don't add duplicates to the list of
    // supported image formats we are gathering.
    for (auto &muxImageFormat : muxImageFormats) {
      // Contract the cl_image_format from a mux_image_format_e which is
      // defined as the cl_channel_order in the lower 16 bits and the
      // cl_channel_type in the upper 16 bits of the 32 bit value..
      const cl_image_format newImageFormat = {
          static_cast<cl_channel_order>(muxImageFormat & 0xffff),
          static_cast<cl_channel_type>((muxImageFormat & 0xffff0000) >> 16)};

      // Check if the image format has been added to the supported list already.
      bool addFormat = true;
      for (auto &supportedImageFormat : supportedImageFormats) {
        // GCOVR_EXCL_START non-deterministic?  I don't know why.
        if (supportedImageFormat.image_channel_order ==
                newImageFormat.image_channel_order &&
            supportedImageFormat.image_channel_data_type ==
                newImageFormat.image_channel_data_type) {
          // This format has already been added to supported list, ignore it.
          addFormat = false;
          break;
        }
        // GCOVR_EXCL_STOP
      }

      // Add the new image format to the supported list if its unique.
      if (addFormat) {
        auto error = supportedImageFormats.push_back(newImageFormat);
        if (error) {
          return CL_OUT_OF_HOST_MEMORY;
        }
      }
    }
  }

  OCL_SET_IF_NOT_NULL(num_image_formats, supportedImageFormats.size());

  if (image_formats) {
    // This for condition ensures if user provides a value < than total
    // supported formats we will only return that number. Otherwise we will
    // return the only the list of supported formats
    for (cl_uint i = 0; i < num_entries && i < supportedImageFormats.size();
         ++i) {
      image_formats[i] = supportedImageFormats[i];
    }
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL cl::EnqueueReadImage(
    cl_command_queue command_queue, cl_mem image_, cl_bool blocking_read,
    const size_t *origin, const size_t *region, size_t row_pitch,
    size_t slice_pitch, void *ptr, cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list, cl_event *event) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clEnqueueReadImage");
  OCL_CHECK(!command_queue, return CL_INVALID_COMMAND_QUEUE);
  OCL_CHECK(!command_queue->device->image_support, return CL_INVALID_OPERATION);
  OCL_CHECK(!(command_queue->context), return CL_INVALID_CONTEXT);
  OCL_CHECK(!image_, return CL_INVALID_MEM_OBJECT);
  auto image = static_cast<cl_mem_image>(image_);
  OCL_CHECK(!image->context, return CL_INVALID_CONTEXT);
  OCL_CHECK((command_queue->context != image->context),
            return CL_INVALID_CONTEXT);

  cl_int error =
      cl::validate::EventWaitList(num_events_in_wait_list, event_wait_list,
                                  command_queue->context, event, blocking_read);
  OCL_CHECK(error != CL_SUCCESS, return error);

  OCL_CHECK(!command_queue->device->image_support, return CL_INVALID_OPERATION);
  OCL_CHECK(!origin, return CL_INVALID_VALUE);
  OCL_CHECK(!region, return CL_INVALID_VALUE);

  error = libimg::ValidateOriginAndRegion(image->image_desc, origin, region);
  OCL_CHECK(error, return error);
  OCL_CHECK(!ptr, return CL_INVALID_VALUE);
  OCL_CHECK(cl::validate::IsInBitSet(image->flags, CL_MEM_HOST_WRITE_ONLY),
            return CL_INVALID_OPERATION);
  OCL_CHECK(cl::validate::IsInBitSet(image->flags, CL_MEM_HOST_NO_ACCESS),
            return CL_INVALID_OPERATION);

  error = libimg::ValidateRowAndSlicePitchForReadWriteImage(
      image->image_format, image->image_desc, region, row_pitch, slice_pitch);
  OCL_CHECK(error, return error);

  libimg::HostSetImagePitches(image->image_format, image->image_desc, region,
                              &row_pitch, &slice_pitch);

  cl_event return_event = nullptr;
  if (blocking_read || (nullptr != event)) {
    auto new_event = _cl_event::create(command_queue, CL_COMMAND_READ_IMAGE);
    if (!new_event) {
      return new_event.error();
    }
    return_event = *new_event;
  }
  cl::release_guard<cl_event> event_release_guard(return_event,
                                                  cl::ref_count_type::EXTERNAL);

  {
    const std::lock_guard<std::mutex> lock(
        command_queue->context->getCommandQueueMutex());

    auto mux_command_buffer = command_queue->getCommandBuffer(
        {event_wait_list, num_events_in_wait_list}, event_release_guard.get());
    if (!mux_command_buffer) {
      return CL_OUT_OF_RESOURCES;
    }

    auto device_index = command_queue->getDeviceIndex();
    auto mux_image = image->mux_images[device_index];
    auto mux_error = muxCommandReadImage(
        *mux_command_buffer, mux_image,
        {static_cast<uint32_t>(origin[0]), static_cast<uint32_t>(origin[1]),
         static_cast<uint32_t>(origin[2])},
        {static_cast<uint32_t>(region[0]), static_cast<uint32_t>(region[1]),
         static_cast<uint32_t>(region[2])},
        row_pitch, slice_pitch, ptr, 0, nullptr, nullptr);
    if (mux_error) {
      auto error = cl::getErrorFrom(mux_error);
      if (event_release_guard) {
        event_release_guard->complete(error);
      }
      return error;
    }

    cl::retainInternal(image);

    if (auto error = command_queue->registerDispatchCallback(
            *mux_command_buffer, return_event,
            [image]() { cl::releaseInternal(image); })) {
      return error;
    }
  }

  if (blocking_read) {
    const cl_int ret = cl::WaitForEvents(1, &event_release_guard.get());
    if (CL_SUCCESS != ret) {
      return ret;
    }
  }

  if ((nullptr != event) && event_release_guard) {
    *event = event_release_guard.dismiss();
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL cl::EnqueueWriteImage(
    cl_command_queue command_queue, cl_mem image_, cl_bool blocking_write,
    const size_t *origin, const size_t *region, size_t input_row_pitch,
    size_t input_slice_pitch, const void *ptr, cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list, cl_event *event) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clEnqueueWriteImage");
  OCL_CHECK(!command_queue, return CL_INVALID_COMMAND_QUEUE);
  OCL_CHECK(!command_queue->device->image_support, return CL_INVALID_OPERATION);
  OCL_CHECK(!(command_queue->context), return CL_INVALID_CONTEXT);
  OCL_CHECK(!image_, return CL_INVALID_MEM_OBJECT);
  auto image = static_cast<cl_mem_image>(image_);
  OCL_CHECK(!image->context, return CL_INVALID_CONTEXT);
  OCL_CHECK((command_queue->context != image->context),
            return CL_INVALID_CONTEXT);

  cl_int error = cl::validate::EventWaitList(
      num_events_in_wait_list, event_wait_list, command_queue->context, event,
      blocking_write);
  OCL_CHECK(error != CL_SUCCESS, return error);

  OCL_CHECK(!origin, return CL_INVALID_VALUE);
  OCL_CHECK(!region, return CL_INVALID_VALUE);

  error = libimg::ValidateOriginAndRegion(image->image_desc, origin, region);
  OCL_CHECK(error, return error);

  OCL_CHECK(!ptr, return CL_INVALID_VALUE);
  OCL_CHECK(cl::validate::IsInBitSet(image->flags, CL_MEM_HOST_READ_ONLY),
            return CL_INVALID_OPERATION);
  OCL_CHECK(cl::validate::IsInBitSet(image->flags, CL_MEM_HOST_NO_ACCESS),
            return CL_INVALID_OPERATION);

  error = libimg::ValidateRowAndSlicePitchForReadWriteImage(
      image->image_format, image->image_desc, region, input_row_pitch,
      input_slice_pitch);
  OCL_CHECK(error, return error);

  libimg::HostSetImagePitches(image->image_format, image->image_desc, region,
                              &input_row_pitch, &input_slice_pitch);

  cl_event return_event = nullptr;
  if (blocking_write || (nullptr != event)) {
    auto new_event = _cl_event::create(command_queue, CL_COMMAND_WRITE_IMAGE);
    if (!new_event) {
      return new_event.error();
    }
    return_event = *new_event;
  }
  cl::release_guard<cl_event> event_release_guard(return_event,
                                                  cl::ref_count_type::EXTERNAL);

  {
    const std::lock_guard<std::mutex> lock(
        command_queue->context->getCommandQueueMutex());

    auto mux_command_buffer = command_queue->getCommandBuffer(
        {event_wait_list, num_events_in_wait_list}, event_release_guard.get());
    if (!mux_command_buffer) {
      return CL_OUT_OF_RESOURCES;
    }

    auto device_index = command_queue->getDeviceIndex();
    auto mux_image = image->mux_images[device_index];
    auto mux_error = muxCommandWriteImage(
        *mux_command_buffer, mux_image,
        {static_cast<uint32_t>(origin[0]), static_cast<uint32_t>(origin[1]),
         static_cast<uint32_t>(origin[2])},
        {static_cast<uint32_t>(region[0]), static_cast<uint32_t>(region[1]),
         static_cast<uint32_t>(region[2])},
        input_row_pitch, input_slice_pitch, ptr, 0, nullptr, nullptr);
    if (mux_error) {
      auto error = cl::getErrorFrom(mux_error);
      if (event_release_guard) {
        event_release_guard->complete(error);
      }
      return error;
    }

    cl::retainInternal(image);

    if (auto error = command_queue->registerDispatchCallback(
            *mux_command_buffer, return_event,
            [image]() { cl::releaseInternal(image); })) {
      return error;
    }
  }

  if (blocking_write) {
    const cl_int ret = cl::WaitForEvents(1, &event_release_guard.get());
    if (CL_SUCCESS != ret) {
      return ret;
    }
  }

  if ((nullptr != event) && event_release_guard) {
    *event = event_release_guard.dismiss();
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_int CL_API_CALL cl::EnqueueFillImage(
    cl_command_queue command_queue, cl_mem image_, const void *fill_color,
    const size_t *origin, const size_t *region, cl_uint num_events_in_wait_list,
    const cl_event *event_wait_list, cl_event *event) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clEnqueueFillImage");
  OCL_CHECK(!command_queue, return CL_INVALID_COMMAND_QUEUE);

  cl_int error = cl::validate::FillImageArguments(command_queue, image_,
                                                  fill_color, origin, region);
  OCL_CHECK(error, return error);

  error = cl::validate::EventWaitList(num_events_in_wait_list, event_wait_list,
                                      command_queue->context, event);
  OCL_CHECK(error, return error);

  cl_event return_event = nullptr;
  if (nullptr != event) {
    auto new_event = _cl_event::create(command_queue, CL_COMMAND_FILL_IMAGE);
    if (!new_event) {
      return new_event.error();
    }
    return_event = *new_event;
    *event = return_event;
  }

  const std::lock_guard<std::mutex> lock(
      command_queue->context->getCommandQueueMutex());

  auto mux_command_buffer = command_queue->getCommandBuffer(
      {event_wait_list, num_events_in_wait_list}, return_event);
  if (!mux_command_buffer) {
    return CL_OUT_OF_RESOURCES;
  }

  auto device_index = command_queue->getDeviceIndex();
  auto image = static_cast<cl_mem_image>(image_);
  auto mux_image = image->mux_images[device_index];
  auto mux_error = muxCommandFillImage(
      *mux_command_buffer, mux_image, fill_color, sizeof(float) * 4,
      {static_cast<uint32_t>(origin[0]), static_cast<uint32_t>(origin[1]),
       static_cast<uint32_t>(origin[2])},
      {static_cast<uint32_t>(region[0]), static_cast<uint32_t>(region[1]),
       static_cast<uint32_t>(region[2])},
      0, nullptr, nullptr);
  if (mux_error) {
    auto error = cl::getErrorFrom(mux_error);
    if (return_event) {
      return_event->complete(error);
    }
    return error;
  }

  cl::retainInternal(image);

  return command_queue->registerDispatchCallback(
      *mux_command_buffer, return_event,
      [image]() { cl::releaseInternal(image); });
}

CL_API_ENTRY cl_int CL_API_CALL cl::EnqueueCopyImage(
    cl_command_queue command_queue, cl_mem src_image_, cl_mem dst_image_,
    const size_t *src_origin, const size_t *dst_origin, const size_t *region,
    cl_uint num_events_in_wait_list, const cl_event *event_wait_list,
    cl_event *event) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clEnqueueCopyImage");
  OCL_CHECK(!command_queue, return CL_INVALID_COMMAND_QUEUE);

  cl_int error = cl::validate::CopyImageArguments(
      command_queue, src_image_, dst_image_, src_origin, dst_origin, region);
  OCL_CHECK(error, return error);

  error = cl::validate::EventWaitList(num_events_in_wait_list, event_wait_list,
                                      command_queue->context, event);
  OCL_CHECK(error, return error);

  cl_event return_event = nullptr;
  if (nullptr != event) {
    auto new_event = _cl_event::create(command_queue, CL_COMMAND_COPY_IMAGE);
    if (!new_event) {
      return new_event.error();
    }
    return_event = *new_event;
    *event = return_event;
  }

  const std::lock_guard<std::mutex> lock(
      command_queue->context->getCommandQueueMutex());

  auto mux_command_buffer = command_queue->getCommandBuffer(
      {event_wait_list, num_events_in_wait_list}, return_event);
  if (!mux_command_buffer) {
    return CL_OUT_OF_RESOURCES;
  }

  auto device_index = command_queue->getDeviceIndex();

  auto src_image = static_cast<cl_mem_image>(src_image_);
  auto dst_image = static_cast<cl_mem_image>(dst_image_);
  auto mux_src_image = src_image->mux_images[device_index];
  auto mux_dst_image = dst_image->mux_images[device_index];
  auto mux_error = muxCommandCopyImage(
      *mux_command_buffer, mux_src_image, mux_dst_image,
      {static_cast<uint32_t>(src_origin[0]),
       static_cast<uint32_t>(src_origin[1]),
       static_cast<uint32_t>(src_origin[2])},
      {static_cast<uint32_t>(dst_origin[0]),
       static_cast<uint32_t>(dst_origin[1]),
       static_cast<uint32_t>(dst_origin[2])},
      {static_cast<uint32_t>(region[0]), static_cast<uint32_t>(region[1]),
       static_cast<uint32_t>(region[2])},
      0, nullptr, nullptr);
  if (mux_error) {
    auto error = cl::getErrorFrom(mux_error);
    if (return_event) {
      return_event->complete(error);
    }
    return error;
  }

  cl::retainInternal(src_image);
  cl::retainInternal(dst_image);

  return command_queue->registerDispatchCallback(
      *mux_command_buffer, return_event, [src_image, dst_image]() {
        cl::releaseInternal(src_image);
        cl::releaseInternal(dst_image);
      });
}

CL_API_ENTRY cl_int CL_API_CALL cl::EnqueueCopyImageToBuffer(
    cl_command_queue command_queue, cl_mem src_image_, cl_mem dst_buffer_,
    const size_t *src_origin, const size_t *region, size_t dst_offset,
    cl_uint num_events_in_wait_list, const cl_event *event_wait_list,
    cl_event *event) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clEnqueueCopyImageToBuffer");

  OCL_CHECK(!command_queue, return CL_INVALID_COMMAND_QUEUE);
  cl_int error = cl::validate::CopyImageToBufferArguments(
      command_queue, src_image_, dst_buffer_, src_origin, region, dst_offset);
  OCL_CHECK(error, return error);

  error = cl::validate::EventWaitList(num_events_in_wait_list, event_wait_list,
                                      command_queue->context, event);
  OCL_CHECK(error, return error);

  cl_event return_event = nullptr;
  if (nullptr != event) {
    auto new_event =
        _cl_event::create(command_queue, CL_COMMAND_COPY_IMAGE_TO_BUFFER);
    if (!new_event) {
      return new_event.error();
    }
    return_event = *new_event;
    *event = return_event;
  }

  const std::lock_guard<std::mutex> lock(
      command_queue->context->getCommandQueueMutex());

  auto mux_command_buffer = command_queue->getCommandBuffer(
      {event_wait_list, num_events_in_wait_list}, return_event);
  if (!mux_command_buffer) {
    return CL_OUT_OF_RESOURCES;
  }

  auto device_index = command_queue->getDeviceIndex();
  auto src_image = static_cast<cl_mem_image>(src_image_);
  auto dst_buffer = static_cast<cl_mem_buffer>(dst_buffer_);
  auto mux_src_image = src_image->mux_images[device_index];
  auto mux_dst_buffer = dst_buffer->mux_buffers[device_index];
  auto mux_error = muxCommandCopyImageToBuffer(
      *mux_command_buffer, mux_src_image, mux_dst_buffer,
      {static_cast<uint32_t>(src_origin[0]),
       static_cast<uint32_t>(src_origin[1]),
       static_cast<uint32_t>(src_origin[2])},
      dst_offset,
      {static_cast<uint32_t>(region[0]), static_cast<uint32_t>(region[1]),
       static_cast<uint32_t>(region[2])},
      0, nullptr, nullptr);
  if (mux_error) {
    auto error = cl::getErrorFrom(mux_error);
    if (return_event) {
      return_event->complete(error);
    }
    return error;
  }

  cl::retainInternal(src_image);
  cl::retainInternal(dst_buffer);

  return command_queue->registerDispatchCallback(
      *mux_command_buffer, return_event, [src_image, dst_buffer]() {
        cl::releaseInternal(src_image);
        cl::releaseInternal(dst_buffer);
      });
}

CL_API_ENTRY cl_int CL_API_CALL cl::EnqueueCopyBufferToImage(
    cl_command_queue command_queue, cl_mem src_buffer_, cl_mem dst_image_,
    size_t src_offset, const size_t *dst_origin, const size_t *region,
    cl_uint num_events_in_wait_list, const cl_event *event_wait_list,
    cl_event *event) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clEnqueueCopyBufferToImage");

  OCL_CHECK(!command_queue, return CL_INVALID_COMMAND_QUEUE);
  cl_int error = cl::validate::CopyBufferToImageArguments(
      command_queue, src_buffer_, dst_image_, src_offset, dst_origin, region);
  OCL_CHECK(error, return error);

  error = cl::validate::EventWaitList(num_events_in_wait_list, event_wait_list,
                                      command_queue->context, event);
  OCL_CHECK(error, return error);

  cl_event return_event = nullptr;
  if (nullptr != event) {
    auto new_event =
        _cl_event::create(command_queue, CL_COMMAND_COPY_BUFFER_TO_IMAGE);
    if (!new_event) {
      return new_event.error();
    }
    return_event = *new_event;
    *event = return_event;
  }

  const std::lock_guard<std::mutex> lock(
      command_queue->context->getCommandQueueMutex());

  auto mux_command_buffer = command_queue->getCommandBuffer(
      {event_wait_list, num_events_in_wait_list}, return_event);
  if (!mux_command_buffer) {
    return CL_OUT_OF_RESOURCES;
  }

  auto device_index = command_queue->getDeviceIndex();

  auto src_buffer = static_cast<cl_mem_buffer>(src_buffer_);
  auto dst_image = static_cast<cl_mem_image>(dst_image_);
  auto mux_src_buffer = src_buffer->mux_buffers[device_index];
  auto mux_dst_image = dst_image->mux_images[device_index];
  auto mux_error = muxCommandCopyBufferToImage(
      *mux_command_buffer, mux_src_buffer, mux_dst_image, src_offset,
      {static_cast<uint32_t>(dst_origin[0]),
       static_cast<uint32_t>(dst_origin[1]),
       static_cast<uint32_t>(dst_origin[2])},
      {static_cast<uint32_t>(region[0]), static_cast<uint32_t>(region[1]),
       static_cast<uint32_t>(region[2])},
      0, nullptr, nullptr);
  if (mux_error) {
    auto error = cl::getErrorFrom(mux_error);
    if (return_event) {
      return_event->complete(error);
    }
    return error;
  }

  cl::retainInternal(src_buffer);
  cl::retainInternal(dst_image);

  return command_queue->registerDispatchCallback(
      *mux_command_buffer, return_event, [src_buffer, dst_image]() {
        cl::releaseInternal(src_buffer);
        cl::releaseInternal(dst_image);
      });
}

CL_API_ENTRY void *CL_API_CALL cl::EnqueueMapImage(
    cl_command_queue command_queue, cl_mem memobj, cl_bool blocking_map,
    cl_map_flags map_flags, const size_t *origin, const size_t *region,
    size_t *image_row_pitch, size_t *image_slice_pitch,
    cl_uint num_events_in_wait_list, const cl_event *event_wait_list,
    cl_event *event, cl_int *errcode_ret) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clEnqueueMapImage");
  // TODO: Extract commonalities with clEnqueueMapBuffer and call into them
  //       from both functions.

  OCL_CHECK(!command_queue,
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_COMMAND_QUEUE);
            return nullptr);
  OCL_CHECK(!memobj, OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_MEM_OBJECT);
            return nullptr);
  OCL_CHECK(command_queue->context != memobj->context,
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_CONTEXT);
            return nullptr);

  OCL_CHECK(CL_MEM_OBJECT_BUFFER == memobj->type,
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_MEM_OBJECT);
            return nullptr);

  auto image = static_cast<cl_mem_image>(memobj);

  // TODO: Check if libimg::ValidateOriginAndRegion checks for nullptr and then
  //       remove this check.
  OCL_CHECK(!origin, OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
            return nullptr);

  // TODO: Check if libimg::ValidateOriginAndRegion checks for nullptr and then
  //       remove this check.
  OCL_CHECK(!region, OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
            return nullptr);

  // TODO: Ensure that the following validation also checks origin and region
  //       to not be nullptr.
  cl_int error =
      libimg::ValidateOriginAndRegion(image->image_desc, origin, region);
  OCL_CHECK(CL_SUCCESS != error, OCL_SET_IF_NOT_NULL(errcode_ret, error);
            return nullptr);

  // TODO: Rename the libimg function as this is HostImage independent
  //       and can also be used when device-specific images are used.
  const size_t origin_offset = libimg::HostGetImageOriginOffset(
      image->image_format, image->image_desc, origin);
  size_t end_pixel[3];
  end_pixel[0] = origin[0] + region[0] - 1;
  end_pixel[1] = origin[1] + region[1] - 1;
  end_pixel[2] = origin[2] + region[2] - 1;

  const size_t end_pixel_offset = libimg::HostGetImageOriginOffset(
      image->image_format, image->image_desc, end_pixel);
  const size_t region_total_bytes =
      end_pixel_offset - origin_offset +
      libimg::HostGetPixelSize(image->image_format);

  // Check for overflow.
  //
  // TODO: Check if the origin and region validation ensures that no overflow
  //       can happen and then remove this extra check.
  OCL_CHECK(origin_offset > origin_offset + region_total_bytes,
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_OUT_OF_RESOURCES);
            return nullptr);
  OCL_CHECK(image->size < origin_offset || image->size < region_total_bytes ||
                image->size < (origin_offset + region_total_bytes),
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
            return nullptr);

  if (cl_map_flags(0) == map_flags) {
    // https://cvs.khronos.org/bugzilla/show_bug.cgi?id=7390 states that a
    // map flag of zero is an implicit read and write mapping.
    map_flags = CL_MAP_READ | CL_MAP_WRITE;
  }
  const bool read_access = cl::validate::IsInBitSet(map_flags, CL_MAP_READ);
  const bool write_access = cl::validate::IsInBitSet(map_flags, CL_MAP_WRITE);
  const bool write_invalidate_region_access =
      cl::validate::IsInBitSet(map_flags, CL_MAP_WRITE_INVALIDATE_REGION);
  OCL_CHECK(map_flags &
                ~(CL_MAP_READ | CL_MAP_WRITE | CL_MAP_WRITE_INVALIDATE_REGION),
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
            return nullptr);
  OCL_CHECK((read_access || write_access) && write_invalidate_region_access,
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
            return nullptr);
  OCL_CHECK(
      read_access &&
          (cl::validate::IsInBitSet(image->flags, CL_MEM_HOST_WRITE_ONLY) ||
           cl::validate::IsInBitSet(image->flags, CL_MEM_HOST_NO_ACCESS)),
      OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_OPERATION);
      return nullptr);
  OCL_CHECK(
      (write_access || write_invalidate_region_access) &&
          (cl::validate::IsInBitSet(image->flags, CL_MEM_HOST_READ_ONLY) ||
           cl::validate::IsInBitSet(image->flags, CL_MEM_HOST_NO_ACCESS)),
      OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_OPERATION);
      return nullptr);
  OCL_CHECK(CL_SUCCESS != error, OCL_SET_IF_NOT_NULL(errcode_ret, error);
            return nullptr);

  OCL_CHECK(nullptr == image_row_pitch,
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
            return nullptr);

  const bool image_slice_pitch_required =
      CL_MEM_OBJECT_IMAGE3D == image->type ||
      CL_MEM_OBJECT_IMAGE1D_ARRAY == image->type ||
      CL_MEM_OBJECT_IMAGE2D_ARRAY == image->type;

  OCL_CHECK(image_slice_pitch_required && nullptr == image_slice_pitch,
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_VALUE);
            return nullptr);

  error =
      cl::validate::EventWaitList(num_events_in_wait_list, event_wait_list,
                                  command_queue->context, event, blocking_map);
  OCL_CHECK(CL_SUCCESS != error, OCL_SET_IF_NOT_NULL(errcode_ret, error);
            return nullptr);

  OCL_CHECK(!command_queue->device->image_support,
            OCL_SET_IF_NOT_NULL(errcode_ret, CL_INVALID_OPERATION);
            return nullptr);

  auto device = command_queue->device;
  error = libimg::ValidateImageSize(
      image->image_desc, device->image2d_max_width, device->image2d_max_height,
      device->image3d_max_width, device->image3d_max_height,
      device->image3d_max_depth, device->image_max_array_size,
      device->image_max_buffer_size);
  OCL_CHECK(error, OCL_SET_IF_NOT_NULL(errcode_ret, error); return nullptr);

  cl_event map_completion_event = nullptr;
  if (blocking_map || (nullptr != event)) {
    auto new_event = _cl_event::create(command_queue, CL_COMMAND_MAP_IMAGE);
    if (!new_event) {
      OCL_SET_IF_NOT_NULL(errcode_ret, new_event.error());
      return nullptr;
    }
    map_completion_event = *new_event;
  }
  cl::release_guard<cl_event> event_release_guard(map_completion_event,
                                                  cl::ref_count_type::EXTERNAL);

  void *mappedImage = nullptr;
  error = image->pushMapMemory(
      command_queue, &mappedImage, origin_offset, region_total_bytes,
      read_access, write_access, write_invalidate_region_access,
      {event_wait_list, num_events_in_wait_list}, event_release_guard.get());
  if (error) {
    OCL_SET_IF_NOT_NULL(errcode_ret, error);
    return nullptr;
  }

  if (blocking_map) {
    const cl_int ret = cl::WaitForEvents(1, &event_release_guard.get());
    if (CL_SUCCESS != ret) {
      OCL_SET_IF_NOT_NULL(errcode_ret, ret);
      return nullptr;
    }
  }

  if ((nullptr != event) && event_release_guard) {
    *event = event_release_guard.dismiss();
  }

  *image_row_pitch = image->image_desc.image_row_pitch;
  OCL_SET_IF_NOT_NULL(image_slice_pitch, image->image_desc.image_slice_pitch);

  OCL_SET_IF_NOT_NULL(errcode_ret, CL_SUCCESS);
  return mappedImage;
}

CL_API_ENTRY cl_int CL_API_CALL cl::GetImageInfo(cl_mem image,
                                                 cl_image_info param_name,
                                                 size_t param_value_size,
                                                 void *param_value,
                                                 size_t *param_value_size_ret) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clGetImageInfo");
  OCL_CHECK(!image, return CL_INVALID_MEM_OBJECT);
#define IMAGE_INFO_CASE(TYPE, SIZE_RET, POINTER, VALUE)              \
  case TYPE: {                                                       \
    OCL_SET_IF_NOT_NULL(param_value_size_ret, SIZE_RET);             \
    OCL_CHECK(param_value &&param_value_size < SIZE_RET,             \
              return CL_INVALID_VALUE);                              \
    OCL_SET_IF_NOT_NULL((static_cast<POINTER>(param_value)), VALUE); \
  } break

  auto ocl_image = static_cast<cl_mem_image>(image);
  switch (param_name) {
    default: {
      return extension::GetImageInfo(image, param_name, param_value_size,
                                     param_value, param_value_size_ret);
    }
      IMAGE_INFO_CASE(CL_IMAGE_FORMAT, sizeof(cl_image_format),
                      cl_image_format *, ocl_image->image_format);
      IMAGE_INFO_CASE(CL_IMAGE_ELEMENT_SIZE, sizeof(size_t), size_t *,
                      libimg::HostGetPixelSize(ocl_image->image_format));
      IMAGE_INFO_CASE(CL_IMAGE_ROW_PITCH, sizeof(size_t), size_t *,
                      ocl_image->image_desc.image_row_pitch);
      IMAGE_INFO_CASE(CL_IMAGE_SLICE_PITCH, sizeof(size_t), size_t *,
                      ocl_image->image_desc.image_slice_pitch);
      IMAGE_INFO_CASE(CL_IMAGE_WIDTH, sizeof(size_t), size_t *,
                      ocl_image->image_desc.image_width);
      IMAGE_INFO_CASE(CL_IMAGE_HEIGHT, sizeof(size_t), size_t *,
                      ocl_image->image_desc.image_height);
      IMAGE_INFO_CASE(CL_IMAGE_DEPTH, sizeof(size_t), size_t *,
                      ocl_image->image_desc.image_depth);
      IMAGE_INFO_CASE(CL_IMAGE_ARRAY_SIZE, sizeof(size_t), size_t *,
                      ocl_image->image_desc.image_array_size);
      IMAGE_INFO_CASE(CL_IMAGE_BUFFER, sizeof(cl_mem), cl_mem *,
                      ocl_image->image_desc.buffer);
      IMAGE_INFO_CASE(CL_IMAGE_NUM_MIP_LEVELS, sizeof(cl_uint), cl_uint *,
                      ocl_image->image_desc.num_mip_levels);
      IMAGE_INFO_CASE(CL_IMAGE_NUM_SAMPLES, sizeof(cl_uint), cl_uint *,
                      ocl_image->image_desc.num_samples);
  }
#undef IMAGE_OBJECT_INFO_CASE

  return CL_SUCCESS;
}

/* OpenCL 1.2 introduces the following new mem flags:
 * - CL_MEM_HOST_WRITE_ONLY,
 * - CL_MEM_HOST_READ_ONLY,
 * - CL_MEM_HOST_NO_ACCESS.
 * We can not allow any of that flags to be used with pre 1.2, deprecated calls
 * to create image (clCreateImage2D, clCreateImage3D).
 */
static cl_int ValidatePreOpenCL12MemoryFlags(const cl_mem_flags flags) {
  const bool hostWrite =
      cl::validate::IsInBitSet(flags, CL_MEM_HOST_WRITE_ONLY);
  const bool hostRead = cl::validate::IsInBitSet(flags, CL_MEM_HOST_READ_ONLY);
  const bool hostNoAccess =
      cl::validate::IsInBitSet(flags, CL_MEM_HOST_NO_ACCESS);

  if (hostWrite || hostRead || hostNoAccess) {
    return CL_INVALID_VALUE;
  }

  return CL_SUCCESS;
}

CL_API_ENTRY cl_mem CL_API_CALL cl::CreateImage2D(
    cl_context context, cl_mem_flags flags, const cl_image_format *image_format,
    size_t image_width, size_t image_height, size_t image_row_pitch,
    void *host_ptr, cl_int *errcode_ret) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clCreateImage2D");
  const cl_int pre12Flags = ValidatePreOpenCL12MemoryFlags(flags);
  OCL_CHECK(pre12Flags, OCL_SET_IF_NOT_NULL(errcode_ret, pre12Flags);
            return nullptr);

  cl_image_desc image_desc;
  image_desc.image_type = CL_MEM_OBJECT_IMAGE2D;
  image_desc.image_width = image_width;
  image_desc.image_height = image_height;
  image_desc.image_depth = 0;
  image_desc.image_array_size = 0;
  image_desc.image_row_pitch = image_row_pitch;
  image_desc.image_slice_pitch = 0;
  image_desc.num_mip_levels = 0;
  image_desc.num_samples = 0;
  image_desc.buffer = nullptr;

  const cl_mem res = cl::CreateImage(context, flags, image_format, &image_desc,
                                     host_ptr, errcode_ret);
  if (errcode_ret) {
    if (CL_INVALID_IMAGE_DESCRIPTOR == *errcode_ret) {
      *errcode_ret = CL_INVALID_IMAGE_SIZE;
    }
  }

  return res;
}

CL_API_ENTRY cl_mem CL_API_CALL cl::CreateImage3D(
    cl_context context, cl_mem_flags flags, const cl_image_format *image_format,
    size_t image_width, size_t image_height, size_t image_depth,
    size_t image_row_pitch, size_t image_slice_pitch, void *host_ptr,
    cl_int *errcode_ret) {
  const tracer::TraceGuard<tracer::OpenCL> guard("clCreateImage3D");
  const cl_int pre12Flags = ValidatePreOpenCL12MemoryFlags(flags);
  OCL_CHECK(pre12Flags, OCL_SET_IF_NOT_NULL(errcode_ret, pre12Flags);
            return nullptr);

  cl_image_desc image_desc;
  image_desc.image_type = CL_MEM_OBJECT_IMAGE3D;
  image_desc.image_width = image_width;
  image_desc.image_height = image_height;
  image_desc.image_depth = image_depth;
  image_desc.image_array_size = 0;
  image_desc.image_row_pitch = image_row_pitch;
  image_desc.image_slice_pitch = image_slice_pitch;
  image_desc.num_mip_levels = 0;
  image_desc.num_samples = 0;
  image_desc.buffer = nullptr;

  const cl_mem res = cl::CreateImage(context, flags, image_format, &image_desc,
                                     host_ptr, errcode_ret);
  if (errcode_ret) {
    if (CL_INVALID_IMAGE_DESCRIPTOR == *errcode_ret) {
      *errcode_ret = CL_INVALID_IMAGE_SIZE;
    }
  }

  return res;
}
