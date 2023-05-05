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

#include <Common.h>

#include "cl_intel_unified_shared_memory.h"

// Fixture for testing performance hint APIs `clEnqueueMemAdviseINTEL` &
// `clEnqueueMigrateMemINTEL` which we implement as no-ops. Because these
// commands are no-ops there isn't observable behaviour to verify on
// completion, so checking that each commands respects its event wait list
// dependencies is the main objective of the USMMemHintTest tests. If this isn't
// the case hint commands dispatching immediately could mean that a following
// command in the in-order queue could start running afterwards, when it still
// should be blocked on a hint wait event.
struct USMMemHintTest : public cl_intel_unified_shared_memory_Test {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(cl_intel_unified_shared_memory_Test::SetUp());

    cl_device_unified_shared_memory_capabilities_intel host_capabilities;
    ASSERT_SUCCESS(clGetDeviceInfo(
        device, CL_DEVICE_HOST_MEM_CAPABILITIES_INTEL,
        sizeof(host_capabilities), &host_capabilities, nullptr));

    cl_int err;
    if (host_capabilities != 0) {
      host_ptr = clHostMemAllocINTEL(context, nullptr, bytes, align, &err);
      ASSERT_SUCCESS(err);
      ASSERT_TRUE(host_ptr != nullptr);
    }

    device_ptr =
        clDeviceMemAllocINTEL(context, device, nullptr, bytes, align, &err);
    ASSERT_SUCCESS(err);
    ASSERT_TRUE(device_ptr != nullptr);

    queue = clCreateCommandQueue(context, device, 0, &err);
    ASSERT_TRUE(queue != nullptr);
    ASSERT_SUCCESS(err);
  }

  void TearDown() override {
    if (device_ptr) {
      cl_int err = clMemBlockingFreeINTEL(context, device_ptr);
      EXPECT_SUCCESS(err);
    }

    if (host_ptr) {
      cl_int err = clMemBlockingFreeINTEL(context, host_ptr);
      EXPECT_SUCCESS(err);
    }

    if (queue) {
      EXPECT_SUCCESS(clReleaseCommandQueue(queue));
    }

    cl_intel_unified_shared_memory_Test::TearDown();
  }
  const size_t bytes = 256;
  const cl_uint align = 4;

  void *host_ptr = nullptr;
  void *device_ptr = nullptr;
  cl_command_queue queue = nullptr;
};

// Test migrating a device USM allocation
TEST_F(USMMemHintTest, MigrateDevice) {
  cl_int err;

  // Create a user event to block migration happening immediately
  cl_event user_event = clCreateUserEvent(context, &err);
  ASSERT_SUCCESS(err);

  // Enqueue a migration hint, implemented as a no-op
  err = clEnqueueMigrateMemINTEL(queue, device_ptr, bytes,
                                 CL_MIGRATE_MEM_OBJECT_HOST, 1, &user_event,
                                 nullptr);
  EXPECT_SUCCESS(err);

  // Set status event status to allow migrate command to be dispatched
  EXPECT_SUCCESS(clSetUserEventStatus(user_event, CL_COMPLETE));
  EXPECT_SUCCESS(clReleaseEvent(user_event));

  // Blocking free flushes command queue
  EXPECT_SUCCESS(clMemBlockingFreeINTEL(context, device_ptr));

  // Prevent free being called in test fixture teardown
  device_ptr = nullptr;
}

// Test migrating a host USM allocation
TEST_F(USMMemHintTest, MigrateHost) {
  if (!host_ptr) {
    GTEST_SKIP();
  }

  // Create a user event to block migration happening immediately
  cl_int err;
  cl_event user_event = clCreateUserEvent(context, &err);
  ASSERT_SUCCESS(err);

  // Enqueue a migration hint, implemented as a no-op
  err = clEnqueueMigrateMemINTEL(queue, host_ptr, bytes,
                                 CL_MIGRATE_MEM_OBJECT_CONTENT_UNDEFINED, 1,
                                 &user_event, nullptr);
  EXPECT_SUCCESS(err);

  // Set status event status to allow migrate command to be dispatched
  EXPECT_SUCCESS(clSetUserEventStatus(user_event, CL_COMPLETE));
  EXPECT_SUCCESS(clReleaseEvent(user_event));

  // Blocking free flushes command queue
  EXPECT_SUCCESS(clMemBlockingFreeINTEL(context, host_ptr));

  // Prevent free being called in test fixture teardown
  host_ptr = nullptr;
}

// Test advice performance hint on a device allocation
TEST_F(USMMemHintTest, AdviseDevice) {
  // Create a user event to block advise command happening immediately
  cl_int err;
  cl_event user_event = clCreateUserEvent(context, &err);
  ASSERT_SUCCESS(err);

  // Enqueue a no-op advise command
  const cl_mem_advice_intel advice = 0;
  err = clEnqueueMemAdviseINTEL(queue, device_ptr, bytes, advice, 1,
                                &user_event, nullptr);
  EXPECT_SUCCESS(err);

  // Set status event status to allow advise command to be dispatched
  EXPECT_SUCCESS(clSetUserEventStatus(user_event, CL_COMPLETE));
  EXPECT_SUCCESS(clReleaseEvent(user_event));

  // Blocking free flushes command queue
  EXPECT_SUCCESS(clMemBlockingFreeINTEL(context, device_ptr));

  // Prevent free being called in test fixture teardown
  device_ptr = nullptr;
}

// Test advice performance hint on a host allocation
TEST_F(USMMemHintTest, AdviseHost) {
  if (!host_ptr) {
    GTEST_SKIP();
  }

  // Create a user event to block advise command happening immediately
  cl_int err;
  cl_event user_event = clCreateUserEvent(context, &err);
  ASSERT_SUCCESS(err);

  // Enqueue a no-op advise command
  const cl_mem_advice_intel advice = 0;
  err = clEnqueueMemAdviseINTEL(queue, host_ptr, bytes, advice, 1, &user_event,
                                nullptr);
  EXPECT_SUCCESS(err);

  // Set status event status to allow advise command to be dispatched
  EXPECT_SUCCESS(clSetUserEventStatus(user_event, CL_COMPLETE));
  EXPECT_SUCCESS(clReleaseEvent(user_event));

  // Blocking free flushes command queue
  EXPECT_SUCCESS(clMemBlockingFreeINTEL(context, host_ptr));

  // Prevent free being called in test fixture teardown
  host_ptr = nullptr;
}
