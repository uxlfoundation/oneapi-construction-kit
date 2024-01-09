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
#include "cl_intel_unified_shared_memory.h"
#include "cl_khr_command_buffer_mutable_dispatch.h"

// Test fixture for checking command-buffer behaviour for commands with USM
// kernel arguments. For this we require a device to support both the
// USM and mutable-dispatch (so we can test updating arguments) extensions.
class MutableDispatchUSMTest : public MutableDispatchTest,
                               public cl_intel_unified_shared_memory_Test {
 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(MutableDispatchTest::SetUp());
    UCL_RETURN_ON_FATAL_FAILURE(cl_intel_unified_shared_memory_Test::SetUp());

    cl_int error = CL_SUCCESS;
    out_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, data_size_in_bytes,
                                nullptr, &error);
    ASSERT_SUCCESS(error);

    const cl_int zero = 0x0;
    ASSERT_SUCCESS(clEnqueueFillBuffer(command_queue, out_buffer, &zero,
                                       sizeof(cl_int), 0, data_size_in_bytes, 0,
                                       nullptr, nullptr));
    ASSERT_SUCCESS(clFinish(command_queue));

    // Create command-buffer with mutable flag so we can update it
    cl_command_buffer_properties_khr properties[3] = {
        CL_COMMAND_BUFFER_FLAGS_KHR, CL_COMMAND_BUFFER_MUTABLE_KHR, 0};
    command_buffer =
        clCreateCommandBufferKHR(1, &command_queue, properties, &error);
    ASSERT_SUCCESS(error);

    if (host_capabilities) {
      for (auto &host_ptr : host_ptrs) {
        host_ptr = clHostMemAllocINTEL(context, nullptr, data_size_in_bytes,
                                       sizeof(cl_int), &error);
        ASSERT_SUCCESS(error);
        ASSERT_NE(host_ptr, nullptr);
      }
    }

    for (auto &device_ptr : device_ptrs) {
      device_ptr = clDeviceMemAllocINTEL(
          context, device, nullptr, data_size_in_bytes, sizeof(cl_int), &error);
      ASSERT_SUCCESS(error);
      ASSERT_NE(device_ptr, nullptr);
    }

    const char *kernel_source = R"(
void kernel usm_copy(__global int* in,
                __global int* out) {
   size_t id = get_global_id(0);
   out[id] = in[id];
}
)";

    const size_t kernel_source_length = std::strlen(kernel_source);
    program = clCreateProgramWithSource(context, 1, &kernel_source,
                                        &kernel_source_length, &error);
    ASSERT_SUCCESS(clBuildProgram(program, 1, &device, nullptr,
                                  ucl::buildLogCallback, nullptr));
    kernel = clCreateKernel(program, "usm_copy", &error);
    ASSERT_SUCCESS(error);

    ASSERT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(cl_mem), &out_buffer));
  }

  void TearDown() override {
    if (nullptr != command_buffer) {
      EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
    }

    if (nullptr != out_buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(out_buffer));
    }

    for (auto ptr : device_ptrs) {
      if (ptr) {
        EXPECT_SUCCESS(clMemBlockingFreeINTEL(context, ptr));
      }
    }

    for (auto ptr : host_ptrs) {
      if (ptr) {
        EXPECT_SUCCESS(clMemBlockingFreeINTEL(context, ptr));
      }
    }

    if (nullptr != kernel) {
      EXPECT_SUCCESS(clReleaseKernel(kernel));
    }

    if (nullptr != program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }

    MutableDispatchTest::TearDown();
    cl_intel_unified_shared_memory_Test::TearDown();
  }

  cl_mutable_command_khr command_handle = nullptr;
  cl_command_buffer_khr command_buffer = nullptr;
  cl_mem out_buffer = nullptr;
  std::array<void *, 2> host_ptrs = {nullptr, nullptr};
  std::array<void *, 2> device_ptrs = {nullptr, nullptr};
  cl_program program = nullptr;
  cl_kernel kernel = nullptr;

  constexpr static size_t global_size = 256;
  constexpr static size_t data_size_in_bytes = global_size * sizeof(cl_int);
};

// Return CL_INVALID_VALUE if arg_svm_list is NULL and num_svm_args > 0,
// or arg_svm_list is not NULL and num_svm_args is 0.
TEST_F(MutableDispatchUSMTest, InvalidArgList) {
  ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 0, device_ptrs[0]));

  cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
      CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
      CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};
  ASSERT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, mutable_properties, kernel, 1, nullptr,
      &global_size, nullptr, 0, nullptr, nullptr, &command_handle));

  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  cl_mutable_dispatch_config_khr dispatch_config{
      CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR,
      nullptr,
      command_handle,
      0,
      1 /* num_svm_args */,
      0,
      0,
      nullptr,
      nullptr /* arg_svm_list */,
      nullptr,
      nullptr,
      nullptr,
      nullptr};
  cl_mutable_base_config_khr mutable_config{
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};

  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, clUpdateMutableCommandsKHR(
                                          command_buffer, &mutable_config));

  cl_mutable_dispatch_arg_khr arg{0, 0, device_ptrs[1]};
  dispatch_config = {CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR,
                     nullptr,
                     command_handle,
                     0,
                     0 /* num_svm_args */,
                     0,
                     0,
                     nullptr,
                     &arg /* arg_svm_list */,
                     nullptr,
                     nullptr,
                     nullptr,
                     nullptr};

  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, clUpdateMutableCommandsKHR(
                                          command_buffer, &mutable_config));
}

// Test clSetKernelMemPointerINTEL error code for CL_INVALID_ARG_INDEX if
// arg_index is not a valid argument index.
TEST_F(MutableDispatchUSMTest, InvalidArgIndex) {
  ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 0, device_ptrs[0]));

  cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
      CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
      CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};
  ASSERT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, mutable_properties, kernel, 1, nullptr,
      &global_size, nullptr, 0, nullptr, nullptr, &command_handle));

  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  cl_mutable_dispatch_arg_khr arg{2 /* arg index */, 0, device_ptrs[1]};
  cl_mutable_dispatch_config_khr dispatch_config{
      CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR,
      nullptr,
      command_handle,
      0,
      1 /* num_svm_args */,
      0,
      0,
      nullptr,
      &arg /* arg_svm_list */,
      nullptr,
      nullptr,
      nullptr,
      nullptr};
  cl_mutable_base_config_khr mutable_config{
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};

  ASSERT_EQ_ERRCODE(CL_INVALID_ARG_INDEX, clUpdateMutableCommandsKHR(
                                              command_buffer, &mutable_config));
}

TEST_F(MutableDispatchUSMTest, InvalidArgValue) {
  ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 0, device_ptrs[0]));

  cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
      CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
      CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};
  ASSERT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, mutable_properties, kernel, 1, nullptr,
      &global_size, nullptr, 0, nullptr, nullptr, &command_handle));

  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  cl_mutable_dispatch_arg_khr arg{0, 0, &out_buffer};
  cl_mutable_dispatch_config_khr dispatch_config{
      CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR,
      nullptr,
      command_handle,
      0,
      1 /* num_svm_args */,
      0,
      0,
      nullptr,
      &arg /* arg_svm_list */,
      nullptr,
      nullptr,
      nullptr,
      nullptr};
  cl_mutable_base_config_khr mutable_config{
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};

  // The interaction between cl_intel_unified_shared_memory and
  // cl_khr_command_buffer_mutable_dispatch is not specified but we assume that
  // if clSetKernelArgMemPointerINTEL would not report invalid values, neither
  // will clUpdateMutableCommandsKHR.
  ASSERT_SUCCESS(clUpdateMutableCommandsKHR(command_buffer, &mutable_config));
}

// Tests for updating USM arguments to a command-buffer kernel command are
// parametrized on a pair on bool values. The first pair item is for the
// original USM kernel arg, and the second for the USM pointer the argument is
// updated to. A bool is true if the USM pointer is a device USM pointer and
// false if it's a host USM pointer. Shared USM pointers are not represented,
// i.e not tested.
using usm_pointers = std::tuple<bool, bool>;
struct MutableDispatchUpdateUSMArgs
    : public MutableDispatchUSMTest,
      public ::testing::WithParamInterface<usm_pointers> {
  static constexpr std::array<bool, 2> usm_update_pair = {true, false};
};

// Test that a USM pointer argument to the kernel (pointer A) can be updated to
// another USM pointer (pointer B).
TEST_P(MutableDispatchUpdateUSMArgs, NoOffset) {
  const bool A_is_device_usm_ptr = std::get<0>(GetParam());
  const bool B_is_device_usm_ptr = std::get<1>(GetParam());

  if (!host_capabilities && (!A_is_device_usm_ptr || !B_is_device_usm_ptr)) {
    GTEST_SKIP();
  }

  void *ptrA, *ptrB;
  if (A_is_device_usm_ptr) {
    ptrA = device_ptrs[0];
    ptrB = B_is_device_usm_ptr ? device_ptrs[1] : host_ptrs[0];
  } else {
    ptrA = host_ptrs[0];
    ptrB = B_is_device_usm_ptr ? device_ptrs[0] : host_ptrs[1];
  }

  ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 0, ptrA));

  std::vector<cl_int> inputA(global_size), inputB(global_size),
      output(global_size);
  ucl::Environment::instance->GetInputGenerator().GenerateIntData(inputA);
  ucl::Environment::instance->GetInputGenerator().GenerateIntData(inputB);

  cl_int error =
      clEnqueueMemcpyINTEL(command_queue, CL_TRUE, ptrA, inputA.data(),
                           data_size_in_bytes, 0, nullptr, nullptr);
  ASSERT_SUCCESS(error);
  error = clEnqueueMemcpyINTEL(command_queue, CL_TRUE, ptrB, inputB.data(),
                               data_size_in_bytes, 0, nullptr, nullptr);
  ASSERT_SUCCESS(error);

  // Record a mutable dispatch to the command buffer.
  cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
      CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
      CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};
  ASSERT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, mutable_properties, kernel, 1, nullptr,
      &global_size, nullptr, 0, nullptr, nullptr, &command_handle));

  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Enqueue the command buffer.
  ASSERT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results.
  ASSERT_SUCCESS(clEnqueueReadBuffer(command_queue, out_buffer, CL_FALSE, 0,
                                     data_size_in_bytes, output.data(), 0,
                                     nullptr, nullptr));
  ASSERT_SUCCESS(clFinish(command_queue));
  ASSERT_EQ(inputA, output);

  // Update both the input and argument to second device USM allocation
  cl_mutable_dispatch_arg_khr arg{0, 0, ptrB};
  cl_mutable_dispatch_config_khr dispatch_config{
      CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR,
      nullptr,
      command_handle,
      0,
      1, /* num_svm_args */
      0,
      0,
      nullptr,
      &arg, /* arg_svm_list */
      nullptr,
      nullptr,
      nullptr,
      nullptr};
  cl_mutable_base_config_khr mutable_config{
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};
  ASSERT_SUCCESS(clUpdateMutableCommandsKHR(command_buffer, &mutable_config));

  // Enqueue the command buffer.
  ASSERT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results.
  ASSERT_SUCCESS(clEnqueueReadBuffer(command_queue, out_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output.data(), 0,
                                     nullptr, nullptr));

  ASSERT_EQ(inputB, output);
}

// Test that a pointer to a offset into a USM allocation can be set as an
// argument to the kernel (pointer A), and then updated to a pointer to an
// offset into another USM allocation.
TEST_P(MutableDispatchUpdateUSMArgs, Offset) {
  const bool A_is_device_usm_ptr = std::get<0>(GetParam());
  const bool B_is_device_usm_ptr = std::get<1>(GetParam());
  if (!host_capabilities && (!A_is_device_usm_ptr || !B_is_device_usm_ptr)) {
    GTEST_SKIP();
  }

  void *ptrA, *ptrB;
  if (A_is_device_usm_ptr) {
    ptrA = device_ptrs[0];
    ptrB = B_is_device_usm_ptr ? device_ptrs[1] : host_ptrs[0];
  } else {
    ptrA = host_ptrs[0];
    ptrB = B_is_device_usm_ptr ? device_ptrs[0] : host_ptrs[1];
  }
  const size_t halfway = data_size_in_bytes / 2;
  void *offset_ptrA = getPointerOffset(ptrA, halfway);
  void *offset_ptrB = getPointerOffset(ptrB, halfway);

  ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 0, offset_ptrA));

  const size_t workitems = global_size / 2;
  std::vector<cl_int> inputA(workitems), inputB(workitems), output(workitems);
  ucl::Environment::instance->GetInputGenerator().GenerateIntData(inputA);
  ucl::Environment::instance->GetInputGenerator().GenerateIntData(inputB);

  cl_int error =
      clEnqueueMemcpyINTEL(command_queue, CL_TRUE, offset_ptrA, inputA.data(),
                           halfway, 0, nullptr, nullptr);
  ASSERT_SUCCESS(error);
  error = clEnqueueMemcpyINTEL(command_queue, CL_TRUE, offset_ptrB,
                               inputB.data(), halfway, 0, nullptr, nullptr);
  ASSERT_SUCCESS(error);

  // Record a mutable dispatch to the command buffer.
  cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
      CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
      CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};
  ASSERT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, mutable_properties, kernel, 1, nullptr,
      &workitems, nullptr, 0, nullptr, nullptr, &command_handle));

  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Enqueue the command buffer.
  ASSERT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results.
  ASSERT_SUCCESS(clEnqueueReadBuffer(command_queue, out_buffer, CL_FALSE, 0,
                                     halfway, output.data(), 0, nullptr,
                                     nullptr));
  ASSERT_SUCCESS(clFinish(command_queue));
  ASSERT_EQ(inputA, output);

  // Update both the input and argument to second device USM allocation
  cl_mutable_dispatch_arg_khr arg{0, 0, offset_ptrB};
  cl_mutable_dispatch_config_khr dispatch_config{
      CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR,
      nullptr,
      command_handle,
      0,
      1, /* num_svm_args */
      0,
      0,
      nullptr,
      &arg, /* arg_svm_list */
      nullptr,
      nullptr,
      nullptr,
      nullptr};
  cl_mutable_base_config_khr mutable_config{
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};
  ASSERT_SUCCESS(clUpdateMutableCommandsKHR(command_buffer, &mutable_config));

  // Enqueue the command buffer.
  ASSERT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results.
  ASSERT_SUCCESS(clEnqueueReadBuffer(command_queue, out_buffer, CL_TRUE, 0,
                                     halfway, output.data(), 0, nullptr,
                                     nullptr));

  ASSERT_EQ(inputB, output);
}

// Tests the following cases:
// * Update device pointer to device pointer
// * Update device pointer to host pointer
// * Update host pointer to device pointer
// * Update host pointer to host pointer
INSTANTIATE_TEST_SUITE_P(
    MutableDispatchUSMTest, MutableDispatchUpdateUSMArgs,
    ::testing::Combine(
        ::testing::ValuesIn(MutableDispatchUpdateUSMArgs::usm_update_pair),
        ::testing::ValuesIn(MutableDispatchUpdateUSMArgs::usm_update_pair)));

// Test that USM blocking free works when a USM allocation is used as a kernel
// argument to a command recorded to a command-buffer
// TODO CA-4308 - USM allocations in command buffer kernels are not tracked yet
TEST_F(MutableDispatchUSMTest, DISABLED_BlockingFree) {
  std::vector<cl_int> input(global_size), output(global_size);
  ucl::Environment::instance->GetInputGenerator().GenerateIntData(input);

  void *usm_ptr = device_ptrs[0];
  cl_int error =
      clEnqueueMemcpyINTEL(command_queue, CL_TRUE, usm_ptr, input.data(),
                           data_size_in_bytes, 0, nullptr, nullptr);
  ASSERT_SUCCESS(error);

  ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 0, usm_ptr));

  // Record a mutable dispatch to the command buffer.
  cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
      CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
      CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};
  ASSERT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, mutable_properties, kernel, 1, nullptr,
      &global_size, nullptr, 0, nullptr, nullptr, &command_handle));

  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Enqueue the command buffer.
  ASSERT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Block until all operations are complete, implicitly flushing all queues
  ASSERT_SUCCESS(clMemBlockingFreeINTEL(context, usm_ptr));

  // Check the results.
  ASSERT_SUCCESS(clEnqueueReadBuffer(command_queue, out_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output.data(), 0,
                                     nullptr, nullptr));
  ASSERT_EQ(input, output);
}

// Test that USM blocking free works when a USM allocation is used as a kernel
// argument which is part of an update.
// TODO CA-4308 - USM allocations in command buffer kernels are not tracked yet
TEST_F(MutableDispatchUSMTest, DISABLED_UpdateBlockingFree) {
  void *usm_ptrA = device_ptrs[0];
  void *usm_ptrB = device_ptrs[1];

  ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 0, usm_ptrA));

  std::vector<cl_int> inputA(global_size), inputB(global_size),
      output(global_size);
  ucl::Environment::instance->GetInputGenerator().GenerateIntData(inputA);
  ucl::Environment::instance->GetInputGenerator().GenerateIntData(inputB);

  cl_int error =
      clEnqueueMemcpyINTEL(command_queue, CL_TRUE, usm_ptrA, inputA.data(),
                           data_size_in_bytes, 0, nullptr, nullptr);
  ASSERT_SUCCESS(error);
  error = clEnqueueMemcpyINTEL(command_queue, CL_TRUE, usm_ptrB, inputB.data(),
                               data_size_in_bytes, 0, nullptr, nullptr);
  ASSERT_SUCCESS(error);

  // Record a mutable dispatch to the command buffer.
  cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
      CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
      CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};
  ASSERT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, mutable_properties, kernel, 1, nullptr,
      &global_size, nullptr, 0, nullptr, nullptr, &command_handle));

  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Enqueue the command buffer.
  ASSERT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results.
  ASSERT_SUCCESS(clEnqueueReadBuffer(command_queue, out_buffer, CL_FALSE, 0,
                                     data_size_in_bytes, output.data(), 0,
                                     nullptr, nullptr));
  ASSERT_SUCCESS(clFinish(command_queue));
  ASSERT_EQ(inputA, output);

  // Update both the input and argument to second device USM allocation
  cl_mutable_dispatch_arg_khr arg{0, 0, usm_ptrB};
  cl_mutable_dispatch_config_khr dispatch_config{
      CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR,
      nullptr,
      command_handle,
      0,
      1, /* num_svm_args */
      0,
      0,
      nullptr,
      &arg, /* arg_svm_list */
      nullptr,
      nullptr,
      nullptr,
      nullptr};
  cl_mutable_base_config_khr mutable_config{
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};
  ASSERT_SUCCESS(clUpdateMutableCommandsKHR(command_buffer, &mutable_config));

  // Enqueue the command buffer.
  ASSERT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Block until all operations are complete, implicitly flushing all queues
  ASSERT_SUCCESS(clMemBlockingFreeINTEL(context, usm_ptrB));

  // Check the results.
  ASSERT_SUCCESS(clEnqueueReadBuffer(command_queue, out_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output.data(), 0,
                                     nullptr, nullptr));

  ASSERT_EQ(inputB, output);
}
