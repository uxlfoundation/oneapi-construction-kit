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
/// @brief Tests for the muxCloneCommandBuffer entry point.

#include <mux/utils/helpers.h>

#include <mutex>
#include <vector>

#include "common.h"

/// @brief Base test fixture for testing the muxCloneCommandBuffer entry point.
///
/// Most tests for muxCloneCommandBuffer are going to require a command buffer
/// to be cloned and a command buffer that is the result of the clone. This test
/// fixture allocates and deallocates these resources so child classes don't
/// have to.
struct muxCloneCommandBufferBaseTest : public DeviceTest {
  /// @brief mux_command_buffer_t to be cloned.
  mux_command_buffer_t command_buffer_to_clone = nullptr;
  /// @brief mux_command_buffer_t clone.
  mux_command_buffer_t out_command_buffer = nullptr;

  /// @brief Virtual method used to setup any resources for the test fixture
  /// that can't be done in the constructor.
  void SetUp() override {
    // Do the setup for the parent class.
    RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());

    // Initialize the command buffer.
    EXPECT_SUCCESS(muxCreateCommandBuffer(device, callback, allocator,
                                          &command_buffer_to_clone));
  }

  /// @brief Virtual method used to tear down any resources for the test
  /// fixture that can't be done in the destructor.
  void TearDown() override {
    // Cleanup.
    if (nullptr != command_buffer_to_clone) {
      muxDestroyCommandBuffer(device, command_buffer_to_clone, allocator);
    }

    if (nullptr != out_command_buffer) {
      muxDestroyCommandBuffer(device, out_command_buffer, allocator);
    }

    // Do the tear down for the parent class.
    DeviceTest::TearDown();
  }
};

/// @brief Test fixture that checks muxCloneCommandBuffer returns the correct
/// error code when cloning command buffers isn't supported by the device.
///
/// This needs to be implemented as its own struct in order to take advantage of
/// the frame work that runs UnitMux uses to run the tests over all devices.
struct muxCloneCommandBufferUnsupportedTest
    : public muxCloneCommandBufferBaseTest {
  /// @brief Virtual method used to setup any resources for the test fixture
  /// that can't be done in the constructor.
  void SetUp() override {
    // Do the setup for the parent class.
    RETURN_ON_FATAL_FAILURE(muxCloneCommandBufferBaseTest::SetUp());

    // We don't need to run any test fixtures derived from this class if cloning
    // kernels is supported.
    if (device->info->can_clone_command_buffers) {
      GTEST_SKIP();
    }
  }
};

// Tests the correct behaviour when the muxCloneCommandBuffer entry point is
// optionally not supported.
TEST_P(muxCloneCommandBufferUnsupportedTest, CloneCommandBufferUnsupported) {
  // If cloning command buffers is not supported muxCloneCommandBuffer must
  // return mux_error_feature_unsupported.
  ASSERT_SUCCESS(muxFinalizeCommandBuffer(command_buffer_to_clone));
  ASSERT_EQ(mux_error_feature_unsupported,
            muxCloneCommandBuffer(device, allocator, command_buffer_to_clone,
                                  &out_command_buffer));
}

/// @brief Base test fixture for testing the functionality of the
/// muxCloneCommandBuffer entry point.
struct muxCloneCommandBufferTest : public muxCloneCommandBufferBaseTest {
  /// @brief mux_queue_t on which command buffers will execute.
  mux_queue_t queue = nullptr;

  /// @brief Virtual method used to setup any resources for the test fixture
  /// that can't be done in the constructor.
  void SetUp() override {
    // Do the setup for the parent class.
    RETURN_ON_FATAL_FAILURE(muxCloneCommandBufferBaseTest::SetUp());

    // We don't need to run any test fixtures derived from this class if cloning
    // kernels is not supported.
    if (!device->info->can_clone_command_buffers) {
      GTEST_SKIP();
    }

    // We also won't run the tests if there is no compute queue.
    if (0 == device->info->queue_types[mux_queue_type_compute]) {
      GTEST_SKIP();
    }
    ASSERT_SUCCESS(muxGetQueue(device, mux_queue_type_compute, 0, &queue));
  }
};

TEST_P(muxCloneCommandBufferTest, NullDevice) {
  ASSERT_SUCCESS(muxFinalizeCommandBuffer(command_buffer_to_clone));
  ASSERT_EQ(mux_error_invalid_value,
            muxCloneCommandBuffer(nullptr, allocator, command_buffer_to_clone,
                                  &out_command_buffer));
}

TEST_P(muxCloneCommandBufferTest, UninitializedAllocator) {
  // An allocator is considered malformed if its alloc or free fields are NULL.
  // The user_data field may or may not be NULL so there is no reason to test
  // that here.
  auto nop_alloc = [](void *, size_t, size_t) -> void * { return nullptr; };
  auto nop_free = [](void *, void *) -> void {};

  // First try with a null alloc function.
  mux_allocator_info_t uninitialized_allocator{};
  uninitialized_allocator.free = nop_free;
  ASSERT_SUCCESS(muxFinalizeCommandBuffer(command_buffer_to_clone));
  ASSERT_EQ(
      mux_error_null_allocator_callback,
      muxCloneCommandBuffer(device, uninitialized_allocator,
                            command_buffer_to_clone, &out_command_buffer));

  // Then with a null free function.
  uninitialized_allocator.alloc = nop_alloc;
  uninitialized_allocator.free = nullptr;
  ASSERT_EQ(
      mux_error_null_allocator_callback,
      muxCloneCommandBuffer(device, uninitialized_allocator,
                            command_buffer_to_clone, &out_command_buffer));

  // Then with a null alloc and null free functions.
  uninitialized_allocator.alloc = nullptr;
  uninitialized_allocator.free = nullptr;
  ASSERT_EQ(
      mux_error_null_allocator_callback,
      muxCloneCommandBuffer(device, uninitialized_allocator,
                            command_buffer_to_clone, &out_command_buffer));
}

TEST_P(muxCloneCommandBufferTest, NullCommandBuffer) {
  ASSERT_SUCCESS(muxFinalizeCommandBuffer(command_buffer_to_clone));
  ASSERT_EQ(
      mux_error_invalid_value,
      muxCloneCommandBuffer(device, allocator, nullptr, &out_command_buffer));
}

TEST_P(muxCloneCommandBufferTest, NullOutCommandBuffer) {
  ASSERT_SUCCESS(muxFinalizeCommandBuffer(command_buffer_to_clone));
  ASSERT_EQ(mux_error_null_out_parameter,
            muxCloneCommandBuffer(device, allocator, command_buffer_to_clone,
                                  nullptr));
}

TEST_P(muxCloneCommandBufferTest, CloneEmptyCommandBuffer) {
  ASSERT_SUCCESS(muxFinalizeCommandBuffer(command_buffer_to_clone));
  EXPECT_SUCCESS(muxCloneCommandBuffer(
      device, allocator, command_buffer_to_clone, &out_command_buffer));
}

/// @brief Helper test fixture for testing muxCloneCommandBuffer where the
/// command buffer being copied contains a command that uses a buffer.
struct muxCloneCommandBufferSingleBufferTest
    : public muxCloneCommandBufferTest {
  /// @brief The memory underlying the buffer.
  mux_memory_t memory = nullptr;
  /// @brief The buffer object.
  mux_buffer_t buffer = nullptr;
  /// @brief The size of the buffer in bytes.
  constexpr static size_t buffer_size_in_bytes = 256;
  /// @brief A block of host memory that can be read from or written to.
  char data[buffer_size_in_bytes];

  /// @brief Virtual method used to setup any resources for the test fixture
  /// that can't be done in the constructor.
  void SetUp() override {
    // Do the setup for the parent class.
    RETURN_ON_FATAL_FAILURE(muxCloneCommandBufferTest::SetUp());
    ASSERT_SUCCESS(
        muxCreateBuffer(device, buffer_size_in_bytes, allocator, &buffer));

    const mux_allocation_type_e allocation_type =
        (mux_allocation_capabilities_alloc_device &
         device->info->allocation_capabilities)
            ? mux_allocation_type_alloc_device
            : mux_allocation_type_alloc_host;

    const uint32_t heap = mux::findFirstSupportedHeap(
        buffer->memory_requirements.supported_heaps);

    ASSERT_SUCCESS(muxAllocateMemory(device, buffer_size_in_bytes, heap,
                                     mux_memory_property_host_visible,
                                     allocation_type, 0, allocator, &memory));

    ASSERT_SUCCESS(muxBindBufferMemory(device, memory, buffer, 0));
  }

  /// @brief Virtual method used to tear down any resources for the test
  /// fixture that can't be done in the destructor.
  void TearDown() override {
    // Cleanup.
    if (nullptr != buffer) {
      muxDestroyBuffer(device, buffer, allocator);
    }

    if (nullptr != memory) {
      muxFreeMemory(device, memory, allocator);
    }

    // Do the tear down for the parent class.
    muxCloneCommandBufferTest::TearDown();
  }
};

TEST_P(muxCloneCommandBufferSingleBufferTest, CloneReadBuffer) {
  ASSERT_SUCCESS(muxCommandReadBuffer(command_buffer_to_clone, buffer, 0, data,
                                      buffer_size_in_bytes, 0, nullptr,
                                      nullptr));
  ASSERT_SUCCESS(muxFinalizeCommandBuffer(command_buffer_to_clone));
  ASSERT_SUCCESS(muxCloneCommandBuffer(
      device, allocator, command_buffer_to_clone, &out_command_buffer));
}

TEST_P(muxCloneCommandBufferSingleBufferTest, CloneReadBufferRegions) {
  mux_buffer_region_info_t info = {
      {1, 1, 1}, {1, 1, 1}, {1, 1, 1}, {1, 1}, {1, 1}};
  ASSERT_SUCCESS(muxCommandReadBufferRegions(
      command_buffer_to_clone, buffer, data, &info, 1, 0, nullptr, nullptr));
  ASSERT_SUCCESS(muxFinalizeCommandBuffer(command_buffer_to_clone));
  ASSERT_SUCCESS(muxCloneCommandBuffer(
      device, allocator, command_buffer_to_clone, &out_command_buffer));
}

TEST_P(muxCloneCommandBufferSingleBufferTest, CloneWriteBuffer) {
  ASSERT_SUCCESS(muxCommandWriteBuffer(command_buffer_to_clone, buffer, 0, data,
                                       buffer_size_in_bytes, 0, nullptr,
                                       nullptr));
  ASSERT_SUCCESS(muxFinalizeCommandBuffer(command_buffer_to_clone));
  ASSERT_SUCCESS(muxCloneCommandBuffer(
      device, allocator, command_buffer_to_clone, &out_command_buffer));
}

TEST_P(muxCloneCommandBufferSingleBufferTest, CloneWriteBufferRegions) {
  mux_buffer_region_info_t info = {
      {1, 1, 1}, {1, 1, 1}, {1, 1, 1}, {1, 1}, {1, 1}};
  ASSERT_SUCCESS(muxCommandWriteBufferRegions(
      command_buffer_to_clone, buffer, data, &info, 1, 0, nullptr, nullptr));
  ASSERT_SUCCESS(muxFinalizeCommandBuffer(command_buffer_to_clone));
  ASSERT_SUCCESS(muxCloneCommandBuffer(
      device, allocator, command_buffer_to_clone, &out_command_buffer));
}

TEST_P(muxCloneCommandBufferSingleBufferTest, CloneFillBuffer) {
  ASSERT_SUCCESS(muxCommandFillBuffer(command_buffer_to_clone, buffer, 0,
                                      buffer_size_in_bytes, &data, 1, 0,
                                      nullptr, nullptr));
  ASSERT_SUCCESS(muxFinalizeCommandBuffer(command_buffer_to_clone));
  ASSERT_SUCCESS(muxCloneCommandBuffer(
      device, allocator, command_buffer_to_clone, &out_command_buffer));
}

TEST_P(muxCloneCommandBufferSingleBufferTest, CloneFillBufferSmokeTest) {
  // Command a fill then read into the command buffer and clone it.
  constexpr char pattern = 0x42;
  ASSERT_SUCCESS(muxCommandFillBuffer(command_buffer_to_clone, buffer, 0,
                                      sizeof(pattern), &pattern,
                                      sizeof(pattern), 0, nullptr, nullptr));
  ASSERT_SUCCESS(muxCommandReadBuffer(command_buffer_to_clone, buffer, 0, data,
                                      sizeof(pattern), 0, nullptr, nullptr));
  ASSERT_SUCCESS(muxFinalizeCommandBuffer(command_buffer_to_clone));
  ASSERT_SUCCESS(muxCloneCommandBuffer(
      device, allocator, command_buffer_to_clone, &out_command_buffer));

  // Enqueue the command buffer and check results.
  ASSERT_SUCCESS(muxDispatch(queue, command_buffer_to_clone, nullptr, nullptr,
                             0, nullptr, 0, nullptr, nullptr));
  ASSERT_SUCCESS(muxWaitAll(queue));
  EXPECT_EQ(pattern, data[0]);

  // Create a new command buffer and zero the data buffer.
  mux_command_buffer_t zero_buffer_command_buffer;
  EXPECT_SUCCESS(muxCreateCommandBuffer(device, callback, allocator,
                                        &zero_buffer_command_buffer));
  constexpr char zero = 0;
  EXPECT_SUCCESS(muxCommandFillBuffer(zero_buffer_command_buffer, buffer, 0,
                                      sizeof(zero), &zero, sizeof(zero), 0,
                                      nullptr, nullptr));
  EXPECT_SUCCESS(muxCommandReadBuffer(zero_buffer_command_buffer, buffer, 0,
                                      data, sizeof(zero), 0, nullptr, nullptr));
  EXPECT_SUCCESS(muxFinalizeCommandBuffer(zero_buffer_command_buffer));
  EXPECT_SUCCESS(muxDispatch(queue, zero_buffer_command_buffer, nullptr,
                             nullptr, 0, nullptr, 0, nullptr, nullptr));
  EXPECT_SUCCESS(muxWaitAll(queue));
  EXPECT_EQ(zero, data[0]);

  // Enqueue the clone and check the result.
  EXPECT_SUCCESS(muxDispatch(queue, out_command_buffer, nullptr, nullptr, 0,
                             nullptr, 0, nullptr, nullptr));
  EXPECT_SUCCESS(muxWaitAll(queue));
  EXPECT_EQ(pattern, data[0]);

  // Clean up command group allocated by this test.
  muxDestroyCommandBuffer(device, zero_buffer_command_buffer, allocator);
}

/// @brief Helper test fixture for testing muxCloneCommandBuffer where the
/// command buffer being copied contains a command that uses two buffers.
struct muxCloneCommandBufferTwoBufferTest : public muxCloneCommandBufferTest {
  /// @brief The memory underlying the buffer.
  mux_memory_t memory = nullptr;
  /// @brief The first buffer object.
  mux_buffer_t buffer_one = nullptr;
  /// @brief The second buffer object.
  mux_buffer_t buffer_two = nullptr;
  /// @brief The size of the buffer in bytes.
  constexpr static size_t buffer_size_in_bytes = 256;

  /// @brief Virtual method used to setup any resources for the test fixture
  /// that can't be done in the constructor.
  void SetUp() override {
    // Do the setup for the parent class.
    RETURN_ON_FATAL_FAILURE(muxCloneCommandBufferTest::SetUp());
    ASSERT_SUCCESS(
        muxCreateBuffer(device, buffer_size_in_bytes, allocator, &buffer_one));
    ASSERT_SUCCESS(
        muxCreateBuffer(device, buffer_size_in_bytes, allocator, &buffer_two));

    const mux_allocation_type_e allocation_type =
        (mux_allocation_capabilities_alloc_device &
         device->info->allocation_capabilities)
            ? mux_allocation_type_alloc_device
            : mux_allocation_type_alloc_host;

    const uint32_t heap = mux::findFirstSupportedHeap(
        buffer_one->memory_requirements.supported_heaps);

    ASSERT_SUCCESS(muxAllocateMemory(device, 2 * buffer_size_in_bytes, heap,
                                     mux_memory_property_host_visible,
                                     allocation_type, 0, allocator, &memory));

    ASSERT_SUCCESS(muxBindBufferMemory(device, memory, buffer_one, 0));
    ASSERT_SUCCESS(
        muxBindBufferMemory(device, memory, buffer_two, buffer_size_in_bytes));
  }

  /// @brief Virtual method used to tear down any resources for the test
  /// fixture that can't be done in the destructor.
  void TearDown() override {
    // Cleanup.
    if (nullptr != buffer_one) {
      muxDestroyBuffer(device, buffer_one, allocator);
    }

    if (nullptr != buffer_two) {
      muxDestroyBuffer(device, buffer_two, allocator);
    }

    if (nullptr != memory) {
      muxFreeMemory(device, memory, allocator);
    }

    // Do the tear down for the parent class.
    muxCloneCommandBufferTest::TearDown();
  }
};

TEST_P(muxCloneCommandBufferTwoBufferTest, CloneCopyBuffer) {
  ASSERT_SUCCESS(muxCommandCopyBuffer(command_buffer_to_clone, buffer_one, 0,
                                      buffer_two, 0, buffer_size_in_bytes, 0,
                                      nullptr, nullptr));
  ASSERT_SUCCESS(muxFinalizeCommandBuffer(command_buffer_to_clone));
  ASSERT_SUCCESS(muxCloneCommandBuffer(
      device, allocator, command_buffer_to_clone, &out_command_buffer));
}

TEST_P(muxCloneCommandBufferTwoBufferTest, CloneCopyBufferRegions) {
  mux_buffer_region_info_t info = {
      {1, 1, 1}, {1, 1, 1}, {1, 1, 1}, {1, 1}, {1, 1}};
  ASSERT_SUCCESS(muxCommandCopyBufferRegions(command_buffer_to_clone,
                                             buffer_one, buffer_two, &info, 1,
                                             0, nullptr, nullptr));
  ASSERT_SUCCESS(muxFinalizeCommandBuffer(command_buffer_to_clone));
  ASSERT_SUCCESS(muxCloneCommandBuffer(
      device, allocator, command_buffer_to_clone, &out_command_buffer));
}

// TODO: Add testing for images (See CA-3476).

/// @brief Helper test fixture for testing muxCloneCommandBuffer where the
/// command buffer being copied contains a command that uses an image.
struct muxCloneCommandBufferSingleImageTest : public muxCloneCommandBufferTest {
};

TEST_P(muxCloneCommandBufferSingleImageTest, DISABLED_CloneReadImage) {
  GTEST_FAIL() << "Not yet implemented";
}

TEST_P(muxCloneCommandBufferSingleImageTest, DISABLED_CloneWriteImage) {
  GTEST_FAIL() << "Not yet implemented";
}

TEST_P(muxCloneCommandBufferSingleImageTest, DISABLED_CloneFillImage) {
  GTEST_FAIL() << "Not yet implemented";
}

/// @brief Helper test fixture for testing muxCloneCommandBuffer where the
/// command buffer being copied contains a command that uses two images.
struct muxCloneCommandBufferTwoImageTest : public muxCloneCommandBufferTest {};

TEST_P(muxCloneCommandBufferTwoImageTest, DISABLED_CloneCopyImage) {
  GTEST_FAIL() << "Not yet implemented";
}

/// @brief Helper test fixture for testing muxCloneCommandBuffer where the
/// command buffer being copied contains a command that uses an image and a
/// buffer.
struct muxCloneCommandBufferImageBufferTest : public muxCloneCommandBufferTest {
};

TEST_P(muxCloneCommandBufferImageBufferTest, DISABLED_CloneCopyImageToBuffer) {
  GTEST_FAIL() << "Not yet implemented";
}

TEST_P(muxCloneCommandBufferImageBufferTest, DISABLED_CloneCopyBufferToImage) {
  GTEST_FAIL() << "Not yet implemented";
}

// TODO: Add ND Range tests (See CA-3476).

/// @brief Helper test fixture for testing muxCloneCommandBuffer where the
/// command buffer being copied contains an nd rangecommand.
struct muxCloneCommandBufferNDRangeTest : public muxCloneCommandBufferTest {};
TEST_P(muxCloneCommandBufferNDRangeTest, DISABLED_CloneNDRange) {
  GTEST_FAIL() << "Not yet implemented";
}

TEST_P(muxCloneCommandBufferTest, CloneUserCallback) {
  ASSERT_SUCCESS(muxCommandUserCallback(
      command_buffer_to_clone,
      [](mux_queue_t, mux_command_buffer_t, void *const) {}, nullptr, 0,
      nullptr, nullptr));
  ASSERT_SUCCESS(muxFinalizeCommandBuffer(command_buffer_to_clone));
  ASSERT_SUCCESS(muxCloneCommandBuffer(
      device, allocator, command_buffer_to_clone, &out_command_buffer));
}

/// @brief Helper test fixture for testing muxCloneCommandBuffer where the
/// command buffer being copied contains a command that uses a query pool.
struct muxCloneCommandBufferQueryTest : muxCloneCommandBufferTest {
  /// @brief Queue on which querying will occur.
  mux_queue_t queue = nullptr;
  /// @brief Query pool used by the query.
  mux_query_pool_t query_pool = nullptr;
  /// @brief Number of queries to allocate storage for.
  constexpr static uint32_t query_count = 1;
  /// @brief Query slot index that will contain the result.
  constexpr static uint32_t query_index = 0;

  /// @brief Virtual method used to setup any resources for the test fixture
  /// that can't be done in the constructor.
  void SetUp() override {
    // Do the setup for the parent class.
    RETURN_ON_FATAL_FAILURE(muxCloneCommandBufferTest::SetUp());

    ASSERT_SUCCESS(muxGetQueue(device, mux_queue_type_compute, 0, &queue));
    ASSERT_SUCCESS(muxCreateQueryPool(queue, mux_query_type_duration,
                                      query_count, nullptr, allocator,
                                      &query_pool));
  }

  /// @brief Virtual method used to tear down any resources for the test
  /// fixture that can't be done in the destructor.
  void TearDown() override {
    // Cleanup.
    if (nullptr != query_pool) {
      muxDestroyQueryPool(queue, query_pool, allocator);
    }

    // Do the tear down for the parent class.
    muxCloneCommandBufferTest::TearDown();
  }
};

TEST_P(muxCloneCommandBufferQueryTest, CloneBeginQuery) {
  ASSERT_SUCCESS(muxCommandBeginQuery(command_buffer_to_clone, query_pool,
                                      query_index, query_count, 0, nullptr,
                                      nullptr));
  ASSERT_SUCCESS(muxFinalizeCommandBuffer(command_buffer_to_clone));
  ASSERT_SUCCESS(muxCloneCommandBuffer(
      device, allocator, command_buffer_to_clone, &out_command_buffer));
}

TEST_P(muxCloneCommandBufferQueryTest, CloneEndQuery) {
  ASSERT_SUCCESS(muxCommandBeginQuery(command_buffer_to_clone, query_pool,
                                      query_index, query_count, 0, nullptr,
                                      nullptr));
  ASSERT_SUCCESS(muxCommandEndQuery(command_buffer_to_clone, query_pool,
                                    query_index, query_count, 0, nullptr,
                                    nullptr));
  ASSERT_SUCCESS(muxFinalizeCommandBuffer(command_buffer_to_clone));
  ASSERT_SUCCESS(muxCloneCommandBuffer(
      device, allocator, command_buffer_to_clone, &out_command_buffer));
}

TEST_P(muxCloneCommandBufferQueryTest, CloneResetQueryPool) {
  ASSERT_SUCCESS(muxCommandResetQueryPool(command_buffer_to_clone, query_pool,
                                          query_index, query_count, 0, nullptr,
                                          nullptr));
  ASSERT_SUCCESS(muxFinalizeCommandBuffer(command_buffer_to_clone));
  ASSERT_SUCCESS(muxCloneCommandBuffer(
      device, allocator, command_buffer_to_clone, &out_command_buffer));
}

TEST_P(muxCloneCommandBufferTest, CloneUserCallbackThenExecute) {
  // Set up some trivial callback to check the command buffer actually ran.
  auto flag = false;
  auto user_callback = [](mux_queue_t, mux_command_buffer_t,
                          void *const user_data) {
    auto *flag = static_cast<bool *>(user_data);
    *flag = true;
  };

  // Enqueue the cloned command buffer.
  ASSERT_SUCCESS(muxCommandUserCallback(command_buffer_to_clone, user_callback,
                                        &flag, 0, nullptr, nullptr));
  ASSERT_SUCCESS(muxFinalizeCommandBuffer(command_buffer_to_clone));
  ASSERT_SUCCESS(muxCloneCommandBuffer(
      device, allocator, command_buffer_to_clone, &out_command_buffer));
  ASSERT_SUCCESS(muxDispatch(queue, out_command_buffer, nullptr, nullptr, 0,
                             nullptr, 0, nullptr, nullptr));

  // Check the callback got executed by the cloned command buffer.
  ASSERT_SUCCESS(muxWaitAll(queue));
  ASSERT_EQ(flag, true);
}

TEST_P(muxCloneCommandBufferTest, CloneUserCallbackThenCheckCommandBuffer) {
  // Set up a callback that stores the address of the command buffer that
  // executes it.
  mux_command_buffer_t command_buffer_executed = nullptr;

  auto user_callback = [](mux_queue_t, mux_command_buffer_t command_buffer,
                          void *const user_data) {
    auto *command_buffer_executed =
        static_cast<mux_command_buffer_t *>(user_data);
    *command_buffer_executed = command_buffer;
  };

  // Enqueue the original command buffer.
  ASSERT_SUCCESS(muxCommandUserCallback(command_buffer_to_clone, user_callback,
                                        &command_buffer_executed, 0, nullptr,
                                        nullptr));
  ASSERT_SUCCESS(muxFinalizeCommandBuffer(command_buffer_to_clone));
  ASSERT_SUCCESS(muxDispatch(queue, command_buffer_to_clone, nullptr, nullptr,
                             0, nullptr, 0, nullptr, nullptr));

  // Check the callback got executed by the original command buffer.
  ASSERT_SUCCESS(muxWaitAll(queue));
  ASSERT_EQ(command_buffer_executed, command_buffer_to_clone);

  // Clone the command buffer and enqueue the clone.
  ASSERT_SUCCESS(muxCloneCommandBuffer(
      device, allocator, command_buffer_to_clone, &out_command_buffer));
  ASSERT_SUCCESS(muxDispatch(queue, out_command_buffer, nullptr, nullptr, 0,
                             nullptr, 0, nullptr, nullptr));

  // Check the callback got executed by the cloned command buffer.
  ASSERT_SUCCESS(muxWaitAll(queue));
  ASSERT_EQ(command_buffer_executed, out_command_buffer);
}

TEST_P(muxCloneCommandBufferTest, CloneUserCallbackInLoop) {
  // Helper class wrapping storage and associated mutex.
  struct command_buffers_executed_t {
    /// @brief Storage for checking each cloned command got executed.
    std::vector<mux_command_buffer_t> command_buffers_executed;
    /// @brief Mutex for protecting concurrent accesses to the storage.
    std::mutex mutex;
  };

  // Number of clones to perform.
  constexpr size_t iterations = 256;

  command_buffers_executed_t command_buffers_executed;
  command_buffers_executed.command_buffers_executed.reserve(iterations + 1);

  // Set up a callback that stores the address of the command buffer that
  // executes it.
  auto user_callback = [](mux_queue_t, mux_command_buffer_t command_buffer,
                          void *const user_data) {
    auto *command_buffers_executed =
        static_cast<command_buffers_executed_t *>(user_data);
    const std::lock_guard<std::mutex> guard(command_buffers_executed->mutex);
    command_buffers_executed->command_buffers_executed.push_back(
        command_buffer);
  };

  // Enqueue the original command buffer.
  ASSERT_SUCCESS(muxCommandUserCallback(command_buffer_to_clone, user_callback,
                                        &command_buffers_executed, 0, nullptr,
                                        nullptr));
  ASSERT_SUCCESS(muxFinalizeCommandBuffer(command_buffer_to_clone));
  ASSERT_SUCCESS(muxDispatch(queue, command_buffer_to_clone, /*fence */ nullptr,
                             nullptr, 0, nullptr, 0, nullptr, nullptr));

  // Clone the command buffer n times in a loop and enqueue it.
  mux_command_buffer_t cloned_command_buffers[iterations];
  for (size_t iteration = 0; iteration < iterations; ++iteration) {
    ASSERT_SUCCESS(muxCloneCommandBuffer(device, allocator,
                                         command_buffer_to_clone,
                                         cloned_command_buffers + iteration));
    EXPECT_SUCCESS(muxDispatch(queue, cloned_command_buffers[iteration],
                               nullptr, nullptr, 0, nullptr, 0, nullptr,
                               nullptr));
  }

  // Finish the queue.
  EXPECT_SUCCESS(muxWaitAll(queue));

  // Check all the command buffers got executed.
  for (const auto &command_buffer : cloned_command_buffers) {
    auto found =
        std::end(command_buffers_executed.command_buffers_executed) !=
        std::find(std::begin(command_buffers_executed.command_buffers_executed),
                  std::end(command_buffers_executed.command_buffers_executed),
                  command_buffer);
    EXPECT_TRUE(found) << "command buffer: " << command_buffer
                       << " was not executed";
  }

  // Cleanup all the cloned command buffers.
  for (auto &command_buffer : cloned_command_buffers) {
    muxDestroyCommandBuffer(device, command_buffer, allocator);
  }
}

TEST_P(muxCloneCommandBufferTest, CloneUserCallbackInLoopBlocking) {
  // Set up a callback that stores the address of the command buffer that
  // executes it.
  mux_command_buffer_t command_buffer_executed = nullptr;

  auto user_callback = [](mux_queue_t, mux_command_buffer_t command_buffer,
                          void *const user_data) {
    auto *command_buffer_executed =
        static_cast<mux_command_buffer_t *>(user_data);
    *command_buffer_executed = command_buffer;
  };

  // Enqueue the original command buffer.
  ASSERT_SUCCESS(muxCommandUserCallback(command_buffer_to_clone, user_callback,
                                        &command_buffer_executed, 0, nullptr,
                                        nullptr));
  ASSERT_SUCCESS(muxFinalizeCommandBuffer(command_buffer_to_clone));
  ASSERT_SUCCESS(muxDispatch(queue, command_buffer_to_clone, nullptr, nullptr,
                             0, nullptr, 0, nullptr, nullptr));

  // Check the callback got executed by the original command buffer.
  ASSERT_SUCCESS(muxWaitAll(queue));
  ASSERT_EQ(command_buffer_executed, command_buffer_to_clone);

  // Clone the command buffer n times in a loop and enqueue it.
  constexpr size_t iterations = 256;
  mux_command_buffer_t cloned_command_buffers[iterations];
  for (size_t iteration = 0; iteration < iterations; ++iteration) {
    ASSERT_SUCCESS(muxCloneCommandBuffer(device, allocator,
                                         command_buffer_to_clone,
                                         cloned_command_buffers + iteration));
    EXPECT_SUCCESS(muxDispatch(queue, cloned_command_buffers[iteration],
                               nullptr, nullptr, 0, nullptr, 0, nullptr,
                               nullptr));
    EXPECT_SUCCESS(muxWaitAll(queue));
    EXPECT_EQ(command_buffer_executed, cloned_command_buffers[iteration]);
  }

  // Cleanup all the cloned command buffers.
  for (auto &command_buffer : cloned_command_buffers) {
    muxDestroyCommandBuffer(device, command_buffer, allocator);
  }
}

TEST_P(muxCloneCommandBufferTest, CloneUserCallbackInLoopSemaphores) {
  // Number of clones to perform.
  constexpr size_t iterations = 256;

  // We need N semaphores to chain the N + 1 commands.
  mux_semaphore_t semaphores[iterations];

  for (auto &semaphore : semaphores) {
    ASSERT_SUCCESS(muxCreateSemaphore(device, allocator, &semaphore));
  }

  std::vector<mux_command_buffer_t> command_buffers_executed;
  command_buffers_executed.reserve(iterations + 1);

  // Set up a callback that stores the address of the command buffer that
  // executes it.
  auto user_callback = [](mux_queue_t, mux_command_buffer_t command_buffer,
                          void *const user_data) {
    auto *command_buffers_executed =
        static_cast<std::vector<mux_command_buffer_t> *>(user_data);
    command_buffers_executed->push_back(command_buffer);
  };

  // Enqueue the original command buffer.
  EXPECT_SUCCESS(muxCommandUserCallback(command_buffer_to_clone, user_callback,
                                        &command_buffers_executed, 0, nullptr,
                                        nullptr));
  ASSERT_SUCCESS(muxFinalizeCommandBuffer(command_buffer_to_clone));
  EXPECT_SUCCESS(muxDispatch(queue, command_buffer_to_clone, nullptr, nullptr,
                             0, semaphores, 1, nullptr, nullptr));

  // Clone the command buffer n times in a loop and enqueue it.
  mux_command_buffer_t cloned_command_buffers[iterations];
  for (size_t iteration = 0; iteration < iterations; ++iteration) {
    EXPECT_SUCCESS(muxCloneCommandBuffer(device, allocator,
                                         command_buffer_to_clone,
                                         cloned_command_buffers + iteration));
    // The nth dispatch should wait on the nth signal semaphore.
    auto wait_semaphore = semaphores + iteration;
    // The nth dispatch should signal the wait semaphore semaphore of the (n +
    // 1)th dispatch, apart from the last iteration which shouldn't signal
    // anything.
    auto signal_semaphore =
        (iterations - 1 != iteration) ? semaphores + iteration + 1 : nullptr;
    auto signal_semaphore_count = (iterations - 1 != iteration) ? 1 : 0;
    EXPECT_SUCCESS(muxDispatch(queue, cloned_command_buffers[iteration],
                               nullptr, wait_semaphore, 1, signal_semaphore,
                               signal_semaphore_count, nullptr, nullptr));
  }

  // Finish the queue.
  EXPECT_SUCCESS(muxWaitAll(queue));

  // Check all the command buffers got executed.
  for (const auto &command_buffer : cloned_command_buffers) {
    auto found = std::end(command_buffers_executed) !=
                 std::find(std::begin(command_buffers_executed),
                           std::end(command_buffers_executed), command_buffer);
    EXPECT_TRUE(found) << "command buffer: " << command_buffer
                       << " was not executed";
  }

  // Cleanup all the cloned command buffers.
  for (auto &command_buffer : cloned_command_buffers) {
    muxDestroyCommandBuffer(device, command_buffer, allocator);
  }

  // Cleanup all the semaphores.
  for (auto &semaphore : semaphores) {
    muxDestroySemaphore(device, semaphore, allocator);
  }
}

// Instantiate the test suites so that they run for all devices.
INSTANTIATE_DEVICE_TEST_SUITE_P(muxCloneCommandBufferUnsupportedTest);
INSTANTIATE_DEVICE_TEST_SUITE_P(muxCloneCommandBufferTest);
INSTANTIATE_DEVICE_TEST_SUITE_P(muxCloneCommandBufferSingleBufferTest);
INSTANTIATE_DEVICE_TEST_SUITE_P(muxCloneCommandBufferTwoBufferTest);
INSTANTIATE_DEVICE_TEST_SUITE_P(muxCloneCommandBufferSingleImageTest);
INSTANTIATE_DEVICE_TEST_SUITE_P(muxCloneCommandBufferTwoImageTest);
INSTANTIATE_DEVICE_TEST_SUITE_P(muxCloneCommandBufferImageBufferTest);
INSTANTIATE_DEVICE_TEST_SUITE_P(muxCloneCommandBufferQueryTest);
INSTANTIATE_DEVICE_TEST_SUITE_P(muxCloneCommandBufferNDRangeTest);
