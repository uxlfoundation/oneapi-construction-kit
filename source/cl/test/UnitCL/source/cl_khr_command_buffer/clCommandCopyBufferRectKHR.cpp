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

namespace {
// For easily handling raw blocks of bytes.
using byte_vector = std::vector<cl_uchar>;
// For easily handling `src_origin` and `dst_origin`.
using Position = std::array<std::size_t, 3>;
// For easily handling  `region`.
using Region = std::array<std::size_t, 3>;

// Holds the values over which the tests is parameterized.
struct test_parameters {
  std::string name;
  size_t src_buffer_size;
  size_t dst_buffer_size;
  Position src_origin;
  Position dst_origin;
  Region region;
  size_t src_row_pitch;
  size_t src_slice_pitch;
  size_t dst_row_pitch;
  size_t dst_slice_pitch;
};

// Does the host side equivalent of a clCopyBufferRect operation.
void copyBufferRect(const byte_vector &src_buffer, byte_vector &dst_buffer,
                    const Position &src_origin, const Position &dst_origin,
                    const Region &region, size_t src_row_pitch,
                    size_t src_slice_pitch, size_t dst_row_pitch,
                    size_t dst_slice_pitch) {
  const size_t src_offset = (src_origin[2] * src_slice_pitch) +
                            (src_origin[1] * src_row_pitch) + src_origin[0];

  const size_t dst_offset = (dst_origin[2] * dst_slice_pitch) +
                            (dst_origin[1] * dst_row_pitch) + dst_origin[0];

  auto src_position = src_buffer.data() + src_offset;
  auto dst_position = dst_buffer.data() + dst_offset;

  // for each slice.
  for (unsigned k = 0; k < region[2]; ++k) {
    auto src_slice = src_position + (k * src_slice_pitch);
    auto dst_slice = dst_position + (k * src_slice_pitch);
    // for each row.
    for (unsigned j = 0; j < region[1]; ++j) {
      // for each element.
      auto src_row = src_slice + (j * src_row_pitch);
      auto dst_row = dst_slice + (j * dst_row_pitch);
      std::memcpy(dst_row, src_row, region[0]);
    }
  }
}

}  // namespace

class CommandCopyBufferRectParamTest
    : public cl_khr_command_buffer_Test,
      public ::testing::WithParamInterface<test_parameters> {};

// Check we can enqueue an arbitrary copy rect between two buffers filled with
// random values.
TEST_P(CommandCopyBufferRectParamTest, CopyArbitraryRect) {
  // Unpack the parameters.
#define UNPACK_PARAM(X) auto X = GetParam().X
  UNPACK_PARAM(src_buffer_size);
  UNPACK_PARAM(dst_buffer_size);
  UNPACK_PARAM(src_origin);
  UNPACK_PARAM(dst_origin);
  UNPACK_PARAM(region);
  UNPACK_PARAM(src_row_pitch);
  UNPACK_PARAM(src_slice_pitch);
  UNPACK_PARAM(dst_row_pitch);
  UNPACK_PARAM(dst_slice_pitch);
#undef UNPACK_PARAM

  // Create two buffers to copy between.
  cl_int error = CL_SUCCESS;
  cl_mem src_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                     src_buffer_size, nullptr, &error);
  EXPECT_SUCCESS(error);

  cl_mem dst_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                     dst_buffer_size, nullptr, &error);
  EXPECT_SUCCESS(error);

  // Fill the first buffer with random values.
  byte_vector input_value(src_buffer_size, 0x0);
  auto &generator = ucl::Environment::instance->GetInputGenerator();
  generator.GenerateIntData(input_value);
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, src_buffer, CL_TRUE, 0,
                                      src_buffer_size, input_value.data(), 0,
                                      nullptr, nullptr));

  // Fill the second buffer with zero, this is just for ease of debugging.
  const cl_uchar zero = 0x0;
  EXPECT_SUCCESS(clEnqueueFillBuffer(command_queue, dst_buffer, &zero,
                                     sizeof(cl_uchar), 0, dst_buffer_size, 0,
                                     nullptr, nullptr));

  // Create a command buffer.
  cl_command_buffer_khr command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
  EXPECT_SUCCESS(error);

  // Push a CopyBufferRect into the command buffer and finalize it.
  EXPECT_SUCCESS(clCommandCopyBufferRectKHR(
      command_buffer, nullptr, src_buffer, dst_buffer, src_origin.data(),
      dst_origin.data(), region.data(), src_row_pitch, src_slice_pitch,
      dst_row_pitch, dst_slice_pitch, 0, nullptr, nullptr, nullptr));
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Enqueue the command buffer containing the copy.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Do a blocking read.
  byte_vector result(dst_buffer_size, 0x0);
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     dst_buffer_size, result.data(), 0, nullptr,
                                     nullptr));

  // Do a host side equivalent of what OpenCL did.
  byte_vector expected(dst_buffer_size, 0x0);
  copyBufferRect(input_value, expected, src_origin, dst_origin, region,
                 src_row_pitch, src_slice_pitch, dst_row_pitch,
                 dst_slice_pitch);

  // Check the results are equal.
  EXPECT_EQ(expected, result);

  // Cleanup.
  EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(src_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(dst_buffer));
}

// Choose parameters so that we get good coverage and catch some edge cases.
static std::vector<test_parameters> generateParameterizations() {
  std::vector<test_parameters> parameterizations;
#define PARAMETERIZATION(name, src_buffer_size, dst_buffer_size, src_origin,   \
                         dst_origin, region, src_row_pitch, src_slice_pitch,   \
                         dst_row_pitch, dst_slice_pitch)                       \
  const test_parameters name{#name,          src_buffer_size, dst_buffer_size, \
                             src_origin,     dst_origin,      region,          \
                             src_row_pitch,  src_slice_pitch, dst_row_pitch,   \
                             dst_slice_pitch};                                 \
  parameterizations.push_back(name);                                           \
  (void)0

  PARAMETERIZATION(whole_buffer_2D, 256, 256, (Position{0, 0, 0}),
                   (Position{0, 0, 0}), (Region{16, 16, 1}), 16, 256, 16, 256);
  PARAMETERIZATION(whole_buffer_2D_zero_strides, 256, 256, (Position{0, 0, 0}),
                   (Position{0, 0, 0}), (Region{16, 16, 1}), 16, 0, 16, 0);
  PARAMETERIZATION(non_zero_source_offset_2D, 256, 256, (Position{16, 8, 0}),
                   (Position{8, 4, 0}), (Region{4, 4, 1}), 4, 16, 8, 0);
  PARAMETERIZATION(different_buffer_sizes_2D, 256, 512, (Position{16, 8, 0}),
                   (Position{8, 4, 0}), (Region{4, 4, 1}), 4, 16, 8, 0);
  PARAMETERIZATION(copy_column_2D, 256, 512, (Position{0, 0, 0}),
                   (Position{1, 0, 0}), (Region{1, 256, 1}), 1, 256, 2, 512);
  PARAMETERIZATION(copy_row_2D, 256, 512, (Position{0, 0, 0}),
                   (Position{0, 1, 0}), (Region{256, 1, 1}), 256, 256, 256,
                   512);
  PARAMETERIZATION(copy_3d, 512, 512, (Position{0, 0, 0}), (Position{0, 0, 0}),
                   (Region{8, 8, 8}), 8, 64, 8, 64);
  PARAMETERIZATION(copy_3d_with_offsets, 512, 512, (Position{1, 2, 3}),
                   (Position{4, 1, 3}), (Region{4, 3, 2}), 8, 64, 8, 64);
  PARAMETERIZATION(copy_2d_3d, 256, 1024, (Position{1, 2, 0}),
                   (Position{4, 1, 3}), (Region{4, 16, 1}), 8, 256, 8, 256);
  PARAMETERIZATION(copy_3d_2d, 512, 16, (Position{7, 3, 3}),
                   (Position{1, 3, 0}), (Region{1, 4, 1}), 8, 128, 2, 16);
#undef PARAMETERIZATION
  return parameterizations;
}

INSTANTIATE_TEST_SUITE_P(
    DifferentParameters, CommandCopyBufferRectParamTest,
    ::testing::ValuesIn(generateParameterizations()),
    [](const ::testing::TestParamInfo<CommandCopyBufferRectParamTest::ParamType>
           &info) { return info.param.name; });

struct CommandCopyBufferRectTest : cl_khr_command_buffer_Test {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(cl_khr_command_buffer_Test::SetUp());

    cl_int error = CL_SUCCESS;
    src_buffer =
        clCreateBuffer(context, CL_MEM_READ_ONLY, buffer_size, nullptr, &error);
    ASSERT_SUCCESS(error);

    dst_buffer =
        clCreateBuffer(context, CL_MEM_READ_ONLY, buffer_size, nullptr, &error);
    ASSERT_SUCCESS(error);

    command_buffer =
        clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
    ASSERT_SUCCESS(error);
  }

  void TearDown() override {
    if (command_buffer) {
      EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
    }
    if (src_buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(src_buffer));
    }

    if (dst_buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(dst_buffer));
    }

    cl_khr_command_buffer_Test::TearDown();
  }
  cl_mem src_buffer = nullptr;
  cl_mem dst_buffer = nullptr;
  cl_command_buffer_khr command_buffer = nullptr;

  static constexpr size_t buffer_size = 512;
};

TEST_F(CommandCopyBufferRectTest, Sync) {
  Position src_origin{0, 0, 0};
  Position dst_origin{0, 0, 0};
  Region region{16, 16, 1};
  const size_t row_pitch = 16;
  const size_t slice_pitch = 256;

  cl_sync_point_khr sync_points[2] = {
      std::numeric_limits<cl_sync_point_khr>::max(),
      std::numeric_limits<cl_sync_point_khr>::max()};

  ASSERT_SUCCESS(clCommandCopyBufferRectKHR(
      command_buffer, nullptr, src_buffer, src_buffer, src_origin.data(),
      dst_origin.data(), region.data(), row_pitch, slice_pitch, row_pitch,
      slice_pitch, 0, nullptr, &sync_points[0], nullptr));

  ASSERT_NE(sync_points[0], std::numeric_limits<cl_sync_point_khr>::max());

  ASSERT_SUCCESS(clCommandCopyBufferRectKHR(
      command_buffer, nullptr, src_buffer, src_buffer, src_origin.data(),
      dst_origin.data(), region.data(), row_pitch, slice_pitch, row_pitch,
      slice_pitch, 0, nullptr, &sync_points[1], nullptr));

  ASSERT_NE(sync_points[1], std::numeric_limits<cl_sync_point_khr>::max());

  ASSERT_SUCCESS(clCommandCopyBufferRectKHR(
      command_buffer, nullptr, src_buffer, src_buffer, src_origin.data(),
      dst_origin.data(), region.data(), row_pitch, slice_pitch, row_pitch,
      slice_pitch, 2, sync_points, nullptr, nullptr));
}

// Tests we can successfully execute a copy rect command where the source and
// destination buffers are the same but don't overlap.
TEST_F(CommandCopyBufferRectTest, SrcDoesntOverlapDst) {
  // Fill the buffer with some random values.
  byte_vector input_value(buffer_size, 0x0);
  auto &generator = ucl::Environment::instance->GetInputGenerator();
  generator.GenerateIntData(input_value);
  ASSERT_SUCCESS(clEnqueueWriteBuffer(command_queue, src_buffer, CL_TRUE, 0,
                                      buffer_size, input_value.data(), 0,
                                      nullptr, nullptr));

  // Push a CopyBufferRect into the command buffer treating the buffer as a 16 x
  // 16 rectangle such that it copies a 4 x 4 region within the buffer.
  const Position src_origin{0, 1, 0};
  const Position dst_origin{0, 1, 0};
  const Region region{2, 2, 1};
  const size_t row_pitch = 4;
  const size_t slice_pitch = 4 * row_pitch;
  ASSERT_SUCCESS(clCommandCopyBufferRectKHR(
      command_buffer, nullptr, src_buffer, src_buffer, src_origin.data(),
      dst_origin.data(), region.data(), row_pitch, slice_pitch, row_pitch,
      slice_pitch, 0, nullptr, nullptr, nullptr));

  // Finalize the buffer.
  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Enqueue the command buffer containing the copy.
  ASSERT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Do a blocking read.
  byte_vector result(buffer_size, 0x0);
  ASSERT_SUCCESS(clEnqueueReadBuffer(command_queue, src_buffer, CL_TRUE, 0,
                                     buffer_size, result.data(), 0, nullptr,
                                     nullptr));

  // Do a host side equivalent of what OpenCL did.
  copyBufferRect(input_value, input_value, src_origin, dst_origin, region,
                 row_pitch, slice_pitch, row_pitch, slice_pitch);

  // Check the results are equal.
  ASSERT_EQ(input_value, result);
}

// Tests we can successfully execute a copy rect command in a command buffer
// with other commands.
TEST_F(CommandCopyBufferRectTest, FillThenCopyRect) {
  // Fill the src buffer with random values and zero the dst buffer.
  auto &generator = ucl::Environment::instance->GetInputGenerator();

  byte_vector src_value(buffer_size, 0x0);
  generator.GenerateIntData(src_value);
  ASSERT_SUCCESS(clEnqueueWriteBuffer(command_queue, src_buffer, CL_TRUE, 0,
                                      buffer_size, src_value.data(), 0, nullptr,
                                      nullptr));

  const cl_uchar zero = 0x0;
  ASSERT_SUCCESS(clEnqueueFillBuffer(command_queue, dst_buffer, &zero,
                                     sizeof(cl_uchar), 0, buffer_size, 0,
                                     nullptr, nullptr));

  // Pick some random offset to start the fill at.
  const size_t fill_offset = generator.GenerateInt<size_t>(0, buffer_size - 1);
  // Pick an appropriate fill size based on the offset.
  const size_t fill_size =
      generator.GenerateInt<size_t>(1, buffer_size - fill_offset);
  // Pick some random byte to fill with.
  const cl_uchar fill_pattern = generator.GenerateInt<cl_uchar>();
  const size_t fill_pattern_size = sizeof(fill_pattern);

  // Push a fill into the command buffer.
  ASSERT_SUCCESS(clCommandFillBufferKHR(
      command_buffer, nullptr, src_buffer, &fill_pattern, fill_pattern_size,
      fill_offset, fill_size, 0, nullptr, nullptr, nullptr));

  // Push a copy buffer rect into the command buffer. Just copy the whole
  // buffer but treating it like an 8 x 8 x 8 cube.
  const Position src_origin{0, 0, 0};
  const Position dst_origin{0, 0, 0};
  const Region region{8, 8, 8};
  const size_t src_row_pitch = 8;
  const size_t src_slice_pitch = 8 * src_row_pitch;
  const size_t dst_row_pitch = 8;
  const size_t dst_slice_pitch = 8 * dst_row_pitch;

  ASSERT_SUCCESS(clCommandCopyBufferRectKHR(
      command_buffer, nullptr, src_buffer, dst_buffer, src_origin.data(),
      dst_origin.data(), region.data(), src_row_pitch, src_slice_pitch,
      dst_row_pitch, dst_slice_pitch, 0, nullptr, nullptr, nullptr));

  // Finalize the command buffer.
  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Enqueue the command buffer containing the fill and copy.
  ASSERT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Do a blocking read on the result buffer.
  byte_vector result(buffer_size, 0x0);
  ASSERT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     buffer_size, result.data(), 0, nullptr,
                                     nullptr));

  // Do a host side equivalent of what OpenCL did.
  std::fill_n(std::next(std::begin(src_value), fill_offset), fill_size,
              fill_pattern);
  byte_vector expected(buffer_size, 0x0);
  copyBufferRect(src_value, expected, src_origin, dst_origin, region,
                 src_row_pitch, src_slice_pitch, dst_row_pitch,
                 dst_slice_pitch);

  // Check the results are equal.
  ASSERT_EQ(expected, result);
}

// Tests we can enqueue multiple non overlapping copy buffer rects into a
// command buffer that act on the same buffer.
TEST_F(CommandCopyBufferRectTest, MultipleCopyRectsSameBuffer) {
  // Create three buffers, then copy rect from the first two to the third one.
  cl_int error = CL_SUCCESS;
  const size_t src_buffer_b_size = 256;
  cl_mem src_buffer_b = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                       src_buffer_b_size, nullptr, &error);
  ASSERT_SUCCESS(error);

  const size_t out_buffer_size = 64;
  cl_mem out_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                     out_buffer_size, nullptr, &error);
  ASSERT_SUCCESS(error);

  // Fill the src buffers with random values and zero the dst buffer.
  auto &generator = ucl::Environment::instance->GetInputGenerator();

  byte_vector src_value_a(buffer_size, 0x0);
  generator.GenerateIntData(src_value_a);
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, src_buffer, CL_TRUE, 0,
                                      buffer_size, src_value_a.data(), 0,
                                      nullptr, nullptr));

  byte_vector src_value_b(src_buffer_b_size, 0x0);
  generator.GenerateIntData(src_value_b);
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, src_buffer_b, CL_TRUE, 0,
                                      src_buffer_b_size, src_value_b.data(), 0,
                                      nullptr, nullptr));

  const cl_uchar zero = 0x0;
  EXPECT_SUCCESS(clEnqueueFillBuffer(command_queue, out_buffer, &zero,
                                     sizeof(cl_uchar), 0, out_buffer_size, 0,
                                     nullptr, nullptr));

  // Push a CopyBufferRect into the command buffer treating the first src buffer
  // as an 8 x 8 x 8 cube such that it copies a 4 x 4 region to the destination
  // buffer which is treated as an 8 x 8 rectangle.
  const Position src_a_origin{1, 2, 3};
  const Position dst_origin_first_copy{0, 0, 0};
  const Region region{4, 4, 1};
  const size_t src_a_row_pitch = 8;
  const size_t src_a_slice_pitch = 8 * src_a_row_pitch;
  const size_t dst_row_pitch = 4;
  const size_t dst_slice_pitch = 4 * dst_row_pitch;

  EXPECT_SUCCESS(clCommandCopyBufferRectKHR(
      command_buffer, nullptr, src_buffer, out_buffer, src_a_origin.data(),
      dst_origin_first_copy.data(), region.data(), src_a_row_pitch,
      src_a_slice_pitch, dst_row_pitch, dst_slice_pitch, 0, nullptr, nullptr,
      nullptr));

  // Push a second CopyBufferRect into the command buffer treating the second
  // src buffer as a 16 x 16 square such that it copies a 4 x 4 region to the
  // destination buffer which is treated as an 8 x 8 rectangle.
  const Position src_b_origin{2, 2, 0};
  const Position dst_origin_second_copy{4, 4, 0};
  const size_t src_b_row_pitch = 16;
  const size_t src_b_slice_pitch = 16 * src_a_row_pitch;

  EXPECT_SUCCESS(clCommandCopyBufferRectKHR(
      command_buffer, nullptr, src_buffer_b, out_buffer, src_b_origin.data(),
      dst_origin_second_copy.data(), region.data(), src_b_row_pitch,
      src_b_slice_pitch, dst_row_pitch, dst_slice_pitch, 0, nullptr, nullptr,
      nullptr));

  // Finalize the buffer.
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Enqueue the command buffer containing the copy.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Do a blocking read.
  byte_vector result(out_buffer_size, 0x0);
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, out_buffer, CL_TRUE, 0,
                                     out_buffer_size, result.data(), 0, nullptr,
                                     nullptr));

  // Do a host side equivalent of what OpenCL did.
  byte_vector expected(out_buffer_size, 0x0);
  copyBufferRect(src_value_a, expected, src_a_origin, dst_origin_first_copy,
                 region, src_a_row_pitch, src_a_slice_pitch, dst_row_pitch,
                 dst_slice_pitch);
  copyBufferRect(src_value_b, expected, src_b_origin, dst_origin_second_copy,
                 region, src_b_row_pitch, src_b_slice_pitch, dst_row_pitch,
                 dst_slice_pitch);

  // Check the results are equal.
  EXPECT_EQ(expected, result);

  // Cleanup.
  EXPECT_SUCCESS(clReleaseMemObject(out_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(src_buffer_b));
}

// Tests we can enqueue multiple overlapping copy rects into a command buffer
// that act on the same buffer.
TEST_F(CommandCopyBufferRectTest, MultipleCopyRectsSameBufferOverlapping) {
  // Create three buffers, then copy rect from the first two to the third one.
  cl_int error = CL_SUCCESS;
  const size_t src_buffer_b_size = 256;
  cl_mem src_buffer_b = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                       src_buffer_b_size, nullptr, &error);
  ASSERT_SUCCESS(error);

  const size_t out_buffer_size = 64;
  cl_mem out_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                     out_buffer_size, nullptr, &error);
  EXPECT_SUCCESS(error);

  // Fill the src buffers with random values and zero the dst buffer.
  auto &generator = ucl::Environment::instance->GetInputGenerator();

  byte_vector src_value_a(buffer_size, 0x0);
  generator.GenerateIntData(src_value_a);
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, src_buffer, CL_TRUE, 0,
                                      buffer_size, src_value_a.data(), 0,
                                      nullptr, nullptr));

  byte_vector src_value_b(src_buffer_b_size, 0x0);
  generator.GenerateIntData(src_value_b);
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, src_buffer_b, CL_TRUE, 0,
                                      src_buffer_b_size, src_value_b.data(), 0,
                                      nullptr, nullptr));

  const cl_uchar zero = 0x0;
  EXPECT_SUCCESS(clEnqueueFillBuffer(command_queue, out_buffer, &zero,
                                     sizeof(cl_uchar), 0, out_buffer_size, 0,
                                     nullptr, nullptr));

  // Push a CopyBufferRect into the command buffer treating the first src buffer
  // as an 8 x 8 x 8 cube such that it copies a 4 x 4 region to the destination
  // buffer which is treated as an 8 x 8 rectangle.
  const Position src_a_origin{1, 2, 3};
  const Position dst_origin_first_copy{1, 1, 0};
  const Region region{4, 4, 1};
  const size_t src_a_row_pitch = 8;
  const size_t src_a_slice_pitch = 8 * src_a_row_pitch;
  const size_t dst_row_pitch = 4;
  const size_t dst_slice_pitch = 4 * dst_row_pitch;

  EXPECT_SUCCESS(clCommandCopyBufferRectKHR(
      command_buffer, nullptr, src_buffer, out_buffer, src_a_origin.data(),
      dst_origin_first_copy.data(), region.data(), src_a_row_pitch,
      src_a_slice_pitch, dst_row_pitch, dst_slice_pitch, 0, nullptr, nullptr,
      nullptr));

  // Push a second CopyBufferRect into the command buffer treating the second
  // src buffer as a 16 x 16 square such that it copies a 4 x 4 region to the
  // destination buffer which is treated as an 8 x 8 rectangle.
  const Position src_b_origin{2, 2, 0};
  const Position dst_origin_second_copy{2, 2, 0};
  const size_t src_b_row_pitch = 16;
  const size_t src_b_slice_pitch = 16 * src_a_row_pitch;

  EXPECT_SUCCESS(clCommandCopyBufferRectKHR(
      command_buffer, nullptr, src_buffer_b, out_buffer, src_b_origin.data(),
      dst_origin_second_copy.data(), region.data(), src_b_row_pitch,
      src_b_slice_pitch, dst_row_pitch, dst_slice_pitch, 0, nullptr, nullptr,
      nullptr));

  // Finalize the buffer.
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Enqueue the command buffer containing the copy.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Do a blocking read.
  byte_vector result(out_buffer_size, 0x0);
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, out_buffer, CL_TRUE, 0,
                                     out_buffer_size, result.data(), 0, nullptr,
                                     nullptr));

  // Do a host side equivalent of what OpenCL did.
  byte_vector expected(out_buffer_size, 0x0);
  copyBufferRect(src_value_a, expected, src_a_origin, dst_origin_first_copy,
                 region, src_a_row_pitch, src_a_slice_pitch, dst_row_pitch,
                 dst_slice_pitch);
  copyBufferRect(src_value_b, expected, src_b_origin, dst_origin_second_copy,
                 region, src_b_row_pitch, src_b_slice_pitch, dst_row_pitch,
                 dst_slice_pitch);

  // Check the results are equal.
  EXPECT_EQ(expected, result);

  // Cleanup.
  EXPECT_SUCCESS(clReleaseMemObject(out_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(src_buffer_b));
}

// Tests we can enqueue multiple copy rects into a command buffer that act on
// different buffers.
TEST_F(CommandCopyBufferRectTest, MultipleCopyDifferentBuffers) {
  // Create three buffers, then copy rect from the first to the second two.
  cl_int error = CL_SUCCESS;
  const size_t dst_buffer_a_size = 256;
  cl_mem dst_buffer_a = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                       dst_buffer_a_size, nullptr, &error);
  ASSERT_SUCCESS(error);

  const size_t dst_buffer_b_size = 64;
  cl_mem dst_buffer_b = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                       dst_buffer_b_size, nullptr, &error);
  ASSERT_SUCCESS(error);

  // Fill the src buffer with random values and zero the dst buffers.
  auto &generator = ucl::Environment::instance->GetInputGenerator();

  byte_vector src_value(buffer_size, 0x0);
  generator.GenerateIntData(src_value);
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, src_buffer, CL_TRUE, 0,
                                      buffer_size, src_value.data(), 0, nullptr,
                                      nullptr));

  const cl_uchar zero = 0x0;
  EXPECT_SUCCESS(clEnqueueFillBuffer(command_queue, dst_buffer_a, &zero,
                                     sizeof(cl_uchar), 0, dst_buffer_a_size, 0,
                                     nullptr, nullptr));
  EXPECT_SUCCESS(clEnqueueFillBuffer(command_queue, dst_buffer_b, &zero,
                                     sizeof(cl_uchar), 0, dst_buffer_b_size, 0,
                                     nullptr, nullptr));

  // Push a CopyBufferRect into the command buffer treating the first src buffer
  // as an 8 x 8 x 8 cube such that it copies a 4 x 4 region to the first
  // destination buffer which is treated as an 32 x 8 rectangle.
  const Position src_origin_first_copy{1, 2, 3};
  const Position dst_a_origin{2, 4, 0};
  const Region region_first_copy{4, 4, 1};
  const size_t src_row_pitch_first_copy = 8;
  const size_t src_slice_pitch_first_copy = 8 * src_row_pitch_first_copy;
  const size_t dst_a_row_pitch = 8;
  const size_t dst_a_slice_pitch = 32 * dst_a_row_pitch;

  EXPECT_SUCCESS(clCommandCopyBufferRectKHR(
      command_buffer, nullptr, src_buffer, dst_buffer_a,
      src_origin_first_copy.data(), dst_a_origin.data(),
      region_first_copy.data(), src_row_pitch_first_copy,
      src_slice_pitch_first_copy, dst_a_row_pitch, dst_a_slice_pitch, 0,
      nullptr, nullptr, nullptr));

  // Push a CopyBufferRect into the command buffer treating the first src buffer
  // as an 1 x 128 x 4 rectangle such that it copies a 1 x 64 region to the
  // second destination buffer which is treated as an 1 x 64 rectangle.
  const Position src_origin_second_copy{0, 17, 3};
  const Position dst_b_origin{0, 0, 0};
  const Region region_second_copy{1, 64, 1};
  const size_t src_row_pitch_second_copy = 1;
  const size_t src_slice_pitch_second_copy = 128 * src_row_pitch_second_copy;
  const size_t dst_b_row_pitch = 1;
  const size_t dst_b_slice_pitch = 64 * dst_b_row_pitch;

  EXPECT_SUCCESS(clCommandCopyBufferRectKHR(
      command_buffer, nullptr, src_buffer, dst_buffer_b,
      src_origin_second_copy.data(), dst_b_origin.data(),
      region_second_copy.data(), src_row_pitch_second_copy,
      src_slice_pitch_second_copy, dst_b_row_pitch, dst_b_slice_pitch, 0,
      nullptr, nullptr, nullptr));

  // Finalize the buffer.
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Enqueue the command buffer containing the copy.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Do a blocking read on the buffers.
  byte_vector result_a(dst_buffer_a_size, 0x0);
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer_a, CL_TRUE, 0,
                                     dst_buffer_a_size, result_a.data(), 0,
                                     nullptr, nullptr));

  byte_vector result_b(dst_buffer_b_size, 0x0);
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer_b, CL_TRUE, 0,
                                     dst_buffer_b_size, result_b.data(), 0,
                                     nullptr, nullptr));

  // Do a host side equivalent of what OpenCL did.
  byte_vector expected_a(dst_buffer_a_size, 0x0);
  copyBufferRect(src_value, expected_a, src_origin_first_copy, dst_a_origin,
                 region_first_copy, src_row_pitch_first_copy,
                 src_slice_pitch_first_copy, dst_a_row_pitch,
                 dst_a_slice_pitch);

  byte_vector expected_b(dst_buffer_b_size, 0x0);
  copyBufferRect(src_value, expected_b, src_origin_second_copy, dst_b_origin,
                 region_second_copy, src_row_pitch_second_copy,
                 src_slice_pitch_second_copy, dst_b_row_pitch,
                 dst_b_slice_pitch);

  // Check the results are equal.
  EXPECT_EQ(expected_a, result_a);
  EXPECT_EQ(expected_b, result_b);

  // Cleanup.
  EXPECT_SUCCESS(clReleaseMemObject(dst_buffer_a));
  EXPECT_SUCCESS(clReleaseMemObject(dst_buffer_b));
}

// Tests we can enqueue a command buffer containing clCommandCopyBufferRectKHR
// to a queue with other commands such that if the commands execute out of order
// the tests fails.
TEST_F(CommandCopyBufferRectTest, MixedQueue) {
  // This test isn't valid for out of order queues.
  if (!UCL::isQueueInOrder(command_queue)) {
    GTEST_SKIP();
  }

  // Fill the output buffer with zeros.
  const cl_uchar zero = 0x0;
  ASSERT_SUCCESS(clEnqueueFillBuffer(command_queue, dst_buffer, &zero,
                                     sizeof(cl_uchar), 0, buffer_size, 0,
                                     nullptr, nullptr));
  // Push a CopyBufferRect into the command queue doing a copy of half the
  // buffer as a 16 * 16 rectangle.
  const Position src_origin{0, 0, 0};
  const Position dst_origin{0, 0, 0};
  const Region region{16, 16, 1};
  const size_t row_pitch = 16;
  const size_t slice_pitch = 16 * row_pitch;

  ASSERT_SUCCESS(clCommandCopyBufferRectKHR(
      command_buffer, nullptr, src_buffer, dst_buffer, src_origin.data(),
      dst_origin.data(), region.data(), row_pitch, slice_pitch, row_pitch,
      slice_pitch, 0, nullptr, nullptr, nullptr));

  // Finalize the buffer.
  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Fill the src buffer with random values using a non-blocking write in the
  // command queue.
  auto &generator = ucl::Environment::instance->GetInputGenerator();
  const size_t copy_size = buffer_size / 2;
  byte_vector src_value(copy_size, 0x0);
  generator.GenerateIntData(src_value);
  ASSERT_SUCCESS(clEnqueueWriteBuffer(command_queue, src_buffer, CL_FALSE, 0,
                                      copy_size, src_value.data(), 0, nullptr,
                                      nullptr));

  // Enqueue the command buffer containing the copy.
  ASSERT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Do a blocking read on the buffers.
  byte_vector result(copy_size, 0x0);
  ASSERT_SUCCESS(clEnqueueReadBuffer(command_queue, dst_buffer, CL_TRUE, 0,
                                     copy_size, result.data(), 0, nullptr,
                                     nullptr));

  // Check the copy was successful i.e. that it happened after the non-blocking
  // write.
  ASSERT_EQ(result, src_value);
}

struct CommandCopyBufferRectErrorTest : CommandCopyBufferRectTest {
  Position src_origin{0, 0, 0};
  Position dst_origin{0, 0, 0};
  Region region{16, 16, 1};
  size_t row_pitch = 16;
  size_t slice_pitch = 256;
};

TEST_F(CommandCopyBufferRectErrorTest, InvalidCommandBuffer) {
  ASSERT_EQ_ERRCODE(
      CL_INVALID_COMMAND_BUFFER_KHR,
      clCommandCopyBufferRectKHR(
          nullptr, nullptr, src_buffer, dst_buffer, src_origin.data(),
          dst_origin.data(), region.data(), row_pitch, slice_pitch, row_pitch,
          slice_pitch, 0, nullptr, nullptr, nullptr));

  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));
  ASSERT_EQ_ERRCODE(
      CL_INVALID_OPERATION,
      clCommandCopyBufferRectKHR(
          command_buffer, nullptr, src_buffer, dst_buffer, src_origin.data(),
          dst_origin.data(), region.data(), row_pitch, slice_pitch, row_pitch,
          slice_pitch, 0, nullptr, nullptr, nullptr));
}

TEST_F(CommandCopyBufferRectErrorTest, InvalidContext) {
  cl_int errcode;
  cl_context other_context =
      clCreateContext(nullptr, 1, &device, nullptr, nullptr, &errcode);
  EXPECT_TRUE(other_context);
  EXPECT_SUCCESS(errcode);

  cl_mem other_buffer =
      clCreateBuffer(other_context, 0, buffer_size, nullptr, &errcode);
  EXPECT_TRUE(other_buffer);
  EXPECT_SUCCESS(errcode);

  EXPECT_EQ_ERRCODE(
      CL_INVALID_CONTEXT,
      clCommandCopyBufferRectKHR(
          command_buffer, nullptr, src_buffer, other_buffer, src_origin.data(),
          dst_origin.data(), region.data(), row_pitch, slice_pitch, row_pitch,
          slice_pitch, 0, nullptr, nullptr, nullptr));

  EXPECT_SUCCESS(clReleaseMemObject(other_buffer));
  EXPECT_SUCCESS(clReleaseContext(other_context));
}

TEST_F(CommandCopyBufferRectErrorTest, InvalidMemObject) {
  ASSERT_EQ_ERRCODE(
      CL_INVALID_MEM_OBJECT,
      clCommandCopyBufferRectKHR(
          command_buffer, nullptr, nullptr, dst_buffer, src_origin.data(),
          dst_origin.data(), region.data(), row_pitch, slice_pitch, row_pitch,
          slice_pitch, 0, nullptr, nullptr, nullptr));

  ASSERT_EQ_ERRCODE(
      CL_INVALID_MEM_OBJECT,
      clCommandCopyBufferRectKHR(
          command_buffer, nullptr, src_buffer, nullptr, src_origin.data(),
          dst_origin.data(), region.data(), row_pitch, slice_pitch, row_pitch,
          slice_pitch, 0, nullptr, nullptr, nullptr));
}

TEST_F(CommandCopyBufferRectErrorTest, InvalidOrigin) {
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clCommandCopyBufferRectKHR(
          command_buffer, nullptr, src_buffer, dst_buffer, nullptr,
          dst_origin.data(), region.data(), row_pitch, slice_pitch, row_pitch,
          slice_pitch, 0, nullptr, nullptr, nullptr));

  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clCommandCopyBufferRectKHR(
          command_buffer, nullptr, src_buffer, dst_buffer, src_origin.data(),
          nullptr, region.data(), row_pitch, slice_pitch, row_pitch,
          slice_pitch, 0, nullptr, nullptr, nullptr));
}

TEST_F(CommandCopyBufferRectErrorTest, InvalidRegion) {
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clCommandCopyBufferRectKHR(
          command_buffer, nullptr, src_buffer, dst_buffer, src_origin.data(),
          dst_origin.data(), nullptr, row_pitch, slice_pitch, row_pitch,
          slice_pitch, 0, nullptr, nullptr, nullptr));
}

TEST_F(CommandCopyBufferRectErrorTest, OutOfBounds) {
  src_origin = {64, 64, 64};
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clCommandCopyBufferRectKHR(
          command_buffer, nullptr, src_buffer, dst_buffer, src_origin.data(),
          dst_origin.data(), region.data(), row_pitch, slice_pitch, row_pitch,
          slice_pitch, 0, nullptr, nullptr, nullptr));
  src_origin = {0, 0, 0};

  dst_origin = {64, 64, 64};
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clCommandCopyBufferRectKHR(
          command_buffer, nullptr, src_buffer, dst_buffer, src_origin.data(),
          dst_origin.data(), region.data(), row_pitch, slice_pitch, row_pitch,
          slice_pitch, 0, nullptr, nullptr, nullptr));
  dst_origin = {0, 0, 0};

  region = {64, 64, 64};
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clCommandCopyBufferRectKHR(
          command_buffer, nullptr, src_buffer, dst_buffer, src_origin.data(),
          dst_origin.data(), region.data(), row_pitch, slice_pitch, row_pitch,
          slice_pitch, 0, nullptr, nullptr, nullptr));
}

TEST_F(CommandCopyBufferRectErrorTest, InvalidRegionElementZero) {
  region[0] = 0;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clCommandCopyBufferRectKHR(
          command_buffer, nullptr, src_buffer, dst_buffer, src_origin.data(),
          dst_origin.data(), region.data(), row_pitch, slice_pitch, row_pitch,
          slice_pitch, 0, nullptr, nullptr, nullptr));
  region[0] = 16;

  region[1] = 0;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clCommandCopyBufferRectKHR(
          command_buffer, nullptr, src_buffer, dst_buffer, src_origin.data(),
          dst_origin.data(), region.data(), row_pitch, slice_pitch, row_pitch,
          slice_pitch, 0, nullptr, nullptr, nullptr));
  region[1] = 16;

  region[2] = 0;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clCommandCopyBufferRectKHR(
          command_buffer, nullptr, src_buffer, dst_buffer, src_origin.data(),
          dst_origin.data(), region.data(), row_pitch, slice_pitch, row_pitch,
          slice_pitch, 0, nullptr, nullptr, nullptr));
  region[2] = 16;
}

TEST_F(CommandCopyBufferRectErrorTest, InvalidRowPitch) {
  size_t src_row_pitch = 1;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clCommandCopyBufferRectKHR(
          command_buffer, nullptr, src_buffer, dst_buffer, src_origin.data(),
          dst_origin.data(), region.data(), src_row_pitch, slice_pitch,
          row_pitch, slice_pitch, 0, nullptr, nullptr, nullptr));

  src_row_pitch = region[0] - 1;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clCommandCopyBufferRectKHR(
          command_buffer, nullptr, src_buffer, dst_buffer, src_origin.data(),
          dst_origin.data(), region.data(), src_row_pitch, slice_pitch,
          row_pitch, slice_pitch, 0, nullptr, nullptr, nullptr));

  size_t dst_row_pitch = 1;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clCommandCopyBufferRectKHR(
          command_buffer, nullptr, src_buffer, dst_buffer, src_origin.data(),
          dst_origin.data(), region.data(), row_pitch, slice_pitch,
          dst_row_pitch, slice_pitch, 0, nullptr, nullptr, nullptr));

  dst_row_pitch = region[0] - 1;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clCommandCopyBufferRectKHR(
          command_buffer, nullptr, src_buffer, dst_buffer, src_origin.data(),
          dst_origin.data(), region.data(), row_pitch, slice_pitch,
          dst_row_pitch, slice_pitch, 0, nullptr, nullptr, nullptr));
}

TEST_F(CommandCopyBufferRectErrorTest, InvalidSlicePitch) {
  size_t src_slice_pitch = 1;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clCommandCopyBufferRectKHR(
          command_buffer, nullptr, src_buffer, dst_buffer, src_origin.data(),
          dst_origin.data(), region.data(), row_pitch, src_slice_pitch,
          row_pitch, slice_pitch, 0, nullptr, nullptr, nullptr));

  src_slice_pitch = (region[1] * row_pitch) - 1;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clCommandCopyBufferRectKHR(
          command_buffer, nullptr, src_buffer, dst_buffer, src_origin.data(),
          dst_origin.data(), region.data(), row_pitch, src_slice_pitch,
          row_pitch, slice_pitch, 0, nullptr, nullptr, nullptr));

  size_t dst_slice_pitch = 1;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clCommandCopyBufferRectKHR(
          command_buffer, nullptr, src_buffer, dst_buffer, src_origin.data(),
          dst_origin.data(), region.data(), row_pitch, slice_pitch, row_pitch,
          dst_slice_pitch, 0, nullptr, nullptr, nullptr));

  dst_slice_pitch = (region[1] * row_pitch) - 1;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clCommandCopyBufferRectKHR(
          command_buffer, nullptr, src_buffer, dst_buffer, src_origin.data(),
          dst_origin.data(), region.data(), row_pitch, slice_pitch, row_pitch,
          dst_slice_pitch, 0, nullptr, nullptr, nullptr));
}

TEST_F(CommandCopyBufferRectErrorTest, InvalidSameBufferPitchMismatch) {
  const size_t dst_row_pitch = row_pitch - 1;

  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clCommandCopyBufferRectKHR(
          command_buffer, nullptr, src_buffer, dst_buffer, src_origin.data(),
          dst_origin.data(), region.data(), row_pitch, slice_pitch,
          dst_row_pitch, slice_pitch, 0, nullptr, nullptr, nullptr));

  const size_t dst_slice_pitch = slice_pitch - 1;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clCommandCopyBufferRectKHR(
          command_buffer, nullptr, src_buffer, dst_buffer, src_origin.data(),
          dst_origin.data(), region.data(), row_pitch, slice_pitch, row_pitch,
          dst_slice_pitch, 0, nullptr, nullptr, nullptr));
}

TEST_F(CommandCopyBufferRectErrorTest, InvalidSyncPoints) {
  ASSERT_EQ_ERRCODE(
      CL_INVALID_SYNC_POINT_WAIT_LIST_KHR,
      clCommandCopyBufferRectKHR(
          command_buffer, nullptr, src_buffer, dst_buffer, src_origin.data(),
          dst_origin.data(), region.data(), row_pitch, slice_pitch, row_pitch,
          slice_pitch, 1, nullptr, nullptr, nullptr));

  cl_sync_point_khr sync_point;
  ASSERT_EQ_ERRCODE(
      CL_INVALID_SYNC_POINT_WAIT_LIST_KHR,
      clCommandCopyBufferRectKHR(
          command_buffer, nullptr, src_buffer, dst_buffer, src_origin.data(),
          dst_origin.data(), region.data(), row_pitch, slice_pitch, row_pitch,
          slice_pitch, 0, &sync_point, nullptr, nullptr));
}
