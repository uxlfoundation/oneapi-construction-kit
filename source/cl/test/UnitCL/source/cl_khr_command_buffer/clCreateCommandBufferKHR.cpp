// Copyright (C) Codeplay Software Limited
//
// Licensed under the Apache License, Version 2.0 (the "License") with LLVM
// Exceptions; you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://github.com/uxlfoundation/oneapi-construction-kit/blob/main/LICENSE.txt
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations
// under the License.
//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "cl_khr_command_buffer.h"

using clCreateCommandBufferAPITest = cl_khr_command_buffer_Test;
TEST_F(clCreateCommandBufferAPITest, ReturnSuccess) {
  cl_int error = CL_OUT_OF_HOST_MEMORY; // Ensure this status is overwritten
  cl_command_buffer_khr command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
  EXPECT_SUCCESS(error);
  EXPECT_TRUE(command_buffer != nullptr);

  EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
}

TEST_F(clCreateCommandBufferAPITest, ReturnNull) {
  cl_command_buffer_khr command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, nullptr, nullptr);
  EXPECT_TRUE(command_buffer != nullptr);

  EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
}

TEST_F(clCreateCommandBufferAPITest, SimultaneousUse) {
  const bool simultaneous_support =
      capabilities & CL_COMMAND_BUFFER_CAPABILITY_SIMULTANEOUS_USE_KHR;

  cl_int err = CL_OUT_OF_HOST_MEMORY;
  cl_command_buffer_properties_khr properties[3] = {
      CL_COMMAND_BUFFER_FLAGS_KHR, CL_COMMAND_BUFFER_SIMULTANEOUS_USE_KHR, 0};
  cl_command_buffer_khr command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, properties, &err);
  if (simultaneous_support) {
    EXPECT_SUCCESS(err);
    EXPECT_TRUE(command_buffer != nullptr);

    EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
  } else {
    // CL_INVALID_PROPERTY is the error code for when properties are valid but
    // not supported by a device
    ASSERT_EQ_ERRCODE(err, CL_INVALID_PROPERTY);
    ASSERT_TRUE(command_buffer == nullptr);
  }
}

TEST_F(clCreateCommandBufferAPITest, DuplicateProperty) {
  cl_int err = CL_SUCCESS;
  cl_command_buffer_properties_khr properties[] = {
      CL_COMMAND_BUFFER_FLAGS_KHR, 0, CL_COMMAND_BUFFER_FLAGS_KHR, 0, 0};

  cl_command_buffer_khr command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, properties, &err);
  ASSERT_EQ_ERRCODE(err, CL_INVALID_VALUE);
  ASSERT_TRUE(command_buffer == nullptr);
}

TEST_F(clCreateCommandBufferAPITest, InvalidPropertyName) {
  cl_int err = CL_SUCCESS;
  cl_command_buffer_properties_khr properties[] = {
      0XFFFF, CL_COMMAND_BUFFER_SIMULTANEOUS_USE_KHR, 0};

  cl_command_buffer_khr command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, properties, &err);
  ASSERT_EQ_ERRCODE(err, CL_INVALID_VALUE);
  ASSERT_TRUE(command_buffer == nullptr);
}

TEST_F(clCreateCommandBufferAPITest, InvalidPropertyValue) {
  cl_int err = CL_SUCCESS;
  cl_command_buffer_properties_khr properties[] = {CL_COMMAND_BUFFER_FLAGS_KHR,
                                                   0xFFFF, 0};

  cl_command_buffer_khr command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, properties, &err);
  ASSERT_EQ_ERRCODE(err, CL_INVALID_VALUE);
  ASSERT_TRUE(command_buffer == nullptr);
}

TEST_F(clCreateCommandBufferAPITest, InvalidCommandQueue) {
  cl_int err = CL_SUCCESS;

  cl_command_buffer_khr command_buffer =
      clCreateCommandBufferKHR(0, &command_queue, nullptr, &err);
  ASSERT_EQ_ERRCODE(err, CL_INVALID_VALUE);
  ASSERT_TRUE(command_buffer == nullptr);

  command_buffer = clCreateCommandBufferKHR(1, nullptr, nullptr, &err);
  ASSERT_EQ_ERRCODE(err, CL_INVALID_VALUE);
  ASSERT_TRUE(command_buffer == nullptr);

  cl_command_queue bad_queue = nullptr;
  command_buffer = clCreateCommandBufferKHR(1, &bad_queue, nullptr, &err);
  ASSERT_EQ_ERRCODE(err, CL_INVALID_COMMAND_QUEUE);
  ASSERT_TRUE(command_buffer == nullptr);
}
