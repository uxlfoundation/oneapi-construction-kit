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

#include <cstdio>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "Common.h"

class clCreateProgramWithBinaryTest : public ucl::ContextTest {
 protected:
  void SetUp() {
    UCL_RETURN_ON_FATAL_FAILURE(ContextTest::SetUp());
#ifndef CA_CL_ENABLE_OFFLINE_KERNEL_TESTS
    // This test requires offline kernels
    GTEST_SKIP();
#endif
    auto bin_src = getDeviceBinaryFromFile("clCreateProgramWithBinaryTest");
    const unsigned char *src_data = bin_src.data();
    const size_t src_size = bin_src.size();
    cl_int errcode;
    const cl_program originalProgram = clCreateProgramWithBinary(
        context, 1, &device, &src_size, &src_data, nullptr, &errcode);

    // Size in bytes
    const size_t binaryLengthsSize = 1 * sizeof(size_t);
    binaryLengths.resize(1);
    ASSERT_SUCCESS(clGetProgramInfo(originalProgram, CL_PROGRAM_BINARY_SIZES,
                                    binaryLengthsSize, binaryLengths.data(),
                                    nullptr));
    binaries = new const unsigned char *[1];
    for (unsigned i = 0; i < 1; ++i) {
      binaries[i] = new unsigned char[binaryLengths[i]];
    }
    ASSERT_SUCCESS(clGetProgramInfo(originalProgram, CL_PROGRAM_BINARIES,
                                    binaryLengthsSize,
                                    static_cast<void *>(binaries), nullptr));
    binaryStatii.resize(1);
    ASSERT_SUCCESS(clReleaseProgram(originalProgram));
  }

  void TearDown() {
    if (nullptr != binaries) {
      for (unsigned i = 0; i < 1; ++i) {
        delete[] binaries[i];
      }
      delete[] binaries;
    }
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    ContextTest::TearDown();
  }

  UCL::vector<size_t> binaryLengths;
  const unsigned char **binaries = {};
  UCL::vector<cl_int> binaryStatii;
  cl_program program = nullptr;
};

TEST_F(clCreateProgramWithBinaryTest, InvalidContext) {
  cl_int errcode = !CL_SUCCESS;
  program = clCreateProgramWithBinary(nullptr, 1, &device, binaryLengths.data(),
                                      binaries, binaryStatii.data(), &errcode);
  EXPECT_FALSE(program);
  ASSERT_EQ_ERRCODE(CL_INVALID_CONTEXT, errcode);
}

TEST_F(clCreateProgramWithBinaryTest, InvalidValueDevice) {
  cl_int errcode = !CL_SUCCESS;
  program = clCreateProgramWithBinary(context, 0, &device, binaryLengths.data(),
                                      binaries, binaryStatii.data(), &errcode);
  EXPECT_FALSE(program);
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, errcode);
  program = clCreateProgramWithBinary(context, 1, nullptr, binaryLengths.data(),
                                      binaries, binaryStatii.data(), &errcode);
  EXPECT_FALSE(program);
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, errcode);
}

TEST_F(clCreateProgramWithBinaryTest, InvalidValueLengths) {
  cl_int errcode = !CL_SUCCESS;
  program = clCreateProgramWithBinary(context, 1, &device, nullptr, binaries,
                                      binaryStatii.data(), &errcode);
  EXPECT_FALSE(program);
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, errcode);
  program = clCreateProgramWithBinary(context, 1, &device, binaryLengths.data(),
                                      nullptr, binaryStatii.data(), &errcode);
  EXPECT_FALSE(program);
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, errcode);
}

// Redmine #5134: Test invalid value of null length and binary values in lists,
// requires multiple devices

/* Redmine #5134: Test for invalid devices, this requires targeting multiple
 * devices at once.
TEST_F(clCreateProgramWithBinaryTest, InvalidDevice)
{
}
*/

TEST_F(clCreateProgramWithBinaryTest, InvalidBinary) {
  cl_int errcode = !CL_SUCCESS;
  const unsigned char **invalidbinaries =
      reinterpret_cast<const unsigned char **>(
          new char[sizeof(unsigned char *) * 1]);
  const size_t invalidBinLen = 64;
  for (unsigned i = 0; i < 1; ++i) {
    invalidbinaries[i] = new unsigned char[invalidBinLen];
    memset(const_cast<unsigned char **>(invalidbinaries)[i], 1, invalidBinLen);
  }
  program =
      clCreateProgramWithBinary(context, 1, &device, binaryLengths.data(),
                                invalidbinaries, binaryStatii.data(), &errcode);
  EXPECT_FALSE(program);
  EXPECT_EQ_ERRCODE(CL_INVALID_BINARY, errcode);
  for (unsigned i = 0; i < 1; ++i) {
    EXPECT_EQ_ERRCODE(CL_INVALID_BINARY, binaryStatii[i]);
    delete[] invalidbinaries[i];
  }
  delete[] invalidbinaries;
}

TEST_F(clCreateProgramWithBinaryTest, InvalidValueBinaryStatus) {
  cl_int errcode = !CL_SUCCESS;
  for (unsigned i = 0; i < 1; ++i) {
    binaryLengths[i] = 0;
  }
  program = clCreateProgramWithBinary(context, 1, &device, binaryLengths.data(),
                                      binaries, binaryStatii.data(), &errcode);
  EXPECT_FALSE(program);
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, errcode);
  for (unsigned i = 0; i < 1; ++i) {
    ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, binaryStatii[i]);
  }
}

/*! Redmine #5134: Test for additional binary_status values */

TEST_F(clCreateProgramWithBinaryTest, Default) {
  cl_int errcode = !CL_SUCCESS;
  program = clCreateProgramWithBinary(context, 1, &device, binaryLengths.data(),
                                      binaries, binaryStatii.data(), &errcode);
  EXPECT_TRUE(program);
  ASSERT_SUCCESS(errcode);
  for (unsigned i = 0; i < 1; ++i) {
    ASSERT_SUCCESS(binaryStatii[i]);
  }
}

TEST_F(clCreateProgramWithBinaryTest, GetBinaryType) {
  cl_int errcode = !CL_SUCCESS;
  program = clCreateProgramWithBinary(context, 1, &device, binaryLengths.data(),
                                      binaries, binaryStatii.data(), &errcode);
  EXPECT_TRUE(program);
  ASSERT_SUCCESS(errcode);

  cl_program_binary_type binary_type;
  ASSERT_SUCCESS(clGetProgramBuildInfo(program, device, CL_PROGRAM_BINARY_TYPE,
                                       sizeof(binary_type), &binary_type,
                                       nullptr));

  ASSERT_EQ(CL_PROGRAM_BINARY_TYPE_EXECUTABLE, binary_type);
}

TEST_F(clCreateProgramWithBinaryTest, DefaultNDRangeKernelWithoutBuild) {
  cl_int errcode = !CL_SUCCESS;
  program = clCreateProgramWithBinary(context, 1, &device, binaryLengths.data(),
                                      binaries, binaryStatii.data(), &errcode);
  EXPECT_TRUE(program);
  ASSERT_SUCCESS(errcode);

  // clBuildProgram() is permitted, but not required, when the binary is an
  // executable. See also `CreateProgramThenTryBuild`.
  cl_kernel kernel = clCreateKernel(program, "foo", &errcode);
  EXPECT_TRUE(kernel);
  ASSERT_SUCCESS(errcode);
  ASSERT_SUCCESS(clReleaseKernel(kernel));
}

TEST_F(clCreateProgramWithBinaryTest, DefaultNDRangeKernel) {
  cl_int errcode = !CL_SUCCESS;
  program = clCreateProgramWithBinary(context, 1, &device, binaryLengths.data(),
                                      binaries, binaryStatii.data(), &errcode);
  EXPECT_TRUE(program);
  ASSERT_SUCCESS(errcode);

  const size_t elements = 64;
  const size_t buflen = elements * sizeof(cl_int);

  cl_mem bufin =
      clCreateBuffer(context, CL_MEM_READ_ONLY, buflen, nullptr, &errcode);
  EXPECT_TRUE(bufin);
  ASSERT_SUCCESS(errcode);

  cl_mem bufout =
      clCreateBuffer(context, CL_MEM_WRITE_ONLY, buflen, nullptr, &errcode);
  EXPECT_TRUE(bufout);
  ASSERT_SUCCESS(errcode);

  cl_kernel kernel = clCreateKernel(program, "foo", &errcode);
  EXPECT_TRUE(kernel);
  ASSERT_SUCCESS(errcode);

  ASSERT_SUCCESS(
      clSetKernelArg(kernel, 0, sizeof(cl_mem), static_cast<void *>(&bufin)));
  ASSERT_SUCCESS(
      clSetKernelArg(kernel, 1, sizeof(cl_mem), static_cast<void *>(&bufout)));

  cl_command_queue command_q =
      clCreateCommandQueue(context, device, 0, &errcode);
  EXPECT_TRUE(command_q);
  ASSERT_SUCCESS(errcode);

  UCL::vector<cl_int> inData(elements);
  for (cl_int i = 0; i < static_cast<cl_int>(elements); ++i) {
    inData[i] = i;
  }

  ASSERT_SUCCESS(clEnqueueWriteBuffer(command_q, bufin, CL_TRUE, 0, buflen,
                                      inData.data(), 0, nullptr, nullptr));
  cl_event ndRangeEvent;
  ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_q, kernel, 1, nullptr,
                                        &elements, nullptr, 0, nullptr,
                                        &ndRangeEvent));
  cl_event readEvent;
  UCL::vector<cl_int> outData(elements);
  ASSERT_SUCCESS(clEnqueueReadBuffer(command_q, bufout, CL_TRUE, 0, buflen,
                                     outData.data(), 1, &ndRangeEvent,
                                     &readEvent));
  ASSERT_SUCCESS(clWaitForEvents(1, &readEvent));
  for (size_t i = 0; i < elements; ++i) {
    EXPECT_EQ(inData[i], outData[i]);
  }

  ASSERT_SUCCESS(clReleaseEvent(readEvent));
  ASSERT_SUCCESS(clReleaseEvent(ndRangeEvent));
  ASSERT_SUCCESS(clReleaseCommandQueue(command_q));
  ASSERT_SUCCESS(clReleaseKernel(kernel));
  ASSERT_SUCCESS(clReleaseMemObject(bufout));
  ASSERT_SUCCESS(clReleaseMemObject(bufin));
}

// The same as DefaultNDRangeKernel, but with many threads.
TEST_F(clCreateProgramWithBinaryTest, ConcurrentNDRangeKernel) {
  auto worker = [&]() {
    UCL::vector<cl_int> status;
    status.resize(1);

    cl_int errcode = !CL_SUCCESS;

    cl_program program =
        clCreateProgramWithBinary(context, 1, &device, binaryLengths.data(),
                                  binaries, status.data(), &errcode);
    EXPECT_TRUE(program);
    ASSERT_SUCCESS(errcode);

    const size_t elements = 64;
    const size_t buflen = elements * sizeof(cl_int);

    cl_mem bufin =
        clCreateBuffer(context, CL_MEM_READ_ONLY, buflen, nullptr, &errcode);
    EXPECT_TRUE(bufin);
    ASSERT_SUCCESS(errcode);

    cl_mem bufout =
        clCreateBuffer(context, CL_MEM_WRITE_ONLY, buflen, nullptr, &errcode);
    EXPECT_TRUE(bufout);
    ASSERT_SUCCESS(errcode);

    cl_kernel kernel = clCreateKernel(program, "foo", &errcode);
    EXPECT_TRUE(kernel);
    ASSERT_SUCCESS(errcode);

    ASSERT_SUCCESS(
        clSetKernelArg(kernel, 0, sizeof(cl_mem), static_cast<void *>(&bufin)));
    ASSERT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(cl_mem),
                                  static_cast<void *>(&bufout)));

    cl_command_queue command_q =
        clCreateCommandQueue(context, device, 0, &errcode);
    EXPECT_TRUE(command_q);
    ASSERT_SUCCESS(errcode);

    UCL::vector<cl_int> inData(elements);
    for (cl_int i = 0; i < static_cast<cl_int>(elements); ++i) {
      inData[i] = i;
    }

    ASSERT_SUCCESS(clEnqueueWriteBuffer(command_q, bufin, CL_TRUE, 0, buflen,
                                        inData.data(), 0, nullptr, nullptr));
    cl_event ndRangeEvent;
    ASSERT_SUCCESS(clEnqueueNDRangeKernel(command_q, kernel, 1, nullptr,
                                          &elements, nullptr, 0, nullptr,
                                          &ndRangeEvent));
    cl_event readEvent;
    UCL::vector<cl_int> outData(elements);
    ASSERT_SUCCESS(clEnqueueReadBuffer(command_q, bufout, CL_TRUE, 0, buflen,
                                       outData.data(), 1, &ndRangeEvent,
                                       &readEvent));
    ASSERT_SUCCESS(clWaitForEvents(1, &readEvent));
    for (size_t i = 0; i < elements; ++i) {
      EXPECT_EQ(inData[i], outData[i]);
    }

    ASSERT_SUCCESS(clReleaseEvent(readEvent));
    ASSERT_SUCCESS(clReleaseEvent(ndRangeEvent));
    ASSERT_SUCCESS(clReleaseCommandQueue(command_q));
    ASSERT_SUCCESS(clReleaseKernel(kernel));
    ASSERT_SUCCESS(clReleaseMemObject(bufout));
    ASSERT_SUCCESS(clReleaseMemObject(bufin));
    ASSERT_SUCCESS(clReleaseProgram(program));
  };

  // Ideally there would be 10+ threads as that is much more reliable for
  // detecting issues, but greatly slows down the test.  Even at 10 threads
  // issues will sometimes trigger, so issues will be caught sooner or later.
  // If the thread sanitizer is enabled then 2 is conceivably enough. The
  // non-LLVM code path used by this test is so light, that more threads are
  // needed here than in other *Concurrent* tests to find races.
  const size_t threads = 10;
  UCL::vector<std::thread> workers(threads);

  for (size_t i = 0; i < threads; i++) {
    workers[i] = std::thread(worker);
  }

  for (size_t i = 0; i < threads; i++) {
    workers[i].join();
  }
}

TEST_F(clCreateProgramWithBinaryTest, CreateProgramThenTryCompile) {
  cl_int errcode = !CL_SUCCESS;
  program = clCreateProgramWithBinary(context, 1, &device, binaryLengths.data(),
                                      binaries, binaryStatii.data(), &errcode);
  EXPECT_TRUE(program);
  ASSERT_SUCCESS(errcode);

  const size_t num_devices = 1;
  cl_device_id *devices = &device;

  for (size_t i = 0; i < num_devices; ++i) {
    ASSERT_SUCCESS(binaryStatii[i]);
  }

  for (size_t i = 0; i < num_devices; ++i) {
    cl_device_id device = devices[i];

    errcode = clCompileProgram(program, 1, &device, nullptr, 0, nullptr,
                               nullptr, nullptr, nullptr);
    if (UCL::hasCompilerSupport(device)) {
      ASSERT_EQ_ERRCODE(CL_INVALID_OPERATION, errcode);
    } else {
      ASSERT_EQ_ERRCODE(CL_COMPILER_NOT_AVAILABLE, errcode);
    }
  }
}

TEST_F(clCreateProgramWithBinaryTest, CreateProgramThenTryBuild) {
  cl_int errcode = !CL_SUCCESS;
  program = clCreateProgramWithBinary(context, 1, &device, binaryLengths.data(),
                                      binaries, binaryStatii.data(), &errcode);
  EXPECT_TRUE(program);
  ASSERT_SUCCESS(errcode);

  for (unsigned i = 0; i < 1; ++i) {
    ASSERT_SUCCESS(binaryStatii[i]);
  }

  ASSERT_SUCCESS(
      clBuildProgram(program, 1, &device, nullptr, nullptr, nullptr));
}

// Redmine #5134: test binary of library, which should fail when build program
// called on it

#if defined(CL_VERSION_3_0)
TEST_F(clCreateProgramWithBinaryTest, IL) {
  // Skip for non OpenCL-3.0 implementations.
  if (!UCL::isDeviceVersionAtLeast({3, 0})) {
    GTEST_SKIP();
  }

  cl_int errcode;
  program = clCreateProgramWithBinary(context, 1, &device, binaryLengths.data(),
                                      binaries, binaryStatii.data(), &errcode);
  ASSERT_SUCCESS(errcode);
  ASSERT_NE(program, nullptr);

  // Query for size of value.
  size_t size{};
  ASSERT_SUCCESS(clGetProgramInfo(program, CL_PROGRAM_IL, 0, nullptr, &size));

  //  If program is created with clCreateProgramWithSource,
  //  clCreateProgramWithBinary or clCreateProgramWithBuiltInKernels the memory
  //  pointed to by param_value will be unchanged and param_value_size_ret will
  //  be set to 0.
  ASSERT_EQ(size, 0);
  UCL::Buffer<char> param_val{1};
  param_val[0] = 42;
  ASSERT_SUCCESS(clGetProgramInfo(program, CL_PROGRAM_IL, param_val.size(),
                                  param_val.data(), nullptr));
  ASSERT_EQ(param_val[0], 42);
}
#endif
