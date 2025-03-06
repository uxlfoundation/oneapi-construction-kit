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

class clGetCommandBufferInfoTest : public cl_khr_command_buffer_Test {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(cl_khr_command_buffer_Test::SetUp());
    cl_int err = !CL_SUCCESS;
    command_buffer = clCreateCommandBufferKHR(1, &command_queue, nullptr, &err);
    ASSERT_SUCCESS(err);
    ASSERT_TRUE(command_buffer != nullptr);
  }

  void TearDown() override {
    if (command_buffer) {
      EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
    }
    cl_khr_command_buffer_Test::TearDown();
  }

  cl_command_buffer_khr command_buffer = nullptr;
};

TEST_F(clGetCommandBufferInfoTest, InvalidCommandBuffer) {
  ASSERT_EQ_ERRCODE(CL_INVALID_COMMAND_BUFFER_KHR,
                    clGetCommandBufferInfoKHR(nullptr, 0, 0, nullptr, nullptr));
}

TEST_F(clGetCommandBufferInfoTest, InvalidParamName) {
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clGetCommandBufferInfoKHR(command_buffer, CL_SUCCESS, 0,
                                              nullptr, nullptr));
}

TEST_F(clGetCommandBufferInfoTest, ReturnBufferSizeTooSmall) {
  size_t param_value = 0;
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clGetCommandBufferInfoKHR(command_buffer,
                                              CL_COMMAND_BUFFER_NUM_QUEUES_KHR,
                                              1, &param_value, nullptr));
}

TEST_F(clGetCommandBufferInfoTest, CommandBufferNumQueues) {
  size_t size;
  ASSERT_SUCCESS(clGetCommandBufferInfoKHR(
      command_buffer, CL_COMMAND_BUFFER_NUM_QUEUES_KHR, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint command_buffer_num_queues;
  ASSERT_SUCCESS(clGetCommandBufferInfoKHR(
      command_buffer, CL_COMMAND_BUFFER_NUM_QUEUES_KHR, size,
      &command_buffer_num_queues, nullptr));
  ASSERT_EQ(1, command_buffer_num_queues);
}

TEST_F(clGetCommandBufferInfoTest, CommandBufferQueues) {
  size_t size;
  ASSERT_SUCCESS(clGetCommandBufferInfoKHR(
      command_buffer, CL_COMMAND_BUFFER_QUEUES_KHR, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_command_queue), size);
  cl_command_queue command_buffer_queue = nullptr;
  ASSERT_SUCCESS(clGetCommandBufferInfoKHR(
      command_buffer, CL_COMMAND_BUFFER_QUEUES_KHR, size,
      static_cast<void *>(&command_buffer_queue), nullptr));
  ASSERT_EQ(command_queue, command_buffer_queue);
}

TEST_F(clGetCommandBufferInfoTest, CommandBufferReferenceCount) {
  size_t size;
  ASSERT_SUCCESS(clGetCommandBufferInfoKHR(
      command_buffer, CL_COMMAND_BUFFER_REFERENCE_COUNT_KHR, 0, nullptr,
      &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint ref_count = 0;
  ASSERT_SUCCESS(clGetCommandBufferInfoKHR(
      command_buffer, CL_COMMAND_BUFFER_REFERENCE_COUNT_KHR, size, &ref_count,
      nullptr));
  ASSERT_GE(ref_count, 1u);

  // Check retain increases ref count
  ASSERT_SUCCESS(clRetainCommandBufferKHR(command_buffer));
  cl_uint inc_ref_count = 0;
  ASSERT_SUCCESS(clGetCommandBufferInfoKHR(
      command_buffer, CL_COMMAND_BUFFER_REFERENCE_COUNT_KHR, size,
      &inc_ref_count, nullptr));
  ASSERT_GT(inc_ref_count, ref_count);

  // Check release decreases ref count
  ASSERT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
  cl_uint dec_ref_count = 0;
  ASSERT_SUCCESS(clGetCommandBufferInfoKHR(
      command_buffer, CL_COMMAND_BUFFER_REFERENCE_COUNT_KHR, size,
      &dec_ref_count, nullptr));
  ASSERT_LT(dec_ref_count, inc_ref_count);
}

TEST_F(clGetCommandBufferInfoTest, CommandBufferState) {
  size_t size;
  ASSERT_SUCCESS(clGetCommandBufferInfoKHR(
      command_buffer, CL_COMMAND_BUFFER_STATE_KHR, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_command_buffer_state_khr), size);

  cl_command_buffer_state_khr state = 0;
  ASSERT_SUCCESS(clGetCommandBufferInfoKHR(
      command_buffer, CL_COMMAND_BUFFER_STATE_KHR, size, &state, nullptr));
  ASSERT_EQ(CL_COMMAND_BUFFER_STATE_RECORDING_KHR, state);

  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  ASSERT_SUCCESS(clGetCommandBufferInfoKHR(
      command_buffer, CL_COMMAND_BUFFER_STATE_KHR, size, &state, nullptr));
  ASSERT_EQ(CL_COMMAND_BUFFER_STATE_EXECUTABLE_KHR, state);

  cl_int error;
  cl_event user_event = clCreateUserEvent(context, &error);
  ASSERT_SUCCESS(error);

  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 1,
                                           &user_event, nullptr));

  EXPECT_SUCCESS(clGetCommandBufferInfoKHR(
      command_buffer, CL_COMMAND_BUFFER_STATE_KHR, size, &state, nullptr));
  EXPECT_EQ(CL_COMMAND_BUFFER_STATE_PENDING_KHR, state);

  EXPECT_SUCCESS(clSetUserEventStatus(user_event, CL_COMPLETE));
  EXPECT_SUCCESS(clFinish(command_queue));

  EXPECT_SUCCESS(clGetCommandBufferInfoKHR(
      command_buffer, CL_COMMAND_BUFFER_STATE_KHR, size, &state, nullptr));
  EXPECT_EQ(CL_COMMAND_BUFFER_STATE_EXECUTABLE_KHR, state);

  ASSERT_SUCCESS(clReleaseEvent(user_event));
}

class clGetCommandBufferInfoPropertiesTest : public clGetCommandBufferInfoTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(clGetCommandBufferInfoTest::SetUp());
    simultaneous_support =
        capabilities & CL_COMMAND_BUFFER_CAPABILITY_SIMULTANEOUS_USE_KHR;

    properties = {CL_COMMAND_BUFFER_FLAGS_KHR,
                  CL_COMMAND_BUFFER_SIMULTANEOUS_USE_KHR, 0};
    if (!simultaneous_support) {
      properties[1] = 0;
    }
    cl_int err = !CL_SUCCESS;
    properties_command_buffer =
        clCreateCommandBufferKHR(1, &command_queue, properties.data(), &err);
    ASSERT_SUCCESS(err);
    ASSERT_TRUE(command_buffer != nullptr);
  }

  void TearDown() override {
    if (properties_command_buffer) {
      EXPECT_SUCCESS(clReleaseCommandBufferKHR(properties_command_buffer));
    }

    clGetCommandBufferInfoTest::TearDown();
  }

  cl_command_buffer_khr properties_command_buffer = nullptr;
  bool simultaneous_support = false;
  std::array<cl_command_buffer_properties_khr, 3> properties;
};

TEST_F(clGetCommandBufferInfoPropertiesTest, NoProperties) {
  size_t size;
  ASSERT_SUCCESS(clGetCommandBufferInfoKHR(
      command_buffer, CL_COMMAND_BUFFER_PROPERTIES_ARRAY_KHR, 0, nullptr,
      &size));
  ASSERT_EQ(0, size);
}

TEST_F(clGetCommandBufferInfoPropertiesTest, PropertiesSet) {
  size_t size;
  ASSERT_SUCCESS(clGetCommandBufferInfoKHR(
      properties_command_buffer, CL_COMMAND_BUFFER_PROPERTIES_ARRAY_KHR, 0,
      nullptr, &size));
  ASSERT_EQ(size, sizeof(cl_command_buffer_properties_khr) * properties.size());

  std::array<cl_command_buffer_properties_khr, 3> queried_properties;
  ASSERT_SUCCESS(clGetCommandBufferInfoKHR(
      properties_command_buffer, CL_COMMAND_BUFFER_PROPERTIES_ARRAY_KHR, size,
      queried_properties.data(), nullptr));
  ASSERT_EQ(properties, queried_properties);
}
