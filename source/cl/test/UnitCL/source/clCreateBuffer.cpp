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

#include <limits>

#include "Common.h"

struct clCreateBufferTest : ucl::ContextTest,
                            testing::WithParamInterface<cl_mem_flags> {};

struct clCreateBufferParamTest : ucl::ContextTest,
                                 testing::WithParamInterface<bool> {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
#if !defined(CL_VERSION_3_0)
    if (GetParam()) {
      GTEST_SKIP();
    }
#endif
  }

  cl_mem CreateBuffer(cl_context context, cl_mem_flags flags, size_t size,
                      void *host_ptr, cl_int *errcode_ret) {
    cl_mem buffer = nullptr;
    if (GetParam()) {
#if defined(CL_VERSION_3_0)
      buffer = clCreateBufferWithProperties(context, nullptr, flags, size,
                                            host_ptr, errcode_ret);
#else
      // We shouldn't get to this stage if OpenCL 3.0 support is unavailable
      // since the test will be skipped at setup. However, we need to set
      // `errcode_ret` or certain compilers will get upset.
      if (errcode_ret) {
        *errcode_ret = CL_INVALID_OPERATION;
      }
#endif
    } else {
      buffer = clCreateBuffer(context, flags, size, host_ptr, errcode_ret);
    }
    return buffer;
  }
};

struct clCreateBufferWithHostTest : ucl::ContextTest,
                                    testing::WithParamInterface<cl_mem_flags> {
};

struct clCreateBufferBadTest : ucl::ContextTest,
                               testing::WithParamInterface<cl_mem_flags> {};

TEST_P(clCreateBufferParamTest, Default) {
  cl_int errorcode;
  cl_mem buffer = CreateBuffer(context, 0, 128, nullptr, &errorcode);
  EXPECT_TRUE(buffer);
  ASSERT_EQ_ERRCODE(CL_SUCCESS, errorcode);

  ASSERT_EQ_ERRCODE(CL_SUCCESS, clReleaseMemObject(buffer));
}

TEST_P(clCreateBufferParamTest, nullptrErrorcode) {
  cl_mem buffer = CreateBuffer(context, 0, 128, nullptr, nullptr);
  ASSERT_TRUE(buffer);

  ASSERT_EQ_ERRCODE(CL_SUCCESS, clReleaseMemObject(buffer));
}

TEST_P(clCreateBufferParamTest, BadContext) {
  cl_int errorcode;
  EXPECT_FALSE(CreateBuffer(nullptr, 0, 128, nullptr, &errorcode));
  ASSERT_EQ_ERRCODE(CL_INVALID_CONTEXT, errorcode);
}

TEST_P(clCreateBufferParamTest, SizeZero) {
  cl_int errorcode;
  EXPECT_FALSE(CreateBuffer(context, 0, 0, nullptr, &errorcode));
  ASSERT_EQ_ERRCODE(CL_INVALID_BUFFER_SIZE, errorcode);
}

TEST_P(clCreateBufferParamTest, HostPtrWithoutFlags) {
  cl_int errorcode;
  cl_int something;
  EXPECT_FALSE(CreateBuffer(context, 0, 4, &something, &errorcode));
  ASSERT_EQ_ERRCODE(CL_INVALID_HOST_PTR, errorcode);
}

TEST_P(clCreateBufferParamTest, NoHostWithUseHostFlag) {
  cl_int errorcode;
  EXPECT_FALSE(
      CreateBuffer(context, CL_MEM_USE_HOST_PTR, 4, nullptr, &errorcode));
  ASSERT_EQ_ERRCODE(CL_INVALID_HOST_PTR, errorcode);
}

TEST_P(clCreateBufferParamTest, NoHostWithCopyHostFlag) {
  cl_int errorcode;
  EXPECT_FALSE(
      CreateBuffer(context, CL_MEM_COPY_HOST_PTR, 4, nullptr, &errorcode));
  ASSERT_EQ_ERRCODE(CL_INVALID_HOST_PTR, errorcode);
}

TEST_P(clCreateBufferParamTest, SizeTooBig) {
  cl_int errorcode;
  cl_ulong max_mem;
  ASSERT_EQ_ERRCODE(CL_SUCCESS,
                    clGetDeviceInfo(device, CL_DEVICE_MAX_MEM_ALLOC_SIZE,
                                    sizeof(cl_ulong), &max_mem, nullptr));

  ASSERT_GE(std::numeric_limits<size_t>::max(), max_mem);

  cl_mem buffer = CreateBuffer(context, 0, static_cast<size_t>(max_mem + 1),
                               nullptr, &errorcode);
  ASSERT_FALSE(buffer);
  ASSERT_EQ_ERRCODE(CL_INVALID_BUFFER_SIZE, errorcode);
}

// Test the default `clCreateBuffer` path.
INSTANTIATE_TEST_CASE_P(clCreateBuffer, clCreateBufferParamTest,
                        ::testing::ValuesIn({false}));

// Test the `clCreateBufferWithProperties` path.
INSTANTIATE_TEST_CASE_P(clCreateBufferWithPropertiesTest,
                        clCreateBufferParamTest, ::testing::ValuesIn({true}));

TEST_P(clCreateBufferTest, GoodWithoutHost) {
  cl_int errorcode;
  cl_mem buffer = clCreateBuffer(context, GetParam(), 128, nullptr, &errorcode);
  EXPECT_TRUE(buffer);
  ASSERT_EQ_ERRCODE(CL_SUCCESS, errorcode);

  ASSERT_EQ_ERRCODE(CL_SUCCESS, clReleaseMemObject(buffer));
}

TEST_P(clCreateBufferWithHostTest, GoodWithHost) {
  UCL::Buffer<cl_char> host(128);
  cl_int errorcode;
  cl_mem buffer =
      clCreateBuffer(context, GetParam(), host.size(), host, &errorcode);
  EXPECT_TRUE(buffer);
  ASSERT_EQ_ERRCODE(CL_SUCCESS, errorcode);

  ASSERT_EQ_ERRCODE(CL_SUCCESS, clReleaseMemObject(buffer));
}

TEST_P(clCreateBufferBadTest, Bad) {
  cl_int errorcode;
  EXPECT_FALSE(clCreateBuffer(context, GetParam(), 0, nullptr, &errorcode));
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, errorcode);
}

static cl_mem_flags GoodWithoutHost[] = {
    CL_MEM_ALLOC_HOST_PTR,
    CL_MEM_HOST_WRITE_ONLY,
    CL_MEM_HOST_READ_ONLY,
    CL_MEM_HOST_NO_ACCESS,
    CL_MEM_READ_WRITE,
    CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,
    CL_MEM_READ_WRITE | CL_MEM_HOST_WRITE_ONLY,
    CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY,
    CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS,
    CL_MEM_WRITE_ONLY,
    CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR,
    CL_MEM_WRITE_ONLY | CL_MEM_HOST_WRITE_ONLY,
    CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY,
    CL_MEM_WRITE_ONLY | CL_MEM_HOST_NO_ACCESS,
    CL_MEM_READ_ONLY,
    CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
    CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY,
    CL_MEM_READ_ONLY | CL_MEM_HOST_READ_ONLY,
    CL_MEM_READ_ONLY | CL_MEM_HOST_NO_ACCESS};

static cl_mem_flags GoodWithHost[] = {
    CL_MEM_USE_HOST_PTR | CL_MEM_HOST_WRITE_ONLY,
    CL_MEM_USE_HOST_PTR | CL_MEM_HOST_READ_ONLY,
    CL_MEM_USE_HOST_PTR | CL_MEM_HOST_NO_ACCESS,
    CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE,
    CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_HOST_WRITE_ONLY,
    CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY,
    CL_MEM_USE_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS,
    CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY,
    CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY | CL_MEM_HOST_WRITE_ONLY,
    CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY,
    CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY | CL_MEM_HOST_NO_ACCESS,
    CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY,
    CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY,
    CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_HOST_READ_ONLY,
    CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_HOST_NO_ACCESS,
    CL_MEM_COPY_HOST_PTR | CL_MEM_ALLOC_HOST_PTR,
    CL_MEM_COPY_HOST_PTR | CL_MEM_HOST_WRITE_ONLY,
    CL_MEM_COPY_HOST_PTR | CL_MEM_HOST_READ_ONLY,
    CL_MEM_COPY_HOST_PTR | CL_MEM_HOST_NO_ACCESS,
    CL_MEM_COPY_HOST_PTR | CL_MEM_READ_WRITE,
    CL_MEM_COPY_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_ALLOC_HOST_PTR,
    CL_MEM_COPY_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_HOST_WRITE_ONLY,
    CL_MEM_COPY_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_HOST_READ_ONLY,
    CL_MEM_COPY_HOST_PTR | CL_MEM_READ_WRITE | CL_MEM_HOST_NO_ACCESS,
    CL_MEM_COPY_HOST_PTR | CL_MEM_WRITE_ONLY,
    CL_MEM_COPY_HOST_PTR | CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR,
    CL_MEM_COPY_HOST_PTR | CL_MEM_WRITE_ONLY | CL_MEM_HOST_WRITE_ONLY,
    CL_MEM_COPY_HOST_PTR | CL_MEM_WRITE_ONLY | CL_MEM_HOST_READ_ONLY,
    CL_MEM_COPY_HOST_PTR | CL_MEM_WRITE_ONLY | CL_MEM_HOST_NO_ACCESS,
    CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY,
    CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR,
    CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_HOST_WRITE_ONLY,
    CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_HOST_READ_ONLY,
    CL_MEM_COPY_HOST_PTR | CL_MEM_READ_ONLY | CL_MEM_HOST_NO_ACCESS};

static cl_mem_flags BadValues[] = {
    CL_MEM_READ_WRITE | CL_MEM_WRITE_ONLY,
    CL_MEM_READ_WRITE | CL_MEM_READ_ONLY,
    CL_MEM_READ_ONLY | CL_MEM_WRITE_ONLY,
    CL_MEM_USE_HOST_PTR | CL_MEM_ALLOC_HOST_PTR,
    CL_MEM_USE_HOST_PTR | CL_MEM_COPY_HOST_PTR,
    CL_MEM_HOST_WRITE_ONLY | CL_MEM_HOST_READ_ONLY,
    CL_MEM_HOST_WRITE_ONLY | CL_MEM_HOST_NO_ACCESS,
    CL_MEM_HOST_NO_ACCESS | CL_MEM_HOST_READ_ONLY};

INSTANTIATE_TEST_CASE_P(clCreateBuffer, clCreateBufferTest,
                        ::testing::ValuesIn(GoodWithoutHost));

INSTANTIATE_TEST_CASE_P(clCreateBuffer, clCreateBufferWithHostTest,
                        ::testing::ValuesIn(GoodWithHost));

INSTANTIATE_TEST_CASE_P(clCreateBuffer, clCreateBufferBadTest,
                        ::testing::ValuesIn(BadValues));

class clCreateBufferHostPtr : public ucl::CommandQueueTest {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
#ifndef CA_CL_ENABLE_OFFLINE_KERNEL_TESTS
    // This test requires offline kernels
    GTEST_SKIP();
#endif

    auto bin_src = getDeviceBinaryFromFile("clCreateBufferHostPtr");
    const unsigned char *src_data = bin_src.data();
    const size_t src_size = bin_src.size();
    cl_int errorcode;
    program = clCreateProgramWithBinary(context, 1, &device, &src_size,
                                        &src_data, nullptr, &errorcode);
    EXPECT_TRUE(program);
    ASSERT_SUCCESS(errorcode);

    kernel = clCreateKernel(program, "add_floats", &errorcode);
    EXPECT_TRUE(kernel);
    ASSERT_SUCCESS(errorcode);

    // Zero out buffers
    for (size_t i = 0; i < buf_sz; ++i) {
      storage_A[i] = (unsigned char)0;
      storage_B[i] = (unsigned char)0;
      storage_C[i] = (unsigned char)0;
    }
  }

  void TearDown() override {
    if (kernel) {
      EXPECT_SUCCESS(clReleaseKernel(kernel));
    }
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    CommandQueueTest::TearDown();
  }

  cl_int run_kernel(float *A, float *B, float *C, size_t elements) {
    const size_t arr_sz = elements * sizeof(cl_float);
    cl_int errorcode;

    cl_mem cl_buf_A = clCreateBuffer(
        context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, arr_sz, A, &errorcode);
    EXPECT_TRUE(cl_buf_A);
    UCL_SUCCESS_OR_RETURN_ERR(errorcode);

    cl_mem cl_buf_B = clCreateBuffer(
        context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, arr_sz, B, &errorcode);
    EXPECT_TRUE(cl_buf_B);
    UCL_SUCCESS_OR_RETURN_ERR(errorcode);

    cl_mem cl_buf_C =
        clCreateBuffer(context, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR, arr_sz,
                       C, &errorcode);
    EXPECT_TRUE(cl_buf_C);
    UCL_SUCCESS_OR_RETURN_ERR(errorcode);

    UCL_SUCCESS_OR_RETURN_ERR(clSetKernelArg(kernel, 0, sizeof(cl_buf_A),
                                             static_cast<void *>(&cl_buf_A)));
    UCL_SUCCESS_OR_RETURN_ERR(clSetKernelArg(kernel, 1, sizeof(cl_buf_B),
                                             static_cast<void *>(&cl_buf_B)));
    UCL_SUCCESS_OR_RETURN_ERR(clSetKernelArg(kernel, 2, sizeof(cl_buf_C),
                                             static_cast<void *>(&cl_buf_C)));

    UCL_SUCCESS_OR_RETURN_ERR(
        clEnqueueNDRangeKernel(command_queue, kernel, 1, nullptr, &elements,
                               nullptr, 0, nullptr, nullptr));

    UCL_SUCCESS_OR_RETURN_ERR(clEnqueueReadBuffer(
        command_queue, cl_buf_C, true, 0, arr_sz, C, 0, nullptr, nullptr));

    UCL_SUCCESS_OR_RETURN_ERR(clReleaseMemObject(cl_buf_A));
    UCL_SUCCESS_OR_RETURN_ERR(clReleaseMemObject(cl_buf_B));
    UCL_SUCCESS_OR_RETURN_ERR(clReleaseMemObject(cl_buf_C));

    return CL_SUCCESS;
  }

  cl_program program = nullptr;
  cl_kernel kernel = nullptr;

  // Three buffers
  static const size_t buf_sz = 1024;
  cl_uchar storage_A[buf_sz] = {};
  cl_uchar storage_B[buf_sz] = {};
  cl_uchar storage_C[buf_sz] = {};
  void *buf_A = storage_A;
  void *buf_B = storage_B;
  void *buf_C = storage_C;
};

// Use CL_MEM_USE_HOST_PTR buffers with at least 4-alignment
TEST_F(clCreateBufferHostPtr, Default) {
  const size_t elements = 128u;

  // Get buffers A, B, and C that are 4-aligned.
  // sz contains a return value, so it needs to be reset.
  size_t sz = buf_sz;
  cl_float *const A = reinterpret_cast<cl_float *>(
      std::align(4, elements * sizeof(cl_float), buf_A, sz));
  sz = buf_sz;
  cl_float *const B = reinterpret_cast<cl_float *>(
      std::align(4, elements * sizeof(cl_float), buf_B, sz));
  sz = buf_sz;
  cl_float *const C = reinterpret_cast<cl_float *>(
      std::align(4, elements * sizeof(cl_float), buf_C, sz));

  cl_float ref_C[elements];

  // Put values into inputs and reference output
  for (size_t i = 0; i < elements; ++i) {
    A[i] = (cl_float)i;
    B[i] = (cl_float)i;
    ref_C[i] = A[i] + B[i];
  }

  ASSERT_SUCCESS(run_kernel(A, B, C, elements));

  for (size_t i = 0; i < elements; ++i) {
    ASSERT_EQ(C[i], ref_C[i]);
  }
}

// Use CL_MEM_USE_HOST_PTR buffers that are exactly 4-aligned
TEST_F(clCreateBufferHostPtr, FourAligned) {
  const size_t elements = 128u;

  // Get buffers A, B, and C that are 16-aligned, then add 4.
  // sz contains a return value, so it needs to be reset.
  size_t sz = buf_sz;
  const uintptr_t ptr_A = reinterpret_cast<uintptr_t>(
      std::align(16, elements * sizeof(uintptr_t), buf_A, sz));
  sz = buf_sz;
  const uintptr_t ptr_B = reinterpret_cast<uintptr_t>(
      std::align(16, elements * sizeof(uintptr_t), buf_B, sz));
  sz = buf_sz;
  const uintptr_t ptr_C = reinterpret_cast<uintptr_t>(
      std::align(16, elements * sizeof(cl_float), buf_C, sz));

  ASSERT_NE(0, ptr_A) << "Failed to get 16-aligned buffer";
  ASSERT_NE(0, ptr_B) << "Failed to get 16-aligned buffer";
  ASSERT_NE(0, ptr_C) << "Failed to get 16-aligned buffer";

  cl_float *const A = reinterpret_cast<cl_float *>(ptr_A + 4);
  cl_float *const B = reinterpret_cast<cl_float *>(ptr_B + 4);
  cl_float *const C = reinterpret_cast<cl_float *>(ptr_C + 4);

  cl_float ref_C[elements];

  // Put values into inputs and reference output
  for (size_t i = 0; i < elements; ++i) {
    A[i] = (cl_float)i;
    B[i] = (cl_float)i;
    ref_C[i] = A[i] + B[i];
  }

  ASSERT_SUCCESS(run_kernel(A, B, C, elements));

  for (size_t i = 0; i < elements; ++i) {
    ASSERT_EQ(C[i], ref_C[i]);
  }
}
