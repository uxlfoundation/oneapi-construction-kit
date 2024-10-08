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

/// @file
///
/// @brief Tests for the muxUpdateDescriptors entry point.

#include <mux/utils/helpers.h>

#include <cstring>
#include <tuple>

#include "common.h"

/// @brief Test fixture that checks muxUpdateDescriptors returns the correct
/// error code when updating descriptors isn't supported by the device.
///
/// This needs to be implemented as its own struct in order to take advantage of
/// the frame work that runs UnitMux uses to run the tests over all devices.
struct muxUpdateDescriptorsUnsupportedTest : public DeviceCompilerTest {};

// Tests the correct behaviour when the muxUpdateDescriptors entry poin is
// optionally not supported.
TEST_P(muxUpdateDescriptorsUnsupportedTest, UpdateDescriptorsUnsupported) {
  if (device->info->descriptors_updatable) {
    GTEST_SKIP();
  }
  // If updating descriptors is not supported muxUpdateDescriptors
  // must return mux_error_feature_unsupported.

  // Construct some dummy arguments to ensure that the target implementation is
  // actually invoked rather than getting mux_error_invalid_value.
  mux_command_buffer_t command_buffer;
  ASSERT_SUCCESS(
      muxCreateCommandBuffer(device, callback, allocator, &command_buffer));
  const mux_command_id_t command_id{};
  const uint64_t num_args = 1;
  uint64_t arg_indices[1] = {0};
  mux_descriptor_info_t descriptors[1] = {};

  EXPECT_EQ(mux_error_feature_unsupported,
            muxUpdateDescriptors(command_buffer, command_id, num_args,
                                 arg_indices, descriptors));

  muxDestroyCommandBuffer(device, command_buffer, allocator);
}

// Instantiate the test suite so that it runs for all devices.
INSTANTIATE_DEVICE_TEST_SUITE_P(muxUpdateDescriptorsUnsupportedTest);

/// @brief Base struct test fixture for testing the functionality of the
/// muxUpdateDescriptors entry point. This test fixture abstracts out common
/// functionality so that tests for various descriptor types e.g.
/// buffer, POD, local buffer, null can reduce code duplication.
struct muxUpdateDescriptorsTest : public DeviceCompilerTest {
  /// @brief Kernel to be enqueued in the nd range and have its
  /// arguments updated.
  mux_kernel_t kernel = nullptr;
  /// @brief Executable containing the kernel enqueued in the nd range and have
  /// its arguments updated.
  mux_executable_t executable = nullptr;
  /// @brief The command buffer containing the nd range command who's
  /// descriptors we will attempt to update.
  mux_command_buffer_t command_buffer = nullptr;
  /// @brief The queue on which command buffers will be executed.
  mux_queue_t queue = nullptr;
  /// @brief Address type of the devices ISA.
  mux_address_type_e address_type;
  /// @brief The local (x, y, z) dimensions of the nd range we will enqueue.
  static constexpr size_t local_size[] = {1, 1, 1};
  /// @brief The global offsets of the nd range we will enqueue.
  static constexpr size_t global_offset[] = {0, 0, 0};
  /// @brief The global dimensions  of the nd range we will enqueue.
  static constexpr size_t global_size[] = {256, 1, 1};
  /// @brief Descriptors for arguments to kernel.
  std::vector<mux_descriptor_info_t> descriptors;
  /// @brief The nd range options for enqueing a kernel.
  mux_ndrange_options_t nd_range_options;

  /// @brief Virtual method used to setup any resources for the test fixture
  /// that can't be done in the constructor.
  void SetUp() override {
    // Do the setup for the parent class.
    RETURN_ON_FATAL_FAILURE(DeviceCompilerTest::SetUp());

    // Updating descriptors is optional - devices express this via the
    // mux_device_info_s::descriptors_updatable field.
    if (!device->info->descriptors_updatable) {
      GTEST_SKIP();
    }

    // Initialize the command buffer.
    EXPECT_SUCCESS(
        muxCreateCommandBuffer(device, callback, allocator, &command_buffer));

    // Initialize the command queue.
    EXPECT_SUCCESS(muxGetQueue(device, mux_queue_type_compute, 0, &queue));

    // Initialize the nd range options.
    std::memcpy(nd_range_options.local_size, local_size, sizeof(local_size));
    nd_range_options.global_offset = global_offset;
    nd_range_options.global_size = global_size;
    nd_range_options.dimensions = 3;
  }

  /// @brief Virtual method used to tear down any resources for the test fixture
  /// that can't be done in the destructor.
  void TearDown() override {
    // Cleanup.
    if (nullptr != command_buffer) {
      muxDestroyCommandBuffer(device, command_buffer, allocator);
    }

    if (nullptr != kernel) {
      muxDestroyKernel(device, kernel, allocator);
    }

    if (nullptr != executable) {
      muxDestroyExecutable(device, executable, allocator);
    }

    // Do the tear down for the parent class.
    DeviceCompilerTest::TearDown();
  }
};

/// @brief Test fixture for checking we can update the descriptors of an nd
/// range where the arguments are of the mux_descriptor_info_buffer_s type.
///
/// Tests derived from this fixture will execute the following parallel copy
/// kernel:
///
/// void kernel parallel_copy(global int* a, global int* b) {
///  const size_t gid = get_global_id(0);
///  a[gid] = b[gid];
/// }
///
struct muxUpdateDescriptorsBufferTest : public muxUpdateDescriptorsTest {
  /// @brief Buffer for the input data to the kernel.
  mux_buffer_t buffer_in = nullptr;
  /// @brief Buffer for the output data to the kernel.
  mux_buffer_t buffer_out = nullptr;
  /// @brief Buffer for the updated output data to the kernel.
  mux_buffer_t buffer_out_updated = nullptr;
  /// @brief Memory that will be bound to the buffers.
  mux_memory_t memory = nullptr;
  /// @brief Descriptor for the updated argument.
  mux_descriptor_info_t descriptor_updated;
  /// @brief Initial input data.
  std::vector<char> data_in;
  /// @brief Initial output data.
  std::vector<char> data_out;
  /// @brief Updated output data.
  std::vector<char> data_out_updated;
  /// @brief Memory size in bytes of input and output buffers.
  static constexpr size_t buffer_size = global_size[0] * sizeof(int32_t);
  /// @brief The initial value that will fill the input buffer.
  static constexpr char input_value = 0x42;

  /// @brief Constructor.
  muxUpdateDescriptorsBufferTest()
      : data_in(buffer_size, input_value),
        data_out(buffer_size, 0x00),
        data_out_updated(buffer_size, 0x00) {};

  /// @brief Virtual method used to setup any resources for the test fixture
  /// that can't be done in the constructor.
  void SetUp() override {
    // Do the setup for the parent class.
    RETURN_ON_FATAL_FAILURE(muxUpdateDescriptorsTest::SetUp());

    // Create input and output buffers to the kernel. The kernel copies 4 byte
    // integers.
    EXPECT_SUCCESS(muxCreateBuffer(device, buffer_size, allocator, &buffer_in));
    EXPECT_SUCCESS(
        muxCreateBuffer(device, buffer_size, allocator, &buffer_out));
    EXPECT_SUCCESS(
        muxCreateBuffer(device, buffer_size, allocator, &buffer_out_updated));

    EXPECT_EQ(buffer_out->memory_requirements.supported_heaps,
              buffer_in->memory_requirements.supported_heaps);
    EXPECT_EQ(buffer_out->memory_requirements.supported_heaps,
              buffer_out_updated->memory_requirements.supported_heaps);

    const uint32_t heap = mux::findFirstSupportedHeap(
        buffer_out->memory_requirements.supported_heaps);

    // Check that we can allocate memory on the device, then allocate enough
    // for two buffers.
    constexpr size_t memory_size = 3 * buffer_size;

    EXPECT_SUCCESS(muxAllocateMemory(
        device, memory_size, heap, mux_memory_property_device_local,
        mux_allocation_type_alloc_device, 0, allocator, &memory));

    EXPECT_SUCCESS(muxBindBufferMemory(device, memory, buffer_in, 0));
    EXPECT_SUCCESS(
        muxBindBufferMemory(device, memory, buffer_out, buffer_size));
    EXPECT_SUCCESS(muxBindBufferMemory(device, memory, buffer_out_updated,
                                       2 * buffer_size));

    // Construct descriptors for the kernel arguments.
    // The output buffer is the first argument and the input buffer the second.
    mux_descriptor_info_t descriptor_out;
    descriptor_out.type = mux_descriptor_info_type_buffer;
    descriptor_out.buffer_descriptor.buffer = buffer_out;
    descriptor_out.buffer_descriptor.offset = 0;

    mux_descriptor_info_t descriptor_in;
    descriptor_in.type = mux_descriptor_info_type_buffer;
    descriptor_in.buffer_descriptor.buffer = buffer_in;
    descriptor_in.buffer_descriptor.offset = 0;

    descriptor_updated.type = mux_descriptor_info_type_buffer;
    descriptor_updated.buffer_descriptor.buffer = buffer_out_updated;
    descriptor_updated.buffer_descriptor.offset = 0;

    constexpr uint64_t descriptor_count = 2;
    descriptors.push_back(descriptor_out);
    descriptors.push_back(descriptor_in);

    // Initialize the descriptors of the nd range options.
    nd_range_options.descriptors = descriptors.data();
    nd_range_options.descriptors_length = descriptor_count;

    // Build the kernel.
    const char *kernel_name = "parallel_copy";
    const char *parallel_copy_opencl_c = R"(
     void kernel parallel_copy(global int* a, global int* b) {
      const size_t gid = get_global_id(0);
      a[gid] = b[gid];
     })";

    ASSERT_SUCCESS(createMuxExecutable(parallel_copy_opencl_c, &executable));
    ASSERT_SUCCESS(muxCreateKernel(device, executable, kernel_name,
                                   std::strlen(kernel_name), allocator,
                                   &kernel));

    // Push the write commands into the command buffer in the parent class.
    EXPECT_SUCCESS(muxCommandWriteBuffer(command_buffer, buffer_in, 0,
                                         data_in.data(), buffer_size, 0,
                                         nullptr, nullptr));
    EXPECT_SUCCESS(muxCommandWriteBuffer(command_buffer, buffer_out, 0,
                                         data_out.data(), buffer_size, 0,
                                         nullptr, nullptr));
    EXPECT_SUCCESS(muxCommandWriteBuffer(command_buffer, buffer_out_updated, 0,
                                         data_out_updated.data(), buffer_size,
                                         0, nullptr, nullptr));

    // We do not finalize and dispatch the command buffer containing the
    // intializing write commands here - this gives tests for this fixture the
    // option to append the nd range command to the same underlying command
    // buffer or to put the nd range in a subsequent command buffer.
  }

  /// @brief Virtual method used to tear down any resources for the test fixture
  /// that can't be done in the destructor.
  void TearDown() override {
    // Cleanup.
    if (nullptr != memory) {
      muxFreeMemory(device, memory, allocator);
    }

    if (nullptr != buffer_in) {
      muxDestroyBuffer(device, buffer_in, allocator);
    }

    if (nullptr != buffer_out) {
      muxDestroyBuffer(device, buffer_out, allocator);
    }

    if (nullptr != buffer_out_updated) {
      muxDestroyBuffer(device, buffer_out_updated, allocator);
    }

    // Do the tear down for the parent class.
    muxUpdateDescriptorsTest::TearDown();
  }
};

// Tests we can successfully update the output argument descriptors to a
// kernel in an nd range command.
TEST_P(muxUpdateDescriptorsBufferTest, ZeroCommandIndexUpdateOutputBuffer) {
  // Create a new command buffer to hold the nd range command - this way we know
  // it'll be at index zero.
  mux_command_buffer_t second_command_buffer{};
  EXPECT_SUCCESS(muxCreateCommandBuffer(device, callback, allocator,
                                        &second_command_buffer));

  // Push the nd range command into this new command buffer.
  EXPECT_SUCCESS(muxCommandNDRange(second_command_buffer, kernel,
                                   nd_range_options, 0, nullptr, nullptr));

  // Now push a read command to get the results - commands within a command
  // group must be executed as if they are in order.
  char results[buffer_size];
  EXPECT_SUCCESS(muxCommandReadBuffer(second_command_buffer, buffer_out, 0,
                                      results, buffer_size, 0, nullptr,
                                      nullptr));

  char results_updated[buffer_size];
  EXPECT_SUCCESS(muxCommandReadBuffer(second_command_buffer, buffer_out_updated,
                                      0, results_updated, buffer_size, 0,
                                      nullptr, nullptr));

  // Finalize both command buffers before dispatch.
  EXPECT_SUCCESS(muxFinalizeCommandBuffer(command_buffer));

  // Dispatch the first command buffer to the device's queue.
  EXPECT_SUCCESS(muxDispatch(queue, command_buffer, nullptr, nullptr, 0,
                             nullptr, 0, nullptr, nullptr));

  // Wait for the command buffer to complete. We could use semaphores to create
  // a dependency chain between the command buffers and then dispatch them
  // together
  // - this would probably be more performant, however since we are testing
  // whether we can update descriptors, not semaphores, we will just do a hard
  // wait on the queue between dispatches.
  EXPECT_SUCCESS(muxWaitAll(queue));

  // Dispatch the second command buffer and wait.
  EXPECT_SUCCESS(muxDispatch(queue, second_command_buffer, nullptr, nullptr, 0,
                             nullptr, 0, nullptr, nullptr));
  EXPECT_SUCCESS(muxWaitAll(queue));

  // Check that the intial enqueue worked.
  for (unsigned i = 0; i < buffer_size; ++i) {
    // Check the initial buffer got copied into.
    EXPECT_EQ(input_value, results[i])
        << "Error: result mismatch at index: " << i
        << " in initial output buffer after initial enqueue of command "
           "buffer\n";
    // And the updated buffer didn't
    EXPECT_EQ(0x00, results_updated[i])
        << "Error: result mismatch at index: " << i
        << " in updated output buffer after initial enqueue of command "
           "buffer\n";
  }

  // Now attempt to update the descriptors for the kernel in the nd
  // range command.

  // We know in this case the nd range command is at index 0 in the second
  // command buffer.
  constexpr mux_command_id_t nd_range_command_index = 0;

  // Attempt to update the output buffer (argument 0) in the nd range.
  uint64_t arg_indices[]{0};
  constexpr uint64_t num_args = 1;
  EXPECT_SUCCESS(muxUpdateDescriptors(second_command_buffer,
                                      nd_range_command_index, num_args,
                                      arg_indices, &descriptor_updated));

  // Dispatch the command buffers to the device's queue a second time.
  EXPECT_SUCCESS(muxDispatch(queue, command_buffer, nullptr, nullptr, 0,
                             nullptr, 0, nullptr, nullptr));
  EXPECT_SUCCESS(muxWaitAll(queue));

  EXPECT_SUCCESS(muxDispatch(queue, second_command_buffer, nullptr, nullptr, 0,
                             nullptr, 0, nullptr, nullptr));
  EXPECT_SUCCESS(muxWaitAll(queue));

  // Check that the update worked and the new command buffer has been filled.
  for (unsigned i = 0; i < buffer_size; ++i) {
    // Check the initial buffer didn't get copied into.
    EXPECT_EQ(0x00, results[i]) << "Error: result mismatch at index: " << i
                                << " in initial output buffer after initial "
                                   "enqueue of command buffer\n";
    // And the udpate buffer did.
    EXPECT_EQ(input_value, results_updated[i])
        << "Error: result mismatch at index: " << i
        << " in updated output buffer after updated enqueue of command "
           "buffer\n";
  }

  // Cleanup the buffers and memory associated with the output and the second
  // command buffer which contains the nd range.
  muxDestroyCommandBuffer(device, second_command_buffer, allocator);
}

// Tests that we can successfully update the descriptors to a kernel
// in an nd range command when that command has a non-zero index in its
// containing command buffer.
TEST_P(muxUpdateDescriptorsBufferTest, NonZeroCommandIndexUpdateOutputBuffer) {
  // Push the nd range command into the same command buffer as the write
  // commands that initialized the input and output buffers.
  EXPECT_SUCCESS(muxCommandNDRange(command_buffer, kernel, nd_range_options, 0,
                                   nullptr, nullptr));

  // Now push a read command to get the results - commands within a command
  // group must be executed as if they are in order.
  char results[buffer_size];
  EXPECT_SUCCESS(muxCommandReadBuffer(command_buffer, buffer_out, 0, results,
                                      buffer_size, 0, nullptr, nullptr));

  char results_updated[buffer_size];
  EXPECT_SUCCESS(muxCommandReadBuffer(command_buffer, buffer_out_updated, 0,
                                      results_updated, buffer_size, 0, nullptr,
                                      nullptr));

  // Finalize the command buffer before dispatch.
  EXPECT_SUCCESS(muxFinalizeCommandBuffer(command_buffer));

  // Dispatch the command buffer to the device's queue.
  EXPECT_SUCCESS(muxDispatch(queue, command_buffer, nullptr, nullptr, 0,
                             nullptr, 0, nullptr, nullptr));

  // Wait for the command buffer to complete.
  EXPECT_SUCCESS(muxWaitAll(queue));

  // Check that the intial enqueue worked.
  for (unsigned i = 0; i < buffer_size; ++i) {
    // Check the initial buffer got copied into.
    EXPECT_EQ(input_value, results[i])
        << "Error: result mismatch at index: " << i
        << " in initial output buffer after initial enqueue of command "
           "buffer\n";
    // And the updated buffer didn't
    EXPECT_EQ(0x00, results_updated[i])
        << "Error: result mismatch at index: " << i
        << " in updated output buffer after initial enqueue of command "
           "buffer\n";
  }

  // Now attempt to update the descriptors for the kernel in the nd range
  // command.

  // We know there have been exactly three write commands proceeding the nd
  // range command in the command buffer, so the nd range command will have
  // index 3 in its containing command buffer.
  constexpr mux_command_id_t nd_range_command_index = 3;

  // Attempt to update the output buffer (argument 0) in the nd range.
  uint64_t arg_indices[]{0};
  constexpr uint64_t num_args = 1;
  EXPECT_SUCCESS(muxUpdateDescriptors(command_buffer, nd_range_command_index,
                                      num_args, arg_indices,
                                      &descriptor_updated));

  // Dispatch the command buffer to the device's queue a second time.
  EXPECT_SUCCESS(muxDispatch(queue, command_buffer, nullptr, nullptr, 0,
                             nullptr, 0, nullptr, nullptr));

  // Wait for the command buffer to complete.
  EXPECT_SUCCESS(muxWaitAll(queue));

  // Check that the update worked and the new command buffer has been filled.
  for (unsigned i = 0; i < buffer_size; ++i) {
    // Check the initial buffer didn't get copied into.
    EXPECT_EQ(0x00, results[i]) << "Error: result mismatch at index: " << i
                                << " in initial output buffer after initial "
                                   "enqueue of command buffer\n";
    // And the udpate buffer did.
    EXPECT_EQ(input_value, results_updated[i])
        << "Error: result mismatch at index: " << i
        << " in updated output buffer after updated enqueue of command "
           "buffer\n";
  }
}

// Instantiate the test suite so that it runs for all devices.
INSTANTIATE_DEVICE_TEST_SUITE_P(muxUpdateDescriptorsBufferTest);

/// @brief Test fixture for checking we can update the descriptors of an nd
/// range where the argument is of the mux_descriptor_info_plain_old_data_s
/// type.
///
/// Tests derived from this fixture will execute the following broadcast kernel:
///
/// void kernel broadcast(int input, global int* output) {
///  const size_t gid = get_global_id(0);
///  output[gid] = input;
/// }
///
struct muxUpdateDescriptorsPODTest : public muxUpdateDescriptorsTest {
  /// @brief Buffer for the output data to the kernel.
  mux_buffer_t buffer_out = nullptr;
  /// @brief Memory that will be bound to the buffers.
  mux_memory_t memory = nullptr;
  /// @brief Descriptor for the updated argument.
  mux_descriptor_info_t descriptor_updated;
  /// @brief Initial output data.
  std::vector<char> data_out;
  /// @brief Memory size in bytes of input and output buffers.
  static constexpr size_t buffer_size = global_size[0] * sizeof(int32_t);
  /// @brief The initial value that will be broadcast to the output buffer.
  static constexpr int32_t input_value = 0x42;
  /// @brief The updated value that will be broadcast to the output buffer.
  static constexpr int32_t input_value_updated = 0x99;

  /// @brief Constructor.
  muxUpdateDescriptorsPODTest() : data_out(buffer_size, 0x00) {};

  /// @brief Virtual method used to setup any resources for the test fixture
  /// that can't be done in the constructor.
  void SetUp() override {
    // Do the setup for the parent class.
    RETURN_ON_FATAL_FAILURE(muxUpdateDescriptorsTest::SetUp());

    // Create output buffer to the kernel. The kernel copies 4 byte integers.
    EXPECT_SUCCESS(
        muxCreateBuffer(device, buffer_size, allocator, &buffer_out));

    const uint32_t heap = mux::findFirstSupportedHeap(
        buffer_out->memory_requirements.supported_heaps);

    // Check that we can allocate memory on the device, then allocate enough
    // for the output buffer.
    constexpr size_t memory_size = buffer_size;

    EXPECT_SUCCESS(muxAllocateMemory(
        device, memory_size, heap, mux_memory_property_device_local,
        mux_allocation_type_alloc_device, 0, allocator, &memory));

    EXPECT_SUCCESS(muxBindBufferMemory(device, memory, buffer_out, 0));

    // Construct descriptors for the kernel arguments.
    // The output buffer is the second argument and the POD value the first.
    mux_descriptor_info_t descriptor_out;
    descriptor_out.type = mux_descriptor_info_type_buffer;
    descriptor_out.buffer_descriptor.buffer = buffer_out;
    descriptor_out.buffer_descriptor.offset = 0;

    mux_descriptor_info_t descriptor_in;
    descriptor_in.type = mux_descriptor_info_type_plain_old_data;
    descriptor_in.plain_old_data_descriptor.data = &input_value;
    descriptor_in.plain_old_data_descriptor.length = sizeof(input_value);

    descriptor_updated.type = mux_descriptor_info_type_plain_old_data;
    descriptor_updated.plain_old_data_descriptor.data = &input_value_updated;
    descriptor_updated.plain_old_data_descriptor.length =
        sizeof(input_value_updated);

    constexpr uint64_t descriptor_count = 2;
    descriptors.push_back(descriptor_in);
    descriptors.push_back(descriptor_out);

    // Initialize the descriptors of the nd range options.
    nd_range_options.descriptors = descriptors.data();
    nd_range_options.descriptors_length = descriptor_count;

    // Build the kernel.
    const char *kernel_name = "broadcast";
    const char *broadcast_opencl_c = R"(
      void kernel broadcast(int input, global int* output) {
        const size_t gid = get_global_id(0);
        output[gid] = input;
     })";

    ASSERT_SUCCESS(createMuxExecutable(broadcast_opencl_c, &executable));
    ASSERT_SUCCESS(muxCreateKernel(device, executable, kernel_name,
                                   std::strlen(kernel_name), allocator,
                                   &kernel));

    // Push the write commands into the command buffer in the parent class.
    EXPECT_SUCCESS(muxCommandWriteBuffer(command_buffer, buffer_out, 0,
                                         data_out.data(), buffer_size, 0,
                                         nullptr, nullptr));

    // We do not finalize and dispatch the command buffer containing the
    // intializing write commands here - this gives tests for this fixture the
    // option to append the nd range command to the same underlying command
    // buffer or to put the nd range in a subsequent command buffer.
  }

  /// @brief Virtual method used to tear down any resources for the test fixture
  /// that can't be done in the destructor.
  void TearDown() override {
    // Cleanup.
    if (nullptr != memory) {
      muxFreeMemory(device, memory, allocator);
    }

    if (nullptr != buffer_out) {
      muxDestroyBuffer(device, buffer_out, allocator);
    }

    // Do the tear down for the parent class.
    muxUpdateDescriptorsTest::TearDown();
  }
};

// Tests that we can successfully update the descriptors for an input POD
// argument to a kernel in an nd range command.
TEST_P(muxUpdateDescriptorsPODTest, UpdateInputValue) {
  // Push the nd range command into the same command buffer as the write
  // commands that initialized the input and output buffers.
  EXPECT_SUCCESS(muxCommandNDRange(command_buffer, kernel, nd_range_options, 0,
                                   nullptr, nullptr));

  // Now push a read command to get the results - commands within a command
  // group must be executed as if they are in order.
  int32_t results[global_size[0]];
  EXPECT_SUCCESS(muxCommandReadBuffer(command_buffer, buffer_out, 0, results,
                                      buffer_size, 0, nullptr, nullptr));

  // Finalize the command buffer before dispatch.
  EXPECT_SUCCESS(muxFinalizeCommandBuffer(command_buffer));

  // Dispatch the command buffer to the device's queue.
  EXPECT_SUCCESS(muxDispatch(queue, command_buffer, nullptr, nullptr, 0,
                             nullptr, 0, nullptr, nullptr));

  // Wait for the command buffer to complete.
  EXPECT_SUCCESS(muxWaitAll(queue));

  // Check that the intial enqueue worked.
  for (unsigned i = 0; i < global_size[0]; ++i) {
    // Check the initial value was broadcast.
    EXPECT_EQ(input_value, results[i])
        << "Error: result mismatch at index: " << i
        << " in initial output buffer after initial enqueue of command "
           "buffer\n";
  }

  // Now attempt to update the descriptors for the kernel in the nd
  // range command.

  // We know there is exactly one write command proceeding the nd
  // range command in the command buffer, so the nd range command will have
  // index 1 in its containing command buffer.
  constexpr mux_command_id_t nd_range_command_index = 1;

  // Attempt to update the input value (argument 0) in the nd range.
  uint64_t arg_indices[]{0};
  constexpr uint64_t num_args = 1;
  EXPECT_SUCCESS(muxUpdateDescriptors(command_buffer, nd_range_command_index,
                                      num_args, arg_indices,
                                      &descriptor_updated));

  // Dispatch the command buffer to the device's queue a second time.
  EXPECT_SUCCESS(muxDispatch(queue, command_buffer, nullptr, nullptr, 0,
                             nullptr, 0, nullptr, nullptr));

  // Wait for the command buffer to complete.
  EXPECT_SUCCESS(muxWaitAll(queue));

  // Check that the update worked and the new command buffer has been filled.
  for (unsigned i = 0; i < global_size[0]; ++i) {
    // Check the updated value was broadcast.
    EXPECT_EQ(input_value_updated, results[i])
        << "Error: result mismatch at index: " << i
        << " in initial output buffer after initial enqueue of command "
           "buffer\n";
  }
}

// Instantiate the test suite so that it runs for all devices.
INSTANTIATE_DEVICE_TEST_SUITE_P(muxUpdateDescriptorsPODTest);

// TODO: Improve coverage of this entry point (see CA-3371):
// * Check that updates are persistent - can we re-enqueue an updated nd
// range and get the same results.
// * Check we can updated multiple arguments in one call to
// muxUpdateDescriptors.
// * Check we can update multiple arguments in multiple calls to
// muxUpdateDescriptors.
// * Check that updates the descriptor to a buffer argument using a
// non-zero offset.
// * Check we can update the same argument multiple times.
// * Write tests for NULL descriptors type.
// * Write tests for local buffer descriptor type.
