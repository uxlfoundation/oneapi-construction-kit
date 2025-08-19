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

#include <cl/device.h>
#include <cl/printf.h>

#if defined(_MSC_VER)
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#endif

namespace {
// Callback function for reading printf buffer data from device, unpacking
// it, and printing it from host to stdout
void PerformPrintf(mux_queue_t, mux_command_buffer_t, void *const user_data) {
  auto printf_info = static_cast<printf_info_t *>(user_data);
  uint8_t *pack{};

  mux_device_t mux_device = printf_info->device->mux_device;
  const size_t printf_buffer_size = printf_info->device->printf_buffer_size;
  mux_result_t error = muxMapMemory(mux_device, printf_info->memory, 0,
                                    printf_buffer_size, (void **)&pack);
  OCL_ASSERT(mux_success == error, "muxMapMemory failed!");
  error = muxFlushMappedMemoryFromDevice(mux_device, printf_info->memory, 0,
                                         printf_buffer_size);
  OCL_ASSERT(mux_success == error, "muxFlushMappedMemoryFromDevice failed!");
  OCL_UNUSED(error);

#if defined(_MSC_VER) && !defined(_DLL)
  // If we are building with /MT rather than /MD, we have our own copy of stdio
  // which has not necessarily picked up any changes to stdout performed by the
  // host application.
  std::FILE *const fp = [&]() -> std::FILE * {
    const HANDLE processHandle = GetCurrentProcess();
    HANDLE stdoutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (stdoutHandle == INVALID_HANDLE_VALUE ||
        DuplicateHandle(processHandle, stdoutHandle, processHandle,
                        &stdoutHandle, 0, FALSE, DUPLICATE_SAME_ACCESS) == 0) {
      return nullptr;
    }
    const int fd =
        _open_osfhandle(reinterpret_cast<intptr_t>(stdoutHandle), _O_WRONLY);
    if (fd == -1) {
      return nullptr;
    }
    auto *const fp = _fdopen(fd, "w");
    if (!fp) {
      _close(fd);
    }
    return fp;
  }();
#else
  std::FILE *const fp = stdout;
#endif

  // Unpack and print the data
  if (fp) {
    builtins::printf::print(fp, pack, printf_info->buffer_group_size,
                            printf_info->printf_calls,
                            printf_info->group_offsets);
#if defined(_MSC_VER) && !defined(_DLL)
    std::fclose(fp);
#endif
  }

  error = muxUnmapMemory(mux_device, printf_info->memory);
  OCL_ASSERT(mux_success == error, "muxUnmapMemory failed!");
}

// Callback function for printing data using PerformPrintf, and then freeing
// the printf resources afterwards. We can do this when we know the callback
// won't be called again, i.e. the clEnqueueNDRangeKernel command was made
// rather than clCommandNDRangeKernelKHR, which could be enqueued multiple
// times.
void PrintfAndFree(mux_queue_t queue, mux_command_buffer_t command_buffer,
                   void *const user_data) {
  PerformPrintf(queue, command_buffer, user_data);

  // Destroy resources as part of callback
  auto printf_info = static_cast<printf_info_t *>(user_data);
  delete printf_info;
}
}  // namespace

printf_info_t::~printf_info_t() {
  if (buffer) {
    muxDestroyBuffer(device->mux_device, buffer, device->mux_allocator);
  }

  if (memory) {
    muxFreeMemory(device->mux_device, memory, device->mux_allocator);
  }
}

mux_result_t createPrintfCallback(mux_command_buffer_t command_buffer,
                                  printf_info_t *printf_info) {
  return muxCommandUserCallback(command_buffer, PrintfAndFree, printf_info, 0,
                                nullptr, nullptr);
}

mux_result_t createPrintfCallback(
    mux_command_buffer_t command_buffer,
    const std::unique_ptr<printf_info_t> &printf_info) {
  return muxCommandUserCallback(command_buffer, PerformPrintf,
                                printf_info.get(), 0, nullptr, nullptr);
}

cl_int createPrintfBuffer(
    cl_device_id device,
    const std::array<size_t, cl::max::WORK_ITEM_DIM> &local_work_size,
    const std::array<size_t, cl::max::WORK_ITEM_DIM> &global_work_size,
    size_t &num_groups, size_t &buffer_group_size, mux_memory_t &printf_memory,
    mux_buffer_t &printf_buffer) {
  // Number of group is total number of work items divided by the size of a
  // work group
  num_groups =
      (global_work_size[0] * global_work_size[1] * global_work_size[2]) /
      (local_work_size[0] * local_work_size[1] * local_work_size[2]);

  OCL_ASSERT(num_groups > 0, "There must be at least one work group!");

  // Ensure the buffer start of each work item is aligned to 4 bytes.
  buffer_group_size = (device->printf_buffer_size / num_groups) & ~3u;

  // if we have less than 8 bytes per work group, we can't print anything,
  // and the kernel will crash, so just abort
  if (buffer_group_size < 8) {
    return CL_OUT_OF_RESOURCES;
  }

  // allocate the memory for the printf buffer
  // TODO: Add mechanism to support allocations best suited to printf.
  const uint32_t alignment = 0;  // Default alignment
  auto mux_device = device->mux_device;
  auto mux_allocator = device->mux_allocator;
  mux_result_t mux_error = muxAllocateMemory(
      mux_device, device->printf_buffer_size, 1,
      mux_memory_property_host_visible, mux_allocation_type_alloc_device,
      alignment, device->mux_allocator, &printf_memory);
  if (mux_error) {
    return CL_OUT_OF_RESOURCES;
  }

  // We need to initialize the first 8 bytes of the printf buffer so that
  // the first printf call can get a valid offset
  uint32_t *buffer;
  mux_error = muxMapMemory(mux_device, printf_memory, 0,
                           device->printf_buffer_size, (void **)&buffer);
  if (mux_error) {
    muxFreeMemory(mux_device, printf_memory, mux_allocator);
    return CL_OUT_OF_RESOURCES;
  }

  // initialize the buffer chunk for each work group
  for (size_t group_id = 0; group_id < num_groups; ++group_id) {
    // index into the 32 bits array
    const size_t index = (group_id * buffer_group_size) / sizeof(uint32_t);

    // 8 bytes for the size of the length value plus the size of the
    // overflow count
    buffer[index] = 8;
    // 0 bytes of overflow
    buffer[index + 1] = 0;
  }

  mux_error = muxFlushMappedMemoryToDevice(mux_device, printf_memory, 0,
                                           device->printf_buffer_size);
  if (mux_error) {
    muxFreeMemory(mux_device, printf_memory, mux_allocator);
    return CL_OUT_OF_RESOURCES;
  }
  mux_error = muxUnmapMemory(mux_device, printf_memory);
  if (mux_error) {
    muxFreeMemory(mux_device, printf_memory, mux_allocator);
    return CL_OUT_OF_RESOURCES;
  }

  // create the printf buffer
  mux_error = muxCreateBuffer(mux_device, device->printf_buffer_size,
                              mux_allocator, &printf_buffer);
  if (mux_error) {
    muxFreeMemory(mux_device, printf_memory, mux_allocator);
    return CL_OUT_OF_RESOURCES;
  }

  // and bind it to the printf memory without offset
  mux_error = muxBindBufferMemory(mux_device, printf_memory, printf_buffer, 0);
  if (mux_error) {
    muxDestroyBuffer(mux_device, printf_buffer, mux_allocator);
    muxFreeMemory(mux_device, printf_memory, mux_allocator);
    return CL_OUT_OF_RESOURCES;
  }
  return CL_SUCCESS;
}
