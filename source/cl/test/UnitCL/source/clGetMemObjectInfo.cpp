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

struct clGetMemObjectInfoTest : ucl::ContextTest {
  size_t size = 128;
  cl_int errcode = 69;
};

TEST_F(clGetMemObjectInfoTest, InvaidValue) {
  cl_mem buffer = clCreateBuffer(context, 0, size, nullptr, &errcode);
  EXPECT_TRUE(buffer);
  ASSERT_SUCCESS(errcode);

  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, clGetMemObjectInfo(buffer, CL_SUCCESS, 0,
                                                         nullptr, nullptr));
  ASSERT_SUCCESS(clReleaseMemObject(buffer));
}

TEST_F(clGetMemObjectInfoTest, InvaidMemObject) {
  ASSERT_EQ_ERRCODE(
      CL_INVALID_MEM_OBJECT,
      clGetMemObjectInfo(nullptr, CL_MEM_TYPE, 0, nullptr, &size));
}

TEST_F(clGetMemObjectInfoTest, MemType) {
  cl_int errcode;
  cl_mem buffer = clCreateBuffer(context, 0, size, nullptr, &errcode);
  EXPECT_TRUE(buffer);
  ASSERT_SUCCESS(errcode);

  ASSERT_SUCCESS(clGetMemObjectInfo(buffer, CL_MEM_TYPE, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_mem_object_type), size);
  cl_mem_object_type type;
  ASSERT_SUCCESS(clGetMemObjectInfo(buffer, CL_MEM_TYPE, size, &type, nullptr));
  const cl_mem_object_type expect = CL_MEM_OBJECT_BUFFER;
  ASSERT_EQ(expect, type);
  // Redmine #5125: Check: Test cl_image_desc.image_type when supported!

  ASSERT_SUCCESS(clReleaseMemObject(buffer));
}

TEST_F(clGetMemObjectInfoTest, MemFlags) {
  cl_int errcode;
  cl_mem buffer =
      clCreateBuffer(context, CL_MEM_READ_WRITE, size, nullptr, &errcode);
  EXPECT_TRUE(buffer);
  ASSERT_SUCCESS(errcode);

  ASSERT_SUCCESS(clGetMemObjectInfo(buffer, CL_MEM_FLAGS, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_mem_flags), size);
  cl_mem_flags flags;
  ASSERT_SUCCESS(
      clGetMemObjectInfo(buffer, CL_MEM_FLAGS, size, &flags, nullptr));
  const cl_mem_flags expect = CL_MEM_READ_WRITE;
  ASSERT_EQ(expect, flags);

  ASSERT_SUCCESS(clReleaseMemObject(buffer));
}

TEST_F(clGetMemObjectInfoTest, MemSize) {
  cl_int errcode;
  cl_mem buffer = clCreateBuffer(context, 0, size, nullptr, &errcode);
  EXPECT_TRUE(buffer);
  ASSERT_SUCCESS(errcode);

  size_t sizeRet;
  ASSERT_SUCCESS(clGetMemObjectInfo(buffer, CL_MEM_SIZE, 0, nullptr, &sizeRet));
  ASSERT_EQ(sizeof(size_t), sizeRet);
  size_t memSize;
  ASSERT_SUCCESS(
      clGetMemObjectInfo(buffer, CL_MEM_SIZE, sizeRet, &memSize, nullptr));
  ASSERT_EQ(size, memSize);

  ASSERT_SUCCESS(clReleaseMemObject(buffer));
}

TEST_F(clGetMemObjectInfoTest, MemHostPtr) {
  UCL::AlignedBuffer<cl_int> data(1);
  data[0] = 69;
  cl_int errcode;
  cl_mem hostBuffer = clCreateBuffer(context, CL_MEM_USE_HOST_PTR,
                                     sizeof(cl_int), &data, &errcode);
  EXPECT_TRUE(hostBuffer);
  ASSERT_SUCCESS(errcode);

  ASSERT_SUCCESS(
      clGetMemObjectInfo(hostBuffer, CL_MEM_HOST_PTR, 0, nullptr, &size));
  ASSERT_EQ(sizeof(void *), size);
  void *ptr = nullptr;
  ASSERT_SUCCESS(clGetMemObjectInfo(hostBuffer, CL_MEM_HOST_PTR, size,
                                    static_cast<void *>(&ptr), nullptr));
  ASSERT_EQ(&data, ptr);

  ASSERT_SUCCESS(clReleaseMemObject(hostBuffer));

  cl_mem devBuffer = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(cl_int),
                                    nullptr, &errcode);
  EXPECT_TRUE(devBuffer);
  ASSERT_SUCCESS(errcode);

  ASSERT_SUCCESS(
      clGetMemObjectInfo(devBuffer, CL_MEM_HOST_PTR, 0, nullptr, &size));
  ASSERT_EQ(sizeof(void *), size);
  ASSERT_SUCCESS(clGetMemObjectInfo(devBuffer, CL_MEM_HOST_PTR, size,
                                    static_cast<void *>(&ptr), nullptr));
  ASSERT_FALSE(ptr);

  ASSERT_SUCCESS(clReleaseMemObject(devBuffer));
}

/*!  Redmine #5135: Test CL_MEM_MAP_COUNT once supported */

TEST_F(clGetMemObjectInfoTest, MemReferenceCount) {
  cl_int errcode;
  cl_mem buffer = clCreateBuffer(context, 0, size, nullptr, &errcode);
  EXPECT_TRUE(buffer);
  ASSERT_SUCCESS(errcode);

  ASSERT_SUCCESS(
      clGetMemObjectInfo(buffer, CL_MEM_REFERENCE_COUNT, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint refCount;
  ASSERT_SUCCESS(clGetMemObjectInfo(buffer, CL_MEM_REFERENCE_COUNT, size,
                                    &refCount, nullptr));
  ASSERT_LT(0u, refCount);

  ASSERT_SUCCESS(clReleaseMemObject(buffer));
}

TEST_F(clGetMemObjectInfoTest, MemContext) {
  cl_int errcode;
  cl_mem buffer = clCreateBuffer(context, 0, size, nullptr, &errcode);
  EXPECT_TRUE(buffer);
  ASSERT_SUCCESS(errcode);

  ASSERT_SUCCESS(clGetMemObjectInfo(buffer, CL_MEM_CONTEXT, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_context), size);
  cl_context thisContext = (cl_context)1;
  ASSERT_SUCCESS(clGetMemObjectInfo(buffer, CL_MEM_CONTEXT, size,
                                    static_cast<void *>(&thisContext),
                                    nullptr));
  ASSERT_EQ(context, thisContext);

  ASSERT_SUCCESS(clReleaseMemObject(buffer));
}

TEST_F(clGetMemObjectInfoTest, MemAssociateMemObject) {
  cl_int errcode;
  cl_mem buffer = clCreateBuffer(context, 0, size, nullptr, &errcode);
  EXPECT_TRUE(buffer);
  ASSERT_SUCCESS(errcode);

  ASSERT_SUCCESS(clGetMemObjectInfo(buffer, CL_MEM_ASSOCIATED_MEMOBJECT, 0,
                                    nullptr, &size));
  ASSERT_EQ(sizeof(cl_mem), size);
  cl_mem otherBuffer = (cl_mem)1;
  ASSERT_SUCCESS(clGetMemObjectInfo(buffer, CL_MEM_ASSOCIATED_MEMOBJECT, size,
                                    static_cast<void *>(&otherBuffer),
                                    nullptr));
  ASSERT_FALSE(otherBuffer);

  ASSERT_SUCCESS(clReleaseMemObject(buffer));
}

TEST_F(clGetMemObjectInfoTest, MemAssociateMemObjectWithSubBuffer) {
  cl_int errcode;
  cl_mem buffer = clCreateBuffer(context, 0, size, nullptr, &errcode);
  EXPECT_TRUE(buffer);
  ASSERT_SUCCESS(errcode);

  struct {
    size_t origin;
    size_t size;
  } info;

  // NOTE: Use zero to create sub buffer at the beginning of the buffer and
  // avoid querying for CL_DEVICE_MEM_BASE_ADDR_ALIGN.
  info.origin = 0;
  info.size = 3;

  cl_mem subBuffer = clCreateSubBuffer(buffer, 0, CL_BUFFER_CREATE_TYPE_REGION,
                                       &info, &errcode);
  EXPECT_TRUE(subBuffer);
  ASSERT_SUCCESS(errcode);

  ASSERT_SUCCESS(clGetMemObjectInfo(subBuffer, CL_MEM_ASSOCIATED_MEMOBJECT, 0,
                                    nullptr, &size));
  ASSERT_EQ(sizeof(cl_mem), size);
  cl_mem otherBuffer = nullptr;
  ASSERT_SUCCESS(clGetMemObjectInfo(subBuffer, CL_MEM_ASSOCIATED_MEMOBJECT,
                                    size, static_cast<void *>(&otherBuffer),
                                    nullptr));
  ASSERT_EQ(buffer, otherBuffer);

  ASSERT_SUCCESS(clReleaseMemObject(subBuffer));
  ASSERT_SUCCESS(clReleaseMemObject(buffer));
}

TEST_F(clGetMemObjectInfoTest, MemOffset) {
  cl_int errcode;
  cl_mem buffer = clCreateBuffer(context, 0, size, nullptr, &errcode);
  EXPECT_TRUE(buffer);
  ASSERT_SUCCESS(errcode);

  ASSERT_SUCCESS(clGetMemObjectInfo(buffer, CL_MEM_OFFSET, 0, nullptr, &size));
  ASSERT_EQ(sizeof(size_t), size);
  size_t bufferSize;
  ASSERT_SUCCESS(
      clGetMemObjectInfo(buffer, CL_MEM_OFFSET, size, &bufferSize, nullptr));
  ASSERT_EQ(0u, bufferSize);
  // Redmine #5120: Additional test when sub buffers supported!
  // cl_mem subBuffer = clCreateSubBuffer(
  ASSERT_SUCCESS(clReleaseMemObject(buffer));
}

/*! Redmine #5115: Test CL_MEM_D3D10_RESOURCE_KHR once supported */
/*! Redmine #5115: Test CL_MEM_DX9_MEDIA_ADAPTER_TYPE_KHR once supported */
/*! Redmine #5115: Test CL_MEM_DX9_MEDIA_SURFACE_INFO_KHR once supported */
/*! Redmine #5115: Test CL_MEM_D3D11_RESOURCE_KHR once supported */

#if defined(CL_VERSION_3_0)
struct clGetMemObjectInfoPropertiesTest : ucl::ContextTest {
  void TearDown() override {
    if (!UCL::isDeviceVersionAtLeast({3, 0})) {
      GTEST_SKIP();
    }
    if (buffer) {
      ASSERT_SUCCESS(clReleaseMemObject(buffer));
    }
    ContextTest::TearDown();
  }

  cl_mem buffer = nullptr;
};

// NOTE: OpenCL 3.0 does not define any optional properties for buffers.

TEST_F(clGetMemObjectInfoPropertiesTest, SuccessNull) {
  cl_int error;
  buffer = clCreateBufferWithProperties(context, nullptr, 0, sizeof(cl_float16),
                                        nullptr, &error);
  ASSERT_SUCCESS(error);
  size_t size;
  ASSERT_SUCCESS(
      clGetMemObjectInfo(buffer, CL_MEM_PROPERTIES, 0, nullptr, &size));
  ASSERT_EQ(0u, size);
}

TEST_F(clGetMemObjectInfoPropertiesTest, clCreateBufferProperties) {
  // Buffer created with clCreateBuffer must have no properties.
  cl_int error{};
  buffer = clCreateBuffer(context, 0, 42, nullptr, &error);
  ASSERT_SUCCESS(error);

  size_t size{};
  ASSERT_SUCCESS(
      clGetMemObjectInfo(buffer, CL_MEM_PROPERTIES, 0, nullptr, &size));
  EXPECT_EQ(size, 0)
      << "buffer created with clCreateBuffer must return "
         "param_value_size_ret out parameter equal to 0, indicating there are "
         "no properties to be returned";
}

TEST_F(clGetMemObjectInfoPropertiesTest, clCreateSubbufferProperties) {
  // Subbuffers created with clCreateSubBuffer must have no properties.
  cl_int error{};
  buffer = clCreateBuffer(context, 0, 42, nullptr, &error);
  ASSERT_SUCCESS(error);

  cl_buffer_region region{0, 24};
  cl_mem subbuffer = clCreateSubBuffer(buffer, 0, CL_BUFFER_CREATE_TYPE_REGION,
                                       &region, &error);
  size_t size{};
  ASSERT_SUCCESS(
      clGetMemObjectInfo(subbuffer, CL_MEM_PROPERTIES, 0, nullptr, &size));
  EXPECT_EQ(size, 0)
      << "subbuffers created with clCreateSubBuffer must return "
         "param_value_size_ret out parameter equal to 0, indicating there are "
         "no properties to be returned";
  EXPECT_SUCCESS(clReleaseMemObject(subbuffer));
}

struct clGetMemObjectInfoUsesSVMPointerTest
    : clGetMemObjectInfoTest,
      ::testing::WithParamInterface<std::tuple<size_t, int>> {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(clGetMemObjectInfoTest::SetUp());
    // Skip for non OpenCL-3.0 implementations.
    if (!UCL::isDeviceVersionAtLeast({3, 0})) {
      GTEST_SKIP();
    }
    cl_int errcode;
    non_svm_buffer = clCreateBuffer(context, 0, size, nullptr, &errcode);
    EXPECT_TRUE(non_svm_buffer);
    ASSERT_SUCCESS(errcode);
  }

  void TearDown() override {
    if (non_svm_buffer) {
      ASSERT_SUCCESS(clReleaseMemObject(non_svm_buffer));
    }
    clGetMemObjectInfoTest::TearDown();
  }

  cl_mem non_svm_buffer = nullptr;
};

TEST_P(clGetMemObjectInfoUsesSVMPointerTest, CheckSizeQuerySucceeds) {
  // Get the enumeration value.
  auto query_enum_value = std::get<1>(GetParam());
  // Query for size of value.
  size_t size{};
  EXPECT_SUCCESS(
      clGetMemObjectInfo(non_svm_buffer, query_enum_value, 0, nullptr, &size));
}

TEST_P(clGetMemObjectInfoUsesSVMPointerTest, CheckSizeQueryIsCorrect) {
  // Get the enumeration value.
  auto query_enum_value = std::get<1>(GetParam());
  // Query for size of value.
  size_t size{};
  ASSERT_SUCCESS(
      clGetMemObjectInfo(non_svm_buffer, query_enum_value, 0, nullptr, &size));
  // Get the correct size of the query.
  auto value_size_in_bytes = std::get<0>(GetParam());
  // Check the queried value is correct.
  EXPECT_EQ(size, value_size_in_bytes);
}

TEST_P(clGetMemObjectInfoUsesSVMPointerTest, CheckQuerySucceeds) {
  // Get the correct size of the query and the query itself.
  auto value_size_in_bytes = std::get<0>(GetParam());
  auto query_enum_value = std::get<1>(GetParam());
  // Query for the value.
  UCL::Buffer<char> value_buffer{value_size_in_bytes};
  EXPECT_SUCCESS(clGetMemObjectInfo(non_svm_buffer, query_enum_value,
                                    value_buffer.size(), value_buffer.data(),
                                    nullptr));
}

TEST_P(clGetMemObjectInfoUsesSVMPointerTest, CheckIncorrectSizeQueryFails) {
  // Get the correct size of the query and the query itself.
  auto value_size_in_bytes = std::get<0>(GetParam());
  auto query_enum_value = std::get<1>(GetParam());
  // Query for the value with buffer that is too small.
  UCL::Buffer<char> value_buffer{value_size_in_bytes};
  EXPECT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clGetMemObjectInfo(non_svm_buffer, query_enum_value,
                                       value_buffer.size() - 1,
                                       value_buffer.data(), nullptr));
}

TEST_F(clGetMemObjectInfoUsesSVMPointerTest, CheckNonSVMBufferIsNonSVM) {
  // Query for CL_MEM_USES_SVM_BUFFER with buffer that was not created with host
  // pointer.
  cl_bool uses_svm{};
  ASSERT_SUCCESS(clGetMemObjectInfo(non_svm_buffer, CL_MEM_USES_SVM_POINTER,
                                    sizeof(cl_bool), &uses_svm, nullptr));
  EXPECT_EQ(uses_svm, CL_FALSE);
}

INSTANTIATE_TEST_CASE_P(
    MemObjectQuery, clGetMemObjectInfoUsesSVMPointerTest,
    testing::Values(std::make_tuple(sizeof(cl_bool), CL_MEM_USES_SVM_POINTER)),
    [](const testing::TestParamInfo<
        clGetMemObjectInfoUsesSVMPointerTest::ParamType> &info) {
      return UCL::memObjectQueryToString(std::get<1>(info.param));
    });
#endif
