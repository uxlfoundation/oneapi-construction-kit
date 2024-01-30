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

#include <gtest/gtest.h>
#include <mux/mux.h>
#include <mux/utils/helpers.h>

#include <algorithm>
#include <cstring>  // For memset.
#include <iterator>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#include "cargo/dynamic_array.h"
#include "common.h"

struct muxApplication : public ::testing::Test {
  mux_callback_info_t callback = nullptr;
  mux_allocator_info_t allocator = {};
  std::vector<mux_device_t> devices;
  cargo::dynamic_array<mux_executable_t> executables;
  cargo::dynamic_array<mux_kernel_t> kernels;

  muxApplication()
      : callback(nullptr), allocator({mux::alloc, mux::free, nullptr}) {}

  void SetUp() override {
    // Get the compiler library.
    auto library_expected = compiler::loadLibrary();
    if (!library_expected.has_value()) {
      GTEST_FAIL() << "Unable to load compiler library: "
                   << library_expected.error();
    }
    auto library = std::move(*library_expected);

    // If there is no compiler library skip the test since it requires a
    // compiler.
    if (nullptr == library) {
      GTEST_SKIP();
    };

    // Count the number of devices.
    uint64_t length = 0;
    ASSERT_SUCCESS(createAllDevices(0, allocator, 0, &length));
    ASSERT_LT(0u, length);

    devices.resize(length);

    // Create all the devices.
    ASSERT_SUCCESS(createAllDevices(length, allocator, devices.data(), 0));

    // Source code the application will compile and execute.
    const char *parallel_copy_opencl_c = R"(
      void kernel parallel_copy(global int* a, global int* b) {
        const size_t gid = get_global_id(0);
        a[gid] = b[gid];
    })";

    // Create a target for each device.
    ASSERT_EQ(cargo::result::success, executables.alloc(length));
    ASSERT_EQ(cargo::result::success, kernels.alloc(length));

    // Compile the source for each device. We need to do this ahead of time to
    // avoid a TSAN failure that results from concurrent access to some global
    // state in clang when called from different contexts (see CA-3583).
    size_t device_index = 0;
    for (const auto &device : devices) {
      const compiler::Info *compiler_info =
          compiler::getCompilerForDevice(library.get(), device->info);

      auto context = compiler::createContext(library.get());
      ASSERT_NE(context, nullptr);

      auto target = compiler_info->createTarget(context.get(), nullptr);
      ASSERT_NE(target, nullptr);
      ASSERT_EQ(compiler::Result::SUCCESS,
                target->init(detectBuiltinCapabilities(device->info)));

      // Create a module.
      uint32_t num_errors = 0;
      std::string log;
      auto module = target->createModule(num_errors, log);
      ASSERT_NE(module, nullptr);

      // Load in and compile the OpenCL C.
      ASSERT_EQ(module->compileOpenCLC(mux::detectOpenCLProfile(device->info),
                                       parallel_copy_opencl_c, {}),
                compiler::Result::SUCCESS);
      std::vector<builtins::printf::descriptor> printf_calls;
      ASSERT_EQ(compiler::Result::SUCCESS,
                module->finalize(nullptr, printf_calls));
      cargo::array_view<std::uint8_t> buffer;
      ASSERT_EQ(module->createBinary(buffer), compiler::Result::SUCCESS);
      mux_executable_t executable = nullptr;
      ASSERT_SUCCESS(muxCreateExecutable(device, buffer.data(), buffer.size(),
                                         allocator, &executable));

      // Pick a kernel out of the executable.
      mux_kernel_t kernel;
      ASSERT_SUCCESS(muxCreateKernel(device, executable, "parallel_copy",
                                     std::strlen("parallel_copy"), allocator,
                                     &kernel));
      executables[device_index] = executable;
      kernels[device_index] = kernel;
      device_index++;
    }
  }

  void TearDown() override {
    // Destroy executables and kernels.
    for (unsigned device_index = 0; device_index < devices.size();
         ++device_index) {
      muxDestroyKernel(devices[device_index], kernels[device_index], allocator);
      muxDestroyExecutable(devices[device_index], executables[device_index],
                           allocator);
    }

    // Destroy every device.
    for (const auto &device : devices) {
      muxDestroyDevice(device, allocator);
    }
  }

  // An end-to-end application that runs a kernel to copy memory on a given
  // device.

  // If any dimension in 'zero_global_size' is true, then the global size in the
  // corresponding dimension will be set to 0. For example if `zero_global_size`
  // is set to {true, false, false}, then the global size will be {0, 1, 1}.
  // If any dimension is 0, we should check that the output buffer has not
  // changed.
  void Application(mux_device_t device, mux_kernel_t kernel,
                   std::array<bool, 3> zero_global_size = {false, false,
                                                           false}) {
    // Create and bind two buffers.
    enum { BUFFER_SIZE = 2048, MEMORY_SIZE = BUFFER_SIZE * 2 };
    mux_buffer_t buffer_out, buffer_in;
    ASSERT_SUCCESS(
        muxCreateBuffer(device, BUFFER_SIZE, allocator, &buffer_out));
    ASSERT_SUCCESS(muxCreateBuffer(device, BUFFER_SIZE, allocator, &buffer_in));

    EXPECT_EQ(buffer_out->memory_requirements.supported_heaps,
              buffer_in->memory_requirements.supported_heaps);
    const uint32_t heap = mux::findFirstSupportedHeap(
        buffer_out->memory_requirements.supported_heaps);

    // Check that we can allocate memory on the device, then allocate enough
    // for two buffers.
    mux_memory_t memory;

    EXPECT_SUCCESS(muxAllocateMemory(
        device, MEMORY_SIZE, heap, mux_memory_property_device_local,
        mux_allocation_type_alloc_device, 0, allocator, &memory));

    EXPECT_SUCCESS(muxBindBufferMemory(device, memory, buffer_out, 0));
    EXPECT_SUCCESS(muxBindBufferMemory(device, memory, buffer_in, BUFFER_SIZE));

    // Set up the nd range options, complete with the buffers to use as the
    // kernel arguements and the global work item dimensions.
    const size_t global_offset[3] = {0, 0, 0};
    size_t global_size[3] = {BUFFER_SIZE / sizeof(int32_t), 1, 1};
    size_t local_size[3] = {1, 1, 1};
    bool zero_sized_kernel = false;
    for (int i = 0; i < 3; ++i) {
      if (zero_global_size[i]) {
        global_size[i] = 0;
        zero_sized_kernel = true;
      }
    }
    mux_descriptor_info_t descriptors[2];
    descriptors[0].type = mux_descriptor_info_type_buffer;
    descriptors[0].buffer_descriptor.buffer = buffer_out;
    descriptors[0].buffer_descriptor.offset = 0;
    descriptors[1].type = mux_descriptor_info_type_buffer;
    descriptors[1].buffer_descriptor.buffer = buffer_in;
    descriptors[1].buffer_descriptor.offset = 0;

    mux_ndrange_options_t nd_range_options{};
    nd_range_options.descriptors = &descriptors[0];
    nd_range_options.descriptors_length = 2;
    std::memcpy(nd_range_options.local_size, local_size, sizeof(local_size));
    nd_range_options.global_offset = &global_offset[0];
    nd_range_options.global_size = &global_size[0];
    nd_range_options.dimensions = 3;

    // Some data to read, and somewhere to copy the output.
    char data_in[BUFFER_SIZE], data_out[BUFFER_SIZE];
    const char initial_data_in = 7;
    const char initial_data_out = 3;
    std::memset(data_in, initial_data_in, BUFFER_SIZE);
    std::memset(data_out, initial_data_out, BUFFER_SIZE);

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
    EXPECT_SUCCESS(muxCreateSemaphore(device, allocator, &semaphore_in));
    EXPECT_SUCCESS(muxCreateSemaphore(device, allocator, &semaphore_work));

    // Create a queue.
    mux_queue_t queue;
    EXPECT_SUCCESS(muxGetQueue(device, mux_queue_type_compute, 0, &queue));

    // Create and enqueue a command buffer to copy the data to device, set
    // semaphore_in to be signalled once the copy is complete.
    mux_command_buffer_t command_in;
    EXPECT_SUCCESS(
        muxCreateCommandBuffer(device, callback, allocator, &command_in));
    EXPECT_SUCCESS(muxCommandWriteBuffer(command_in, buffer_in, 0, data_in,
                                         BUFFER_SIZE, 0, nullptr, nullptr));
    EXPECT_SUCCESS(muxCommandWriteBuffer(command_in, buffer_out, 0, data_out,
                                         BUFFER_SIZE, 0, nullptr, nullptr));
    EXPECT_SUCCESS(muxDispatch(queue, command_in, nullptr, nullptr, 0,
                               &semaphore_in, 1, nullptr, nullptr));

    // Create and enqueue a command buffer to push execute the kernel, wait for
    // semaphore_in to be signalled before starting, set semaphore_work to be
    // signalled once the kernel is complete.
    mux_command_buffer_t command_work;
    EXPECT_SUCCESS(
        muxCreateCommandBuffer(device, callback, allocator, &command_work));
    EXPECT_SUCCESS(muxCommandNDRange(command_work, kernel, nd_range_options, 0,
                                     nullptr, nullptr));
    EXPECT_SUCCESS(muxDispatch(queue, command_work, nullptr, &semaphore_in, 1,
                               &semaphore_work, 1, nullptr, nullptr));

    // Create and enqueue a command buffer to copy the data from the device,
    // wait for semaphore_work to be signalled before starting.
    mux_command_buffer_t command_out;
    EXPECT_SUCCESS(
        muxCreateCommandBuffer(device, callback, allocator, &command_out));
    EXPECT_SUCCESS(muxCommandReadBuffer(command_out, buffer_out, 0, data_out,
                                        BUFFER_SIZE, 0, nullptr, nullptr));
    EXPECT_SUCCESS(muxDispatch(queue, command_out, nullptr, &semaphore_work, 1,
                               nullptr, 0, nullptr, nullptr));

    // Wait for all work on the queue to complete.
    EXPECT_SUCCESS(muxWaitAll(queue));

    // Check that the group of commands executed correctly.
    const char expected_output =
        zero_sized_kernel ? initial_data_out : initial_data_in;
    for (unsigned j = 0; j < BUFFER_SIZE; j++) {
      EXPECT_EQ(expected_output, data_out[j]);
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
  }
};

// An end-to-end application that runs a kernel to copy memory.
TEST_F(muxApplication, Default) {
  for (uint64_t i = 0; i < devices.size(); i++) {
    EXPECT_NO_FATAL_FAILURE(Application(devices[i], kernels[i]))
        << "whilst testing device " << devices[i]->info->device_name;
  }
}

// An end-to-end application that runs a kernel to copy memory with a global
// size of 0.
TEST_F(muxApplication, ZeroSizedKernel) {
  auto check_application = [&](std::array<bool, 3> global_size) {
    for (uint64_t i = 0; i < devices.size(); i++) {
      EXPECT_NO_FATAL_FAILURE(Application(devices[i], kernels[i], global_size))
          << "whilst testing device " << devices[i]->info->device_name;
    }
  };
  const std::vector<std::array<bool, 3>> global_sizes = {
      {0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1},
      {0, 1, 1}, {1, 0, 1}, {1, 1, 0}};
  for (const auto &global_size : global_sizes) {
    EXPECT_NO_FATAL_FAILURE(check_application(global_size));
  }
}

// An end-to-end application that concurrently runs many kernels to copy
// memory, this is intended to provide a basic sanity test for concurrency
// safety, but it does not use every Mux entry point and certainly doesn't
// trigger every possible combination.  Best used in combination with the
// thread sanitizer, or perhaps valgrind.
TEST_F(muxApplication, Concurrent) {
  auto worker = [&]() {
    for (uint64_t i = 0; i < devices.size(); i++) {
      ASSERT_NO_FATAL_FAILURE(Application(devices[i], kernels[i]));
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
