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

#include "cl_khr_command_buffer_mutable_dispatch.h"

struct MutableCommandInfoTest : MutableDispatchTest {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(MutableDispatchTest::SetUp());

    cl_int error = !CL_SUCCESS;
    cl_command_buffer_properties_khr properties[3] = {
        CL_COMMAND_BUFFER_FLAGS_KHR, CL_COMMAND_BUFFER_MUTABLE_KHR, 0};
    command_buffer =
        clCreateCommandBufferKHR(1, &command_queue, properties, &error);
    EXPECT_SUCCESS(error);

    const char *kernel_source = R"(
        void kernel nop_kernel() {
        }
        )";
    const size_t kernel_source_length = std::strlen(kernel_source);
    program = clCreateProgramWithSource(context, 1, &kernel_source,
                                        &kernel_source_length, &error);
    EXPECT_SUCCESS(clBuildProgram(program, 1, &device, nullptr,
                                  ucl::buildLogCallback, nullptr));
    kernel = clCreateKernel(program, "nop_kernel", &error);
    EXPECT_SUCCESS(error);

    // Enqueue a mutable dispatch to the command buffer.
    mutable_properties = {CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
                          CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};
    EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
        command_buffer, nullptr, mutable_properties.data(), kernel, 1, nullptr,
        &global_size, nullptr, 0, nullptr, nullptr, &command_handle));
  }

  void TearDown() override {
    if (nullptr != command_buffer) {
      EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
    }
    if (nullptr != kernel) {
      EXPECT_SUCCESS(clReleaseKernel(kernel));
    }

    if (nullptr != program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }

    MutableDispatchTest::TearDown();
  }

  cl_mutable_command_khr command_handle = nullptr;
  cl_command_buffer_khr command_buffer = nullptr;
  cl_program program = nullptr;
  cl_kernel kernel = nullptr;

  std::array<cl_ndrange_kernel_command_properties_khr, 3> mutable_properties;
  static constexpr size_t global_size = 8;
};

TEST_F(MutableCommandInfoTest, InvalidCommandBuffer) {
  ASSERT_EQ_ERRCODE(
      CL_INVALID_MUTABLE_COMMAND_KHR,
      clGetMutableCommandInfoKHR(nullptr, 0, 0, nullptr, nullptr));
}

TEST_F(MutableCommandInfoTest, InvalidParamName) {
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clGetMutableCommandInfoKHR(command_handle, CL_SUCCESS, 0,
                                               nullptr, nullptr));
}

TEST_F(MutableCommandInfoTest, ReturnBufferSizeTooSmall) {
  size_t param_value = 0;
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clGetMutableCommandInfoKHR(
                        command_handle, CL_MUTABLE_COMMAND_COMMAND_BUFFER_KHR,
                        1, &param_value, nullptr));
}

TEST_F(MutableCommandInfoTest, MutableCommandCommandQueue) {
  size_t size;
  ASSERT_SUCCESS(clGetMutableCommandInfoKHR(
      command_handle, CL_MUTABLE_COMMAND_COMMAND_QUEUE_KHR, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_command_queue), size);
  cl_command_queue command_handle_queue = nullptr;
  ASSERT_SUCCESS(clGetMutableCommandInfoKHR(
      command_handle, CL_MUTABLE_COMMAND_COMMAND_QUEUE_KHR, size,
      static_cast<void *>(&command_handle_queue), nullptr));
  ASSERT_EQ(command_queue, command_handle_queue);
}

TEST_F(MutableCommandInfoTest, MutableCommandCommandBuffer) {
  size_t size;
  ASSERT_SUCCESS(clGetMutableCommandInfoKHR(
      command_handle, CL_MUTABLE_COMMAND_COMMAND_BUFFER_KHR, 0, nullptr,
      &size));
  ASSERT_EQ(sizeof(cl_command_buffer_khr), size);
  cl_command_buffer_khr command_handle_buffer = nullptr;
  ASSERT_SUCCESS(clGetMutableCommandInfoKHR(
      command_handle, CL_MUTABLE_COMMAND_COMMAND_BUFFER_KHR, size,
      static_cast<void *>(&command_handle_buffer), nullptr));
  ASSERT_EQ(command_buffer, command_handle_buffer);
}

TEST_F(MutableCommandInfoTest, PropertiesSet) {
  size_t size;
  ASSERT_SUCCESS(clGetMutableCommandInfoKHR(
      command_handle, CL_MUTABLE_DISPATCH_PROPERTIES_ARRAY_KHR, 0, nullptr,
      &size));
  ASSERT_EQ(size, sizeof(cl_ndrange_kernel_command_properties_khr) *
                      mutable_properties.size());

  std::array<cl_ndrange_kernel_command_properties_khr, 3> queried_properties;
  ASSERT_SUCCESS(clGetMutableCommandInfoKHR(
      command_handle, CL_MUTABLE_DISPATCH_PROPERTIES_ARRAY_KHR, size,
      queried_properties.data(), nullptr));
  ASSERT_EQ(mutable_properties, queried_properties);
}

TEST_F(MutableCommandInfoTest, NoPropertiesSet) {
  cl_mutable_command_khr no_properties_command_handle = nullptr;
  EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, nullptr, kernel, 1, nullptr, &global_size,
      nullptr, 0, nullptr, nullptr, &no_properties_command_handle));

  size_t size;
  EXPECT_SUCCESS(clGetMutableCommandInfoKHR(
      no_properties_command_handle, CL_MUTABLE_DISPATCH_PROPERTIES_ARRAY_KHR, 0,
      nullptr, &size));
  EXPECT_EQ(size, 0);
}

TEST_F(MutableCommandInfoTest, MutableDispatchKernel) {
  size_t size;
  ASSERT_SUCCESS(clGetMutableCommandInfoKHR(
      command_handle, CL_MUTABLE_DISPATCH_KERNEL_KHR, 0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_kernel), size);
  cl_kernel command_handle_kernel = nullptr;
  ASSERT_SUCCESS(clGetMutableCommandInfoKHR(
      command_handle, CL_MUTABLE_DISPATCH_KERNEL_KHR, size,
      static_cast<void *>(&command_handle_kernel), nullptr));
  ASSERT_EQ(kernel, command_handle_kernel);
}

struct NDimMutableCommandInfoTest : MutableCommandInfoTest,
                                    testing::WithParamInterface<cl_uint> {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(MutableCommandInfoTest::SetUp());

    work_dim = GetParam();
    if (work_dim > getDeviceMaxWorkItemDimensions()) {
      // Device does not support this many dimensions
      GTEST_SKIP();
    }
    std::vector<size_t> global_size_array(work_dim, global_size);

    // Enqueue a mutable dispatch to the command buffer.
    EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
        command_buffer, nullptr, mutable_properties.data(), kernel, work_dim,
        nullptr, global_size_array.data(), nullptr, 0, nullptr, nullptr,
        &ndim_command_handle));
  }

  cl_mutable_command_khr ndim_command_handle = nullptr;
  cl_uint work_dim;
};

TEST_P(NDimMutableCommandInfoTest, MutableDispatchDimensions) {
  size_t size;
  ASSERT_SUCCESS(clGetMutableCommandInfoKHR(ndim_command_handle,
                                            CL_MUTABLE_DISPATCH_DIMENSIONS_KHR,
                                            0, nullptr, &size));
  ASSERT_EQ(sizeof(cl_uint), size);
  cl_uint command_handle_work_dim = 0;
  ASSERT_SUCCESS(clGetMutableCommandInfoKHR(
      ndim_command_handle, CL_MUTABLE_DISPATCH_DIMENSIONS_KHR, size,
      &command_handle_work_dim, nullptr));
  ASSERT_EQ(work_dim, command_handle_work_dim);
}

TEST_P(NDimMutableCommandInfoTest, MutableDispatchGlobalOffsetDefault) {
  size_t size;
  ASSERT_SUCCESS(clGetMutableCommandInfoKHR(
      ndim_command_handle, CL_MUTABLE_DISPATCH_GLOBAL_WORK_OFFSET_KHR, 0,
      nullptr, &size));
  ASSERT_EQ(sizeof(size_t) * work_dim, size);
  std::vector<size_t> command_handle_global_offset(work_dim);
  ASSERT_SUCCESS(clGetMutableCommandInfoKHR(
      ndim_command_handle, CL_MUTABLE_DISPATCH_GLOBAL_WORK_OFFSET_KHR, size,
      command_handle_global_offset.data(), nullptr));
  const std::vector<size_t> reference_global_offset(work_dim, 0);
  ASSERT_EQ(reference_global_offset, command_handle_global_offset);
}

TEST_P(NDimMutableCommandInfoTest, MutableDispatchGlobalOffsetSet) {
  std::vector<size_t> global_size_array(work_dim, global_size);
  std::vector<size_t> global_offset_array(work_dim, 1u);

  cl_mutable_command_khr new_command_handle = nullptr;
  ASSERT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, nullptr, kernel, work_dim,
      global_offset_array.data(), global_size_array.data(), nullptr, 0, nullptr,
      nullptr, &new_command_handle));

  size_t size;
  EXPECT_SUCCESS(clGetMutableCommandInfoKHR(
      new_command_handle, CL_MUTABLE_DISPATCH_GLOBAL_WORK_OFFSET_KHR, 0,
      nullptr, &size));
  EXPECT_EQ(sizeof(size_t) * work_dim, size);
  std::vector<size_t> command_handle_global_offset(work_dim);
  EXPECT_SUCCESS(clGetMutableCommandInfoKHR(
      new_command_handle, CL_MUTABLE_DISPATCH_GLOBAL_WORK_OFFSET_KHR, size,
      command_handle_global_offset.data(), nullptr));
  EXPECT_EQ(global_offset_array, command_handle_global_offset);
}

TEST_P(NDimMutableCommandInfoTest, MutableDispatchGlobalSize) {
  size_t size;
  ASSERT_SUCCESS(clGetMutableCommandInfoKHR(
      ndim_command_handle, CL_MUTABLE_DISPATCH_GLOBAL_WORK_SIZE_KHR, 0, nullptr,
      &size));
  ASSERT_EQ(sizeof(size_t) * work_dim, size);
  std::vector<size_t> command_handle_global_size(work_dim);
  ASSERT_SUCCESS(clGetMutableCommandInfoKHR(
      ndim_command_handle, CL_MUTABLE_DISPATCH_GLOBAL_WORK_SIZE_KHR, size,
      command_handle_global_size.data(), nullptr));
  const std::vector<size_t> reference_global_size(work_dim, global_size);
  ASSERT_EQ(reference_global_size, command_handle_global_size);
}

TEST_P(NDimMutableCommandInfoTest, MutableDispatchLocalSizeImplicitNoFinalize) {
  size_t size;
  ASSERT_SUCCESS(clGetMutableCommandInfoKHR(
      ndim_command_handle, CL_MUTABLE_DISPATCH_LOCAL_WORK_SIZE_KHR, 0, nullptr,
      &size));
  ASSERT_EQ(sizeof(size_t) * work_dim, size);
  std::vector<size_t> command_handle_local_size(work_dim);
  ASSERT_SUCCESS(clGetMutableCommandInfoKHR(
      ndim_command_handle, CL_MUTABLE_DISPATCH_LOCAL_WORK_SIZE_KHR, size,
      command_handle_local_size.data(), nullptr));
  const std::vector<size_t> reference_local_size(work_dim, 0);
  ASSERT_EQ(reference_local_size, command_handle_local_size);
}

TEST_P(NDimMutableCommandInfoTest, MutableDispatchLocalSizeImplicitFinalized) {
  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  size_t size;
  ASSERT_SUCCESS(clGetMutableCommandInfoKHR(
      ndim_command_handle, CL_MUTABLE_DISPATCH_LOCAL_WORK_SIZE_KHR, 0, nullptr,
      &size));
  ASSERT_EQ(sizeof(size_t) * work_dim, size);
  std::vector<size_t> command_handle_local_size(work_dim);
  ASSERT_SUCCESS(clGetMutableCommandInfoKHR(
      ndim_command_handle, CL_MUTABLE_DISPATCH_LOCAL_WORK_SIZE_KHR, size,
      command_handle_local_size.data(), nullptr));
  const std::vector<size_t> reference_local_size(work_dim, 0);
  ASSERT_EQ(reference_local_size, command_handle_local_size);
}

TEST_P(NDimMutableCommandInfoTest, MutableDispatchLocalSizeExplicit) {
  std::vector<size_t> global_size_array(work_dim, global_size);
  const cl_uint local_size = global_size / 4;
  std::vector<size_t> local_size_array(work_dim, local_size);

  cl_mutable_command_khr new_command_handle = nullptr;
  ASSERT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, nullptr, kernel, work_dim, nullptr,
      global_size_array.data(), local_size_array.data(), 0, nullptr, nullptr,
      &new_command_handle));

  size_t size;
  EXPECT_SUCCESS(clGetMutableCommandInfoKHR(
      new_command_handle, CL_MUTABLE_DISPATCH_LOCAL_WORK_SIZE_KHR, 0, nullptr,
      &size));
  EXPECT_EQ(sizeof(size_t) * work_dim, size);
  std::vector<size_t> command_handle_local_size(work_dim);
  EXPECT_SUCCESS(clGetMutableCommandInfoKHR(
      new_command_handle, CL_MUTABLE_DISPATCH_LOCAL_WORK_SIZE_KHR, size,
      command_handle_local_size.data(), nullptr));
  const std::vector<size_t> reference_local_size(work_dim, local_size);
  EXPECT_EQ(reference_local_size, command_handle_local_size);
}

static cl_uint work_dims[3] = {1, 2, 3};
INSTANTIATE_TEST_CASE_P(MutableCommandInfoTest, NDimMutableCommandInfoTest,
                        ::testing::ValuesIn(work_dims),
                        [](const ::testing::TestParamInfo<
                            NDimMutableCommandInfoTest::ParamType> &info) {
                          ::std::stringstream ss;
                          ss << info.param;
                          ss << "D";
                          return ss.str();
                        });
