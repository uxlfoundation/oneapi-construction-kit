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

struct USMEventInfoTest : public cl_intel_unified_shared_memory_Test {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(cl_intel_unified_shared_memory_Test::SetUp());

    cl_device_unified_shared_memory_capabilities_intel host_capabilities;
    ASSERT_SUCCESS(clGetDeviceInfo(
        device, CL_DEVICE_HOST_MEM_CAPABILITIES_INTEL,
        sizeof(host_capabilities), &host_capabilities, nullptr));

    cl_int err;
    if (host_capabilities) {
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

  template <typename T>
  cl_int GetEventInfoHelper(cl_event event, cl_event_info param_name,
                            T expected) {
    size_t size_needed;
    cl_int errcode =
        clGetEventInfo(event, param_name, 0, nullptr, &size_needed);
    if (CL_SUCCESS != errcode) {
      return errcode;
    }

    EXPECT_EQ(sizeof(T), size_needed);

    T result;
    errcode = clGetEventInfo(event, param_name, size_needed, &result, nullptr);
    EXPECT_SUCCESS(errcode);
    if (expected != result) {
      EXPECT_EQ(expected, result);
      errcode = CL_INVALID_VALUE;
    }
    return errcode;
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

  static const size_t bytes = 512;
  static const cl_uint align = 4;

  void *host_ptr = nullptr;
  void *device_ptr = nullptr;

  cl_command_queue queue = nullptr;
};

// Test for valid API usage of clEnqueueMemFillINTEL()
TEST_F(USMEventInfoTest, clEnqueueMemFillINTEL_EventInfo) {
  const cl_int pattern[1] = {CL_INT_MAX};

  cl_int err;

  std::array<cl_event, 2> wait_events;
  err = clEnqueueMemFillINTEL(queue, device_ptr, pattern, sizeof(pattern),
                              sizeof(pattern) * 2, 0, nullptr, &wait_events[0]);
  EXPECT_SUCCESS(err);

  ASSERT_SUCCESS(GetEventInfoHelper(
      wait_events[0], CL_EVENT_COMMAND_EXECUTION_STATUS, CL_QUEUED));

  ASSERT_SUCCESS(clWaitForEvents(1, &wait_events[0]));

  ASSERT_SUCCESS(GetEventInfoHelper(
      wait_events[0], CL_EVENT_COMMAND_EXECUTION_STATUS, CL_COMPLETE));

  ASSERT_SUCCESS(GetEventInfoHelper(wait_events[0], CL_EVENT_COMMAND_TYPE,
                                    CL_COMMAND_MEMFILL_INTEL));

  if (host_ptr) {
    err =
        clEnqueueMemFillINTEL(queue, host_ptr, pattern, sizeof(pattern),
                              sizeof(pattern) * 2, 0, nullptr, &wait_events[1]);
    EXPECT_SUCCESS(err);

    ASSERT_SUCCESS(GetEventInfoHelper(
        wait_events[1], CL_EVENT_COMMAND_EXECUTION_STATUS, CL_QUEUED));

    ASSERT_SUCCESS(clWaitForEvents(1, &wait_events[1]));

    ASSERT_SUCCESS(GetEventInfoHelper(
        wait_events[1], CL_EVENT_COMMAND_EXECUTION_STATUS, CL_COMPLETE));

    ASSERT_SUCCESS(GetEventInfoHelper(wait_events[1], CL_EVENT_COMMAND_TYPE,
                                      CL_COMMAND_MEMFILL_INTEL));
  }

  EXPECT_SUCCESS(clReleaseEvent(wait_events[0]));
  if (host_ptr) {
    EXPECT_SUCCESS(clReleaseEvent(wait_events[1]));
  }
}

TEST_F(USMEventInfoTest, clEnqueueMemcpyINTEL_EventInfo) {
  std::array<cl_event, 2> events;
  void *offset_device_ptr = getPointerOffset(device_ptr, sizeof(cl_int));
  cl_int err =
      clEnqueueMemcpyINTEL(queue, CL_TRUE, offset_device_ptr, device_ptr,
                           sizeof(cl_int), 0, nullptr, &events[0]);
  EXPECT_SUCCESS(err);

  ASSERT_SUCCESS(GetEventInfoHelper(
      events[0], CL_EVENT_COMMAND_EXECUTION_STATUS, CL_COMPLETE));

  ASSERT_SUCCESS(GetEventInfoHelper(events[0], CL_EVENT_COMMAND_TYPE,
                                    CL_COMMAND_MEMCPY_INTEL));

  if (host_ptr) {
    err = clEnqueueMemcpyINTEL(queue, CL_FALSE, device_ptr, host_ptr,
                               sizeof(cl_int), 0, nullptr, &events[1]);
    EXPECT_SUCCESS(err);

    ASSERT_SUCCESS(GetEventInfoHelper(
        events[1], CL_EVENT_COMMAND_EXECUTION_STATUS, CL_QUEUED));

    ASSERT_SUCCESS(clWaitForEvents(1, &events[1]));

    ASSERT_SUCCESS(GetEventInfoHelper(
        events[1], CL_EVENT_COMMAND_EXECUTION_STATUS, CL_COMPLETE));

    ASSERT_SUCCESS(GetEventInfoHelper(events[1], CL_EVENT_COMMAND_TYPE,
                                      CL_COMMAND_MEMCPY_INTEL));

    EXPECT_SUCCESS(clReleaseEvent(events[1]));
  }
  EXPECT_SUCCESS(clReleaseEvent(events[0]));
}

TEST_F(USMEventInfoTest, clEnqueueMigrateMemINTEL_EventInfo) {
  std::array<cl_event, 2> events;
  cl_int err = clEnqueueMigrateMemINTEL(queue, device_ptr, bytes,
                                        CL_MIGRATE_MEM_OBJECT_HOST, 0, nullptr,
                                        &events[0]);
  EXPECT_SUCCESS(err);

  ASSERT_SUCCESS(GetEventInfoHelper(
      events[0], CL_EVENT_COMMAND_EXECUTION_STATUS, CL_QUEUED));

  ASSERT_SUCCESS(clWaitForEvents(1, &events[0]));

  ASSERT_SUCCESS(GetEventInfoHelper(
      events[0], CL_EVENT_COMMAND_EXECUTION_STATUS, CL_COMPLETE));

  ASSERT_SUCCESS(GetEventInfoHelper(events[0], CL_EVENT_COMMAND_TYPE,
                                    CL_COMMAND_MIGRATEMEM_INTEL));

  if (host_ptr) {
    err = clEnqueueMigrateMemINTEL(queue, host_ptr, bytes,
                                   CL_MIGRATE_MEM_OBJECT_HOST, 0, nullptr,
                                   &events[1]);
    EXPECT_SUCCESS(err);

    ASSERT_SUCCESS(GetEventInfoHelper(
        events[1], CL_EVENT_COMMAND_EXECUTION_STATUS, CL_QUEUED));

    ASSERT_SUCCESS(clWaitForEvents(1, &events[1]));

    ASSERT_SUCCESS(GetEventInfoHelper(events[1], CL_EVENT_COMMAND_TYPE,
                                      CL_COMMAND_MIGRATEMEM_INTEL));

    ASSERT_SUCCESS(GetEventInfoHelper(
        events[1], CL_EVENT_COMMAND_EXECUTION_STATUS, CL_COMPLETE));
  }

  EXPECT_SUCCESS(clReleaseEvent(events[0]));
  if (host_ptr) {
    EXPECT_SUCCESS(clReleaseEvent(events[1]));
  }
}

TEST_F(USMEventInfoTest, clEnqueueMemAdviseINTEL_EventInfo) {
  if (!device_ptr) {
    GTEST_SKIP();
  }

  // Create a user event to block advise command happening immediately
  cl_int err;
  cl_event user_event = clCreateUserEvent(context, &err);
  cl_event wait_event;
  ASSERT_SUCCESS(err);

  // Enqueue a no-op advise command
  const cl_mem_advice_intel advice = 0;
  err = clEnqueueMemAdviseINTEL(queue, device_ptr, bytes, advice, 1,
                                &user_event, &wait_event);
  EXPECT_SUCCESS(err);

  ASSERT_SUCCESS(GetEventInfoHelper(
      wait_event, CL_EVENT_COMMAND_EXECUTION_STATUS, CL_QUEUED));

  // Set status event status to allow advise command to be dispatched
  EXPECT_SUCCESS(clSetUserEventStatus(user_event, CL_COMPLETE));

  EXPECT_SUCCESS(clReleaseEvent(user_event));

  ASSERT_SUCCESS(clWaitForEvents(1, &wait_event));

  ASSERT_SUCCESS(GetEventInfoHelper(
      wait_event, CL_EVENT_COMMAND_EXECUTION_STATUS, CL_COMPLETE));

  ASSERT_SUCCESS(GetEventInfoHelper(wait_event, CL_EVENT_COMMAND_TYPE,
                                    CL_COMMAND_MEMADVISE_INTEL));

  EXPECT_SUCCESS(clReleaseEvent(wait_event));
}
