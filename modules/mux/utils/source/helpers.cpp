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

#include <cargo/small_vector.h>
#include <mux/utils/helpers.h>
#include <stdint.h>
#include <stdlib.h>

#include <algorithm>

uint32_t mux::findFirstSupportedHeap(uint32_t heaps) {
  // https://graphics.stanford.edu/~seander/bithacks.html#ZerosOnRightMultLookup
  static const uint32_t lookupTable[32] = {
      0,  1,  28, 2,  29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4,  8,
      31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6,  11, 5,  10, 9};
  return 0x1 << lookupTable[(((heaps & -heaps) * 0x077CB531U)) >> 27];
}

cargo::string_view mux::detectOpenCLProfile(mux_device_info_t device) {
  bool is_full_profile = true;
  // CL_DEVICE_MAX_MEM_ALLOC_SIZE
  is_full_profile &= device->allocation_size >= 128 * 1024L * 1024L;
  // CL_DEVICE_MAX_PARAMETER_SIZE - not in mux
  // CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE - not in mux
  // CL_DEVICE_MAX_CONSTANT_ARGS - not in mux
  // CL_DEVICE_LOCAL_MEM_SIZE
  is_full_profile &= device->shared_local_memory_size >= 32 * 1024L;
  // CL_DEVICE_PRINTF_BUFFER_SIZE - not in mux
  if (device->image_support) {
    // CL_DEVICE_MAX_READ_IMAGE_ARGS
    is_full_profile &= device->max_sampled_images >= 128;
    // CL_DEVICE_MAX_WRITE_IMAGE_ARGS
    is_full_profile &= device->max_storage_images >= 8;
    // CL_DEVICE_MAX_SAMPLERS
    is_full_profile &= device->max_samplers >= 16;
    // CL_DEVICE_IMAGE2D_MAX_WIDTH, CL_DEVICE_IMAGE2D_MAX_HEIGHT
    is_full_profile &= device->max_image_dimension_2d >= 8192;
    // CL_DEVICE_IMAGE3D_MAX_WIDTH, CL_DEVICE_IMAGE3D_MAX_HEIGHT,
    // CL_DEVICE_IMAGE3D_MAX_DEPTH
    is_full_profile &= device->max_image_dimension_3d >= 2048;
    // CL_DEVICE_IMAGE_MAX_BUFFER_SIZE
    is_full_profile &= device->max_image_dimension_1d >= 65536;
    // CL_DEVICE_IMAGE_MAX_ARRAY_SIZE
    is_full_profile &= device->max_image_array_layers >= 2048;
  }

  return is_full_profile ? "FULL_PROFILE" : "EMBEDDED_PROFILE";
}

void *mux::alloc(void *, size_t size, size_t alignment) {
  void *pointer = 0;

  // our minimum alignment is the pointer width
  alignment = std::max(alignment, sizeof(void *));

  size_t v = alignment;

  // http://graphics.stanford.edu/~seander/bithacks.html#DetermineIfPowerOf2
  if (!(v && !(v & (v - 1)))) {
    // http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
#if SIZE_MAX > 4294967295
    static_assert(8 == sizeof(size_t), "Can only shift 64-bit integers by 32");
    v |= v >> 32;
#endif
    v++;
  }

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
  pointer = _aligned_malloc(size, v);
#else
  if (0 != posix_memalign(&pointer, v, size)) {
    pointer = 0;
  }
#endif

  return pointer;
}

void mux::free(void *user_data, void *pointer) {
  (void)user_data;

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__)
  _aligned_free(pointer);
#else
  ::free(pointer);
#endif
}

mux_result_t mux::synchronizeMemory(mux_device_t src_device,
                                    mux_device_t dst_device,
                                    mux_memory_t src_memory,
                                    mux_memory_t dst_memory, void *src_host_ptr,
                                    void *dst_host_ptr, uint64_t offset,
                                    uint64_t size) {
  // Check if the caller passed an already mapped host pointer for the source.
  void *src = nullptr;
  if (src_host_ptr) {
    src = src_host_ptr;
  } else {
    if (const auto error =
            muxMapMemory(src_device, src_memory, offset, size, &src)) {
      return error;
    }
  }

  // TODO: Check if flushing is required.
  if (const auto error = muxFlushMappedMemoryFromDevice(src_device, src_memory,
                                                        offset, size)) {
    return error;
  }

  // Check if the caller passed an already mapped host pointer for the
  // destination.
  void *dst = nullptr;
  if (dst_host_ptr) {
    dst = dst_host_ptr;
  } else {
    if (const auto error =
            muxMapMemory(dst_device, dst_memory, offset, size, &dst)) {
      return error;
    }
  }

  // TODO: Check if flushing is required.
  if (const auto error = muxFlushMappedMemoryFromDevice(dst_device, dst_memory,
                                                        offset, size)) {
    return error;
  }

  // Perform the synchronizing copy.
  std::memcpy(dst, src, size);

  // TODO: Check if flushing is required.
  if (const auto error =
          muxFlushMappedMemoryToDevice(dst_device, dst_memory, offset, size)) {
    return error;
  }

  if (!dst_host_ptr) {
    if (const auto error = muxUnmapMemory(dst_device, dst_memory)) {
      return error;
    }
  }

  if (!src_host_ptr) {
    if (const auto error = muxUnmapMemory(src_device, src_memory)) {
      return error;
    }
  }

  return mux_success;
}
