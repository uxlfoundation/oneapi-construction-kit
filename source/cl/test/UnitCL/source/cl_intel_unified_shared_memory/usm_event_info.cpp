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

    initPointers(bytes, align);

    cl_int err;
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
    if (queue) {
      EXPECT_SUCCESS(clReleaseCommandQueue(queue));
    }

    cl_intel_unified_shared_memory_Test::TearDown();
  }

  static const size_t bytes = 512;
  static const cl_uint align = 4;

  cl_command_queue queue = nullptr;
};

// Test for valid API usage of clEnqueueMemFillINTEL()
TEST_F(USMEventInfoTest, clEnqueueMemFillINTEL_EventInfo) {
  const cl_int pattern[1] = {CL_INT_MAX};

  cl_int err;

  for (auto ptr : allPointers()) {
    cl_event wait_event;
    err = clEnqueueMemFillINTEL(queue, ptr, pattern, sizeof(pattern),
                                sizeof(pattern) * 2, 0, nullptr, &wait_event);
    EXPECT_SUCCESS(err);

    ASSERT_SUCCESS(GetEventInfoHelper(
        wait_event, CL_EVENT_COMMAND_EXECUTION_STATUS, CL_QUEUED));

    ASSERT_SUCCESS(clWaitForEvents(1, &wait_event));

    ASSERT_SUCCESS(GetEventInfoHelper(
        wait_event, CL_EVENT_COMMAND_EXECUTION_STATUS, CL_COMPLETE));

    ASSERT_SUCCESS(GetEventInfoHelper(wait_event, CL_EVENT_COMMAND_TYPE,
                                      CL_COMMAND_MEMFILL_INTEL));

    EXPECT_SUCCESS(clReleaseEvent(wait_event));
  }
}

TEST_F(USMEventInfoTest, clEnqueueMemcpyINTEL_EventInfo) {
  std::array<std::pair<void *, void *>, 6> pairs = {{
      {host_ptr, device_ptr},
      {host_ptr, shared_ptr},
      {device_ptr, host_ptr},
      {shared_ptr, host_ptr},
      {shared_ptr, device_ptr},
      {device_ptr, shared_ptr},
  }};

  for (auto pair : pairs) {
    auto *ptr_a = pair.first;
    auto *ptr_b = pair.second;
    if (!ptr_a) {
      continue;
    }

    std::array<cl_event, 2> events;
    void *offset_ptr = getPointerOffset(ptr_a, sizeof(cl_int));
    cl_int err = clEnqueueMemcpyINTEL(queue, CL_TRUE, offset_ptr, ptr_a,
                                      sizeof(cl_int), 0, nullptr, &events[0]);
    EXPECT_SUCCESS(err);

    ASSERT_SUCCESS(GetEventInfoHelper(
        events[0], CL_EVENT_COMMAND_EXECUTION_STATUS, CL_COMPLETE));

    ASSERT_SUCCESS(GetEventInfoHelper(events[0], CL_EVENT_COMMAND_TYPE,
                                      CL_COMMAND_MEMCPY_INTEL));

    if (ptr_b) {
      err = clEnqueueMemcpyINTEL(queue, CL_FALSE, ptr_a, ptr_b, sizeof(cl_int),
                                 0, nullptr, &events[1]);
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
}

TEST_F(USMEventInfoTest, clEnqueueMigrateMemINTEL_EventInfo) {
  for (auto ptr : allPointers()) {
    cl_event event;
    cl_int err = clEnqueueMigrateMemINTEL(
        queue, ptr, bytes, CL_MIGRATE_MEM_OBJECT_HOST, 0, nullptr, &event);
    EXPECT_SUCCESS(err);

    ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_COMMAND_EXECUTION_STATUS,
                                      CL_QUEUED));

    ASSERT_SUCCESS(clWaitForEvents(1, &event));

    ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_COMMAND_EXECUTION_STATUS,
                                      CL_COMPLETE));

    ASSERT_SUCCESS(GetEventInfoHelper(event, CL_EVENT_COMMAND_TYPE,
                                      CL_COMMAND_MIGRATEMEM_INTEL));

    EXPECT_SUCCESS(clReleaseEvent(event));
  }
}

TEST_F(USMEventInfoTest, clEnqueueMemAdviseINTEL_EventInfo) {
  for (auto ptr : allPointers()) {
    // Create a user event to block advise command happening immediately
    cl_int err;
    cl_event user_event = clCreateUserEvent(context, &err);
    cl_event wait_event;
    ASSERT_SUCCESS(err);

    // Enqueue a no-op advise command
    const cl_mem_advice_intel advice = 0;
    err = clEnqueueMemAdviseINTEL(queue, ptr, bytes, advice, 1, &user_event,
                                  &wait_event);
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
}
