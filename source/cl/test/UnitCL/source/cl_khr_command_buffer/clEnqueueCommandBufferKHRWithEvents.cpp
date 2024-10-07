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
#include "cl_khr_command_buffer.h"

// Tests that a command buffer can wait on a single regular command in the same
// queue.
TEST_F(cl_khr_command_buffer_Test, SameQueueSingleEventTest) {
  cl_int error = CL_SUCCESS;
  // Two buffers, one for the initial command to write in and second one for the
  // command buffer to copy to.
  cl_mem input_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                       sizeof(cl_int), nullptr, &error);
  ASSERT_SUCCESS(error);
  cl_mem output_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                        sizeof(cl_int), nullptr, &error);
  EXPECT_SUCCESS(error);

  // Initialize the buffers, so we know their state.
  const cl_int minus_one = -1;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, input_buffer, CL_TRUE, 0,
                                      sizeof(cl_int), &minus_one, 0, nullptr,
                                      nullptr));
  const cl_int minus_two = -2;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, output_buffer, CL_TRUE, 0,
                                      sizeof(cl_int), &minus_two, 0, nullptr,
                                      nullptr));

  // Create a command buffer and have it copy between the two data buffers. If
  // the commands execute out of order, we will know.
  cl_command_buffer_khr command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
  EXPECT_SUCCESS(error);
  EXPECT_SUCCESS(clCommandCopyBufferKHR(command_buffer, nullptr, input_buffer,
                                        output_buffer, 0, 0, sizeof(cl_int), 0,
                                        nullptr, nullptr, nullptr));
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Enqueue a non-blocking write and have the command buffer enqueue wait on
  // its signal event.
  const cl_int zero = 0;
  cl_event event = nullptr;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, input_buffer, CL_FALSE, 0,
                                      sizeof(cl_int), &zero, 0, nullptr,
                                      &event));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 1,
                                           &event, nullptr));

  // Check the result.
  cl_int result = -3;
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, output_buffer, CL_TRUE, 0,
                                     sizeof(cl_int), &result, 0, nullptr,
                                     nullptr));
  EXPECT_EQ(result, zero);

  // Clean up.
  EXPECT_SUCCESS(clReleaseMemObject(input_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(output_buffer));
  EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
  EXPECT_SUCCESS(clReleaseEvent(event));
}

// Tests that a command buffer can wait on a multiple regular commands in the
// same queue.
TEST_F(cl_khr_command_buffer_Test, SameQueueMultipleEventTest) {
  cl_int error = CL_SUCCESS;
  // Two buffers, one for the initial command to write in and second one for the
  // command buffer to copy to.
  cl_mem input_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                       2 * sizeof(cl_int), nullptr, &error);
  ASSERT_SUCCESS(error);
  cl_mem output_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                        2 * sizeof(cl_int), nullptr, &error);
  EXPECT_SUCCESS(error);

  // Initialize the buffers, so we know their state.
  const cl_int minus_one[]{-1, -1};
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, input_buffer, CL_TRUE, 0,
                                      2 * sizeof(cl_int), &minus_one, 0,
                                      nullptr, nullptr));
  const cl_int minus_two[]{-2, -2};
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, output_buffer, CL_TRUE, 0,
                                      2 * sizeof(cl_int), &minus_two, 0,
                                      nullptr, nullptr));

  // Create a command buffer and have it copy between the two data buffers. If
  // the commands execute out of order, we will know.
  cl_command_buffer_khr command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
  EXPECT_SUCCESS(error);
  EXPECT_SUCCESS(clCommandCopyBufferKHR(command_buffer, nullptr, input_buffer,
                                        output_buffer, 0, 0, 2 * sizeof(cl_int),
                                        0, nullptr, nullptr, nullptr));
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Enqueue two non-blocking writes and have the command buffer enqueue wait on
  // their signal events.
  const cl_int zero = 0;
  cl_event first_event = nullptr;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, input_buffer, CL_FALSE, 0,
                                      sizeof(cl_int), &zero, 0, nullptr,
                                      &first_event));

  // This second write just overwrites the first one, but OpenCL doesn't know
  // this.
  const cl_int one = 1;
  cl_event second_event = nullptr;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, input_buffer, CL_FALSE,
                                      sizeof(cl_int), sizeof(cl_int), &one, 0,
                                      nullptr, &second_event));

  const cl_event wait_events[] = {first_event, second_event};
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 2,
                                           wait_events, nullptr));

  // Check the result.
  std::vector<cl_int> result{-3, -3};
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, output_buffer, CL_TRUE, 0,
                                     2 * sizeof(cl_int), result.data(), 0,
                                     nullptr, nullptr));
  const std::vector<cl_int> expected_result{zero, one};
  EXPECT_EQ(result, expected_result);

  // Clean up.
  EXPECT_SUCCESS(clReleaseMemObject(input_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(output_buffer));
  EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
  EXPECT_SUCCESS(clReleaseEvent(first_event));
  EXPECT_SUCCESS(clReleaseEvent(second_event));
}

// Tests that a regular command can wait on a single command buffer in the same
// queue.
TEST_F(cl_khr_command_buffer_Test, SameQueueSingleCommandBufferEventTest) {
  cl_int error = CL_SUCCESS;
  // Two buffers, for the command buffer to copy between.
  cl_mem input_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                       sizeof(cl_int), nullptr, &error);
  ASSERT_SUCCESS(error);
  cl_mem output_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                        sizeof(cl_int), nullptr, &error);
  EXPECT_SUCCESS(error);

  // Initialize the buffers, so we know their state.
  const cl_int zero = 0;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, input_buffer, CL_TRUE, 0,
                                      sizeof(cl_int), &zero, 0, nullptr,
                                      nullptr));
  const cl_int minus_one = -1;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, output_buffer, CL_TRUE, 0,
                                      sizeof(cl_int), &minus_one, 0, nullptr,
                                      nullptr));

  // Create a command buffer and have it copy between the two data buffers. If
  // the commands execute out of order, we will know.
  cl_command_buffer_khr command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
  EXPECT_SUCCESS(error);
  EXPECT_SUCCESS(clCommandCopyBufferKHR(command_buffer, nullptr, input_buffer,
                                        output_buffer, 0, 0, sizeof(cl_int), 0,
                                        nullptr, nullptr, nullptr));
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Enqueue the command buffer which contains the copy and get its signal
  // event.
  cl_event event;
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, &event));

  // Check the result.
  cl_int result = -2;
  // Have the read non-blocking but waiting on the signal event from the command
  // buffer.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, output_buffer, CL_FALSE, 0,
                                     sizeof(cl_int), &result, 1, &event,
                                     nullptr));
  // We need to flush explicitly here to make sure the read is finished before
  // we check the result.
  EXPECT_SUCCESS(clFinish(command_queue));
  EXPECT_EQ(result, zero);

  // Clean up.
  EXPECT_SUCCESS(clReleaseMemObject(input_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(output_buffer));
  EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
  EXPECT_SUCCESS(clReleaseEvent(event));
}

// Tests that a regular command can wait on multiple command buffers in the same
// queue.
TEST_F(cl_khr_command_buffer_Test, SameQueueMultipleCommandBufferEventTest) {
  cl_int error = CL_SUCCESS;
  // Three buffers, for the command buffer to copy between.
  cl_mem input_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                       2 * sizeof(cl_int), nullptr, &error);
  EXPECT_SUCCESS(error);
  cl_mem output_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                        2 * sizeof(cl_int), nullptr, &error);
  EXPECT_SUCCESS(error);

  // Initialize the buffers, so we know their state.
  const std::vector<cl_int> zero_one{0, 1};
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, input_buffer, CL_TRUE, 0,
                                      2 * sizeof(cl_int), zero_one.data(), 0,
                                      nullptr, nullptr));
  const cl_int minus_one[]{-1, -1};
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, output_buffer, CL_TRUE, 0,
                                      2 * sizeof(cl_int), &minus_one, 0,
                                      nullptr, nullptr));

  // Create a command buffer and have it copy the first cl_int between the
  // buffers buffers. If the commands execute out of order, we will know.
  cl_command_buffer_khr first_command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
  EXPECT_SUCCESS(error);
  EXPECT_SUCCESS(clCommandCopyBufferKHR(
      first_command_buffer, nullptr, input_buffer, output_buffer, 0, 0,
      sizeof(cl_int), 0, nullptr, nullptr, nullptr));
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(first_command_buffer));

  // Create a command buffer and have it copy the second cl_int between the
  // buffers. If the commands execute out of order, we will know.
  cl_command_buffer_khr second_command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
  EXPECT_SUCCESS(error);
  EXPECT_SUCCESS(
      clCommandCopyBufferKHR(second_command_buffer, nullptr, input_buffer,
                             output_buffer, sizeof(cl_int), sizeof(cl_int),
                             sizeof(cl_int), 0, nullptr, nullptr, nullptr));
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(second_command_buffer));

  // Enqueue the command buffer which contains the copy and get its signal
  // event.
  cl_event first_event;
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, first_command_buffer, 0,
                                           nullptr, &first_event));
  cl_event second_event;
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, second_command_buffer, 0,
                                           nullptr, &second_event));

  const cl_event events[]{first_event, second_event};

  // Check the result.
  std::vector<cl_int> result{-3, -3};
  // Have the read non-blocking but waiting on the signal events from the
  // command buffers.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, output_buffer, CL_FALSE, 0,
                                     2 * sizeof(cl_int), result.data(), 2,
                                     events, nullptr));
  // We need to flush explicitly here to make sure the read is finished before
  // we check the result.
  EXPECT_SUCCESS(clFinish(command_queue));
  EXPECT_EQ(result, zero_one);

  // Clean up.
  EXPECT_SUCCESS(clReleaseMemObject(input_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(output_buffer));
  EXPECT_SUCCESS(clReleaseCommandBufferKHR(first_command_buffer));
  EXPECT_SUCCESS(clReleaseCommandBufferKHR(second_command_buffer));
  EXPECT_SUCCESS(clReleaseEvent(first_event));
  EXPECT_SUCCESS(clReleaseEvent(second_event));
}

// Tests that a one command buffer can wait on another command buffer in the
// same queue.
TEST_F(cl_khr_command_buffer_Test, SameQueueInterCommandBufferEventTest) {
  cl_int error = CL_SUCCESS;
  // Three buffers, for the command buffer to copy between.
  cl_mem input_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                       sizeof(cl_int), nullptr, &error);
  ASSERT_SUCCESS(error);
  cl_mem intermediate_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                              sizeof(cl_int), nullptr, &error);
  EXPECT_SUCCESS(error);
  cl_mem output_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                        sizeof(cl_int), nullptr, &error);
  EXPECT_SUCCESS(error);

  // Initialize the buffers, so we know their state.
  const cl_int zero = 0;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, input_buffer, CL_TRUE, 0,
                                      sizeof(cl_int), &zero, 0, nullptr,
                                      nullptr));
  const cl_int minus_one = -1;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, intermediate_buffer,
                                      CL_TRUE, 0, sizeof(cl_int), &minus_one, 0,
                                      nullptr, nullptr));
  const cl_int minus_two = -2;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, output_buffer, CL_TRUE, 0,
                                      sizeof(cl_int), &minus_two, 0, nullptr,
                                      nullptr));

  // Create a command buffer and have it copy between the first two data
  // buffers. If the commands execute out of order, we will know.
  cl_command_buffer_khr first_command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
  EXPECT_SUCCESS(error);
  EXPECT_SUCCESS(clCommandCopyBufferKHR(
      first_command_buffer, nullptr, input_buffer, intermediate_buffer, 0, 0,
      sizeof(cl_int), 0, nullptr, nullptr, nullptr));
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(first_command_buffer));

  // Create a command buffer and have it copy between the second two data
  // buffers. If the commands execute out of order, we will know.
  cl_command_buffer_khr second_command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
  EXPECT_SUCCESS(error);
  EXPECT_SUCCESS(clCommandCopyBufferKHR(
      second_command_buffer, nullptr, intermediate_buffer, output_buffer, 0, 0,
      sizeof(cl_int), 0, nullptr, nullptr, nullptr));
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(second_command_buffer));

  // Enqueue the first command buffer and get its signal event.
  cl_event event;
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, first_command_buffer, 0,
                                           nullptr, &event));

  // Enqueue the second command buffer and have it wait on the first.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, second_command_buffer, 1,
                                           &event, nullptr));

  // Check the result.
  cl_int result = -4;
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, output_buffer, CL_TRUE, 0,
                                     sizeof(cl_int), &result, 0, nullptr,
                                     nullptr));
  EXPECT_EQ(result, zero);

  // Clean up.
  EXPECT_SUCCESS(clReleaseMemObject(input_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(intermediate_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(output_buffer));
  EXPECT_SUCCESS(clReleaseCommandBufferKHR(first_command_buffer));
  EXPECT_SUCCESS(clReleaseCommandBufferKHR(second_command_buffer));
  EXPECT_SUCCESS(clReleaseEvent(event));
}

// Tests that a one command buffer can wait on mulitple command buffers in the
// same queue.
TEST_F(cl_khr_command_buffer_Test,
       SameQueueMultipleInterCommandBufferEventTest) {
  cl_int error = CL_SUCCESS;
  // Three buffers, for the command buffer to copy between.
  cl_mem input_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                       2 * sizeof(cl_int), nullptr, &error);
  ASSERT_SUCCESS(error);
  cl_mem intermediate_buffer = clCreateBuffer(
      context, CL_MEM_READ_WRITE, 2 * sizeof(cl_int), nullptr, &error);
  EXPECT_SUCCESS(error);
  cl_mem output_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                        2 * sizeof(cl_int), nullptr, &error);
  EXPECT_SUCCESS(error);

  // Initialize the buffers, so we know their state.
  const std::vector<cl_int> zero_one{0, 1};
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, input_buffer, CL_TRUE, 0,
                                      2 * sizeof(cl_int), zero_one.data(), 0,
                                      nullptr, nullptr));
  const cl_int minus_one[]{-1, -1};
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, intermediate_buffer,
                                      CL_TRUE, 0, 2 * sizeof(cl_int),
                                      &minus_one, 0, nullptr, nullptr));
  const cl_int minus_two[]{-2, -2};
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, output_buffer, CL_TRUE, 0,
                                      2 * sizeof(cl_int), &minus_two, 0,
                                      nullptr, nullptr));

  // Create a command buffer and have it copy the first cl_int between the first
  // two data buffers. If the commands execute out of order, we will know.
  cl_command_buffer_khr first_command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
  EXPECT_SUCCESS(error);
  EXPECT_SUCCESS(clCommandCopyBufferKHR(
      first_command_buffer, nullptr, input_buffer, intermediate_buffer, 0, 0,
      sizeof(cl_int), 0, nullptr, nullptr, nullptr));
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(first_command_buffer));

  // Create a command buffer and have it copy the second cl_int between the
  // first two data buffers. If the commands execute out of order, we will know.
  cl_command_buffer_khr second_command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
  EXPECT_SUCCESS(error);
  EXPECT_SUCCESS(clCommandCopyBufferKHR(
      second_command_buffer, nullptr, input_buffer, intermediate_buffer,
      sizeof(cl_int), sizeof(cl_int), sizeof(cl_int), 0, nullptr, nullptr,
      nullptr));
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(second_command_buffer));

  // Create a command buffer and have it copy both cl_ints between the last two
  // data buffers. If the commands execute out of order, we will know.
  cl_command_buffer_khr third_command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
  EXPECT_SUCCESS(error);
  EXPECT_SUCCESS(clCommandCopyBufferKHR(
      third_command_buffer, nullptr, intermediate_buffer, output_buffer, 0, 0,
      2 * sizeof(cl_int), 0, nullptr, nullptr, nullptr));
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(third_command_buffer));

  // Enqueue the first two command buffers and get their
  // signal events.
  cl_event first_event;
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, first_command_buffer, 0,
                                           nullptr, &first_event));
  cl_event second_event;
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, second_command_buffer, 0,
                                           nullptr, &second_event));

  // Enqueue the third command buffer and have it wait on the first two.
  const cl_event events[]{first_event, second_event};
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, third_command_buffer, 2,
                                           events, nullptr));

  // Check the result.
  std::vector<cl_int> result{-3, -3};
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, output_buffer, CL_TRUE, 0,
                                     2 * sizeof(cl_int), result.data(), 0,
                                     nullptr, nullptr));
  EXPECT_EQ(result, zero_one);

  // Clean up.
  EXPECT_SUCCESS(clReleaseMemObject(input_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(intermediate_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(output_buffer));
  EXPECT_SUCCESS(clReleaseCommandBufferKHR(first_command_buffer));
  EXPECT_SUCCESS(clReleaseCommandBufferKHR(second_command_buffer));
  EXPECT_SUCCESS(clReleaseCommandBufferKHR(third_command_buffer));
  EXPECT_SUCCESS(clReleaseEvent(first_event));
  EXPECT_SUCCESS(clReleaseEvent(second_event));
}

// TODO: We currently do not support enqueing the same command buffer multiple
// times without a partitioning flush (see CA-3232). When this is supported we
// should add tests analogous to those above but using events returned from the
// same command buffer on multiple enqueues.

// Tests that a command buffer can wait on a single user event.
TEST_F(cl_khr_command_buffer_Test, SingleUserEventTest) {
  cl_int error = CL_SUCCESS;
  // Two buffers, for the command buffer to copy between.
  cl_mem input_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                       sizeof(cl_int), nullptr, &error);
  ASSERT_SUCCESS(error);
  cl_mem output_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                        sizeof(cl_int), nullptr, &error);
  EXPECT_SUCCESS(error);

  // Initialize the buffers, so we know their state.
  const cl_int zero = 0;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, input_buffer, CL_TRUE, 0,
                                      sizeof(cl_int), &zero, 0, nullptr,
                                      nullptr));
  const cl_int minus_one = -1;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, output_buffer, CL_TRUE, 0,
                                      sizeof(cl_int), &minus_one, 0, nullptr,
                                      nullptr));

  // Create a command buffer and have it copy between the two data buffers. If
  // the commands execute out of order, we will know.
  cl_command_buffer_khr command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
  EXPECT_SUCCESS(error);
  EXPECT_SUCCESS(clCommandCopyBufferKHR(command_buffer, nullptr, input_buffer,
                                        output_buffer, 0, 0, sizeof(cl_int), 0,
                                        nullptr, nullptr, nullptr));
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Enqueue the command buffer and have it wait on a user event that we then
  // release.
  cl_event user_event = clCreateUserEvent(context, &error);
  EXPECT_SUCCESS(error);
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 1,
                                           &user_event, nullptr));
  EXPECT_SUCCESS(clSetUserEventStatus(user_event, CL_COMPLETE));

  // Check the result.
  cl_int result = -3;
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, output_buffer, CL_TRUE, 0,
                                     sizeof(cl_int), &result, 0, nullptr,
                                     nullptr));
  EXPECT_EQ(result, zero);

  // Clean up.
  EXPECT_SUCCESS(clReleaseMemObject(input_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(output_buffer));
  EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
  EXPECT_SUCCESS(clReleaseEvent(user_event));
}

// Tests that a command buffer can wait on a multiple user events.
TEST_F(cl_khr_command_buffer_Test, MultipleUserEventTest) {
  cl_int error = CL_SUCCESS;
  // Two buffers, for the command buffer to copy between.
  cl_mem input_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                       sizeof(cl_int), nullptr, &error);
  ASSERT_SUCCESS(error);
  cl_mem output_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                        sizeof(cl_int), nullptr, &error);
  EXPECT_SUCCESS(error);

  // Initialize the buffers, so we know their state.
  const cl_int zero = 0;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, input_buffer, CL_TRUE, 0,
                                      sizeof(cl_int), &zero, 0, nullptr,
                                      nullptr));
  const cl_int minus_one = -1;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, output_buffer, CL_TRUE, 0,
                                      sizeof(cl_int), &minus_one, 0, nullptr,
                                      nullptr));

  // Create a command buffer and have it copy between the two data buffers. If
  // the commands execute out of order, we will know.
  cl_command_buffer_khr command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
  EXPECT_SUCCESS(error);
  EXPECT_SUCCESS(clCommandCopyBufferKHR(command_buffer, nullptr, input_buffer,
                                        output_buffer, 0, 0, sizeof(cl_int), 0,
                                        nullptr, nullptr, nullptr));
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Enqueue the command buffer and have it wait on a user event that we then
  // release.
  cl_event first_user_event = clCreateUserEvent(context, &error);
  EXPECT_SUCCESS(error);
  cl_event second_user_event = clCreateUserEvent(context, &error);
  EXPECT_SUCCESS(error);
  cl_event user_events[]{first_user_event, second_user_event};
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 2,
                                           user_events, nullptr));
  EXPECT_SUCCESS(clSetUserEventStatus(first_user_event, CL_COMPLETE));
  EXPECT_SUCCESS(clSetUserEventStatus(second_user_event, CL_COMPLETE));

  // Check the result.
  cl_int result = -3;
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, output_buffer, CL_TRUE, 0,
                                     sizeof(cl_int), &result, 0, nullptr,
                                     nullptr));
  EXPECT_EQ(result, zero);

  // Clean up.
  EXPECT_SUCCESS(clReleaseMemObject(input_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(output_buffer));
  EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
  EXPECT_SUCCESS(clReleaseEvent(first_user_event));
  EXPECT_SUCCESS(clReleaseEvent(second_user_event));
}

// Tests an edge case that has been found in OpenCL drivers previously.
TEST_F(cl_khr_command_buffer_Test, BlockQueueOnUserEventWithCommandEvent) {
  // This test is not valid for out of order queues.
  if (!UCL::isQueueInOrder(command_queue)) {
    GTEST_SKIP();
  }

  cl_int error = CL_SUCCESS;
  // We need 3 buffers, two for the intermediate values and one for the final
  // value.
  cl_mem input_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                       sizeof(cl_int), nullptr, &error);
  ASSERT_SUCCESS(error);
  const cl_int minus_one = -1;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, input_buffer, CL_TRUE, 0,
                                      sizeof(cl_int), &minus_one, 0, nullptr,
                                      nullptr));

  cl_mem intermediate_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                              sizeof(cl_int), nullptr, &error);
  EXPECT_SUCCESS(error);
  const cl_int minus_two = -2;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, intermediate_buffer,
                                      CL_TRUE, 0, sizeof(cl_int), &minus_two, 0,
                                      nullptr, nullptr));

  cl_mem output_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                        sizeof(cl_int), nullptr, &error);
  EXPECT_SUCCESS(error);
  const cl_int minus_three = -3;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, output_buffer, CL_TRUE, 0,
                                      sizeof(cl_int), &minus_three, 0, nullptr,
                                      nullptr));

  // Create two command buffers which do the transitive copying between them.
  cl_command_buffer_khr first_command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
  EXPECT_SUCCESS(error);
  EXPECT_SUCCESS(clCommandCopyBufferKHR(
      first_command_buffer, nullptr, input_buffer, intermediate_buffer, 0, 0,
      sizeof(cl_int), 0, nullptr, nullptr, nullptr));
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(first_command_buffer));

  cl_command_buffer_khr second_command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
  EXPECT_SUCCESS(error);
  EXPECT_SUCCESS(clCommandCopyBufferKHR(
      second_command_buffer, nullptr, intermediate_buffer, output_buffer, 0, 0,
      sizeof(cl_int), 0, nullptr, nullptr, nullptr));
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(second_command_buffer));

  // Create a user event which the second copy will wait on.
  cl_event user_event = clCreateUserEvent(context, &error);
  EXPECT_SUCCESS(error);

  // Now we enqueue the copies but have the second one wait on a user event.
  const cl_int zero = 0;
  cl_event command_event;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, input_buffer, CL_FALSE, 0,
                                      sizeof(cl_int), &zero, 0, nullptr,
                                      &command_event));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, first_command_buffer, 1,
                                           &user_event, nullptr));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, second_command_buffer, 1,
                                           &command_event, nullptr));

  EXPECT_SUCCESS(clSetUserEventStatus(user_event, CL_COMPLETE));

  // Check that the commands executed in the expected order.
  cl_int result = -3;
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, output_buffer, CL_TRUE, 0,
                                     sizeof(cl_int), &result, 0, nullptr,
                                     nullptr));
  EXPECT_EQ(result, 0);

  // Cleanup.
  EXPECT_SUCCESS(clReleaseEvent(command_event));
  EXPECT_SUCCESS(clReleaseEvent(user_event));
  EXPECT_SUCCESS(clReleaseCommandBufferKHR(first_command_buffer));
  EXPECT_SUCCESS(clReleaseCommandBufferKHR(second_command_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(output_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(intermediate_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(input_buffer));
}

// Tests an edge case that has been found in OpenCL drivers previously.
TEST_F(cl_khr_command_buffer_Test, BlockQueueOnUserEvent) {
  // This test is not valid for out of order queues.
  if (!UCL::isQueueInOrder(command_queue)) {
    GTEST_SKIP();
  }

  cl_int error = CL_SUCCESS;
  // We need 2 buffers, one for the intermediate value and one for the final
  // value.
  cl_mem input_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                       sizeof(cl_int), nullptr, &error);
  ASSERT_SUCCESS(error);
  const cl_int minus_one = -1;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, input_buffer, CL_TRUE, 0,
                                      sizeof(cl_int), &minus_one, 0, nullptr,
                                      nullptr));

  cl_mem output_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                        sizeof(cl_int), nullptr, &error);
  EXPECT_SUCCESS(error);
  const cl_int minus_two = -2;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, output_buffer, CL_TRUE, 0,
                                      sizeof(cl_int), &minus_two, 0, nullptr,
                                      nullptr));

  // Create the command buffer holding the copy.
  cl_command_buffer_khr command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
  EXPECT_SUCCESS(error);
  EXPECT_SUCCESS(clCommandCopyBufferKHR(command_buffer, nullptr, input_buffer,
                                        output_buffer, 0, 0, sizeof(cl_int), 0,
                                        nullptr, nullptr, nullptr));
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Create a user event which the second copy will wait on.
  cl_event user_event = clCreateUserEvent(context, &error);
  EXPECT_SUCCESS(error);

  // Now we enqueue the copies but have the second one wait on a user event.
  const cl_int zero = 0;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, input_buffer, CL_FALSE, 0,
                                      sizeof(cl_int), &zero, 0, nullptr,
                                      nullptr));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 1,
                                           &user_event, nullptr));

  EXPECT_SUCCESS(clSetUserEventStatus(user_event, CL_COMPLETE));

  // Check that the commands executed in the expected order.
  cl_int result = -3;
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, output_buffer, CL_TRUE, 0,
                                     sizeof(cl_int), &result, 0, nullptr,
                                     nullptr));
  EXPECT_EQ(result, 0);

  // Cleanup.
  EXPECT_SUCCESS(clReleaseEvent(user_event));
  EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(output_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(input_buffer));
}

// Tests an edge case that has been found in OpenCL drivers previously.
TEST_F(cl_khr_command_buffer_Test, BlockQueueOnTwoUserEvents) {
  // This test is not valid for out of order queues.
  if (!UCL::isQueueInOrder(command_queue)) {
    GTEST_SKIP();
  }

  cl_int error = CL_SUCCESS;
  // We need 3 buffers to do a transitive copy.
  cl_mem input_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                       sizeof(cl_int), nullptr, &error);
  ASSERT_SUCCESS(error);
  const cl_int zero = 0;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, input_buffer, CL_TRUE, 0,
                                      sizeof(cl_int), &zero, 0, nullptr,
                                      nullptr));

  cl_mem intermediate_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                              sizeof(cl_int), nullptr, &error);
  EXPECT_SUCCESS(error);
  const cl_int minus_one = -1;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, intermediate_buffer,
                                      CL_TRUE, 0, sizeof(cl_int), &minus_one, 0,
                                      nullptr, nullptr));

  cl_mem output_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                        sizeof(cl_int), nullptr, &error);
  EXPECT_SUCCESS(error);
  const cl_int minus_two = -2;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, output_buffer, CL_TRUE, 0,
                                      sizeof(cl_int), &minus_two, 0, nullptr,
                                      nullptr));

  // Create two command buffers which do the transitive copying between them.
  cl_command_buffer_khr first_command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
  EXPECT_SUCCESS(error);
  EXPECT_SUCCESS(clCommandCopyBufferKHR(
      first_command_buffer, nullptr, input_buffer, intermediate_buffer, 0, 0,
      sizeof(cl_int), 0, nullptr, nullptr, nullptr));
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(first_command_buffer));

  cl_command_buffer_khr second_command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
  EXPECT_SUCCESS(error);
  EXPECT_SUCCESS(clCommandCopyBufferKHR(
      second_command_buffer, nullptr, intermediate_buffer, output_buffer, 0, 0,
      sizeof(cl_int), 0, nullptr, nullptr, nullptr));
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(second_command_buffer));

  // Create users events which the copies will wait on.
  cl_event user_event_a = clCreateUserEvent(context, &error);
  cl_event user_event_b = clCreateUserEvent(context, &error);
  EXPECT_SUCCESS(error);

  // Now we enqueue the copies but have them wait on user events.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, first_command_buffer, 1,
                                           &user_event_a, nullptr));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, second_command_buffer, 1,
                                           &user_event_b, nullptr));

  EXPECT_SUCCESS(clSetUserEventStatus(user_event_a, CL_COMPLETE));
  EXPECT_SUCCESS(clSetUserEventStatus(user_event_b, CL_COMPLETE));

  // Check that the commands executed in the expected order.
  cl_int result = -3;
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, output_buffer, CL_TRUE, 0,
                                     sizeof(cl_int), &result, 0, nullptr,
                                     nullptr));
  EXPECT_EQ(result, 0);

  // Cleanup.
  EXPECT_SUCCESS(clReleaseEvent(user_event_a));
  EXPECT_SUCCESS(clReleaseEvent(user_event_b));
  EXPECT_SUCCESS(clReleaseCommandBufferKHR(first_command_buffer));
  EXPECT_SUCCESS(clReleaseCommandBufferKHR(second_command_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(output_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(intermediate_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(input_buffer));
}

// Tests an edge case that has been found in OpenCL drivers previously.
TEST_F(cl_khr_command_buffer_Test, BlockQueueOnTwoUserEventsReversed) {
  // This test is not valid for out of order queues.
  if (!UCL::isQueueInOrder(command_queue)) {
    GTEST_SKIP();
  }

  cl_int error = CL_SUCCESS;
  // We need 3 buffers to do a transitive copy.
  cl_mem input_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                       sizeof(cl_int), nullptr, &error);
  ASSERT_SUCCESS(error);
  const cl_int zero = 0;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, input_buffer, CL_TRUE, 0,
                                      sizeof(cl_int), &zero, 0, nullptr,
                                      nullptr));

  cl_mem intermediate_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                              sizeof(cl_int), nullptr, &error);
  EXPECT_SUCCESS(error);
  const cl_int minus_one = -1;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, intermediate_buffer,
                                      CL_TRUE, 0, sizeof(cl_int), &minus_one, 0,
                                      nullptr, nullptr));

  cl_mem output_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                        sizeof(cl_int), nullptr, &error);
  EXPECT_SUCCESS(error);
  const cl_int minus_two = -2;
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, output_buffer, CL_TRUE, 0,
                                      sizeof(cl_int), &minus_two, 0, nullptr,
                                      nullptr));

  // Create two command buffers which do the transitive copying between them.
  cl_command_buffer_khr first_command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
  EXPECT_SUCCESS(error);
  EXPECT_SUCCESS(clCommandCopyBufferKHR(
      first_command_buffer, nullptr, input_buffer, intermediate_buffer, 0, 0,
      sizeof(cl_int), 0, nullptr, nullptr, nullptr));
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(first_command_buffer));

  cl_command_buffer_khr second_command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
  EXPECT_SUCCESS(error);
  EXPECT_SUCCESS(clCommandCopyBufferKHR(
      second_command_buffer, nullptr, intermediate_buffer, output_buffer, 0, 0,
      sizeof(cl_int), 0, nullptr, nullptr, nullptr));
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(second_command_buffer));

  // Create users events which the copies will wait on.
  cl_event user_event_a = clCreateUserEvent(context, &error);
  cl_event user_event_b = clCreateUserEvent(context, &error);
  EXPECT_SUCCESS(error);

  // Now we enqueue the copies but have them wait on user events.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, first_command_buffer, 1,
                                           &user_event_a, nullptr));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, second_command_buffer, 1,
                                           &user_event_b, nullptr));

  EXPECT_SUCCESS(clSetUserEventStatus(user_event_b, CL_COMPLETE));
  EXPECT_SUCCESS(clSetUserEventStatus(user_event_a, CL_COMPLETE));

  // Check that the commands executed in the expected order.
  cl_int result = -3;
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, output_buffer, CL_TRUE, 0,
                                     sizeof(cl_int), &result, 0, nullptr,
                                     nullptr));
  EXPECT_EQ(result, 0);

  // Cleanup.
  EXPECT_SUCCESS(clReleaseEvent(user_event_a));
  EXPECT_SUCCESS(clReleaseEvent(user_event_b));
  EXPECT_SUCCESS(clReleaseCommandBufferKHR(first_command_buffer));
  EXPECT_SUCCESS(clReleaseCommandBufferKHR(second_command_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(output_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(intermediate_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(input_buffer));
}

// Abstracts common code for testing the event related APIs
// when passed an event returned from clEnqueueCommandBufferKHR.
class CommandBufferEventTest : public cl_khr_command_buffer_Test {
 public:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(cl_khr_command_buffer_Test::SetUp());

    // Since we are testing the values returned by event APIs we don't
    // really care about the content of our command buffer so we just
    // leave it empty.
    cl_int error = CL_SUCCESS;
    command_buffer =
        clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
    ASSERT_SUCCESS(error);
    EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

    // Enqueue the command buffer and get its signal event for querying.
    EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                             nullptr, &event));
  }

  void TearDown() override {
    if (event) {
      EXPECT_SUCCESS(clReleaseEvent(event));
    }
    if (command_buffer) {
      EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
    }

    cl_khr_command_buffer_Test::TearDown();
  }

  cl_command_buffer_khr command_buffer = nullptr;
  cl_event event = nullptr;
};

// Abstracts common code for testing the values returned from clWaitForEvents
// when called on an event returned from clEnqueueCommandBufferKHR.
class WaitForEventsTest : public CommandBufferEventTest {};

// Tests we can wait on a single event returned from a
// clEnqueueCommandBufferKHR.
TEST_F(WaitForEventsTest, SingleCommandBufferEvent) {
  EXPECT_SUCCESS(clWaitForEvents(1, &event));
}

// Tests we can wait on multiple events returned from difference
// clEnqueueCommandBufferKHR calls.
TEST_F(WaitForEventsTest, MultipleCommandBufferEvents) {
  // We need another command buffer.
  cl_int error = CL_SUCCESS;
  cl_command_buffer_khr second_command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
  EXPECT_SUCCESS(error);
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(second_command_buffer));

  // Enqueue the second command buffer and get its event.
  cl_event second_event;
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, second_command_buffer, 0,
                                           nullptr, &second_event));

  // Wait on the two events.
  const cl_event events[]{event, second_event};
  EXPECT_SUCCESS(clWaitForEvents(2, events));

  // Clean up the second command group.
  EXPECT_SUCCESS(clReleaseEvent(second_event));
  EXPECT_SUCCESS(clReleaseCommandBufferKHR(second_command_buffer));
}

// Tests we can wait on the same event returned from clEnqueueCommandBufferKHR
// twice.
TEST_F(WaitForEventsTest, SingleCommandBufferEventWaitTwice) {
  EXPECT_SUCCESS(clWaitForEvents(1, &event));
  // The event will be complete here, but we should still be able to
  // successfully wait on it.
  EXPECT_SUCCESS(clWaitForEvents(1, &event));
}

// Tests we can wait on the event returned from clEnqueueCommandBufferKHR
// if it appears twice in the wait list.
TEST_F(WaitForEventsTest, SingleCommandBufferTwoCopiesEvent) {
  const cl_event wait_list[]{event, event};
  EXPECT_SUCCESS(clWaitForEvents(2, wait_list));
}

// Tests the status of an event returned from clEnqueueCommandBufferKHR is
// correct after calling clWaitForEvents.
TEST_F(WaitForEventsTest, CheckStatus) {
  EXPECT_SUCCESS(clWaitForEvents(1, &event));
  cl_int command_execution_status;
  EXPECT_SUCCESS(clGetEventInfo(event, CL_EVENT_COMMAND_EXECUTION_STATUS,
                                sizeof(cl_int), &command_execution_status,
                                nullptr));
  EXPECT_EQ(command_execution_status, CL_COMPLETE);
}

// Abstracts common code for testing the values returned from clGetEventInfo
// when called on an event returned from clEnqueueCommandBufferKHR.
class GetEventInfoTest : public CommandBufferEventTest {};

// Tests the event is returning the correct command queue.
TEST_F(GetEventInfoTest, EventCommandQueue) {
  cl_command_queue command_queue = nullptr;
  EXPECT_SUCCESS(clGetEventInfo(event, CL_EVENT_COMMAND_QUEUE,
                                sizeof(cl_command_queue),
                                static_cast<void *>(&command_queue), nullptr));
  EXPECT_EQ(command_queue, this->command_queue);
}

// Tests the event is returning the correct context.
TEST_F(GetEventInfoTest, EventContext) {
  cl_context context = nullptr;
  EXPECT_SUCCESS(clGetEventInfo(event, CL_EVENT_CONTEXT, sizeof(cl_context),
                                static_cast<void *>(&context), nullptr));
  EXPECT_EQ(context, this->context);
}

// Test the event returns the correct command type.
TEST_F(GetEventInfoTest, EventCommandType) {
  cl_command_type command_type;
  EXPECT_SUCCESS(clGetEventInfo(event, CL_EVENT_COMMAND_TYPE,
                                sizeof(cl_command_type), &command_type,
                                nullptr));
  EXPECT_EQ(command_type, CL_COMMAND_COMMAND_BUFFER_KHR);
}

// Tests the event returns the correct execution status.
TEST_F(GetEventInfoTest, EventCommandExecutionStatus) {
  cl_int status;
  EXPECT_SUCCESS(clGetEventInfo(event, CL_EVENT_COMMAND_EXECUTION_STATUS,
                                sizeof(cl_int), &status, nullptr));

  // We know at this point the command has been enqueued, therefore it should be
  // in any valid state.
  EXPECT_TRUE(status == CL_QUEUED || status == CL_SUBMITTED ||
              status == CL_RUNNING || status == CL_COMPLETE);

  // After flushing we know the command can no longer be queued.
  EXPECT_SUCCESS(clFlush(command_queue));
  EXPECT_SUCCESS(clGetEventInfo(event, CL_EVENT_COMMAND_EXECUTION_STATUS,
                                sizeof(cl_int), &status, nullptr));
  EXPECT_TRUE(status == CL_SUBMITTED || status == CL_RUNNING ||
              status == CL_COMPLETE);

  // After a blocking flush we know the command must be completed.
  EXPECT_SUCCESS(clFinish(command_queue));
  EXPECT_SUCCESS(clGetEventInfo(event, CL_EVENT_COMMAND_EXECUTION_STATUS,
                                sizeof(cl_int), &status, nullptr));
  EXPECT_EQ(status, CL_COMPLETE);
}

// Test the event returns a valid reference count.
// note: the OpenCL spec says: "The reference count returned should be
// considered immediately stale. It is unsuitable for general use in
// applications. This feature is provided for identifying memory leaks."
// However we can reasonably assume the reference count should be one in this
// case since the command buffer is only created and destroyed by a singlet
// thread.
TEST_F(GetEventInfoTest, EventReferenceCount) {
  cl_uint reference_count;
  EXPECT_SUCCESS(clGetEventInfo(event, CL_EVENT_REFERENCE_COUNT,
                                sizeof(cl_uint), &reference_count, nullptr));
  EXPECT_EQ(reference_count, 1u);
}

// Abstracts common code for testing the functionality of clSetEventCallback
// when called on an event returned from clEnqueueCommandBufferKHR.
class SetEventCallbackTest : public cl_khr_command_buffer_Test {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(cl_khr_command_buffer_Test::SetUp());
    // Create an empty command queue we can get a signal event from.
    cl_int error = CL_SUCCESS;
    command_buffer =
        clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
    ASSERT_SUCCESS(error);
    EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));
    // So we can control when the command group gets submitted we need the
    // enqueue to wait on something.
    user_event = clCreateUserEvent(context, &error);
    EXPECT_SUCCESS(error);
    EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 1,
                                             &user_event, &event));
  }

  void TearDown() override {
    if (event) {
      EXPECT_SUCCESS(clReleaseEvent(event));
    }
    if (user_event) {
      EXPECT_SUCCESS(clReleaseEvent(user_event));
    }
    if (command_buffer) {
      EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
    }
    cl_khr_command_buffer_Test::TearDown();
  }

  cl_command_buffer_khr command_buffer = nullptr;
  cl_event user_event = nullptr;
  cl_event event = nullptr;

 public:
  struct event_status_pair {
    cl_event event;
    cl_int status;
  };
  static void CL_CALLBACK event_callback(cl_event event,
                                         cl_int event_command_status,
                                         void *user_data) {
    auto event_status = static_cast<event_status_pair *>(user_data);
    event_status->event = event;
    event_status->status = event_command_status;
  };
};

// Note: There is a bug in how we are calling event callbacks. At the moment we
// are passing the current status of the command to the callback, not the status
// passed to the clSetUserEventStatus call which is what should be happening.
// In the following tests EXPECT_GE(CL_STATUS, event_status.event) should be
// EXPECT_EQ(CL_STATUS, event_status.event). See CA-3324, when this is fixed
// these tests should be updated.

// Tests that the clSetEventCallback entry point registers the correct callbacks
// on a cl_event returned via a clEnqueueCommandBufferKHR call for the
// CL_SUBMITTED state.
TEST_F(SetEventCallbackTest, SetEventCallbackSubmitted) {
  // We can't really know a command is in the CL_SUBMITTED state, so we need to
  // flush and block the queue to ensure this command group is executed.
  event_status_pair event_status;
  EXPECT_SUCCESS(
      clSetEventCallback(event, CL_SUBMITTED, &event_callback, &event_status));
  // Release the holding event now that we have set up the callback.
  EXPECT_SUCCESS(clSetUserEventStatus(user_event, CL_COMPLETE));
  EXPECT_SUCCESS(clFinish(command_queue));

  // Check the callback was called on the correct event.
  EXPECT_EQ(event, event_status.event);

  // Check the callback was called for the correct status.
  EXPECT_GE(CL_SUBMITTED, event_status.status);
}

// Tests that the clSetEventCallback entry point registers the correct callbacks
// on a cl_event returned via a clEnqueueCommandBufferKHR call for the
// CL_RUNNING state.
TEST_F(SetEventCallbackTest, SetEventCallbackRunning) {
  event_status_pair event_status;
  EXPECT_SUCCESS(
      clSetEventCallback(event, CL_RUNNING, &event_callback, &event_status));

  // Release the holding event now that we have set up the callback.
  EXPECT_SUCCESS(clSetUserEventStatus(user_event, CL_COMPLETE));
  EXPECT_SUCCESS(clFinish(command_queue));

  // Check the callback was called on the correct event.
  EXPECT_EQ(event, event_status.event);

  // Check the callback was called for the correct status.
  EXPECT_GE(CL_RUNNING, event_status.status);
}

// Tests that the clSetEventCallback entry point registers the correct callbacks
// on a cl_event returned via a clEnqueueCommandBufferKHR call for the
// CL_COMPLETE state.
TEST_F(SetEventCallbackTest, SetEventCallbackComplete) {
  event_status_pair event_status;
  EXPECT_SUCCESS(
      clSetEventCallback(event, CL_COMPLETE, &event_callback, &event_status));
  // Release the holding event now that we have set up the callback.
  EXPECT_SUCCESS(clSetUserEventStatus(user_event, CL_COMPLETE));

  // We know a command is in the CL_FINISH state if we were successfully able to
  // block and flush the queue. flush and block the queue to ensure this command
  EXPECT_SUCCESS(clFinish(command_queue));

  // Check the callback was called on the correct event.
  EXPECT_EQ(event, event_status.event);

  // Check the callback was called for the correct status.
  EXPECT_GE(CL_COMPLETE, event_status.status);
}

// Tests that the clSetEventCallback entry point registers the correct callbacks
// on a cl_event returned via a clEnqueueCommandBufferKHR call for multiple
// states.
TEST_F(SetEventCallbackTest, SetEventCallbackMultiple) {
  // Add a callback for each possible state.
  event_status_pair submitted_event_status;
  EXPECT_SUCCESS(clSetEventCallback(event, CL_COMPLETE, &event_callback,
                                    &submitted_event_status));

  event_status_pair running_event_status;
  EXPECT_SUCCESS(clSetEventCallback(event, CL_COMPLETE, &event_callback,
                                    &running_event_status));

  event_status_pair complete_event_status;
  EXPECT_SUCCESS(clSetEventCallback(event, CL_COMPLETE, &event_callback,
                                    &complete_event_status));

  // Release the gate event and flush the queue, so we know the event must have
  // the CL_COMPLETE state and all callbacks should have been called.
  EXPECT_SUCCESS(clSetUserEventStatus(user_event, CL_COMPLETE));
  EXPECT_SUCCESS(clFinish(command_queue));

  // Check the callback was called and has the correct status for each callback.
  EXPECT_EQ(event, submitted_event_status.event);
  EXPECT_GE(CL_SUBMITTED, submitted_event_status.status);

  EXPECT_EQ(event, running_event_status.event);
  EXPECT_GE(CL_RUNNING, running_event_status.status);

  EXPECT_EQ(event, complete_event_status.event);
  EXPECT_EQ(CL_COMPLETE, complete_event_status.status);
}

// Tests we can successfully retain and release a cl_event from a
// clEnqueueCommandBufferKHR.
TEST_F(CommandBufferEventTest, RetainRelease) {
  cl_uint reference_count;
  EXPECT_SUCCESS(clGetEventInfo(event, CL_EVENT_REFERENCE_COUNT,
                                sizeof(cl_uint), &reference_count, nullptr));
  EXPECT_EQ(reference_count, 1u);

  // Although the OpenCL spec says the value returned from clGetEventInfo with
  // CL_EVENT_REFERENCE_COUNT is immediately stale, we can be confident here
  // that the reference count will increment and de-increment sequentially.
  ASSERT_SUCCESS(clRetainEvent(event));
  EXPECT_SUCCESS(clGetEventInfo(event, CL_EVENT_REFERENCE_COUNT,
                                sizeof(cl_uint), &reference_count, nullptr));
  EXPECT_EQ(reference_count, 2u);

  ASSERT_SUCCESS(clReleaseEvent(event));
  EXPECT_SUCCESS(clGetEventInfo(event, CL_EVENT_REFERENCE_COUNT,
                                sizeof(cl_uint), &reference_count, nullptr));
  EXPECT_EQ(reference_count, 1u);
}

struct CommandBufferEnqueueEventTest : public cl_khr_command_buffer_Test {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(cl_khr_command_buffer_Test::SetUp());

    // Tests inheriting from this class build programs from source and hence
    // require an online compiler.
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }

    const bool simultaneous_support =
        capabilities & CL_COMMAND_BUFFER_CAPABILITY_SIMULTANEOUS_USE_KHR;

    if (!simultaneous_support) {
      GTEST_SKIP();
    }
  }
};

// Tests whether we can reuse a command list twice where the first enqueue
// depends on a user event triggered after the second enqueue.
TEST_F(CommandBufferEnqueueEventTest, IncrementKernelTwiceWithUserEvent) {
  // Set up the kernel.
  // We need something we can check was enqueued twice.
  const char *code = R"OpenCLC(
  __kernel void increment_kernel(global int *counter) {
    ++(counter[0]);
  }
)OpenCLC";
  const size_t code_length = std::strlen(code);

  cl_int error = CL_SUCCESS;
  cl_program program =
      clCreateProgramWithSource(context, 1, &code, &code_length, &error);
  EXPECT_SUCCESS(error);
  EXPECT_SUCCESS(
      clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr));

  cl_kernel kernel = clCreateKernel(program, "increment_kernel", &error);
  EXPECT_SUCCESS(error);

  cl_mem counter_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                         sizeof(cl_int), nullptr, &error);
  EXPECT_SUCCESS(error);

  // Initialize the counter to zero.
  const cl_int zero = 0;
  EXPECT_SUCCESS(clEnqueueFillBuffer(command_queue, counter_buffer, &zero,
                                     sizeof(cl_int), 0, sizeof(cl_int), 0,
                                     nullptr, nullptr));

  EXPECT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem),
                                static_cast<void *>(&counter_buffer)));

  // Set up the command buffer and run the command buffer.
  cl_command_buffer_properties_khr properties[3] = {
      CL_COMMAND_BUFFER_FLAGS_KHR, CL_COMMAND_BUFFER_SIMULTANEOUS_USE_KHR, 0};
  cl_command_buffer_khr command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, properties, &error);
  EXPECT_SUCCESS(error);

  constexpr size_t global_size = 1;
  EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, nullptr, kernel, 1, nullptr, &global_size,
      nullptr, 0, nullptr, nullptr, nullptr));
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  cl_event user_event = clCreateUserEvent(context, &error);
  EXPECT_SUCCESS(error);

  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 1,
                                           &user_event, nullptr));
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));
  EXPECT_SUCCESS(clSetUserEventStatus(user_event, CL_COMPLETE));
  EXPECT_SUCCESS(clFinish(command_queue));

  // Check the results, at this point the command buffer should have been
  // enqueued twice, so the counter should have value two.
  cl_int counter_result = -1;
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, counter_buffer, CL_TRUE, 0,
                                     sizeof(cl_int), &counter_result, 0,
                                     nullptr, nullptr));
  EXPECT_EQ(2, counter_result);

  // Clean up.
  EXPECT_SUCCESS(clReleaseEvent(user_event));
  EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(counter_buffer));
  EXPECT_SUCCESS(clReleaseKernel(kernel));
  EXPECT_SUCCESS(clReleaseProgram(program));
}

// TODO: Add tests for event profiling with clGetEventProfilingInfo for command
// buffers enqueue via clEnqueueCommandBufferKHR (see CA-3322).
