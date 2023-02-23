// Copyright (C) Codeplay Software Limited. All Rights Reserved.
#include "cl_khr_command_buffer.h"

namespace {
struct test_parameters {
  size_t pattern_size;
  size_t offset;
  size_t size;
  size_t buffer_size;
};

// Does the host side equivalent of a clFill operation.
static void fillBuffer(std::vector<cl_uchar> &input, const cl_uchar *pattern,
                       size_t pattern_size, size_t size, size_t offset) {
  auto start = std::next(std::begin(input), offset);
  auto end = start + size;
  for (auto current_position = start; current_position != end;
       current_position += pattern_size) {
    std::memmove(&*current_position, pattern, pattern_size);
  }
}
}  // namespace

struct CommandFillBufferKHRTest : cl_khr_command_buffer_Test {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(cl_khr_command_buffer_Test::SetUp());

    cl_int error = CL_SUCCESS;
    buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, buffer_size, nullptr,
                            &error);
    ASSERT_SUCCESS(error);

    other_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE, buffer_size,
                                  nullptr, &error);
    ASSERT_SUCCESS(error);

    command_buffer =
        clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
    ASSERT_SUCCESS(error);
  }
  void TearDown() override {
    if (command_buffer) {
      EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
    }
    if (buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(buffer));
    }

    if (other_buffer) {
      EXPECT_SUCCESS(clReleaseMemObject(other_buffer));
    }

    cl_khr_command_buffer_Test::TearDown();
  }
  cl_mem buffer = nullptr;
  cl_mem other_buffer = nullptr;
  cl_command_buffer_khr command_buffer = nullptr;

  static constexpr size_t buffer_size = 256;
};

#if __cplusplus < 201703L
// C++14 and below require static member definitions be defined outside the
// class even if they are initialized inline. TODO: Remove condition once we no
// longer support earlier than LLVM 15.
constexpr size_t CommandFillBufferKHRTest::buffer_size;
#endif

class CommandFillBufferKHRParamTest
    : public cl_khr_command_buffer_Test,
      public ::testing::WithParamInterface<test_parameters> {};

// Checks we can fill a buffer with an arbitrary bit pattern for various pattern
// sizes, fill sizes and offset.
TEST_P(CommandFillBufferKHRParamTest, FillBufferWithRandomPattern) {
  // Unpack the parameters.
  const size_t pattern_size = GetParam().pattern_size;
  const size_t offset = GetParam().offset;
  const size_t size = GetParam().size;
  const size_t buffer_size = GetParam().buffer_size;

  // Create a pattern of the given pattern size.
  std::vector<cl_uchar> pattern(pattern_size, 0x0);
  auto &generator = ucl::Environment::instance->GetInputGenerator();
  generator.GenerateIntData(pattern);

  // Create the command buffer.
  cl_int error = CL_SUCCESS;
  cl_command_buffer_khr command_buffer =
      clCreateCommandBufferKHR(1, &command_queue, nullptr, &error);
  EXPECT_SUCCESS(error);

  // Create a buffer to fill.
  cl_mem buffer =
      clCreateBuffer(context, CL_MEM_READ_WRITE, buffer_size, nullptr, &error);
  EXPECT_SUCCESS(error);

  // Fill the buffer with some initial random values.
  std::vector<cl_uchar> initial_value(buffer_size, 0x0);
  generator.GenerateIntData(initial_value);
  EXPECT_SUCCESS(clEnqueueWriteBuffer(command_queue, buffer, CL_TRUE, 0,
                                      buffer_size, initial_value.data(), 0,
                                      nullptr, nullptr));

  // Add the fill command to the buffer and finalize it.
  EXPECT_SUCCESS(clCommandFillBufferKHR(command_buffer, nullptr, buffer,
                                        pattern.data(), pattern_size, offset,
                                        size, 0, nullptr, nullptr, nullptr));
  EXPECT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Enqueue the command buffer.
  EXPECT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Do a blocking read.
  std::vector<cl_uchar> result(buffer_size, 0x42);
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, buffer, CL_TRUE, 0,
                                     buffer_size, result.data(), 0, nullptr,
                                     nullptr));

  // Do a host side equivalent of the what OpenCL did.
  fillBuffer(initial_value, pattern.data(), pattern_size, size, offset);

  // Check the results are equal.
  EXPECT_EQ(initial_value, result);

  // Cleanup.
  EXPECT_SUCCESS(clReleaseCommandBufferKHR(command_buffer));
  EXPECT_SUCCESS(clReleaseMemObject(buffer));
}

// The pattern size must be one of { 1, 2, 4, 8, 16, 32, 64, 128 }.
// Choose the other parameters appropriately such that between them we get good
// testing coverage and catch some edge cases
INSTANTIATE_TEST_SUITE_P(ValidPatternSizes, CommandFillBufferKHRParamTest,
                         ::testing::Values(test_parameters{1, 16, 32, 64},
                                           test_parameters{2, 0, 16, 64},
                                           test_parameters{4, 48, 4, 64},
                                           test_parameters{8, 48, 16, 64},
                                           test_parameters{16, 0, 64, 64},
                                           test_parameters{32, 32, 32, 64},
                                           test_parameters{64, 0, 64, 128},
                                           test_parameters{128, 0, 128, 128}));

TEST_F(CommandFillBufferKHRTest, Sync) {
  cl_uchar pattern[] = {0xde, 0xad, 0xbe, 0xaf};
  const size_t pattern_size = sizeof(pattern);
  const size_t size = 32;
  const size_t offset = 0;

  cl_sync_point_khr sync_points[2] = {
      std::numeric_limits<cl_sync_point_khr>::max(),
      std::numeric_limits<cl_sync_point_khr>::max()};

  ASSERT_SUCCESS(clCommandFillBufferKHR(command_buffer, nullptr, buffer,
                                        &pattern, pattern_size, offset, size, 0,
                                        nullptr, &sync_points[0], nullptr));

  ASSERT_NE(sync_points[0], std::numeric_limits<cl_sync_point_khr>::max());

  ASSERT_SUCCESS(clCommandFillBufferKHR(command_buffer, nullptr, buffer,
                                        &pattern, pattern_size, offset, size, 0,
                                        nullptr, &sync_points[1], nullptr));

  ASSERT_NE(sync_points[1], std::numeric_limits<cl_sync_point_khr>::max());

  ASSERT_SUCCESS(clCommandFillBufferKHR(command_buffer, nullptr, buffer,
                                        &pattern, pattern_size, offset, size, 2,
                                        sync_points, nullptr, nullptr));
}

// Tests we can reuse the pattern memory after putting the command in the
// buffer.
TEST_F(CommandFillBufferKHRTest, ReusePatternMemory) {
  // Fill the buffer with some initial random values.
  std::vector<cl_uchar> initial_value(buffer_size, 0x0);
  auto &generator = ucl::Environment::instance->GetInputGenerator();
  generator.GenerateIntData(initial_value);
  ASSERT_SUCCESS(clEnqueueWriteBuffer(command_queue, buffer, CL_TRUE, 0,
                                      buffer_size, initial_value.data(), 0,
                                      nullptr, nullptr));
  // Create a pattern. Since we tested all pattern sizes above, here we just
  // pick a simple 64 bit pattern to test.
  cl_uchar pattern[] = {0xde, 0xad, 0xbe, 0xaf};
  const size_t pattern_size = sizeof(pattern);
  const size_t size = 32;
  const size_t offset = 4;

  // Do a host side equivalent of the what OpenCL will do.
  fillBuffer(initial_value, pattern, pattern_size, size, offset);

  // Add the fill command to the buffer.
  ASSERT_SUCCESS(clCommandFillBufferKHR(command_buffer, nullptr, buffer,
                                        &pattern, pattern_size, offset, size, 0,
                                        nullptr, nullptr, nullptr));

  // Finalize the buffer.
  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Update the pattern.
  pattern[0] = 0x99;
  pattern[1] = 0x66;
  pattern[2] = 0x33;
  pattern[3] = 0x11;

  // Enqueue the command buffer.
  ASSERT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Do a blocking read.
  std::vector<cl_uchar> result(buffer_size, 0x42);
  ASSERT_SUCCESS(clEnqueueReadBuffer(command_queue, buffer, CL_TRUE, 0,
                                     buffer_size, result.data(), 0, nullptr,
                                     nullptr));

  // Check the results are equal.
  ASSERT_EQ(initial_value, result);
}

// Tests we can successfully execute a fill command in a buffer with other
// commands.
TEST_F(CommandFillBufferKHRTest, FillThenCopy) {
  // Zero out the output buffer so we know its state.
  const cl_uchar zero = 0x0;
  ASSERT_SUCCESS(clEnqueueFillBuffer(command_queue, other_buffer, &zero,
                                     sizeof(cl_uchar), 0, buffer_size, 0,
                                     nullptr, nullptr));
  ASSERT_SUCCESS(clFinish(command_queue));

  // Fill the buffer with some initial random values.
  std::vector<cl_uchar> initial_value(buffer_size, 0x0);
  auto &generator = ucl::Environment::instance->GetInputGenerator();
  generator.GenerateIntData(initial_value);
  ASSERT_SUCCESS(clEnqueueWriteBuffer(command_queue, buffer, CL_TRUE, 0,
                                      buffer_size, initial_value.data(), 0,
                                      nullptr, nullptr));

  // Create a pattern. Since we tested all pattern sizes above, here we just
  // pick a simple 64 bit pattern to test.
  const cl_uchar pattern[] = {0xde, 0xad, 0xbe, 0xaf};
  const size_t pattern_size = sizeof(pattern);
  const size_t size = 32;
  const size_t offset = 4;

  // Add the fill command to the buffer.
  ASSERT_SUCCESS(clCommandFillBufferKHR(command_buffer, nullptr, buffer,
                                        &pattern, pattern_size, offset, size, 0,
                                        nullptr, nullptr, nullptr));

  // Add the copy command to copy to the output buffer.
  ASSERT_SUCCESS(clCommandCopyBufferKHR(command_buffer, nullptr, buffer,
                                        other_buffer, 0, 0, buffer_size, 0,
                                        nullptr, nullptr, nullptr));
  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Enqueue the command buffer.
  ASSERT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Do a blocking read.
  std::vector<cl_uchar> result(buffer_size, 0x42);
  ASSERT_SUCCESS(clEnqueueReadBuffer(command_queue, other_buffer, CL_TRUE, 0,
                                     buffer_size, result.data(), 0, nullptr,
                                     nullptr));

  // Do a host side equivalent of the what OpenCL did.
  fillBuffer(initial_value, pattern, pattern_size, size, offset);

  // Check the results are equal.
  ASSERT_EQ(initial_value, result);
}

// Tests we can enqueue multiple non overlapping fills into a command buffer
// that act on the same buffer.
TEST_F(CommandFillBufferKHRTest, MultipleFillSameBuffer) {
  // Fill the buffer with some initial random values.
  std::vector<cl_uchar> initial_value(buffer_size, 0x0);
  auto &generator = ucl::Environment::instance->GetInputGenerator();
  generator.GenerateIntData(initial_value);
  ASSERT_SUCCESS(clEnqueueWriteBuffer(command_queue, buffer, CL_TRUE, 0,
                                      buffer_size, initial_value.data(), 0,
                                      nullptr, nullptr));
  // Create the first pattern.
  const cl_uchar pattern_a[] = {0xde, 0xad, 0xbe, 0xaf};
  const size_t pattern_a_size = sizeof(pattern_a);
  const size_t size_a = 32;
  const size_t offset_a = 4;

  // Create the second pattern.
  const cl_uchar pattern_b[] = {0xba, 0xde};
  const size_t pattern_b_size = sizeof(pattern_b);
  const size_t size_b = 16;
  const size_t offset_b = 128;

  // Add the fill commands to the buffer.
  EXPECT_SUCCESS(clCommandFillBufferKHR(command_buffer, nullptr, buffer,
                                        &pattern_a, pattern_a_size, offset_a,
                                        size_a, 0, nullptr, nullptr, nullptr));
  ASSERT_SUCCESS(clCommandFillBufferKHR(command_buffer, nullptr, buffer,
                                        &pattern_b, pattern_b_size, offset_b,
                                        size_b, 0, nullptr, nullptr, nullptr));

  // Finalize the buffer.
  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Enqueue the command buffer.
  ASSERT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Do a blocking read.
  std::vector<cl_uchar> result(buffer_size, 0x42);
  ASSERT_SUCCESS(clEnqueueReadBuffer(command_queue, buffer, CL_TRUE, 0,
                                     buffer_size, result.data(), 0, nullptr,
                                     nullptr));

  // Do a host side equivalent of the what OpenCL did.
  fillBuffer(initial_value, pattern_a, pattern_a_size, size_a, offset_a);
  fillBuffer(initial_value, pattern_b, pattern_b_size, size_b, offset_b);

  // Check the results are equal.
  ASSERT_EQ(initial_value, result);
}

// Tests we can enqueue multiple overlapping fills into a command buffer
// that act on the same buffer.
TEST_F(CommandFillBufferKHRTest, MultipleFillSameBufferOverlapping) {
  // Fill the buffer with some initial random values.
  std::vector<cl_uchar> initial_value(buffer_size, 0x0);
  auto &generator = ucl::Environment::instance->GetInputGenerator();
  generator.GenerateIntData(initial_value);
  ASSERT_SUCCESS(clEnqueueWriteBuffer(command_queue, buffer, CL_TRUE, 0,
                                      buffer_size, initial_value.data(), 0,
                                      nullptr, nullptr));
  // Create the first pattern.
  const cl_uchar pattern_a[] = {0xde, 0xad, 0xbe, 0xaf};
  const size_t pattern_a_size = sizeof(pattern_a);
  const size_t size_a = 32;
  const size_t offset_a = 4;

  // Create the second pattern choosing the size and offset such that the fill
  // overlaps the first.
  const cl_uchar pattern_b[] = {0xba, 0xde};
  const size_t pattern_b_size = sizeof(pattern_b);
  const size_t size_b = 16;
  const size_t offset_b = 18;

  // Add the fill commands to the buffer.
  ASSERT_SUCCESS(clCommandFillBufferKHR(command_buffer, nullptr, buffer,
                                        &pattern_a, pattern_a_size, offset_a,
                                        size_a, 0, nullptr, nullptr, nullptr));
  ASSERT_SUCCESS(clCommandFillBufferKHR(command_buffer, nullptr, buffer,
                                        &pattern_b, pattern_b_size, offset_b,
                                        size_b, 0, nullptr, nullptr, nullptr));

  // Finalize the buffer.
  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Enqueue the command buffer.
  ASSERT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Do a blocking read.
  std::vector<cl_uchar> result(buffer_size, 0x42);
  EXPECT_SUCCESS(clEnqueueReadBuffer(command_queue, buffer, CL_TRUE, 0,
                                     buffer_size, result.data(), 0, nullptr,
                                     nullptr));

  // Do a host side equivalent of the what OpenCL did.
  fillBuffer(initial_value, pattern_a, pattern_a_size, size_a, offset_a);
  fillBuffer(initial_value, pattern_b, pattern_b_size, size_b, offset_b);

  // Check the results are equal.
  ASSERT_EQ(initial_value, result);
}

// Tests we can enqueue multiple fills into a command buffer that act on
// different buffers.
TEST_F(CommandFillBufferKHRTest, MultipleFillDifferentBuffers) {
  // Fill the buffers with some initial random values.
  std::vector<cl_uchar> initial_value_a(buffer_size, 0x0);
  auto &generator = ucl::Environment::instance->GetInputGenerator();
  generator.GenerateIntData(initial_value_a);
  ASSERT_SUCCESS(clEnqueueWriteBuffer(command_queue, buffer, CL_TRUE, 0,
                                      buffer_size, initial_value_a.data(), 0,
                                      nullptr, nullptr));

  std::vector<cl_uchar> initial_value_b(buffer_size, 0x0);
  generator.GenerateIntData(initial_value_b);
  ASSERT_SUCCESS(clEnqueueWriteBuffer(command_queue, other_buffer, CL_TRUE, 0,
                                      buffer_size, initial_value_b.data(), 0,
                                      nullptr, nullptr));
  // Create the first pattern.
  const cl_uchar pattern_a[] = {0xde, 0xad, 0xbe, 0xaf};
  const size_t pattern_a_size = sizeof(pattern_a);
  const size_t size_a = 32;
  const size_t offset_a = 4;

  // Create the second pattern.
  const cl_uchar pattern_b[] = {0xba, 0xde};
  const size_t pattern_b_size = sizeof(pattern_b);
  const size_t size_b = 16;
  const size_t offset_b = 18;

  // Add the fill commands to the buffers.
  ASSERT_SUCCESS(clCommandFillBufferKHR(command_buffer, nullptr, buffer,
                                        &pattern_a, pattern_a_size, offset_a,
                                        size_a, 0, nullptr, nullptr, nullptr));
  ASSERT_SUCCESS(clCommandFillBufferKHR(command_buffer, nullptr, other_buffer,
                                        &pattern_b, pattern_b_size, offset_b,
                                        size_b, 0, nullptr, nullptr, nullptr));

  // Finalize the buffer.
  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));

  // Enqueue the command buffer.
  ASSERT_SUCCESS(clEnqueueCommandBufferKHR(0, nullptr, command_buffer, 0,
                                           nullptr, nullptr));

  // Do a blocking read of the buffers.
  std::vector<cl_uchar> result_a(buffer_size, 0x42);
  ASSERT_SUCCESS(clEnqueueReadBuffer(command_queue, buffer, CL_TRUE, 0,
                                     buffer_size, result_a.data(), 0, nullptr,
                                     nullptr));

  std::vector<cl_uchar> result_b(buffer_size, 0x42);
  ASSERT_SUCCESS(clEnqueueReadBuffer(command_queue, other_buffer, CL_TRUE, 0,
                                     buffer_size, result_b.data(), 0, nullptr,
                                     nullptr));

  // Do a host side equivalent of the what OpenCL did.
  fillBuffer(initial_value_a, pattern_a, pattern_a_size, size_a, offset_a);
  fillBuffer(initial_value_b, pattern_b, pattern_b_size, size_b, offset_b);

  // Check the results are equal.
  ASSERT_EQ(initial_value_a, result_a);
  ASSERT_EQ(initial_value_b, result_b);
}

TEST_F(CommandFillBufferKHRTest, InvalidCommandBuffer) {
  const cl_uchar pattern[] = {0xab, 0xef};
  const size_t pattern_size = sizeof(pattern);
  const size_t size = 16;
  const size_t offset = 0;

  ASSERT_EQ_ERRCODE(
      CL_INVALID_COMMAND_BUFFER_KHR,
      clCommandFillBufferKHR(nullptr, nullptr, buffer, &pattern, pattern_size,
                             offset, size, 0, nullptr, nullptr, nullptr));

  ASSERT_SUCCESS(clFinalizeCommandBufferKHR(command_buffer));
  ASSERT_EQ_ERRCODE(CL_INVALID_OPERATION,
                    clCommandFillBufferKHR(command_buffer, nullptr, buffer,
                                           &pattern, pattern_size, offset, size,
                                           0, nullptr, nullptr, nullptr));
}

TEST_F(CommandFillBufferKHRTest, InvalidMemObject) {
  const cl_uchar pattern[] = {0xab, 0xef};
  const size_t pattern_size = sizeof(pattern);
  const size_t size = 16;
  const size_t offset = 0;

  ASSERT_EQ_ERRCODE(CL_INVALID_MEM_OBJECT,
                    clCommandFillBufferKHR(command_buffer, nullptr, nullptr,
                                           &pattern, pattern_size, offset, size,
                                           0, nullptr, nullptr, nullptr));
}

TEST_F(CommandFillBufferKHRTest, InvalidContext) {
  cl_int errcode;
  cl_context other_context =
      clCreateContext(nullptr, 1, &device, nullptr, nullptr, &errcode);
  EXPECT_TRUE(other_context);
  EXPECT_SUCCESS(errcode);

  const cl_uchar pattern[] = {0xab, 0xef};
  const size_t pattern_size = sizeof(pattern);
  const size_t size = 16;
  const size_t offset = 0;

  cl_mem other_buffer =
      clCreateBuffer(other_context, 0, size, nullptr, &errcode);
  EXPECT_TRUE(other_buffer);
  EXPECT_SUCCESS(errcode);

  EXPECT_EQ_ERRCODE(
      CL_INVALID_CONTEXT,
      clCommandFillBufferKHR(command_buffer, nullptr, other_buffer, &pattern,
                             pattern_size, offset, size, 0, nullptr, nullptr,
                             nullptr));

  EXPECT_SUCCESS(clReleaseMemObject(other_buffer));
  EXPECT_SUCCESS(clReleaseContext(other_context));
}

TEST_F(CommandFillBufferKHRTest, InvalidOffset) {
  const cl_uchar pattern[] = {0xab, 0xef};
  const size_t pattern_size = sizeof(pattern);
  const size_t size = 16;

  EXPECT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clCommandFillBufferKHR(command_buffer, nullptr, buffer,
                                           &pattern, pattern_size, size + 1,
                                           size, 0, nullptr, nullptr, nullptr));

  const size_t half_size = size / 2;
  EXPECT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clCommandFillBufferKHR(command_buffer, nullptr, buffer, &pattern,
                             pattern_size, half_size + 1, half_size, 0, nullptr,
                             nullptr, nullptr));
}

TEST_F(CommandFillBufferKHRTest, InvalidPattern) {
  const cl_uchar pattern[] = {0xab, 0xef, 0xcd, 0x34};
  const size_t pattern_size = sizeof(pattern);
  const size_t size = 16;

  EXPECT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clCommandFillBufferKHR(command_buffer, nullptr, buffer,
                                           nullptr, pattern_size, size, size, 0,
                                           nullptr, nullptr, nullptr));

  EXPECT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clCommandFillBufferKHR(command_buffer, nullptr, buffer, &pattern, 0, 0,
                             size, 0, nullptr, nullptr, nullptr));

  EXPECT_EQ_ERRCODE(
      CL_INVALID_VALUE,
      clCommandFillBufferKHR(command_buffer, nullptr, buffer, &pattern, 3, 0,
                             size, 0, nullptr, nullptr, nullptr));
  EXPECT_EQ_ERRCODE(CL_INVALID_VALUE,
                    clCommandFillBufferKHR(command_buffer, nullptr, buffer,
                                           &pattern, pattern_size, 6, size, 0,
                                           nullptr, nullptr, nullptr));
}

TEST_F(CommandFillBufferKHRTest, InvalidSyncPoints) {
  const cl_uchar pattern[] = {0xab, 0xef, 0xcd, 0x34};
  const size_t pattern_size = sizeof(pattern);
  const size_t size = 16;

  ASSERT_EQ_ERRCODE(CL_INVALID_SYNC_POINT_WAIT_LIST_KHR,
                    clCommandFillBufferKHR(command_buffer, nullptr, buffer,
                                           &pattern, pattern_size, 0, size, 1,
                                           nullptr, nullptr, nullptr));

  cl_sync_point_khr sync_point;
  ASSERT_EQ_ERRCODE(CL_INVALID_SYNC_POINT_WAIT_LIST_KHR,
                    clCommandFillBufferKHR(command_buffer, nullptr, buffer,
                                           &pattern, pattern_size, 0, size, 0,
                                           &sync_point, nullptr, nullptr));
}
