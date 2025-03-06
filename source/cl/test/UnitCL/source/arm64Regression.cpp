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

// This test recreates a hardware failure seen on a 'APM X-Gene Mustang board'
// 64-bit ARM device running Ubuntu 14.04. Reduced from OpenCL CTS test
// 'conformance_test_basic async_copy_global_to_local'. The failure caused the
// CPU voltage to drop to zero, needing the machine to be restarted.

#include <cstdio>
#include <random>

#include "Common.h"

namespace {
void generateRandomData(void *data, size_t count, std::mt19937 &generator) {
  size_t i;
  // We can cause the failure without the mt19937 generator calls in this
  // function, using hardcoded values instead. However this requires more
  // test iterations to fail.
  cl_uint bits = generator();
  cl_uint bitsLeft = 32;

  cl_char *charPtr = (cl_char *)data;
  for (i = 0; i < count; i++) {
    if (0 == bitsLeft) {
      bits = generator();
      bitsLeft = 32;
    }
    charPtr[i] = (cl_char)((cl_int)(bits & 255) - 127);
    bits >>= 8;
    bitsLeft -= 8;
  }
}

int test_copy(cl_context context, cl_command_queue queue, cl_kernel kernel,
              size_t local_size) {
  int ret_code = 0;
  cl_mem buffers[2];
  std::mt19937 generator(42 /* seed */);

  // Setup workgroup dimensions
  const size_t num_wg = 1111u;
  const size_t global_size = local_size * num_wg;
  const cl_int copies_per_wi = 13;
  const cl_int copies_per_wg = copies_per_wi * (cl_int)local_size;
  const size_t local_buffer_size = copies_per_wg * sizeof(cl_char) * 8;
  const size_t global_buffer_size = local_buffer_size * num_wg;
  cl_int error = 0;

  // Fill input buffer with random data
  void *inData = malloc(global_buffer_size);
  generateRandomData(inData, global_buffer_size, generator);

  buffers[0] = clCreateBuffer(context, CL_MEM_COPY_HOST_PTR, global_buffer_size,
                              inData, &error);
  EXPECT_SUCCESS(error);

  void *outData = malloc(global_buffer_size);
  buffers[1] = clCreateBuffer(context, CL_MEM_COPY_HOST_PTR, global_buffer_size,
                              outData, &error);
  EXPECT_SUCCESS(error);

  error = clSetKernelArg(kernel, 0, sizeof(buffers[0]),
                         static_cast<void *>(&buffers[0]));
  EXPECT_SUCCESS(error);
  error = clSetKernelArg(kernel, 1, sizeof(buffers[1]),
                         static_cast<void *>(&buffers[1]));
  EXPECT_SUCCESS(error);
  error = clSetKernelArg(kernel, 2, local_buffer_size, NULL);
  EXPECT_SUCCESS(error);
  error = clSetKernelArg(kernel, 3, sizeof(copies_per_wg), &copies_per_wg);
  EXPECT_SUCCESS(error);
  error = clSetKernelArg(kernel, 4, sizeof(copies_per_wi), &copies_per_wi);
  EXPECT_SUCCESS(error);

  size_t threads[1], localThreads[1];
  threads[0] = global_size;
  localThreads[0] = local_size;
  error = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, threads, localThreads,
                                 0, NULL, NULL);
  EXPECT_SUCCESS(error);

  error = clEnqueueReadBuffer(queue, buffers[1], CL_TRUE, 0, global_buffer_size,
                              outData, 0, NULL, NULL);
  EXPECT_SUCCESS(error);

  if (memcmp(inData, outData, global_buffer_size) != 0) {
    (void)fprintf(stderr, "Error: Output is incorrect\n");
    ret_code = 1;
  }

  free(inData);
  free(outData);
  clReleaseMemObject(buffers[0]);
  clReleaseMemObject(buffers[1]);
  return ret_code;
}
}  // end anonymous namespace

struct Arm64KillerTest : public ucl::CommandQueueTest {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    if (!getDeviceCompilerAvailable()) {
      GTEST_SKIP();
    }
    const char *source =
        "__kernel void arm64_killer(const __global char8 *src,\n"
        "                           __global char8 *dst,\n"
        "                           __local char8 *localBuffer,\n"
        "                           int copiesPerWorkgroup,\n"
        "                           int copiesPerWorkItem) {\n"
        "  event_t event;\n"
        "  event = async_work_group_copy(\n"
        "      localBuffer,\n"
        "      src + copiesPerWorkgroup * get_group_id(0),\n"
        "      (size_t)copiesPerWorkgroup, 0);\n"
        "  wait_group_events(1, &event);\n"
        "\n"
        "  for (int i = 0; i < copiesPerWorkItem; i++) {\n"
        "    dst[get_global_id(0) * copiesPerWorkItem + i] =\n"
        "        localBuffer[get_local_id(0) * copiesPerWorkItem + i];\n"
        "  }\n"
        "}\n";
    cl_int errorcode;
    program =
        clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
    EXPECT_TRUE(program);
    EXPECT_SUCCESS(errorcode);
    ASSERT_SUCCESS(
        clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));

    kernel = clCreateKernel(program, "arm64_killer", &errorcode);
    EXPECT_TRUE(kernel);
    EXPECT_SUCCESS(errorcode);

    errorcode =
        clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_ITEM_SIZES,
                        sizeof(max_workgroup_size), &max_workgroup_size, NULL);
    EXPECT_SUCCESS(errorcode);
    EXPECT_GE(max_workgroup_size[0], 1u);
  }

  void TearDown() override {
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    if (kernel) {
      EXPECT_SUCCESS(clReleaseKernel(kernel));
    }
    CommandQueueTest::TearDown();
  }

  cl_program program = nullptr;
  cl_kernel kernel = nullptr;
  size_t max_workgroup_size[3] = {};
};

TEST_F(Arm64KillerTest, Default) {
  int errors = 0;

  const size_t local_size = std::min((size_t)157, max_workgroup_size[0]);
  // Need several iterations for issue to occur
  for (int i = 0; i < 10; i++) {
    errors += test_copy(context, command_queue, kernel, local_size);
  }
  EXPECT_TRUE(errors == 0);
}
