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

#include <cargo/string_algorithm.h>
#include <cargo/string_view.h>
#include <gtest/gtest.h>
#include <mux/mux.h>
#include <mux/utils/helpers.h>
#include <stdint.h>

#include <cstring>  // For memset.
#include <thread>
#include <vector>

#include "common.h"

struct muxBuiltinKernelApplication : public ::testing::Test {
  mux_callback_info_t callback = nullptr;
  mux_allocator_info_t allocator = {};
  std::vector<mux_device_t> devices;
  uint64_t length;

  muxBuiltinKernelApplication()
      : callback(nullptr), allocator({mux::alloc, mux::free, nullptr}) {}

  void SetUp() override {
    // Count the number of devices.
    ASSERT_SUCCESS(createAllDevices(0, allocator, 0, &length));

    ASSERT_LT(0u, length);

    devices.resize(length);

    // Create all the devices.
    ASSERT_SUCCESS(createAllDevices(length, allocator, devices.data(), 0));
  }

  void TearDown() override {
    // Destroy every device.
    for (const auto &device : devices) {
      muxDestroyDevice(device, allocator);
    }
  }

  // An end-to-end application that runs a kernel to copy memory on a given
  // device.
  void Application(mux_device_t device) {
    const cargo::string_view kernel_name = "copy_buffer";
    // Check if the 'copy_buffer' kernel is present in the list of available
    // kernels. If not, exit this test early since we can't test it.
    auto split_device_names =
        cargo::split_all(device->info->builtin_kernel_declarations, ";");

    if (std::none_of(split_device_names.begin(), split_device_names.end(),
                     [&kernel_name](const cargo::string_view &reported_name) {
                       return reported_name.find(kernel_name) !=
                              std::string::npos;
                     })) {
      GTEST_SKIP();
    }

    // Create a kernel.
    mux_kernel_t kernel;
    ASSERT_SUCCESS(muxCreateBuiltInKernel(
        device, kernel_name.data(), kernel_name.size(), allocator, &kernel));

    // Create and bind two buffers.
    enum { BUFFER_SIZE = 2048, MEMORY_SIZE = BUFFER_SIZE * 2 };
    mux_buffer_t buffer_out, buffer_in;
    ASSERT_SUCCESS(
        muxCreateBuffer(device, BUFFER_SIZE, allocator, &buffer_out));
    ASSERT_SUCCESS(muxCreateBuffer(device, BUFFER_SIZE, allocator, &buffer_in));

    ASSERT_EQ(buffer_out->memory_requirements.supported_heaps,
              buffer_in->memory_requirements.supported_heaps);
    const uint32_t heap = mux::findFirstSupportedHeap(
        buffer_out->memory_requirements.supported_heaps);

    // Check that we can allocate memory on the device, then allocate enough
    // for two buffers.
    mux_memory_t memory;

    ASSERT_SUCCESS(muxAllocateMemory(
        device, MEMORY_SIZE, heap, mux_memory_property_device_local,
        mux_allocation_type_alloc_device, 0, allocator, &memory));

    ASSERT_SUCCESS(muxBindBufferMemory(device, memory, buffer_in, 0));
    ASSERT_SUCCESS(
        muxBindBufferMemory(device, memory, buffer_out, BUFFER_SIZE));

    // Set up the nd range options, complete with the buffers to use as the
    // kernel arguements and the global work item dimensions.
    const size_t global_offset[3] = {0, 0, 0};
    const size_t global_size[3] = {BUFFER_SIZE / sizeof(int32_t), 1, 1};
    size_t local_size[3] = {1, 1, 1};
    mux_descriptor_info_t descriptors[2];
    descriptors[0].type = mux_descriptor_info_type_buffer;
    descriptors[0].buffer_descriptor.buffer = buffer_in;
    descriptors[0].buffer_descriptor.offset = 0;
    descriptors[1].type = mux_descriptor_info_type_buffer;
    descriptors[1].buffer_descriptor.buffer = buffer_out;
    descriptors[1].buffer_descriptor.offset = 0;

    // Some data to read, and somewhere to copy the output.
    char data_in[BUFFER_SIZE], data_out[BUFFER_SIZE];
    std::memset(data_in, 7, BUFFER_SIZE);
    std::memset(data_out, 3, BUFFER_SIZE);

    mux_ndrange_options_t nd_range_options{};
    nd_range_options.descriptors = &descriptors[0];
    nd_range_options.descriptors_length = 2;
    std::memcpy(nd_range_options.local_size, local_size, sizeof(local_size));
    nd_range_options.global_offset = &global_offset[0];
    nd_range_options.global_size = &global_size[0];
    nd_range_options.dimensions = 3;

    // Create a semaphore to signal when the data has been copied to the
    // device, and a semaphore to signal when the compute has been done.  This
    // is required because both command buffers and queues are out of order, so
    // all dependencies must be explicitly described.  No semaphore is required
    // for when the data is copied of the device because no command buffer will
    // be waiting on that to complete (we will wait on the entire queue to
    // complete instead).
    //
    // Note, in the future it may be possible to include pipeline barriers in a
    // command buffer, then no semaphore would be required and all the work
    // could be placed in a single command buffer.
    mux_semaphore_t semaphore_in, semaphore_work;
    ASSERT_SUCCESS(muxCreateSemaphore(device, allocator, &semaphore_in));
    ASSERT_SUCCESS(muxCreateSemaphore(device, allocator, &semaphore_work));

    // Create a queue.
    mux_queue_t queue;
    ASSERT_SUCCESS(muxGetQueue(device, mux_queue_type_compute, 0, &queue));

    // Create and enqueue a command buffer to copy the data to device, set
    // semaphore_in to be signalled once the copy is complete.
    mux_command_buffer_t command_in;
    ASSERT_SUCCESS(
        muxCreateCommandBuffer(device, callback, allocator, &command_in));
    ASSERT_SUCCESS(muxCommandWriteBuffer(command_in, buffer_in, 0, data_in,
                                         BUFFER_SIZE, 0, nullptr, nullptr));
    ASSERT_SUCCESS(muxDispatch(queue, command_in, nullptr, nullptr, 0,
                               &semaphore_in, 1, nullptr, nullptr));

    // Create and enqueue a command buffer to push execute the kernel, wait for
    // semaphore_in to be signalled before starting, set semaphore_work to be
    // signalled once the kernel is complete.
    mux_command_buffer_t command_work;
    ASSERT_SUCCESS(
        muxCreateCommandBuffer(device, callback, allocator, &command_work));
    ASSERT_SUCCESS(muxCommandNDRange(command_work, kernel, nd_range_options, 0,
                                     nullptr, nullptr));
    ASSERT_SUCCESS(muxDispatch(queue, command_work, nullptr, &semaphore_in, 1,
                               &semaphore_work, 1, nullptr, nullptr));

    // Create and enqueue a command buffer to copy the data from the device,
    // wait for semaphore_work to be signalled before starting.
    mux_command_buffer_t command_out;
    ASSERT_SUCCESS(
        muxCreateCommandBuffer(device, callback, allocator, &command_out));
    ASSERT_SUCCESS(muxCommandReadBuffer(command_out, buffer_out, 0, data_out,
                                        BUFFER_SIZE, 0, nullptr, nullptr));
    ASSERT_SUCCESS(muxDispatch(queue, command_out, nullptr, &semaphore_work, 1,
                               nullptr, 0, nullptr, nullptr));

    // Wait for all work on the queue to complete.
    ASSERT_SUCCESS(muxWaitAll(queue));

    // Check that the group of commands executed correctly.
    for (unsigned j = 0; j < BUFFER_SIZE; j++) {
      EXPECT_EQ(data_in[j], data_out[j]);
      EXPECT_EQ(7, data_out[j]);
    }

    // Clean-up, note: many of these could have been called earlier.
    muxDestroyCommandBuffer(device, command_in, allocator);
    muxDestroyCommandBuffer(device, command_work, allocator);
    muxDestroyCommandBuffer(device, command_out, allocator);
    muxDestroySemaphore(device, semaphore_in, allocator);
    muxDestroySemaphore(device, semaphore_work, allocator);
    muxDestroyBuffer(device, buffer_in, allocator);
    muxDestroyBuffer(device, buffer_out, allocator);
    muxFreeMemory(device, memory, allocator);
    muxDestroyKernel(device, kernel, allocator);
  }
};

// An end-to-end application that runs a kernel to copy memory.
TEST_F(muxBuiltinKernelApplication, Default) {
  for (uint64_t i = 0; i < length; i++) {
    Application(devices[i]);
  }
}

// An end-to-end application that concurrently runs many kernels to copy
// memory, this is intended to provide a basic sanity test for concurrency
// safety, but it does not use every Mux entry point and certainly doesn't
// trigger every possible combination.  Best used in combination with the
// thread sanitizer, or perhaps valgrind.
TEST_F(muxBuiltinKernelApplication, Concurrent) {
  auto worker = [&]() {
    for (uint64_t i = 0; i < length; i++) {
      Application(devices[i]);
    }
  };

  // Ideally there would be 10+ threads as that is much more reliable for
  // detecting issues, but greatly slows down the test.  If the thread
  // sanitizer is enabled then 2 is enough to report most issues, go with 5
  const size_t threads = 5;
  std::vector<std::thread> workers(threads);

  for (size_t i = 0; i < threads; i++) {
    workers[i] = std::thread(worker);
  }

  for (size_t i = 0; i < threads; i++) {
    workers[i].join();
  }
}
