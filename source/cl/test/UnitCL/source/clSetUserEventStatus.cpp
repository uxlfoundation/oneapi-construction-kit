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

#include "Common.h"

class clSetUserEventStatusTest : public ucl::ContextTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
    cl_int errorcode;
    event = clCreateUserEvent(context, &errorcode);
    EXPECT_TRUE(event);
    ASSERT_SUCCESS(errorcode);
  }

  void TearDown() override {
    if (event) {
      EXPECT_SUCCESS(clReleaseEvent(event));
    }
    ContextTest::TearDown();
  }

  cl_event event = nullptr;
};

TEST_F(clSetUserEventStatusTest, Default) {
  ASSERT_SUCCESS(clSetUserEventStatus(event, CL_COMPLETE));

  ASSERT_SUCCESS(clWaitForEvents(1, &event));
}

TEST_F(clSetUserEventStatusTest, FromAnotherEventsCallback) {
  cl_int errorcode;
  cl_command_queue queue = clCreateCommandQueue(context, device, 0, &errorcode);
  EXPECT_NE(nullptr, queue);
  ASSERT_SUCCESS(errorcode);

  cl_event markerEvent;
  ASSERT_SUCCESS(clEnqueueMarkerWithWaitList(queue, 0, nullptr, &markerEvent));
  ASSERT_NE(nullptr, markerEvent);

  ASSERT_SUCCESS(clSetEventCallback(
      markerEvent, CL_COMPLETE,
      [](cl_event, cl_int, void *user_data) CL_LAMBDA_CALLBACK {
        clSetUserEventStatus(static_cast<cl_event>(user_data), CL_COMPLETE);
      },
      event));

  ASSERT_SUCCESS(clFlush(queue));

  ASSERT_SUCCESS(clWaitForEvents(1, &event));

  ASSERT_SUCCESS(clReleaseEvent(markerEvent));
  ASSERT_SUCCESS(clReleaseCommandQueue(queue));
}

TEST_F(clSetUserEventStatusTest, BadEvent) {
  ASSERT_EQ_ERRCODE(CL_INVALID_EVENT,
                    clSetUserEventStatus(nullptr, CL_COMPLETE));
}

TEST_F(clSetUserEventStatusTest, NonUserEvent) {
  cl_int errorcode;
  cl_command_queue queue = clCreateCommandQueue(context, device, 0, &errorcode);
  ASSERT_TRUE(queue);
  ASSERT_SUCCESS(errorcode);
  cl_mem mem = clCreateBuffer(context, 0, 1, nullptr, &errorcode);
  ASSERT_TRUE(mem);
  ASSERT_SUCCESS(errorcode);

  ASSERT_SUCCESS(clReleaseEvent(event));

  const char foo = 42;
  ASSERT_SUCCESS(clEnqueueWriteBuffer(queue, mem, CL_FALSE, 0, 1, &foo, 0,
                                      nullptr, &event));

  ASSERT_EQ_ERRCODE(CL_INVALID_EVENT, clSetUserEventStatus(event, CL_COMPLETE));

  ASSERT_SUCCESS(clReleaseCommandQueue(queue));
  ASSERT_SUCCESS(clReleaseMemObject(mem));
}

TEST_F(clSetUserEventStatusTest, NegativeOkValue) {
  ASSERT_SUCCESS(clSetUserEventStatus(event, CL_COMPLETE - 42));

  ASSERT_EQ_ERRCODE(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST,
                    clWaitForEvents(1, &event));
}

TEST_F(clSetUserEventStatusTest, SetValueTwice) {
  ASSERT_SUCCESS(clSetUserEventStatus(event, CL_COMPLETE));

  ASSERT_EQ_ERRCODE(CL_INVALID_OPERATION,
                    clSetUserEventStatus(event, CL_COMPLETE));
}

TEST_F(clSetUserEventStatusTest, EnsureTerminatedDependentCommandDidNothing) {
  cl_int errorcode;

  cl_command_queue queue = clCreateCommandQueue(context, device, 0, &errorcode);
  EXPECT_TRUE(queue);
  ASSERT_SUCCESS(errorcode);

  int payload = 42;
  const int original_payload = 13;

  cl_mem mem = clCreateBuffer(context, CL_MEM_COPY_HOST_PTR, sizeof(payload),
                              &payload, &errorcode);
  EXPECT_TRUE(mem);
  ASSERT_SUCCESS(errorcode);

  int read_payload = original_payload;
  cl_event other_event;
  ASSERT_SUCCESS(clEnqueueReadBuffer(queue, mem, false, 0, sizeof(read_payload),
                                     &read_payload, 1, &event, &other_event));

  ASSERT_SUCCESS(clSetUserEventStatus(event, CL_INVALID_OPERATION));

  ASSERT_EQ(CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST,
            clWaitForEvents(1, &other_event));

  cl_int execution_status;
  ASSERT_SUCCESS(clGetEventInfo(other_event, CL_EVENT_COMMAND_EXECUTION_STATUS,
                                sizeof(execution_status), &execution_status,
                                nullptr));

  ASSERT_GT(0, execution_status);

  ASSERT_EQ(original_payload, read_payload);

  EXPECT_SUCCESS(clReleaseEvent(other_event));
  EXPECT_SUCCESS(clReleaseMemObject(mem));
  EXPECT_SUCCESS(clReleaseCommandQueue(queue));
}

TEST_F(clSetUserEventStatusTest, ReleaseUserEventInItsCallback) {
  cl_int releaseStatus;

  auto func = [](cl_event event, cl_int, void *user_data) CL_LAMBDA_CALLBACK {
    *static_cast<cl_int *>(user_data) = clReleaseEvent(event);
  };

  ASSERT_SUCCESS(clSetEventCallback(event, CL_COMPLETE, func, &releaseStatus));
  ASSERT_SUCCESS(clSetUserEventStatus(event, CL_COMPLETE));
  ASSERT_SUCCESS(releaseStatus);

  // we've already destroyed the event in the callback
  event = nullptr;
}

TEST_F(clSetUserEventStatusTest, CompletedBeforeWaitList) {
  cl_int error;
  cl_command_queue command_queue =
      clCreateCommandQueue(context, device, 0, &error);
  EXPECT_TRUE(command_queue);

  ASSERT_SUCCESS(clSetUserEventStatus(event, CL_COMPLETE));

  int before = 23;
  cl_mem buffer = clCreateBuffer(context, CL_MEM_COPY_HOST_PTR, sizeof(before),
                                 &before, &error);
  ASSERT_SUCCESS(error);

  int answer = 42;
  cl_event write_event;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, buffer, CL_FALSE, 0,
                                      sizeof(answer), &answer, 1, &event,
                                      &write_event));

  auto bufferPtr = static_cast<int *>(
      clEnqueueMapBuffer(command_queue, buffer, CL_TRUE, CL_MAP_READ, 0,
                         sizeof(answer), 1, &write_event, nullptr, &error));
  EXPECT_SUCCESS(error);

  EXPECT_EQ(answer, *bufferPtr);

  EXPECT_SUCCESS(clReleaseEvent(write_event));
  EXPECT_SUCCESS(clReleaseMemObject(buffer));
  ASSERT_SUCCESS(clReleaseCommandQueue(command_queue));
}

// Abstracts out some of the common code in the user event in-order testing..
class clSetUserEventStatusInOrderTest : public ucl::CommandQueueTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    // This class compiles some kernels so we need a compiler.
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
    // Since these tests test the behavoir of in order queues we should only run
    // them if the queue is in order.
    cl_command_queue_properties command_queue_properties;
    ASSERT_SUCCESS(clGetCommandQueueInfo(command_queue, CL_QUEUE_PROPERTIES,
                                         sizeof(cl_command_queue_properties),
                                         &command_queue_properties, nullptr));
    if (command_queue_properties & CL_QUEUE_OUT_OF_ORDER_EXEC_MODE_ENABLE) {
      GTEST_SKIP();
    }

    cl_int error = CL_SUCCESS;
    const char *program_source = R"OPENCLC(
    kernel void store(global int *dst) { *dst = 0; }
    kernel void load_and_store(global int *src, global int *dst) { *dst = *src; }
    )OPENCLC";
    const size_t code_length = std::strlen(program_source);
    program = clCreateProgramWithSource(context, 1, &program_source,
                                        &code_length, &error);
    ASSERT_SUCCESS(error);
    EXPECT_SUCCESS(
        clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr));
  }

  void TearDown() override {
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    CommandQueueTest::TearDown();
  }

  cl_program program = nullptr;
};

TEST_F(clSetUserEventStatusInOrderTest, BlockQueueOnUserEventWithCommandEvent) {
  cl_int error = CL_SUCCESS;
  // Create three kernels which store and load and store single values.
  cl_kernel store = clCreateKernel(program, "store", &error);
  EXPECT_SUCCESS(error);
  cl_kernel load_and_store_a =
      clCreateKernel(program, "load_and_store", &error);
  EXPECT_SUCCESS(error);
  cl_kernel load_and_store_b =
      clCreateKernel(program, "load_and_store", &error);
  EXPECT_SUCCESS(error);

  // We need 3 buffers, two for the intermediate values and one for the final
  // value.
  cl_mem intermediate_buffer_a = clCreateBuffer(
      context, CL_MEM_READ_WRITE, sizeof(cl_int), nullptr, &error);
  EXPECT_SUCCESS(error);
  cl_int initial_value = -1;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, intermediate_buffer_a,
                                      CL_TRUE, 0, sizeof(cl_int),
                                      &initial_value, 0, nullptr, nullptr));

  cl_mem intermediate_buffer_b = clCreateBuffer(
      context, CL_MEM_READ_WRITE, sizeof(cl_int), nullptr, &error);
  EXPECT_SUCCESS(error);
  initial_value = -2;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, intermediate_buffer_b,
                                      CL_TRUE, 0, sizeof(cl_int),
                                      &initial_value, 0, nullptr, nullptr));

  cl_mem final_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                       sizeof(cl_int), nullptr, &error);
  EXPECT_SUCCESS(error);
  initial_value = -3;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, final_buffer, CL_TRUE, 0,
                                      sizeof(cl_int), &initial_value, 0,
                                      nullptr, nullptr));

  // Set up the kernel args.
  EXPECT_SUCCESS(clSetKernelArg(store, 0, sizeof(intermediate_buffer_a),
                                static_cast<void *>(&intermediate_buffer_a)));
  EXPECT_SUCCESS(clSetKernelArg(load_and_store_a, 0,
                                sizeof(intermediate_buffer_a),
                                static_cast<void *>(&intermediate_buffer_a)));
  EXPECT_SUCCESS(clSetKernelArg(load_and_store_a, 1,
                                sizeof(intermediate_buffer_b),
                                static_cast<void *>(&intermediate_buffer_b)));
  EXPECT_SUCCESS(clSetKernelArg(load_and_store_b, 0,
                                sizeof(intermediate_buffer_b),
                                static_cast<void *>(&intermediate_buffer_b)));
  EXPECT_SUCCESS(clSetKernelArg(load_and_store_b, 1, sizeof(final_buffer),
                                static_cast<void *>(&final_buffer)));

  // Create a user event which the second kernel enqueue will wait on.
  cl_event user_event = clCreateUserEvent(context, &error);
  EXPECT_SUCCESS(error);

  // Now we enqueue the kernels but have the second one wait on a user event.
  const size_t global_size = 1;
  cl_event command_event;
  EXPECT_SUCCESS(clEnqueueNDRangeKernel(command_queue, store, 1, nullptr,
                                        &global_size, nullptr, 0, nullptr,
                                        &command_event));
  EXPECT_SUCCESS(clEnqueueNDRangeKernel(command_queue, load_and_store_a, 1,
                                        nullptr, &global_size, nullptr, 1,
                                        &user_event, nullptr));
  EXPECT_SUCCESS(clEnqueueNDRangeKernel(command_queue, load_and_store_b, 1,
                                        nullptr, &global_size, nullptr, 1,
                                        &command_event, nullptr));

  EXPECT_SUCCESS(clSetUserEventStatus(user_event, CL_COMPLETE));

  // Check that the commands executed in the expected order.
  cl_int result = -3;
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, final_buffer, CL_TRUE, 0,
                                     sizeof(cl_int), &result, 0, nullptr,
                                     nullptr));
  EXPECT_EQ(result, 0);

  // Cleanup.
  EXPECT_SUCCESS(clReleaseEvent(command_event));
  EXPECT_SUCCESS(clReleaseEvent(user_event));
  EXPECT_SUCCESS(clReleaseMemObject(final_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(intermediate_buffer_a));
  EXPECT_SUCCESS(clReleaseMemObject(intermediate_buffer_b));
  EXPECT_SUCCESS(clReleaseKernel(store));
  EXPECT_SUCCESS(clReleaseKernel(load_and_store_a));
  EXPECT_SUCCESS(clReleaseKernel(load_and_store_b));
}

using clSetUserEventStatusBlockingQueueTest = ucl::CommandQueueTest;

TEST_F(clSetUserEventStatusBlockingQueueTest,
       OnUserEventWithCommandEventCopies) {
  cl_int error = CL_SUCCESS;
  // We need 3 buffers, two for the intermediate values and one for the final
  // value.
  cl_mem intermediate_buffer_a = clCreateBuffer(
      context, CL_MEM_READ_WRITE, sizeof(cl_int), nullptr, &error);
  ASSERT_SUCCESS(error);
  cl_int initial_value = -1;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, intermediate_buffer_a,
                                      CL_TRUE, 0, sizeof(cl_int),
                                      &initial_value, 0, nullptr, nullptr));

  cl_mem intermediate_buffer_b = clCreateBuffer(
      context, CL_MEM_READ_WRITE, sizeof(cl_int), nullptr, &error);
  EXPECT_SUCCESS(error);
  initial_value = -2;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, intermediate_buffer_b,
                                      CL_TRUE, 0, sizeof(cl_int),
                                      &initial_value, 0, nullptr, nullptr));

  cl_mem final_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                       sizeof(cl_int), nullptr, &error);
  EXPECT_SUCCESS(error);
  initial_value = -3;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, final_buffer, CL_TRUE, 0,
                                      sizeof(cl_int), &initial_value, 0,
                                      nullptr, nullptr));

  // Create a user event which the second copy will wait on.
  cl_event user_event = clCreateUserEvent(context, &error);
  EXPECT_SUCCESS(error);

  // Now we enqueue the copies but have the second one wait on a user event.
  const cl_int zero = 0;
  cl_event command_event;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, intermediate_buffer_a,
                                      CL_FALSE, 0, sizeof(cl_int), &zero, 0,
                                      nullptr, &command_event));
  EXPECT_SUCCESS(clEnqueueCopyBuffer(command_queue, intermediate_buffer_a,
                                     intermediate_buffer_b, 0, 0,
                                     sizeof(cl_int), 1, &user_event, nullptr));
  EXPECT_SUCCESS(clEnqueueCopyBuffer(command_queue, intermediate_buffer_b,
                                     final_buffer, 0, 0, sizeof(cl_int), 1,
                                     &command_event, nullptr));

  EXPECT_SUCCESS(clSetUserEventStatus(user_event, CL_COMPLETE));

  // Check that the commands executed in the expected order.
  cl_int result = -3;
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, final_buffer, CL_TRUE, 0,
                                     sizeof(cl_int), &result, 0, nullptr,
                                     nullptr));
  EXPECT_EQ(result, 0);

  // Cleanup.
  EXPECT_SUCCESS(clReleaseEvent(command_event));
  EXPECT_SUCCESS(clReleaseEvent(user_event));
  EXPECT_SUCCESS(clReleaseMemObject(final_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(intermediate_buffer_a));
  EXPECT_SUCCESS(clReleaseMemObject(intermediate_buffer_b));
}

TEST_F(clSetUserEventStatusInOrderTest, BlockQueueOnUserEvent) {
  cl_int error = CL_SUCCESS;
  // Create two kernels which store and load and store single values.
  cl_kernel store = clCreateKernel(program, "store", &error);
  EXPECT_SUCCESS(error);
  cl_kernel load_and_store = clCreateKernel(program, "load_and_store", &error);
  EXPECT_SUCCESS(error);

  // We need 2 buffers, one for the intermediate value and one for the final
  // value.
  cl_mem intermediate_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                              sizeof(cl_int), nullptr, &error);
  EXPECT_SUCCESS(error);
  cl_int initial_value = -1;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, intermediate_buffer,
                                      CL_TRUE, 0, sizeof(cl_int),
                                      &initial_value, 0, nullptr, nullptr));

  cl_mem final_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                       sizeof(cl_int), nullptr, &error);
  EXPECT_SUCCESS(error);
  initial_value = -2;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, final_buffer, CL_TRUE, 0,
                                      sizeof(cl_int), &initial_value, 0,
                                      nullptr, nullptr));

  // Set up the kernel args.
  EXPECT_SUCCESS(clSetKernelArg(store, 0, sizeof(intermediate_buffer),
                                static_cast<void *>(&intermediate_buffer)));
  EXPECT_SUCCESS(clSetKernelArg(load_and_store, 0, sizeof(intermediate_buffer),
                                static_cast<void *>(&intermediate_buffer)));
  EXPECT_SUCCESS(clSetKernelArg(load_and_store, 1, sizeof(final_buffer),
                                static_cast<void *>(&final_buffer)));

  // Create a user event which the second kernel enqueue will wait on.
  cl_event user_event = clCreateUserEvent(context, &error);
  EXPECT_SUCCESS(error);

  // Now we enqueue the kernels but have the second one wait on a user event.
  const size_t global_size = 1;
  EXPECT_SUCCESS(clEnqueueNDRangeKernel(command_queue, store, 1, nullptr,
                                        &global_size, nullptr, 0, nullptr,
                                        nullptr));
  EXPECT_SUCCESS(clEnqueueNDRangeKernel(command_queue, load_and_store, 1,
                                        nullptr, &global_size, nullptr, 1,
                                        &user_event, nullptr));

  EXPECT_SUCCESS(clSetUserEventStatus(user_event, CL_COMPLETE));

  // Check that the commands executed in the expected order.
  cl_int result = -3;
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, final_buffer, CL_TRUE, 0,
                                     sizeof(cl_int), &result, 0, nullptr,
                                     nullptr));
  EXPECT_EQ(result, 0);

  // Cleanup.
  EXPECT_SUCCESS(clReleaseEvent(user_event));
  EXPECT_SUCCESS(clReleaseMemObject(final_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(intermediate_buffer));
  EXPECT_SUCCESS(clReleaseKernel(store));
  EXPECT_SUCCESS(clReleaseKernel(load_and_store));
}

TEST_F(clSetUserEventStatusBlockingQueueTest, OnUserEventCopies) {
  cl_int error = CL_SUCCESS;
  // We need 2 buffers, one for the intermediate value and one for the final
  // value.
  cl_mem intermediate_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                              sizeof(cl_int), nullptr, &error);
  ASSERT_SUCCESS(error);
  cl_int initial_value = -1;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, intermediate_buffer,
                                      CL_TRUE, 0, sizeof(cl_int),
                                      &initial_value, 0, nullptr, nullptr));

  cl_mem final_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                       sizeof(cl_int), nullptr, &error);
  EXPECT_SUCCESS(error);
  initial_value = -2;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, final_buffer, CL_TRUE, 0,
                                      sizeof(cl_int), &initial_value, 0,
                                      nullptr, nullptr));

  // Create a user event which the second copy will wait on.
  cl_event user_event = clCreateUserEvent(context, &error);
  EXPECT_SUCCESS(error);

  // Now we enqueue the copies but have the second one wait on a user event.
  const cl_int zero = 0;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, intermediate_buffer,
                                      CL_FALSE, 0, sizeof(cl_int), &zero, 0,
                                      nullptr, nullptr));
  EXPECT_SUCCESS(clEnqueueCopyBuffer(command_queue, intermediate_buffer,
                                     final_buffer, 0, 0, sizeof(cl_int), 1,
                                     &user_event, nullptr));

  EXPECT_SUCCESS(clSetUserEventStatus(user_event, CL_COMPLETE));

  // Check that the commands executed in the expected order.
  cl_int result = -3;
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, final_buffer, CL_TRUE, 0,
                                     sizeof(cl_int), &result, 0, nullptr,
                                     nullptr));
  EXPECT_EQ(result, 0);

  // Cleanup.
  EXPECT_SUCCESS(clReleaseEvent(user_event));
  EXPECT_SUCCESS(clReleaseMemObject(final_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(intermediate_buffer));
}

TEST_F(clSetUserEventStatusInOrderTest, BlockQueueOnTwoUserEvents) {
  cl_int error = CL_SUCCESS;
  // Create two kernels which store and load and store single values.
  cl_kernel store = clCreateKernel(program, "store", &error);
  EXPECT_SUCCESS(error);
  cl_kernel load_and_store = clCreateKernel(program, "load_and_store", &error);
  EXPECT_SUCCESS(error);

  // We need 2 buffers, one for the intermediate value and one for the final
  // value.
  cl_mem intermediate_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                              sizeof(cl_int), nullptr, &error);
  EXPECT_SUCCESS(error);
  cl_int initial_value = -1;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, intermediate_buffer,
                                      CL_TRUE, 0, sizeof(cl_int),
                                      &initial_value, 0, nullptr, nullptr));

  cl_mem final_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                       sizeof(cl_int), nullptr, &error);
  EXPECT_SUCCESS(error);
  initial_value = -2;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, final_buffer, CL_TRUE, 0,
                                      sizeof(cl_int), &initial_value, 0,
                                      nullptr, nullptr));

  // Set up the kernel args.
  EXPECT_SUCCESS(clSetKernelArg(store, 0, sizeof(intermediate_buffer),
                                static_cast<void *>(&intermediate_buffer)));
  EXPECT_SUCCESS(clSetKernelArg(load_and_store, 0, sizeof(intermediate_buffer),
                                static_cast<void *>(&intermediate_buffer)));
  EXPECT_SUCCESS(clSetKernelArg(load_and_store, 1, sizeof(final_buffer),
                                static_cast<void *>(&final_buffer)));

  // Create user events which the kernel enqueues will wait on.
  cl_event user_event_a = clCreateUserEvent(context, &error);
  cl_event user_event_b = clCreateUserEvent(context, &error);
  EXPECT_SUCCESS(error);

  // Now we enqueue the kernels but have them wait on user events.
  const size_t global_size = 1;
  EXPECT_SUCCESS(clEnqueueNDRangeKernel(command_queue, store, 1, nullptr,
                                        &global_size, nullptr, 1, &user_event_a,
                                        nullptr));
  EXPECT_SUCCESS(clEnqueueNDRangeKernel(command_queue, load_and_store, 1,
                                        nullptr, &global_size, nullptr, 1,
                                        &user_event_b, nullptr));

  EXPECT_SUCCESS(clSetUserEventStatus(user_event_a, CL_COMPLETE));
  EXPECT_SUCCESS(clSetUserEventStatus(user_event_b, CL_COMPLETE));

  // Check that the commands executed in the expected order.
  cl_int result = -3;
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, final_buffer, CL_TRUE, 0,
                                     sizeof(cl_int), &result, 0, nullptr,
                                     nullptr));
  EXPECT_EQ(result, 0);

  // Cleanup.
  EXPECT_SUCCESS(clReleaseEvent(user_event_a));
  EXPECT_SUCCESS(clReleaseEvent(user_event_b));
  EXPECT_SUCCESS(clReleaseMemObject(final_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(intermediate_buffer));
  EXPECT_SUCCESS(clReleaseKernel(store));
  EXPECT_SUCCESS(clReleaseKernel(load_and_store));
}

TEST_F(clSetUserEventStatusBlockingQueueTest, OnTwoUserEventsCopies) {
  cl_int error = CL_SUCCESS;
  // We need 2 buffers, one for the intermediate value and one for the final
  // value.
  cl_mem intermediate_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                              sizeof(cl_int), nullptr, &error);
  ASSERT_SUCCESS(error);
  cl_int initial_value = -1;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, intermediate_buffer,
                                      CL_TRUE, 0, sizeof(cl_int),
                                      &initial_value, 0, nullptr, nullptr));

  cl_mem final_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                       sizeof(cl_int), nullptr, &error);
  EXPECT_SUCCESS(error);
  initial_value = -2;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, final_buffer, CL_TRUE, 0,
                                      sizeof(cl_int), &initial_value, 0,
                                      nullptr, nullptr));

  // Create users events which the copies will wait on.
  cl_event user_event_a = clCreateUserEvent(context, &error);
  cl_event user_event_b = clCreateUserEvent(context, &error);
  EXPECT_SUCCESS(error);

  // Now we enqueue the copies but have them wait on user events.
  const cl_int zero = 0;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, intermediate_buffer,
                                      CL_FALSE, 0, sizeof(cl_int), &zero, 1,
                                      &user_event_a, nullptr));
  EXPECT_SUCCESS(clEnqueueCopyBuffer(command_queue, intermediate_buffer,
                                     final_buffer, 0, 0, sizeof(cl_int), 1,
                                     &user_event_b, nullptr));

  EXPECT_SUCCESS(clSetUserEventStatus(user_event_a, CL_COMPLETE));
  EXPECT_SUCCESS(clSetUserEventStatus(user_event_b, CL_COMPLETE));

  // Check that the commands executed in the expected order.
  cl_int result = -3;
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, final_buffer, CL_TRUE, 0,
                                     sizeof(cl_int), &result, 0, nullptr,
                                     nullptr));
  EXPECT_EQ(result, 0);

  // Cleanup.
  EXPECT_SUCCESS(clReleaseEvent(user_event_a));
  EXPECT_SUCCESS(clReleaseEvent(user_event_b));
  EXPECT_SUCCESS(clReleaseMemObject(final_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(intermediate_buffer));
}

TEST_F(clSetUserEventStatusInOrderTest, BlockQueueOnTwoUserEventsReversed) {
  cl_int error = CL_SUCCESS;
  cl_kernel store = clCreateKernel(program, "store", &error);
  EXPECT_SUCCESS(error);
  cl_kernel load_and_store = clCreateKernel(program, "load_and_store", &error);
  EXPECT_SUCCESS(error);

  // We need 2 buffers, one for the intermediate value and one for the final
  // value.
  cl_mem intermediate_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                              sizeof(cl_int), nullptr, &error);
  EXPECT_SUCCESS(error);
  cl_int initial_value = -1;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, intermediate_buffer,
                                      CL_TRUE, 0, sizeof(cl_int),
                                      &initial_value, 0, nullptr, nullptr));

  cl_mem final_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                       sizeof(cl_int), nullptr, &error);
  EXPECT_SUCCESS(error);
  initial_value = -2;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, final_buffer, CL_TRUE, 0,
                                      sizeof(cl_int), &initial_value, 0,
                                      nullptr, nullptr));

  // Set up the kernel args.
  EXPECT_SUCCESS(clSetKernelArg(store, 0, sizeof(intermediate_buffer),
                                static_cast<void *>(&intermediate_buffer)));
  EXPECT_SUCCESS(clSetKernelArg(load_and_store, 0, sizeof(intermediate_buffer),
                                static_cast<void *>(&intermediate_buffer)));
  EXPECT_SUCCESS(clSetKernelArg(load_and_store, 1, sizeof(final_buffer),
                                static_cast<void *>(&final_buffer)));

  // Create users event which the kernel enqueues will wait on.
  cl_event user_event_a = clCreateUserEvent(context, &error);
  cl_event user_event_b = clCreateUserEvent(context, &error);
  EXPECT_SUCCESS(error);

  // Now we enqueue the kernels but have them wait on user events.
  const size_t global_size = 1;
  EXPECT_SUCCESS(clEnqueueNDRangeKernel(command_queue, store, 1, nullptr,
                                        &global_size, nullptr, 1, &user_event_a,
                                        nullptr));
  EXPECT_SUCCESS(clEnqueueNDRangeKernel(command_queue, load_and_store, 1,
                                        nullptr, &global_size, nullptr, 1,
                                        &user_event_b, nullptr));

  EXPECT_SUCCESS(clSetUserEventStatus(user_event_b, CL_COMPLETE));
  EXPECT_SUCCESS(clSetUserEventStatus(user_event_a, CL_COMPLETE));

  // Check that the commands executed in the expected order.
  cl_int result = -3;
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, final_buffer, CL_TRUE, 0,
                                     sizeof(cl_int), &result, 0, nullptr,
                                     nullptr));
  EXPECT_EQ(result, 0);

  // Cleanup.
  EXPECT_SUCCESS(clReleaseEvent(user_event_a));
  EXPECT_SUCCESS(clReleaseEvent(user_event_b));
  EXPECT_SUCCESS(clReleaseMemObject(final_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(intermediate_buffer));
  EXPECT_SUCCESS(clReleaseKernel(store));
  EXPECT_SUCCESS(clReleaseKernel(load_and_store));
}

TEST_F(clSetUserEventStatusBlockingQueueTest, OnTwoUserEventsReversedCopies) {
  cl_int error = CL_SUCCESS;
  // We need 2 buffers, one for the intermediate value and one for the final
  // value.
  cl_mem intermediate_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                              sizeof(cl_int), nullptr, &error);
  ASSERT_SUCCESS(error);
  cl_int initial_value = -1;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, intermediate_buffer,
                                      CL_TRUE, 0, sizeof(cl_int),
                                      &initial_value, 0, nullptr, nullptr));

  cl_mem final_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                       sizeof(cl_int), nullptr, &error);
  EXPECT_SUCCESS(error);
  initial_value = -2;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, final_buffer, CL_TRUE, 0,
                                      sizeof(cl_int), &initial_value, 0,
                                      nullptr, nullptr));

  // Create user events which the copies will wait on.
  cl_event user_event_a = clCreateUserEvent(context, &error);
  cl_event user_event_b = clCreateUserEvent(context, &error);
  EXPECT_SUCCESS(error);

  // Now we enqueue the copies but have them wait on user events.
  const cl_int zero = 0;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, intermediate_buffer,
                                      CL_FALSE, 0, sizeof(cl_int), &zero, 1,
                                      &user_event_a, nullptr));
  EXPECT_SUCCESS(clEnqueueCopyBuffer(command_queue, intermediate_buffer,
                                     final_buffer, 0, 0, sizeof(cl_int), 1,
                                     &user_event_b, nullptr));

  EXPECT_SUCCESS(clSetUserEventStatus(user_event_b, CL_COMPLETE));
  EXPECT_SUCCESS(clSetUserEventStatus(user_event_a, CL_COMPLETE));

  // Check that the commands executed in the expected order.
  cl_int result = -3;
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, final_buffer, CL_TRUE, 0,
                                     sizeof(cl_int), &result, 0, nullptr,
                                     nullptr));
  EXPECT_EQ(result, 0);

  // Cleanup.
  EXPECT_SUCCESS(clReleaseEvent(user_event_a));
  EXPECT_SUCCESS(clReleaseEvent(user_event_b));
  EXPECT_SUCCESS(clReleaseMemObject(final_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(intermediate_buffer));
}
