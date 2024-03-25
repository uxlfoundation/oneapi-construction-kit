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

class clCreateSubBufferTest : public ucl::CommandQueueTest {
 protected:
  void SetUp() {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    cl_uint mem_base_addr_align = 0;
    ASSERT_EQ(CL_SUCCESS,
              clGetDeviceInfo(device, CL_DEVICE_MEM_BASE_ADDR_ALIGN,
                              sizeof(cl_uint), &mem_base_addr_align, nullptr));
    // NOTE: Ensure the buffer is large enough to create a sub buffer of any
    // scalar or vector type.
    size = 3 * static_cast<size_t>(mem_base_addr_align);
    region.origin = static_cast<size_t>(mem_base_addr_align);
    region.size = sizeof(cl_int);
  }

  void TearDown() {
    if (buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(buffer));
    }
    CommandQueueTest::TearDown();
  }

  size_t size = 0;
  cl_mem buffer = nullptr;
  cl_buffer_region region = {};
};

TEST_F(clCreateSubBufferTest, InvalidMemObject) {
  cl_int errcode;
  buffer = clCreateSubBuffer(nullptr, CL_MEM_READ_WRITE,
                             CL_BUFFER_CREATE_TYPE_REGION, &region, &errcode);
  EXPECT_FALSE(buffer);
  ASSERT_EQ_ERRCODE(CL_INVALID_MEM_OBJECT, errcode);
}

TEST_F(clCreateSubBufferTest, InvalidValueWriteOnly) {
  cl_int errcode;
  buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, size, nullptr, &errcode);
  EXPECT_TRUE(buffer);
  ASSERT_SUCCESS(errcode);
  cl_mem subBuffer =
      clCreateSubBuffer(buffer, CL_MEM_READ_WRITE, CL_BUFFER_CREATE_TYPE_REGION,
                        &region, &errcode);
  EXPECT_FALSE(subBuffer);
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, errcode);
  subBuffer =
      clCreateSubBuffer(buffer, CL_MEM_READ_ONLY, CL_BUFFER_CREATE_TYPE_REGION,
                        &region, &errcode);
  EXPECT_FALSE(subBuffer);
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, errcode);
}

TEST_F(clCreateSubBufferTest, InvalidValueReadOnly) {
  cl_int errcode;
  buffer = clCreateBuffer(context, CL_MEM_READ_ONLY, size, nullptr, &errcode);
  EXPECT_TRUE(buffer);
  ASSERT_SUCCESS(errcode);
  cl_mem subBuffer =
      clCreateSubBuffer(buffer, CL_MEM_READ_WRITE, CL_BUFFER_CREATE_TYPE_REGION,
                        &region, &errcode);
  ASSERT_FALSE(subBuffer);
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, errcode);
  subBuffer =
      clCreateSubBuffer(buffer, CL_MEM_WRITE_ONLY, CL_BUFFER_CREATE_TYPE_REGION,
                        &region, &errcode);
  EXPECT_FALSE(subBuffer);
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, errcode);
}

TEST_F(clCreateSubBufferTest, InvalidValueUseHostPtr) {
  cl_int errcode;
  buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, size, nullptr, &errcode);
  EXPECT_TRUE(buffer);
  ASSERT_SUCCESS(errcode);
  cl_mem subBuffer =
      clCreateSubBuffer(buffer, CL_MEM_USE_HOST_PTR,
                        CL_BUFFER_CREATE_TYPE_REGION, &region, &errcode);
  EXPECT_FALSE(subBuffer);
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, errcode);
}

TEST_F(clCreateSubBufferTest, InvalidValueAllocHostPtr) {
  cl_int errcode;
  buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, size, nullptr, &errcode);
  EXPECT_TRUE(buffer);
  ASSERT_SUCCESS(errcode);
  cl_mem subBuffer =
      clCreateSubBuffer(buffer, CL_MEM_ALLOC_HOST_PTR,
                        CL_BUFFER_CREATE_TYPE_REGION, &region, &errcode);
  EXPECT_FALSE(subBuffer);
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, errcode);
}

TEST_F(clCreateSubBufferTest, InvalidValueCopyHostPtr) {
  cl_int errcode;
  buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, size, nullptr, &errcode);
  EXPECT_TRUE(buffer);
  ASSERT_SUCCESS(errcode);
  cl_mem subBuffer =
      clCreateSubBuffer(buffer, CL_MEM_COPY_HOST_PTR,
                        CL_BUFFER_CREATE_TYPE_REGION, &region, &errcode);
  EXPECT_FALSE(subBuffer);
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, errcode);
}

TEST_F(clCreateSubBufferTest, InvalidValueHostWriteOnly) {
  cl_int errcode;
  buffer =
      clCreateBuffer(context, CL_MEM_HOST_WRITE_ONLY, size, nullptr, &errcode);
  EXPECT_TRUE(buffer);
  ASSERT_SUCCESS(errcode);
  cl_mem subBuffer =
      clCreateSubBuffer(buffer, CL_MEM_HOST_READ_ONLY,
                        CL_BUFFER_CREATE_TYPE_REGION, &region, &errcode);
  EXPECT_FALSE(subBuffer);
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, errcode);
}

TEST_F(clCreateSubBufferTest, InvalidValueHostReadOnly) {
  cl_int errcode;
  buffer =
      clCreateBuffer(context, CL_MEM_HOST_READ_ONLY, size, nullptr, &errcode);
  EXPECT_TRUE(buffer);
  ASSERT_SUCCESS(errcode);
  cl_mem subBuffer =
      clCreateSubBuffer(buffer, CL_MEM_HOST_WRITE_ONLY,
                        CL_BUFFER_CREATE_TYPE_REGION, &region, &errcode);
  EXPECT_FALSE(subBuffer);
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, errcode);
}

TEST_F(clCreateSubBufferTest, InvalidValueHostNoAccess) {
  cl_int errcode;
  buffer =
      clCreateBuffer(context, CL_MEM_HOST_NO_ACCESS, size, nullptr, &errcode);
  EXPECT_TRUE(buffer);
  ASSERT_SUCCESS(errcode);
  cl_mem subBuffer =
      clCreateSubBuffer(buffer, CL_MEM_HOST_READ_ONLY,
                        CL_BUFFER_CREATE_TYPE_REGION, &region, &errcode);
  ASSERT_FALSE(subBuffer);
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, errcode);
  subBuffer =
      clCreateSubBuffer(buffer, CL_MEM_HOST_WRITE_ONLY,
                        CL_BUFFER_CREATE_TYPE_REGION, &region, &errcode);
  EXPECT_FALSE(subBuffer);
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, errcode);
}

TEST_F(clCreateSubBufferTest, InvalidValueBufferCreateType) {
  cl_int errcode;
  buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, size, nullptr, &errcode);
  EXPECT_TRUE(buffer);
  ASSERT_SUCCESS(errcode);
  cl_mem subBuffer =
      clCreateSubBuffer(buffer, CL_MEM_READ_WRITE, 0, &region, &errcode);
  EXPECT_FALSE(subBuffer);
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, errcode);
}

TEST_F(clCreateSubBufferTest, InvalidValueNullBufferCreateInfo) {
  cl_int errcode;
  buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, size, nullptr, &errcode);
  EXPECT_TRUE(buffer);
  ASSERT_SUCCESS(errcode);
  cl_mem subBuffer =
      clCreateSubBuffer(buffer, CL_MEM_READ_WRITE, CL_BUFFER_CREATE_TYPE_REGION,
                        nullptr, &errcode);
  EXPECT_FALSE(subBuffer);
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, errcode);
}

TEST_F(clCreateSubBufferTest, InvalidValueBufferCreateInfoOutOfBounds) {
  cl_int errcode;
  buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_int), nullptr,
                          &errcode);
  EXPECT_TRUE(buffer);
  ASSERT_SUCCESS(errcode);
  cl_mem subBuffer =
      clCreateSubBuffer(buffer, CL_MEM_READ_WRITE, CL_BUFFER_CREATE_TYPE_REGION,
                        &region, &errcode);
  EXPECT_FALSE(subBuffer);
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, errcode);
}

TEST_F(clCreateSubBufferTest, InvalidBufferSize) {
  cl_int errcode;
  buffer = clCreateBuffer(context, CL_MEM_READ_ONLY, size, nullptr, &errcode);
  EXPECT_TRUE(buffer);
  ASSERT_SUCCESS(errcode);
  region.size = 0;
  cl_mem subBuffer =
      clCreateSubBuffer(buffer, CL_MEM_READ_ONLY, CL_BUFFER_CREATE_TYPE_REGION,
                        &region, &errcode);
  EXPECT_FALSE(subBuffer);
  ASSERT_EQ_ERRCODE(CL_INVALID_BUFFER_SIZE, errcode);
}

TEST_F(clCreateSubBufferTest, InvalidBufferAlign) {
  cl_int errcode;
  buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, size, nullptr, &errcode);
  EXPECT_TRUE(buffer);
  ASSERT_SUCCESS(errcode);
  region.origin = 3;
  cl_mem subBuffer = clCreateSubBuffer(buffer, 0, CL_BUFFER_CREATE_TYPE_REGION,
                                       &region, &errcode);
  EXPECT_FALSE(subBuffer);
  ASSERT_EQ_ERRCODE(CL_MISALIGNED_SUB_BUFFER_OFFSET, errcode);
}

/*! Redmine #5120: Add tests for the following error codes
CL_MEM_OBJECT_ALLOCATION_FAILURE
CL_OUT_OF_RESOURCES
CL_OUT_OF_HOST_MEMORY
*/

TEST_F(clCreateSubBufferTest, DefaultWriteOnly) {
  cl_int errcode;
  buffer = clCreateBuffer(context, CL_MEM_WRITE_ONLY, size, nullptr, &errcode);
  EXPECT_TRUE(buffer);
  ASSERT_SUCCESS(errcode);
  cl_mem subBuffer =
      clCreateSubBuffer(buffer, CL_MEM_WRITE_ONLY, CL_BUFFER_CREATE_TYPE_REGION,
                        &region, &errcode);
  EXPECT_TRUE(subBuffer);
  ASSERT_SUCCESS(errcode);
  ASSERT_SUCCESS(clReleaseMemObject(subBuffer));
}

TEST_F(clCreateSubBufferTest, DefaultReadOnly) {
  cl_int errcode;
  buffer = clCreateBuffer(context, CL_MEM_READ_ONLY, size, nullptr, &errcode);
  EXPECT_TRUE(buffer);
  ASSERT_SUCCESS(errcode);
  cl_mem subBuffer =
      clCreateSubBuffer(buffer, CL_MEM_READ_ONLY, CL_BUFFER_CREATE_TYPE_REGION,
                        &region, &errcode);
  EXPECT_TRUE(subBuffer);
  ASSERT_SUCCESS(errcode);
  ASSERT_SUCCESS(clReleaseMemObject(subBuffer));
}

TEST_F(clCreateSubBufferTest, DefaultReadWrite) {
  cl_int errcode;
  buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, size, nullptr, &errcode);
  EXPECT_TRUE(buffer);
  ASSERT_SUCCESS(errcode);
  cl_mem subBuffer =
      clCreateSubBuffer(buffer, CL_MEM_READ_WRITE, CL_BUFFER_CREATE_TYPE_REGION,
                        &region, &errcode);
  EXPECT_TRUE(subBuffer);
  ASSERT_SUCCESS(errcode);
  ASSERT_SUCCESS(clReleaseMemObject(subBuffer));
}

TEST_F(clCreateSubBufferTest, DefaultHostWriteOnly) {
  cl_int errcode;
  buffer =
      clCreateBuffer(context, CL_MEM_HOST_WRITE_ONLY, size, nullptr, &errcode);
  EXPECT_TRUE(buffer);
  ASSERT_SUCCESS(errcode);
  cl_mem subBuffer =
      clCreateSubBuffer(buffer, CL_MEM_HOST_WRITE_ONLY,
                        CL_BUFFER_CREATE_TYPE_REGION, &region, &errcode);
  EXPECT_TRUE(subBuffer);
  ASSERT_SUCCESS(errcode);
  ASSERT_SUCCESS(clReleaseMemObject(subBuffer));
}

TEST_F(clCreateSubBufferTest, DefaultHostReadOnly) {
  cl_int errcode;
  buffer =
      clCreateBuffer(context, CL_MEM_HOST_READ_ONLY, size, nullptr, &errcode);
  EXPECT_TRUE(buffer);
  ASSERT_SUCCESS(errcode);
  cl_mem subBuffer =
      clCreateSubBuffer(buffer, CL_MEM_HOST_READ_ONLY,
                        CL_BUFFER_CREATE_TYPE_REGION, &region, &errcode);
  EXPECT_TRUE(subBuffer);
  ASSERT_SUCCESS(errcode);
  ASSERT_SUCCESS(clReleaseMemObject(subBuffer));
}

/* Redmine #5120: Check: Add tests for the following cl_mem types
CL_MEM_USE_HOST_PTR
CL_MEM_ALLOC_HOST_PTR
CL_MEM_COPY_HOST_PTR
CL_MEM_HOST_NO_ACCESS
*/
