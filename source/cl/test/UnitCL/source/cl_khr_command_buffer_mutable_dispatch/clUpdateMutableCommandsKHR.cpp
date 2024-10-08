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

// Test fixture for checking clUpdateMutableCommandsKHR() behaviour. In order
// to do this we create a command-buffer to update, as well as cl_mem objects
// and a kernel to set as parameters when recording operations.
class CommandBufferUpdateNDKernel : public MutableDispatchTest {
 protected:
  CommandBufferUpdateNDKernel()
      : input_data(global_size, 42), output_data(global_size, 0) {};

  virtual void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(MutableDispatchTest::SetUp());

    cl_int error = CL_SUCCESS;
    src_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, data_size_in_bytes,
                                nullptr, &error);
    ASSERT_SUCCESS(error);

    ucl::Environment::instance->GetInputGenerator().GenerateIntData(input_data);
    ASSERT_SUCCESS(clEnqueueWriteBuffer(command_queue, src_buffer, CL_TRUE, 0,
                                        data_size_in_bytes, input_data.data(),
                                        0, nullptr, nullptr));

    dst_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, data_size_in_bytes,
                                nullptr, &error);
    ASSERT_SUCCESS(error);

    const cl_int zero = 0x0;
    ASSERT_SUCCESS(clEnqueueFillBuffer(command_queue, dst_buffer, &zero,
                                       sizeof(cl_int), 0, data_size_in_bytes, 0,
                                       nullptr, nullptr));
    ASSERT_SUCCESS(clFinish(command_queue));

    // Create command-buffer with mutable flag so we can update it
    cl_command_buffer_properties_khr properties[3] = {
        CL_COMMAND_BUFFER_FLAGS_KHR, CL_COMMAND_BUFFER_MUTABLE_KHR, 0};
    command_buffer =
        clCreateCommandBufferKHR(1, &command_queue, properties, &error);
    ASSERT_SUCCESS(error);

    const char *kernel_source = R"(
        void kernel parallel_copy(global int *src, global int *dst) {
          size_t gid = get_global_id(0);
          dst[gid] = src[gid];
          }
        )";
    const size_t kernel_source_length = std::strlen(kernel_source);
    program = clCreateProgramWithSource(context, 1, &kernel_source,
                                        &kernel_source_length, &error);
    ASSERT_SUCCESS(clBuildProgram(program, 1, &device, nullptr,
                                  ucl::buildLogCallback, nullptr));
    kernel = clCreateKernel(program, "parallel_copy", &error);
    ASSERT_SUCCESS(error);

    ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem),
                                  static_cast<void *>(&src_buffer)));
    ASSERT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(cl_mem),
                                  static_cast<void *>(&dst_buffer)));
  }

  virtual void TearDown() override {
    if (nullptr != command_buffer) {
      EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
    }

    if (nullptr != src_buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(src_buffer));
    }

    if (nullptr != dst_buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(dst_buffer));
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
  cl_mem src_buffer = nullptr;
  cl_mem dst_buffer = nullptr;
  cl_program program = nullptr;
  cl_kernel kernel = nullptr;
  std::vector<cl_int> input_data;
  std::vector<cl_int> output_data;

  static constexpr size_t global_size = 256;
  static constexpr size_t data_size_in_bytes = global_size * sizeof(cl_int);
};

// Return CL_INVALID_COMMAND_BUFFER_KHR if command_buffer is not a valid
// command-buffer.
TEST_F(CommandBufferUpdateNDKernel, NullCommandBuffer) {
  const cl_mutable_base_config_khr mutable_config{
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 0, nullptr};
  ASSERT_EQ_ERRCODE(CL_INVALID_COMMAND_BUFFER_KHR,
                    clUpdateMutableCommandsKHR(nullptr /* command_buffer */,
                                               &mutable_config));
}

// Return CL_INVALID_OPERATION if command_buffer has not been finalized.
TEST_F(CommandBufferUpdateNDKernel, InvalidCommandBuffer) {
  const cl_mutable_base_config_khr mutable_config{
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 0, nullptr};
  ASSERT_EQ_ERRCODE(CL_INVALID_OPERATION, clUpdateMutableCommandsKHR(
                                              command_buffer, &mutable_config));
}

// Return CL_INVALID_OPERATION if command_buffer was not created with the
// CL_COMMAND_BUFFER_MUTABLE_KHR flag and the user tries to update it
TEST_F(CommandBufferUpdateNDKernel, InvalidMutableFlag) {
  cl_int error = !CL_SUCCESS;
  cl_command_buffer_khr immutable_command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
  ASSERT_SUCCESS(error);

  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(immutable_command_buffer));

  const cl_mutable_dispatch_config_khr dispatch_config{
      CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR,
      nullptr,
      nullptr,
      0,
      0,
      0,
      0,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr};
  const cl_mutable_base_config_khr mutable_config = {
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 0, &dispatch_config};
  EXPECT_EQ_ERRCODE(
      CL_INVALID_OPERATION,
      clUpdateMutableCommandsKHR(immutable_command_buffer, &mutable_config));
  EXPECT_SUCCESS(clReleaseCommandBufferKHR(immutable_command_buffer));
}

// Return CL_INVALID_OPERATION if command_buffer was not created with the
// CL_COMMAND_BUFFER_SIMULTANEOUS_USE_KHR flag and is in the pending state.
TEST_F(CommandBufferUpdateNDKernel, InvalidSimulataneousUse) {
  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  cl_int error = !CL_SUCCESS;
  cl_event user_event = clCreateUserEvent(context, &error);
  EXPECT_SUCCESS(error);
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 1,
                                           &user_event, nullptr));

  const cl_mutable_dispatch_config_khr dispatch_config{
      CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR,
      nullptr,
      nullptr,
      0,
      0,
      0,
      0,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr};
  const cl_mutable_base_config_khr mutable_config = {
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 0, &dispatch_config};
  EXPECT_EQ_ERRCODE(CL_INVALID_OPERATION, clUpdateMutableCommandsKHR(
                                              command_buffer, &mutable_config));

  // We need to complete the user event to avoid any possible hangs.
  ASSERT_SUCCESS(clSetUserEventStatus(user_event, CL_COMPLETE));

  EXPECT_SUCCESS(clReleaseEvent(user_event));
}

// Return CL_INVALID_VALUE if the type member of mutable_config is not
// CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR.
TEST_F(CommandBufferUpdateNDKernel, InvalidBaseConfigType) {
  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));
  const cl_mutable_base_config_khr mutable_config{
      0xBAD /* type should be CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR */,
      nullptr, 0, nullptr};
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, clUpdateMutableCommandsKHR(
                                          command_buffer, &mutable_config));
}

// Return CL_INVALID_VALUE if the mutable_dispatch_list member of mutable_config
// is NULL and num_mutable_dispatch > 0, or mutable_dispatch_list is not NULL
// and num_mutable_dispatch is 0.
TEST_F(CommandBufferUpdateNDKernel, InvalidMutableDispatchList) {
  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  cl_mutable_base_config_khr mutable_config{
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr,
      1 /* num_mutable_dispatch */, nullptr /* mutable_dispatch_list */
  };

  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, clUpdateMutableCommandsKHR(
                                          command_buffer, &mutable_config));

  const cl_mutable_dispatch_config_khr dispatch_config{
      CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR,
      nullptr,
      nullptr,
      0,
      0,
      0,
      0,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr};

  mutable_config = {
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr,
      0 /* num_mutable_dispatch */, &dispatch_config /* mutable_dispatch_list*/
  };
  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, clUpdateMutableCommandsKHR(
                                          command_buffer, &mutable_config));
}

// Return CL_INVALID_VALUE if the next member of mutable_config is not NULL and
// any iteration of the structure pointer chain does not contain valid type and
// next members.
TEST_F(CommandBufferUpdateNDKernel, InvalidNext) {
  ASSERT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, nullptr, kernel, 1, nullptr, &global_size,
      nullptr, 0, nullptr, nullptr, nullptr));
  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  const cl_ulong next = 0xDEADBEEF;
  const cl_mutable_base_config_khr dispatch_config{
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, &next /* next is invalid */, 0,
      nullptr};
  ASSERT_EQ(CL_INVALID_VALUE,
            clUpdateMutableCommandsKHR(command_buffer, &dispatch_config));
}

// Return CL_INVALID_VALUE if mutable_config is NULL
TEST_F(CommandBufferUpdateNDKernel, NullUpdate) {
  ASSERT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, nullptr, kernel, 1, nullptr, &global_size,
      nullptr, 0, nullptr, nullptr, nullptr));
  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  ASSERT_EQ(CL_INVALID_VALUE, clUpdateMutableCommandsKHR(
                                  command_buffer, nullptr /* mutable_config*/));
}

// return CL_INVALID_VALUE if ... both next and mutable_dispatch_list members of
// mutable_config are NULL.
TEST_F(CommandBufferUpdateNDKernel, NopUpdate) {
  ASSERT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, nullptr, kernel, 1, nullptr, &global_size,
      nullptr, 0, nullptr, nullptr, nullptr));
  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  const cl_mutable_base_config_khr nop_update{
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr /* next */, 0,
      nullptr /* mutable_dispatch_list */};
  ASSERT_EQ(CL_INVALID_VALUE,
            clUpdateMutableCommandsKHR(command_buffer, &nop_update));
}

// Return CL_INVALID_MUTABLE_COMMAND_KHR if command is not a valid mutable
// command object
TEST_F(CommandBufferUpdateNDKernel, NullHandle) {
  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));
  const cl_mutable_dispatch_config_khr dispatch_config{
      CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR,
      nullptr,
      nullptr /* command */,
      0,
      0,
      0,
      0,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr};
  const cl_mutable_base_config_khr mutable_config{
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};

  ASSERT_EQ_ERRCODE(
      CL_INVALID_MUTABLE_COMMAND_KHR,
      clUpdateMutableCommandsKHR(command_buffer, &mutable_config));
}

// Return CL_INVALID_MUTABLE_COMMAND_KHR if command is not created from
// command_buffer
TEST_F(CommandBufferUpdateNDKernel, InvalidHandle) {
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));
  cl_int error = !CL_SUCCESS;
  cl_command_buffer_khr new_command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
  EXPECT_SUCCESS(error);

  cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
      CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
      CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};
  cl_mutable_command_khr new_command_handle;
  EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
      new_command_buffer, nullptr, mutable_properties, kernel, 1, nullptr,
      &global_size, nullptr, 0, nullptr, nullptr, &new_command_handle));
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(new_command_buffer));

  const cl_mutable_dispatch_arg_khr arg{1, sizeof(cl_mem),
                                        static_cast<void *>(&dst_buffer)};
  const cl_mutable_dispatch_config_khr dispatch_config{
      CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR,
      nullptr,
      new_command_handle /* command */,
      1,
      0,
      0,
      0,
      &arg,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr};
  const cl_mutable_base_config_khr mutable_config{
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};

  EXPECT_EQ_ERRCODE(
      CL_INVALID_MUTABLE_COMMAND_KHR,
      clUpdateMutableCommandsKHR(command_buffer, &mutable_config));
  EXPECT_SUCCESS(clReleaseCommandBufferKHR(new_command_buffer));
}

// Return CL_INVALID_OPERATION if a property is set that the device doesn't
// support
TEST_F(CommandBufferUpdateNDKernel, UnsupportedPropertyBit) {
  // Check device doesn't support every capability
  const cl_mutable_dispatch_fields_khr all_mutable_capabilities =
      CL_MUTABLE_DISPATCH_GLOBAL_OFFSET_KHR |
      CL_MUTABLE_DISPATCH_GLOBAL_SIZE_KHR | CL_MUTABLE_DISPATCH_LOCAL_SIZE_KHR |
      CL_MUTABLE_DISPATCH_ARGUMENTS_KHR | CL_MUTABLE_DISPATCH_EXEC_INFO_KHR;

  cl_mutable_dispatch_fields_khr device_capabilities;
  const cl_int err = clGetDeviceInfo(
      device, CL_DEVICE_MUTABLE_DISPATCH_CAPABILITIES_KHR,
      sizeof(cl_mutable_dispatch_fields_khr), &device_capabilities, nullptr);
  ASSERT_SUCCESS(err);

  // Skip if device supports all the mutable capabilities, as we can't check
  // the error reported for any being unsupported
  if (all_mutable_capabilities == device_capabilities) {
    GTEST_SKIP();
  }

  cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
      CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR, all_mutable_capabilities, 0};
  ASSERT_EQ_ERRCODE(
      CL_INVALID_OPERATION,
      clCommandNDRangeKernelKHR(command_buffer, nullptr, mutable_properties,
                                kernel, 1, nullptr, &global_size, nullptr, 0,
                                nullptr, nullptr, &command_handle));
}

// Return CL_INVALID_VALUE if a property is set that isn't defined
TEST_F(CommandBufferUpdateNDKernel, InvalidPropertyBit) {
  // CL_MUTABLE_DISPATCH_EXEC_INFO_KHR  is max representatble value
  cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
      CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
      CL_MUTABLE_DISPATCH_EXEC_INFO_KHR << 1, 0};
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clCommandNDRangeKernelKHR(command_buffer, nullptr, mutable_properties,
                                kernel, 1, nullptr, &global_size, nullptr, 0,
                                nullptr, nullptr, &command_handle));
}

// Return CL_INVALID_VALUE if cl_mutable_dispatch_config_khr type is not
// CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR
TEST_F(CommandBufferUpdateNDKernel, InvalidDispatchConfigStuctType) {
  cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
      CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
      CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};
  ASSERT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, mutable_properties, kernel, 1, nullptr,
      &global_size, nullptr, 0, nullptr, nullptr, &command_handle));

  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  const cl_mutable_dispatch_config_khr dispatch_config{
      0 /* This field should be CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR*/,
      nullptr,
      command_handle,
      0,
      0,
      0,
      0,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr};
  const cl_mutable_base_config_khr mutable_config{
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};

  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, clUpdateMutableCommandsKHR(
                                          command_buffer, &mutable_config));
}

// Return CL_INVALID_VALUE if a bad property was set
TEST_F(CommandBufferUpdateNDKernel, InvalidCommandProperty) {
  cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
      1 /* Invalid property */, CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clCommandNDRangeKernelKHR(command_buffer, nullptr, mutable_properties,
                                kernel, 1, nullptr, &global_size, nullptr, 0,
                                nullptr, nullptr, &command_handle));
}

// Return CL_INVALID_OPERATION if the CL_MUTABLE_DISPATCH_ARGUMENTS_KHR
// property was not set on command recording and num_args is non-zero.
TEST_F(CommandBufferUpdateNDKernel, ImmutablePropertyBit) {
  cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
      CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
      0 /* should have CL_MUTABLE_DISPATCH_ARGUMENTS_KHR*/, 0};
  ASSERT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, mutable_properties, kernel, 1, nullptr,
      &global_size, nullptr, 0, nullptr, nullptr, &command_handle));
  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  const cl_mutable_dispatch_arg_khr arg{1, sizeof(cl_mem),
                                        static_cast<void *>(&dst_buffer)};
  const cl_mutable_dispatch_config_khr dispatch_config{
      CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR,
      nullptr,
      command_handle,
      1,
      0,
      0,
      0,
      &arg,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr};

  const cl_mutable_base_config_khr mutable_config{
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};
  ASSERT_EQ_ERRCODE(CL_INVALID_OPERATION, clUpdateMutableCommandsKHR(
                                              command_buffer, &mutable_config));
}

// Return CL_INVALID_VALUE if arg_list is NULL and num_args > 0,
// or arg_list is not NULL and num_args is 0.
TEST_F(CommandBufferUpdateNDKernel, InvalidArgList) {
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
      1 /* num_args */,
      0,
      0,
      0,
      nullptr /* arg_list */,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr};
  const cl_mutable_base_config_khr mutable_config{
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};

  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, clUpdateMutableCommandsKHR(
                                          command_buffer, &mutable_config));

  const cl_mutable_dispatch_arg_khr arg{1, sizeof(cl_mem),
                                        static_cast<void *>(&dst_buffer)};
  dispatch_config = {CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR,
                     nullptr,
                     command_handle,
                     0 /* num_args */,
                     0,
                     0,
                     0,
                     &arg /* arg_list*/,
                     nullptr,
                     nullptr,
                     nullptr,
                     nullptr,
                     nullptr};

  ASSERT_EQ_ERRCODE(CL_INVALID_VALUE, clUpdateMutableCommandsKHR(
                                          command_buffer, &mutable_config));
}

// Test clSetKernelArg error code for CL_INVALID_ARG_INDEX if arg_index is not
// a valid argument index.
TEST_F(CommandBufferUpdateNDKernel, InvalidArgIndex) {
  cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
      CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
      CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};
  ASSERT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, mutable_properties, kernel, 1, nullptr,
      &global_size, nullptr, 0, nullptr, nullptr, &command_handle));

  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));
  const cl_mutable_dispatch_arg_khr arg = {3 /* arg_index */, sizeof(cl_mem),
                                           static_cast<void *>(&src_buffer)};
  const cl_mutable_dispatch_config_khr dispatch_config{
      CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR,
      nullptr,
      command_handle,
      1,
      0,
      0,
      0,
      &arg,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr};

  const cl_mutable_base_config_khr mutable_config{
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};
  ASSERT_EQ_ERRCODE(CL_INVALID_ARG_INDEX, clUpdateMutableCommandsKHR(
                                              command_buffer, &mutable_config));
}

// Test clSetKernelArg error code for CL_INVALID_ARG_VALUE if arg_value
// specified is not a valid value.
TEST_F(CommandBufferUpdateNDKernel, InvalidArgValue) {
  // Tests assume images are supported by the device
  if (!UCL::hasImageSupport(device)) {
    GTEST_SKIP();
  }

  const cl_image_format image_format = {CL_RGBA, CL_SIGNED_INT32};
  const cl_mem_object_type image_type = CL_MEM_OBJECT_IMAGE1D;
  const cl_mem_flags image_flags = CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR;
  if (!UCL::isImageFormatSupported(context, {image_flags}, image_type,
                                   image_format)) {
    GTEST_SKIP();
  }

  const size_t data_size = 16;
  std::vector<cl_uint4> data(16);
  // 1D image
  cl_image_desc image_desc;
  image_desc.image_type = image_type;
  image_desc.image_width = data_size;
  image_desc.image_height = 0;
  image_desc.image_depth = 0;
  image_desc.image_array_size = 1;
  image_desc.image_row_pitch = 0;
  image_desc.image_slice_pitch = 0;
  image_desc.num_mip_levels = 0;
  image_desc.num_samples = 0;
  image_desc.buffer = nullptr;

  cl_int err = !CL_SUCCESS;
  cl_mem image = clCreateImage(context, image_flags, &image_format, &image_desc,
                               data.data(), &err);
  EXPECT_SUCCESS(err);

  cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
      CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
      CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};
  EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, mutable_properties, kernel, 1, nullptr,
      &global_size, nullptr, 0, nullptr, nullptr, &command_handle));

  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));
  const cl_mutable_dispatch_arg_khr arg = {
      0, sizeof(cl_mem), static_cast<void *>(&image) /* arg_value */};
  const cl_mutable_dispatch_config_khr dispatch_config{
      CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR,
      nullptr,
      command_handle,
      1,
      0,
      0,
      0,
      &arg,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr};

  const cl_mutable_base_config_khr mutable_config{
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};
  EXPECT_EQ_ERRCODE(CL_INVALID_ARG_VALUE, clUpdateMutableCommandsKHR(
                                              command_buffer, &mutable_config));

  EXPECT_SUCCESS(clReleaseMemObject(image));
}

// Test clSetKernelArg error code for CL_INVALID_ARG_SIZE if arg_size does not
// match the size of the data type for an argument
TEST_F(CommandBufferUpdateNDKernel, InvalidArgSize) {
  cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
      CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
      CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};
  ASSERT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, mutable_properties, kernel, 1, nullptr,
      &global_size, nullptr, 0, nullptr, nullptr, &command_handle));

  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));
  const cl_mutable_dispatch_arg_khr arg = {0, 2 /* arg_size */,
                                           static_cast<void *>(&src_buffer)};
  const cl_mutable_dispatch_config_khr dispatch_config{
      CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR,
      nullptr,
      command_handle,
      1,
      0,
      0,
      0,
      &arg,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr};

  const cl_mutable_base_config_khr mutable_config{
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};
  ASSERT_EQ_ERRCODE(CL_INVALID_ARG_SIZE, clUpdateMutableCommandsKHR(
                                             command_buffer, &mutable_config));
}

// Test update being called multiple times on the same command before an
// enqueue, but with different arguments. Reusing the same
// cl_mutable_dispatch_arg_khr struct to verify this works rather than creating
// an new instance for each update.
TEST_F(CommandBufferUpdateNDKernel, IterativeArgumentUpdate) {
  // Create a new input & output buffers to update to.
  cl_int error = CL_SUCCESS;
  cl_mem updated_src_buffer = clCreateBuffer(
      context, CL_MEM_WRITE_ONLY, data_size_in_bytes, nullptr, &error);
  EXPECT_SUCCESS(error);

  cl_mem updated_dst_buffer = clCreateBuffer(
      context, CL_MEM_READ_ONLY, data_size_in_bytes, nullptr, &error);
  EXPECT_SUCCESS(error);

  std::vector<cl_int> updated_input_data(global_size);
  ucl::Environment::instance->GetInputGenerator().GenerateData(
      updated_input_data);
  EXPECT_SUCCESS(clEnqueueWriteBuffer(
      command_queue, updated_src_buffer, CL_TRUE, 0, data_size_in_bytes,
      updated_input_data.data(), 0, nullptr, nullptr));

  // Record a mutable dispatch to the command buffer we'll change the arguments
  // to
  cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
      CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
      CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};
  EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, mutable_properties, kernel, 1, nullptr,
      &global_size, nullptr, 0, nullptr, nullptr, &command_handle));

  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Do an explicit flush (see CA-3358).
  EXPECT_SUCCESS(clFinish(command_queue));

  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));

  EXPECT_EQ(input_data, output_data);

  // Update both the input and output buffer, reusing the same
  // cl_mutable_dispatch_arg_khr struct
  cl_mutable_dispatch_arg_khr arg{0, sizeof(cl_mem),
                                  static_cast<void *>(&updated_src_buffer)};
  const cl_mutable_dispatch_config_khr dispatch_config{
      CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR,
      nullptr,
      command_handle,
      1,
      0,
      0,
      0,
      &arg,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr};
  const cl_mutable_base_config_khr mutable_config{
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};
  EXPECT_SUCCESS(clUpdateMutableCommandsKHR(command_buffer, &mutable_config));

  // Reuse argument update struct for output buffer
  arg.arg_index = 1;
  arg.arg_value = static_cast<void *>(&updated_dst_buffer);
  EXPECT_SUCCESS(clUpdateMutableCommandsKHR(command_buffer, &mutable_config));

  // Enqueue the command buffer again
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Do an explicit flush (see CA-3358).
  EXPECT_SUCCESS(clFinish(command_queue));

  // Check that we were able to successfully update the buffers.
  std::vector<cl_int> updated_output_data(global_size);
  EXPECT_SUCCESS(clEnqueueReadBuffer(
      command_queue, updated_dst_buffer, CL_TRUE, 0, data_size_in_bytes,
      updated_output_data.data(), 0, nullptr, nullptr));

  EXPECT_EQ(updated_input_data, updated_output_data);

  // Cleanup.
  EXPECT_SUCCESS(clReleaseMemObject(updated_src_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(updated_dst_buffer));
}

// Test that updating a command-buffer multiple times, overwriting the same
// kernel argument on each occasion, correctly sets the argument to the final
// value.
TEST_F(CommandBufferUpdateNDKernel, OverwriteArgumentUpdate) {
  // Create a new input & output buffers to update to.
  cl_int error = CL_SUCCESS;
  cl_mem unused_dst_buffer = clCreateBuffer(
      context, CL_MEM_READ_WRITE, data_size_in_bytes, nullptr, &error);
  EXPECT_SUCCESS(error);

  cl_mem updated_dst_buffer = clCreateBuffer(
      context, CL_MEM_READ_ONLY, data_size_in_bytes, nullptr, &error);
  EXPECT_SUCCESS(error);

  const cl_int pattern = 0;
  EXPECT_SUCCESS(clEnqueueFillBuffer(command_queue, unused_dst_buffer, &pattern,
                                     sizeof(pattern), 0, data_size_in_bytes, 0,
                                     nullptr, nullptr));

  // Record a mutable dispatch to the command buffer.
  cl_ndrange_kernel_command_properties_khr mutable_properties[3] = {
      CL_MUTABLE_DISPATCH_UPDATABLE_FIELDS_KHR,
      CL_MUTABLE_DISPATCH_ARGUMENTS_KHR, 0};
  EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, mutable_properties, kernel, 1, nullptr,
      &global_size, nullptr, 0, nullptr, nullptr, &command_handle));

  // Finalize the command buffer.
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Enqueue the command buffer.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check the results.
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     data_size_in_bytes, output_data.data(), 0,
                                     nullptr, nullptr));

  // Do an explicit flush (see CA-3358).
  EXPECT_SUCCESS(clFinish(command_queue));

  EXPECT_EQ(input_data, output_data);

  // Update both the input and output buffer, reusing the same
  // cl_mutable_dispatch_arg_khr struct
  cl_mutable_dispatch_arg_khr arg{1, sizeof(cl_mem),
                                  static_cast<void *>(&unused_dst_buffer)};
  const cl_mutable_dispatch_config_khr dispatch_config{
      CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR,
      nullptr,
      command_handle,
      1,
      0,
      0,
      0,
      &arg,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr};
  const cl_mutable_base_config_khr mutable_config{
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};
  EXPECT_SUCCESS(clUpdateMutableCommandsKHR(command_buffer, &mutable_config));

  // Reuse argument update struct for output buffer
  arg.arg_index = 1;
  arg.arg_value = static_cast<void *>(&updated_dst_buffer);
  EXPECT_SUCCESS(clUpdateMutableCommandsKHR(command_buffer, &mutable_config));

  // Enqueue the command buffer again
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check that we were able to successfully update the buffers.
  std::vector<cl_int> updated_output_data(global_size);
  EXPECT_SUCCESS(clEnqueueReadBuffer(
      command_queue, updated_dst_buffer, CL_TRUE, 0, data_size_in_bytes,
      updated_output_data.data(), 0, nullptr, nullptr));
  EXPECT_EQ(input_data, updated_output_data);

  EXPECT_SUCCESS(clEnqueueReadBuffer(
      command_queue, unused_dst_buffer, CL_TRUE, 0, data_size_in_bytes,
      updated_output_data.data(), 0, nullptr, nullptr));

  // Do an explicit flush (see CA-3358).
  EXPECT_SUCCESS(clFinish(command_queue));

  const std::vector<cl_int> zero_ref(global_size, 0);
  EXPECT_EQ(zero_ref, updated_output_data);

  // Cleanup.
  EXPECT_SUCCESS(clReleaseMemObject(unused_dst_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(updated_dst_buffer));
}

// Test we can update kernel arguments without passing any property values
// when recording the kernel commands. This should default the property values
// to those supported by device, which we have already checked includes
// support for updating arguments.
TEST_F(CommandBufferUpdateNDKernel, NoMutablePropertiesSet) {
  // Create a new buffer to update to.
  cl_int error = CL_SUCCESS;
  cl_mem updated_dst_buffer = clCreateBuffer(
      context, CL_MEM_READ_ONLY, data_size_in_bytes, nullptr, &error);
  EXPECT_SUCCESS(error);

  const cl_int pattern = 0;
  EXPECT_SUCCESS(clEnqueueFillBuffer(command_queue, updated_dst_buffer,
                                     &pattern, sizeof(pattern), 0,
                                     data_size_in_bytes, 0, nullptr, nullptr));

  // Record a mutable dispatch to the command buffer but without passing
  // properties
  EXPECT_SUCCESS(clCommandNDRangeKernelKHR(
      command_buffer, nullptr, nullptr, kernel, 1, nullptr, &global_size,
      nullptr, 0, nullptr, nullptr, &command_handle));

  // Finalize the command buffer.
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Update the output buffer
  const cl_mutable_dispatch_arg_khr arg{
      1, sizeof(cl_mem), static_cast<void *>(&updated_dst_buffer)};
  const cl_mutable_dispatch_config_khr dispatch_config{
      CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR,
      nullptr,
      command_handle,
      1,
      0,
      0,
      0,
      &arg,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr};
  const cl_mutable_base_config_khr mutable_config{
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};
  EXPECT_SUCCESS(clUpdateMutableCommandsKHR(command_buffer, &mutable_config));

  // Enqueue the command buffer
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Check that we were able to successfully update the buffers.
  std::vector<cl_int> updated_output_data(global_size);
  EXPECT_SUCCESS(clEnqueueReadBuffer(
      command_queue, updated_dst_buffer, CL_TRUE, 0, data_size_in_bytes,
      updated_output_data.data(), 0, nullptr, nullptr));
  EXPECT_EQ(input_data, updated_output_data);

  // Do an explicit flush (see CA-3358).
  EXPECT_SUCCESS(clFinish(command_queue));

  EXPECT_EQ(input_data, updated_output_data);

  // Cleanup.
  EXPECT_SUCCESS(clReleaseMemObject(updated_dst_buffer));
}

// Fixture for testing updating command-buffer enqueued simultaneously
class CommandBufferSimultaneousUpdate : public CommandBufferUpdateNDKernel {
 protected:
  CommandBufferSimultaneousUpdate()
      : CommandBufferUpdateNDKernel(), update_data(global_size) {}

  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandBufferUpdateNDKernel::SetUp());

    const bool simultaneous_support =
        capabilities & CL_COMMAND_BUFFER_CAPABILITY_SIMULTANEOUS_USE_KHR;
    if (!simultaneous_support) {
      GTEST_SKIP();
    }

    cl_int error = !CL_SUCCESS;
    cl_command_buffer_properties_khr properties[3] = {
        CL_COMMAND_BUFFER_FLAGS_KHR,
        CL_COMMAND_BUFFER_SIMULTANEOUS_USE_KHR | CL_COMMAND_BUFFER_MUTABLE_KHR,
        0};
    simultaneous_command_buffer =
        clCreateCommandBufferKHR(1, &command_queue, properties, &error);
    ASSERT_SUCCESS(error);

    user_event = clCreateUserEvent(context, &error);
    ASSERT_SUCCESS(error);

    updated_src_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                        data_size_in_bytes, nullptr, &error);
    ASSERT_SUCCESS(error);

    updated_dst_buffer = clCreateBuffer(context, CL_MEM_READ_ONLY,
                                        data_size_in_bytes, nullptr, &error);
    ASSERT_SUCCESS(error);

    ucl::Environment::instance->GetInputGenerator().GenerateIntData(
        update_data);
    ASSERT_SUCCESS(clEnqueueWriteBuffer(
        command_queue, updated_src_buffer, CL_TRUE, 0, data_size_in_bytes,
        update_data.data(), 0, nullptr, nullptr));

    const cl_int pattern = 0;
    ASSERT_SUCCESS(clEnqueueFillBuffer(
        command_queue, updated_dst_buffer, &pattern, sizeof(pattern), 0,
        data_size_in_bytes, 0, nullptr, nullptr));
    ASSERT_SUCCESS(clFinish(command_queue));
  }

  void TearDown() override {
    if (user_event) {
      EXPECT_SUCCESS(clReleaseEvent(user_event));
    }

    if (simultaneous_command_buffer) {
      EXPECT_SUCCESS(clReleaseCommandBufferKHR(simultaneous_command_buffer));
    }

    if (updated_src_buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(updated_src_buffer));
    }

    if (updated_dst_buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(updated_dst_buffer));
    }
    CommandBufferUpdateNDKernel::TearDown();
  }
  cl_event user_event = nullptr;
  cl_command_buffer_khr simultaneous_command_buffer = nullptr;
  cl_mem updated_src_buffer = nullptr;
  cl_mem updated_dst_buffer = nullptr;

  std::vector<cl_int> update_data;
};

TEST_F(CommandBufferSimultaneousUpdate, UpdatePending) {
  // Record command-buffer as a single kernel command
  ASSERT_SUCCESS(clCommandNDRangeKernelKHR(
      simultaneous_command_buffer, nullptr, nullptr, kernel, 1, nullptr,
      &global_size, nullptr, 0, nullptr, nullptr, &command_handle));
  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(simultaneous_command_buffer));

  // Enqueue the command-buffer blocked on user event so it is not executed
  // and remains as pending execution
  ASSERT_SUCCESS(clEnqueueCommandBufferKHR(
      0, nullptr, simultaneous_command_buffer, 1, &user_event, nullptr));

  // Update the kernel inputs & output arguments
  cl_mutable_dispatch_arg_khr args[2] = {
      {0, sizeof(cl_mem), static_cast<void *>(&updated_src_buffer)},
      {1, sizeof(cl_mem), static_cast<void *>(&updated_dst_buffer)}};
  const cl_mutable_dispatch_config_khr dispatch_config{
      CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR,
      nullptr,
      command_handle,
      2,
      0,
      0,
      0,
      args,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr};
  const cl_mutable_base_config_khr mutable_config{
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};
  ASSERT_SUCCESS(
      clUpdateMutableCommandsKHR(simultaneous_command_buffer, &mutable_config));

  // Enqueue the command buffer again after update
  ASSERT_SUCCESS(clEnqueueCommandBufferKHR(
      0, nullptr, simultaneous_command_buffer, 0, nullptr, nullptr));

  // Execute the command-buffers
  ASSERT_SUCCESS(clSetUserEventStatus(user_event, CL_COMPLETE));
  ASSERT_SUCCESS(clFinish(command_queue));

  // Check that we were able to successfully update the buffers to new data
  std::vector<cl_int> updated_output_data(global_size);
  ASSERT_SUCCESS(clEnqueueReadBuffer(
      command_queue, updated_dst_buffer, CL_TRUE, 0, data_size_in_bytes,
      updated_output_data.data(), 0, nullptr, nullptr));
  ASSERT_EQ(update_data, updated_output_data);

  // Check that the first enqueue before the update produced the correct output
  std::vector<cl_int> original_output_data(global_size);
  ASSERT_SUCCESS(clEnqueueReadBuffer(
      command_queue, dst_buffer, CL_TRUE, 0, data_size_in_bytes,
      original_output_data.data(), 0, nullptr, nullptr));
  ASSERT_EQ(input_data, original_output_data);
}

TEST_F(CommandBufferSimultaneousUpdate, ConsecutiveUpdate) {
  // Record command-buffer as a single kernel command
  ASSERT_SUCCESS(clCommandNDRangeKernelKHR(
      simultaneous_command_buffer, nullptr, nullptr, kernel, 1, nullptr,
      &global_size, nullptr, 0, nullptr, nullptr, &command_handle));
  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(simultaneous_command_buffer));

  // Enqueue the command-buffer blocked on user event so it is not executed
  // and remains as pending
  ASSERT_SUCCESS(clEnqueueCommandBufferKHR(
      0, nullptr, simultaneous_command_buffer, 1, &user_event, nullptr));

  // Update the kernel input argument
  const cl_mutable_dispatch_arg_khr input_arg = {
      0, sizeof(cl_mem), static_cast<void *>(&updated_src_buffer)};
  cl_mutable_dispatch_config_khr dispatch_config{
      CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR,
      nullptr,
      command_handle,
      1,
      0,
      0,
      0,
      &input_arg,
      nullptr,
      nullptr,
      nullptr,
      nullptr,
      nullptr};
  const cl_mutable_base_config_khr mutable_config{
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 1, &dispatch_config};
  ASSERT_SUCCESS(
      clUpdateMutableCommandsKHR(simultaneous_command_buffer, &mutable_config));

  // Update the kernel output argument
  const cl_mutable_dispatch_arg_khr output_args = {
      1, sizeof(cl_mem), static_cast<void *>(&updated_dst_buffer)};
  dispatch_config.arg_list = &output_args;
  ASSERT_SUCCESS(
      clUpdateMutableCommandsKHR(simultaneous_command_buffer, &mutable_config));

  // Enqueue the command buffer again after update
  ASSERT_SUCCESS(clEnqueueCommandBufferKHR(
      0, nullptr, simultaneous_command_buffer, 0, nullptr, nullptr));

  // Execute the command-buffers
  ASSERT_SUCCESS(clSetUserEventStatus(user_event, CL_COMPLETE));
  ASSERT_SUCCESS(clFinish(command_queue));

  // Check that we were able to successfully update the buffers to new data
  std::vector<cl_int> updated_output_data(global_size);
  ASSERT_SUCCESS(clEnqueueReadBuffer(
      command_queue, updated_dst_buffer, CL_TRUE, 0, data_size_in_bytes,
      updated_output_data.data(), 0, nullptr, nullptr));
  ASSERT_EQ(update_data, updated_output_data);

  // Check that the first enqueue before the update produced the correct output
  std::vector<cl_int> original_output_data(global_size);
  ASSERT_SUCCESS(clEnqueueReadBuffer(
      command_queue, dst_buffer, CL_TRUE, 0, data_size_in_bytes,
      original_output_data.data(), 0, nullptr, nullptr));
  ASSERT_EQ(input_data, original_output_data);
}

// Test using more than one command in the command-buffer
TEST_F(CommandBufferSimultaneousUpdate, MultipleCommands) {
  // Pre-Update
  // Kernel 1: Copy src_buffer -> dst_buffer
  // Kernel 2: Copy dst_buffer -> updated_src_buffer
  //
  // Post-Update
  // Kernel 1: Copy src_buffer -> updated_dst_buffer
  // Kernel 2: Copy updated_dst_buffer -> updated_src_buffer

  ASSERT_SUCCESS(clCommandNDRangeKernelKHR(
      simultaneous_command_buffer, nullptr, nullptr, kernel, 1, nullptr,
      &global_size, nullptr, 0, nullptr, nullptr, &command_handle));

  ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem),
                                static_cast<void *>(&dst_buffer)));
  ASSERT_SUCCESS(clSetKernelArg(kernel, 1, sizeof(cl_mem),
                                static_cast<void *>(&updated_src_buffer)));

  cl_mutable_command_khr command_handle2;
  ASSERT_SUCCESS(clCommandNDRangeKernelKHR(
      simultaneous_command_buffer, nullptr, nullptr, kernel, 1, nullptr,
      &global_size, nullptr, 0, nullptr, nullptr, &command_handle2));

  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(simultaneous_command_buffer));

  // Enqueue the command-buffer blocked on user event so it is not executed
  // as remains pending
  ASSERT_SUCCESS(clEnqueueCommandBufferKHR(
      0, nullptr, simultaneous_command_buffer, 1, &user_event, nullptr));

  // Update the output of command 1 and input of command 2
  const cl_mutable_dispatch_arg_khr arg1 = {
      1, sizeof(cl_mem), static_cast<void *>(&updated_dst_buffer)};
  const cl_mutable_dispatch_arg_khr arg2 = {
      0, sizeof(cl_mem), static_cast<void *>(&updated_dst_buffer)};
  cl_mutable_dispatch_config_khr dispatch_configs[2] = {
      {CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR, nullptr, command_handle,
       1, 0, 0, 0, &arg1, nullptr, nullptr, nullptr, nullptr, nullptr},
      {CL_STRUCTURE_TYPE_MUTABLE_DISPATCH_CONFIG_KHR, nullptr, command_handle2,
       1, 0, 0, 0, &arg2, nullptr, nullptr, nullptr, nullptr, nullptr}};

  const cl_mutable_base_config_khr mutable_config{
      CL_STRUCTURE_TYPE_MUTABLE_BASE_CONFIG_KHR, nullptr, 2, dispatch_configs};
  ASSERT_SUCCESS(
      clUpdateMutableCommandsKHR(simultaneous_command_buffer, &mutable_config));

  // Enqueue the command buffer again after update
  std::vector<cl_int> run1_output_data(global_size);
  ASSERT_SUCCESS(clEnqueueReadBuffer(
      command_queue, updated_src_buffer, CL_FALSE, 0, data_size_in_bytes,
      run1_output_data.data(), 0, nullptr, nullptr));

  ASSERT_SUCCESS(clEnqueueWriteBuffer(command_queue, src_buffer, CL_FALSE, 0,
                                      data_size_in_bytes, update_data.data(), 0,
                                      nullptr, nullptr));

  ASSERT_SUCCESS(clEnqueueCommandBufferKHR(
      0, nullptr, simultaneous_command_buffer, 0, nullptr, nullptr));

  // Execute the command-buffers
  ASSERT_SUCCESS(clSetUserEventStatus(user_event, CL_COMPLETE));
  ASSERT_SUCCESS(clFinish(command_queue));
  ASSERT_EQ(run1_output_data, input_data);

  // Check that we were able to successfully update the buffers to new data
  std::vector<cl_int> run2_output_data(global_size);
  ASSERT_SUCCESS(clEnqueueReadBuffer(
      command_queue, updated_src_buffer, CL_TRUE, 0, data_size_in_bytes,
      run2_output_data.data(), 0, nullptr, nullptr));
  ASSERT_EQ(run2_output_data, update_data);
}
