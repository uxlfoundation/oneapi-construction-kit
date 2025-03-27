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

#include <thread>

#include "Common.h"

enum {
  buffer_size = 128,
};

class clGetEventProfilingInfoNegativeTest : public ucl::CommandQueueTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    cl_int status;
    buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, buffer_size, nullptr,
                            &status);
    ASSERT_SUCCESS(status);
    char data[buffer_size];
    ASSERT_SUCCESS(clEnqueueWriteBuffer(command_queue, buffer, CL_TRUE, 0,
                                        buffer_size, data, 0, nullptr, &event));
  }

  void TearDown() override {
    if (event) {
      ASSERT_SUCCESS(clReleaseEvent(event));
    }
    if (buffer) {
      ASSERT_SUCCESS(clReleaseMemObject(buffer));
    }
    CommandQueueTest::TearDown();
  }

  cl_mem buffer = nullptr;
  cl_event event = nullptr;
};

TEST_F(clGetEventProfilingInfoNegativeTest, ProfilingNotAvailable) {
  cl_ulong val;
  size_t size;
  ASSERT_EQ_ERRCODE(CL_PROFILING_INFO_NOT_AVAILABLE,
                    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_QUEUED,
                                            sizeof(cl_ulong), &val, &size));
}

class clGetEventProfilingInfoTest : public ucl::ContextTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    cl_int status;
    command_queue = clCreateCommandQueue(context, device,
                                         CL_QUEUE_PROFILING_ENABLE, &status);
    ASSERT_SUCCESS(status);
    buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, buffer_size, nullptr,
                            &status);
    EXPECT_TRUE(buffer);
    ASSERT_SUCCESS(status);
    char data[buffer_size];
    ASSERT_SUCCESS(clEnqueueWriteBuffer(command_queue, buffer, CL_FALSE, 0,
                                        buffer_size, data, 0, nullptr, &event));
    ASSERT_SUCCESS(clWaitForEvents(1, &event));
  }

  void TearDown() override {
    if (event) {
      EXPECT_SUCCESS(clReleaseEvent(event));
    }
    if (buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(buffer));
    }
    if (command_queue) {
      EXPECT_SUCCESS(clReleaseCommandQueue(command_queue));
    }
    ContextTest::TearDown();
  }

  cl_mem buffer = nullptr;
  cl_event event = nullptr;
  cl_command_queue command_queue = nullptr;
};

TEST_F(clGetEventProfilingInfoTest, InvalidValue) {
  cl_ulong val;
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clGetEventProfilingInfo(event, CL_SUCCESS, sizeof(cl_ulong),
                                            &val, nullptr));
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_QUEUED,
                                            0, &val, nullptr));
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, clGetEventProfilingInfo(
                                          event, CL_PROFILING_COMMAND_SUBMIT,
                                          sizeof(cl_ulong) - 1, &val, nullptr));
}

TEST_F(clGetEventProfilingInfoTest, InvalidEvent) {
  cl_ulong val;
  ASSERT_EQ_ERRCODE(CL_INVALID_EVENT, clGetEventProfilingInfo(
                                          nullptr, CL_PROFILING_COMMAND_QUEUED,
                                          sizeof(cl_ulong), &val, nullptr));
}

TEST_F(clGetEventProfilingInfoTest, Default) {
  size_t size;
  ASSERT_SUCCESS(clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_QUEUED, 0,
                                         nullptr, &size));
  ASSERT_EQ(sizeof(cl_ulong), size);
  cl_ulong queued;
  ASSERT_SUCCESS(clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_QUEUED,
                                         size, &queued, nullptr));

  ASSERT_SUCCESS(clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_SUBMIT, 0,
                                         nullptr, &size));
  ASSERT_EQ(sizeof(cl_ulong), size);
  cl_ulong submit;
  ASSERT_SUCCESS(clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_SUBMIT,
                                         size, &submit, nullptr));

  ASSERT_SUCCESS(clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, 0,
                                         nullptr, &size));
  ASSERT_EQ(sizeof(cl_ulong), size);
  cl_ulong start;
  ASSERT_SUCCESS(clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START,
                                         size, &start, nullptr));

  ASSERT_SUCCESS(clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, 0,
                                         nullptr, &size));
  ASSERT_EQ(sizeof(cl_ulong), size);
  cl_ulong end;
  ASSERT_SUCCESS(clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, size,
                                         &end, nullptr));
  EXPECT_GE(submit, queued);
  EXPECT_GE(start, queued);
  EXPECT_GE(end, start);
}

TEST_F(clGetEventProfilingInfoTest, EarlyRequest) {
  // SetUp already created a command with an event and waited on it, that's
  // fine, now we're going to put some more things into the queue.

  cl_int err = !CL_SUCCESS;
  cl_event delay = clCreateUserEvent(context, &err);
  ASSERT_SUCCESS(err);

  cl_event write;
  char data[buffer_size];
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, buffer, CL_FALSE, 0,
                                      buffer_size, data, 1, &delay, &write));

  // We know that 'delay' has not yet been triggered and thus 'write' is
  // definitely not yet CL_COMPLETE as it is blocked by 'delay'.  Thus profiling
  // info can not possibly be ready yet.
  cl_ulong end;
  EXPECT_EQ_ERRCODE(CL_PROFILING_INFO_NOT_AVAILABLE,
                    clGetEventProfilingInfo(write, CL_PROFILING_COMMAND_END,
                                            sizeof(end), &end, nullptr));

  // Trigger 'delay' and wait for 'write' to complete.
  EXPECT_SUCCESS(clSetUserEventStatus(delay, CL_SUCCESS));
  EXPECT_SUCCESS(clWaitForEvents(1, &write));

  // Now that 'write' is complete we *can* get profiling info.
  EXPECT_SUCCESS(clGetEventProfilingInfo(write, CL_PROFILING_COMMAND_END,
                                         sizeof(end), &end, nullptr));

  ASSERT_SUCCESS(clReleaseEvent(write));
  ASSERT_SUCCESS(clReleaseEvent(delay));
}

// This test is like EarlyRequest above, except we will spin on
// clGetEventProfilingInfo to try and catch any locking issues between reading
// and writing profiling counters.  This test is most interesting when run in a
// thread sanitizer type environment.
TEST_F(clGetEventProfilingInfoTest, Race) {
  // SetUp already created a command with an event and waited on it, that's
  // fine, now we're going to put some more things into the queue.

  cl_int err = !CL_SUCCESS;
  cl_event delay = clCreateUserEvent(context, &err);
  ASSERT_SUCCESS(err);

  cl_event write;
  char data[buffer_size];
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, buffer, CL_FALSE, 0,
                                      buffer_size, data, 1, &delay, &write));

  // We know that 'delay' has not yet been triggered and thus 'write' is
  // definitely not yet CL_COMPLETE as it is blocked by 'delay'.  Thus
  // profiling info can not possibly be ready yet.  So we launch a thread that
  // will just keep on checking for event profiling info, it should fail
  // initially, and then pass once we unblock the events below.
  cl_int status = !CL_SUCCESS;
  auto thread = std::thread([&write, &status]() {
    while (status != CL_SUCCESS) {
      cl_ulong end;
      status = clGetEventProfilingInfo(write, CL_PROFILING_COMMAND_END,
                                       sizeof(end), &end, nullptr);
    }
  });

  // Trigger 'delay' and wait for 'write' to complete.
  EXPECT_SUCCESS(clSetUserEventStatus(delay, CL_SUCCESS));
  EXPECT_SUCCESS(clWaitForEvents(1, &write));

  // The thread should be able to finish now, and the final value of 'status'
  // should be CL_SUCCESS.
  thread.join();
  EXPECT_SUCCESS(status);

  ASSERT_SUCCESS(clReleaseEvent(write));
  ASSERT_SUCCESS(clReleaseEvent(delay));
}

#if defined(CL_VERSION_3_0)
class clGetEventProfilingInfoTestScalarQueryOpenCL30
    : public clGetEventProfilingInfoTest,
      public testing::WithParamInterface<std::tuple<size_t, int>> {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(clGetEventProfilingInfoTest::SetUp());
    // Skip for non OpenCL-3.0 implementations.
    if (!UCL::isDeviceVersionAtLeast({3, 0})) {
      GTEST_SKIP();
    }
  }
};

TEST_P(clGetEventProfilingInfoTestScalarQueryOpenCL30, CheckSizeQuerySucceeds) {
  // Get the enumeration value.
  auto query_enum_value = std::get<1>(GetParam());
  // Query for size of value.
  size_t size{};
  EXPECT_SUCCESS(
      clGetEventProfilingInfo(event, query_enum_value, 0, nullptr, &size));
}

TEST_P(clGetEventProfilingInfoTestScalarQueryOpenCL30,
       CheckSizeQueryIsCorrect) {
  // Get the enumeration value.
  auto query_enum_value = std::get<1>(GetParam());
  // Query for size of value.
  size_t size{};
  ASSERT_SUCCESS(
      clGetEventProfilingInfo(event, query_enum_value, 0, nullptr, &size));
  // Get the correct size of the query.
  auto value_size_in_bytes = std::get<0>(GetParam());
  // Check the queried value is correct.
  EXPECT_EQ(size, value_size_in_bytes);
}

TEST_P(clGetEventProfilingInfoTestScalarQueryOpenCL30, CheckQuerySucceeds) {
  // Get the correct size of the query and the query itself.
  auto value_size_in_bytes = std::get<0>(GetParam());
  auto query_enum_value = std::get<1>(GetParam());
  // Query for the value.
  UCL::Buffer<char> value_buffer{value_size_in_bytes};
  EXPECT_SUCCESS(clGetEventProfilingInfo(event, query_enum_value,
                                         value_buffer.size(),
                                         value_buffer.data(), nullptr));
}

TEST_P(clGetEventProfilingInfoTestScalarQueryOpenCL30,
       CheckIncorrectSizeQueryFails) {
  // Get the correct size of the query and the query itself.
  auto value_size_in_bytes = std::get<0>(GetParam());
  auto query_enum_value = std::get<1>(GetParam());
  // Query for the value with buffer that is too small.
  UCL::Buffer<char> value_buffer{value_size_in_bytes};
  EXPECT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clGetEventProfilingInfo(event, query_enum_value, value_buffer.size() - 1,
                              value_buffer.data(), nullptr));
}

INSTANTIATE_TEST_CASE_P(
    EventProfilingQuery, clGetEventProfilingInfoTestScalarQueryOpenCL30,
    testing::Values(std::make_tuple(sizeof(cl_ulong),
                                    CL_PROFILING_COMMAND_COMPLETE)),
    [](const testing::TestParamInfo<
        clGetEventProfilingInfoTestScalarQueryOpenCL30::ParamType> &Info) {
      return UCL::profilingQueryToString(std::get<1>(Info.param));
    });

TEST_F(clGetEventProfilingInfoTestScalarQueryOpenCL30,
       CommandCompleteNoDeviceSideEnqueue) {
  // Skip for non OpenCL-3.0 implementations.
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }
  // When passing CL_PROFILING_COMMAND_COMPLETE clGetEventProfilingInfo
  // returns a value equivalent to passing CL_PROFILING_COMMAND_END if the
  // device associated with event does not support On-Device Enqueue.

  // Check whether device side enqueue is supported.
  cl_device_device_enqueue_capabilities device_enqueue_capabilities{};
  ASSERT_SUCCESS(clGetDeviceInfo(device, CL_DEVICE_DEVICE_ENQUEUE_CAPABILITIES,
                                 sizeof(device_enqueue_capabilities),
                                 &device_enqueue_capabilities, nullptr));

  if (device_enqueue_capabilities != 0) {
    // If it is, query the CL_PROFILING_COMMAND_COMPLETE and check it matches
    // CL_PROFILING_COMMAND_END.
    cl_profiling_info command_complete{};
    ASSERT_SUCCESS(clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_COMPLETE,
                                           sizeof(cl_profiling_info),
                                           &command_complete, nullptr));
    cl_profiling_info command_end{};
    ASSERT_SUCCESS(clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_COMPLETE,
                                           sizeof(cl_profiling_info),
                                           &command_end, nullptr));
    EXPECT_EQ(command_complete, command_end);
  }
}
#endif
