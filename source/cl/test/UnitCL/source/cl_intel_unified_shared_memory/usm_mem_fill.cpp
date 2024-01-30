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

#include <Common.h>

#include "cl_intel_unified_shared_memory.h"

namespace {
// Number of Elements of Type 'TypeParam' allocated for in USM
// TYPED_TESTs
const cl_uint elements = 8;

template <typename T>
struct USMMemFillTest : public cl_intel_unified_shared_memory_Test {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(cl_intel_unified_shared_memory_Test::SetUp());

    initPointers(bytes, align);
    cl_int err;
    queue = clCreateCommandQueue(context, device, 0, &err);
    ASSERT_TRUE(queue != nullptr);
    ASSERT_SUCCESS(err);
  }

  void TearDown() override {
    if (queue) {
      EXPECT_SUCCESS(clReleaseCommandQueue(queue));
    }

    cl_intel_unified_shared_memory_Test::TearDown();
  }

  void validateResults(T *ptr);

  static const size_t bytes = sizeof(T) * elements;
  static const cl_uint align = sizeof(T);

  cl_command_queue queue = nullptr;
};

#define SCALAR_PATTERN(T)                        \
  template <>                                    \
  struct test_patterns<T> {                      \
    constexpr static T zero_pattern = 0;         \
    constexpr static T pattern1 = 42;            \
    constexpr static T pattern2 = 0xA;           \
    constexpr static const char *as_string = #T; \
  };

#define VECTOR2_PATTERN(T)                       \
  template <>                                    \
  struct test_patterns<T> {                      \
    constexpr static T zero_pattern = {{0, 0}};  \
    constexpr static T pattern1 = {{42, 43}};    \
    constexpr static T pattern2 = {{0xA, 0xB}};  \
    constexpr static const char *as_string = #T; \
  };

#define VECTOR4_PATTERN(T)                                \
  template <>                                             \
  struct test_patterns<T> {                               \
    constexpr static T zero_pattern = {{0, 0, 0, 0}};     \
    constexpr static T pattern1 = {{42, 43, 44, 45}};     \
    constexpr static T pattern2 = {{0xA, 0xB, 0xC, 0xD}}; \
    constexpr static const char *as_string = #T;          \
  };

#define VECTOR8_PATTERN(T)                                            \
  template <>                                                         \
  struct test_patterns<T> {                                           \
    constexpr static T zero_pattern = {{0, 0, 0, 0, 0, 0, 0}};        \
    constexpr static T pattern1 = {{42, 43, 44, 45, 46, 47, 48, 49}}; \
    constexpr static T pattern2 = {                                   \
        {0xA, 0xB, 0xC, 0xD, 0xE, 0xF, 0x10, 0x11}};                  \
    constexpr static const char *as_string = #T;                      \
  };

#define VECTOR16_PATTERN(T)                                                   \
  template <>                                                                 \
  struct test_patterns<T> {                                                   \
    constexpr static T zero_pattern = {                                       \
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};                    \
    constexpr static T pattern1 = {                                           \
        {42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57}};    \
    constexpr static T pattern2 = {{0xA, 0xB, 0xC, 0xD, 0xE, 0xF, 0x10, 0x11, \
                                    0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, \
                                    0x19}};                                   \
    constexpr static const char *as_string = #T;                              \
  };

template <typename T>
struct test_patterns {};

#if !defined(__clang_analyzer__)
SCALAR_PATTERN(cl_char)
SCALAR_PATTERN(cl_uchar)
SCALAR_PATTERN(cl_short)
SCALAR_PATTERN(cl_ushort)
SCALAR_PATTERN(cl_int)
SCALAR_PATTERN(cl_uint)
SCALAR_PATTERN(cl_long)
SCALAR_PATTERN(cl_ulong)
SCALAR_PATTERN(cl_float)

VECTOR2_PATTERN(cl_char2)
VECTOR2_PATTERN(cl_uchar2)
VECTOR2_PATTERN(cl_short2)
VECTOR2_PATTERN(cl_ushort2)
VECTOR2_PATTERN(cl_int2)
VECTOR2_PATTERN(cl_uint2)
VECTOR2_PATTERN(cl_long2)
VECTOR2_PATTERN(cl_ulong2)
VECTOR2_PATTERN(cl_float2)

VECTOR4_PATTERN(cl_char4)
VECTOR4_PATTERN(cl_uchar4)
VECTOR4_PATTERN(cl_short4)
VECTOR4_PATTERN(cl_ushort4)
VECTOR4_PATTERN(cl_int4)
VECTOR4_PATTERN(cl_uint4)
VECTOR4_PATTERN(cl_long4)
VECTOR4_PATTERN(cl_ulong4)
VECTOR4_PATTERN(cl_float4)

VECTOR8_PATTERN(cl_char8)
VECTOR8_PATTERN(cl_uchar8)
VECTOR8_PATTERN(cl_short8)
VECTOR8_PATTERN(cl_ushort8)
VECTOR8_PATTERN(cl_int8)
VECTOR8_PATTERN(cl_uint8)
VECTOR8_PATTERN(cl_long8)
VECTOR8_PATTERN(cl_ulong8)
VECTOR8_PATTERN(cl_float8)

VECTOR16_PATTERN(cl_char16)
VECTOR16_PATTERN(cl_uchar16)
VECTOR16_PATTERN(cl_short16)
VECTOR16_PATTERN(cl_ushort16)
VECTOR16_PATTERN(cl_int16)
VECTOR16_PATTERN(cl_uint16)
VECTOR16_PATTERN(cl_long16)
VECTOR16_PATTERN(cl_ulong16)
VECTOR16_PATTERN(cl_float16)
#else
SCALAR_PATTERN(cl_int)
#endif
}  // namespace

#if !defined(__clang_analyzer__)
using OpenCLTypes = ::testing::Types<
    cl_char, cl_char2, cl_char3, cl_char4, cl_char8, cl_char16, cl_uchar,
    cl_uchar2, cl_uchar3, cl_uchar4, cl_uchar8, cl_uchar16, cl_short, cl_short2,
    cl_short3, cl_short4, cl_short8, cl_short16, cl_ushort, cl_ushort2,
    cl_ushort3, cl_ushort4, cl_ushort8, cl_ushort16, cl_int, cl_int2, cl_int3,
    cl_int4, cl_int8, cl_int16, cl_uint, cl_uint2, cl_uint3, cl_uint4, cl_uint8,
    cl_uint16, cl_long, cl_long2, cl_long3, cl_long4, cl_long8, cl_long16,
    cl_ulong, cl_ulong2, cl_ulong3, cl_ulong4, cl_ulong8, cl_ulong16, cl_float,
    cl_float2, cl_float3, cl_float4, cl_float8, cl_float16>;
#else
// Reduce the number of types to test if running clang analyzer (or
// clang-tidy), they'll all result in basically the same code but it takes a
// long time to analyze all of them.
using OpenCLTypes = ::testing::Types<cl_int>;
#endif
TYPED_TEST_SUITE(USMMemFillTest, OpenCLTypes);

template <typename TypeParam>
void USMMemFillTest<TypeParam>::validateResults(TypeParam *ptr) {
  // Verify results
  const TypeParam pattern1 = test_patterns<TypeParam>::pattern1;
  const TypeParam pattern2 = test_patterns<TypeParam>::pattern2;

  const char *tested_type_string = test_patterns<TypeParam>::as_string;
  // Elements [0,1] should be set to pattern1
  EXPECT_TRUE(0 == std::memcmp(&ptr[0], &pattern1, sizeof(TypeParam)))
      << "For type " << tested_type_string;
  EXPECT_TRUE(0 == std::memcmp(&ptr[1], &pattern1, sizeof(TypeParam)))
      << "For type " << tested_type_string;

  // Elements [2,6] should still be zero
  TypeParam zero[5];
  std::memset(zero, 0, sizeof(TypeParam) * 5);
  EXPECT_TRUE(0 == std::memcmp(&ptr[2], zero, sizeof(TypeParam) * 5))
      << "For type " << tested_type_string;

  // Final element [7] should be pattern2
  EXPECT_TRUE(0 == std::memcmp(&ptr[7], &pattern2, sizeof(pattern2)))
      << "For type " << tested_type_string;
}

// Test expected behaviour of clEnqueueMemFillINTEL for a host USM allocation
TYPED_TEST(USMMemFillTest, HostAllocation) {
  // gtest TYPED_TEST uses a derived class template, requiring us to visit
  // members of USMMemFillTest via 'this'.
  auto host_ptr = this->host_ptr;
  auto bytes = this->bytes;
  auto queue = this->queue;
  auto getPointerOffset = this->getPointerOffset;
  auto clEnqueueMemFillINTEL = this->clEnqueueMemFillINTEL;
  if (!host_ptr) {
    GTEST_SKIP();
  }

  // Zero initialize host memory containing 8 TypeParam elements before
  // beginning testing
  std::memset(host_ptr, 0, bytes);

  // Fill first two elements
  const TypeParam pattern = test_patterns<TypeParam>::pattern1;
  cl_int err = clEnqueueMemFillINTEL(queue, host_ptr, &pattern, sizeof(pattern),
                                     sizeof(pattern) * 2, 0, nullptr, nullptr);
  EXPECT_SUCCESS(err);

  // Fill last element
  const TypeParam pattern2 = test_patterns<TypeParam>::pattern2;
  const size_t offset = bytes - sizeof(TypeParam);

  void *offset_host_ptr = getPointerOffset(host_ptr, offset);
  err =
      clEnqueueMemFillINTEL(queue, offset_host_ptr, &pattern2, sizeof(pattern2),
                            sizeof(pattern2), 0, nullptr, nullptr);
  EXPECT_SUCCESS(err);

  EXPECT_SUCCESS(clFinish(queue));

  // Verify results
  this->validateResults(reinterpret_cast<TypeParam *>(host_ptr));
}

// Test expected behaviour of clEnqueueMemFillINTEL for a device USM allocation
TYPED_TEST(USMMemFillTest, DeviceAllocation) {
  // gtest TYPED_TEST uses a derived class template, requiring us to visit
  // members of USMMemFillTest via 'this'.
  auto device_ptr = this->device_ptr;
  auto host_ptr = this->host_ptr;
  auto bytes = this->bytes;
  auto queue = this->queue;
  auto getPointerOffset = this->getPointerOffset;
  auto clEnqueueMemFillINTEL = this->clEnqueueMemFillINTEL;

  // Zero initialize host and device memory containing 8 TypeParam elements
  // before beginning testing
  if (host_ptr) {
    std::memset(host_ptr, 0, bytes);
  }

  std::array<cl_event, 3> wait_events;
  const TypeParam zero_pattern = test_patterns<TypeParam>::zero_pattern;
  cl_int err = clEnqueueMemFillINTEL(queue, device_ptr, &zero_pattern,
                                     sizeof(zero_pattern), bytes, 0, nullptr,
                                     &wait_events[0]);

  // Fill first two elements
  const TypeParam pattern = test_patterns<TypeParam>::pattern1;
  err = clEnqueueMemFillINTEL(queue, device_ptr, &pattern, sizeof(pattern),
                              sizeof(pattern) * 2, 1, &wait_events[0],
                              &wait_events[1]);
  EXPECT_SUCCESS(err);

  // Fill last element
  const TypeParam pattern2 = test_patterns<TypeParam>::pattern2;
  const size_t offset = bytes - sizeof(TypeParam);

  void *offset_device_ptr = getPointerOffset(device_ptr, offset);
  err = clEnqueueMemFillINTEL(queue, offset_device_ptr, &pattern2,
                              sizeof(pattern2), sizeof(pattern2), 1,
                              &wait_events[0], &wait_events[2]);
  EXPECT_SUCCESS(err);

  if (host_ptr) {
    auto clEnqueueMemcpyINTEL = this->clEnqueueMemcpyINTEL;
    // Copy whole device allocation to host for result validation
    err = clEnqueueMemcpyINTEL(queue, CL_FALSE, host_ptr, device_ptr, bytes, 3,
                               wait_events.data(), nullptr);
    EXPECT_SUCCESS(err);
  }

  EXPECT_SUCCESS(clFinish(queue));

  // Verify results
  if (host_ptr) {
    this->validateResults(reinterpret_cast<TypeParam *>(host_ptr));
  }

  for (cl_event &event : wait_events) {
    EXPECT_SUCCESS(clReleaseEvent(event));
  }
}

// Test expected behaviour of clEnqueueMemFillINTEL for a shared USM allocation
TYPED_TEST(USMMemFillTest, SharedAllocation) {
  // gtest TYPED_TEST uses a derived class template, requiring us to visit
  // members of USMMemFillTest via 'this'.
  auto shared_ptr = this->shared_ptr;
  auto host_ptr = this->host_ptr;
  auto bytes = this->bytes;
  auto queue = this->queue;
  auto getPointerOffset = this->getPointerOffset;
  auto clEnqueueMemFillINTEL = this->clEnqueueMemFillINTEL;
  if (!shared_ptr) {
    GTEST_SKIP();
  }

  // Zero initialize host and shared memory containing 8 TypeParam elements
  // before beginning testing
  if (host_ptr) {
    std::memset(host_ptr, 0, bytes);
  }

  std::array<cl_event, 3> wait_events;
  const TypeParam zero_pattern = test_patterns<TypeParam>::zero_pattern;
  cl_int err = clEnqueueMemFillINTEL(queue, shared_ptr, &zero_pattern,
                                     sizeof(zero_pattern), bytes, 0, nullptr,
                                     &wait_events[0]);

  // Fill first two elements
  const TypeParam pattern = test_patterns<TypeParam>::pattern1;
  err = clEnqueueMemFillINTEL(queue, shared_ptr, &pattern, sizeof(pattern),
                              sizeof(pattern) * 2, 1, &wait_events[0],
                              &wait_events[1]);
  EXPECT_SUCCESS(err);

  // Fill last element
  const TypeParam pattern2 = test_patterns<TypeParam>::pattern2;
  const size_t offset = bytes - sizeof(TypeParam);

  void *offset_shared_ptr = getPointerOffset(shared_ptr, offset);
  err = clEnqueueMemFillINTEL(queue, offset_shared_ptr, &pattern2,
                              sizeof(pattern2), sizeof(pattern2), 1,
                              &wait_events[0], &wait_events[2]);
  EXPECT_SUCCESS(err);

  if (host_ptr) {
    auto clEnqueueMemcpyINTEL = this->clEnqueueMemcpyINTEL;
    // Copy whole device allocation to host for result validation
    err = clEnqueueMemcpyINTEL(queue, CL_FALSE, host_ptr, shared_ptr, bytes, 3,
                               wait_events.data(), nullptr);
    EXPECT_SUCCESS(err);
  }

  EXPECT_SUCCESS(clFinish(queue));

  // Verify results
  if (host_ptr) {
    this->validateResults(reinterpret_cast<TypeParam *>(host_ptr));
  }

  for (cl_event &event : wait_events) {
    EXPECT_SUCCESS(clReleaseEvent(event));
  }
}

// Test expected behaviour of clEnqueueMemFillINTEL for a user allocation
TYPED_TEST(USMMemFillTest, UserAllocation) {
  // gtest TYPED_TEST uses a derived class template, requiring us to visit
  // members of USMMemFillTest via 'this'.
  auto bytes = this->bytes;
  auto queue = this->queue;
  auto getPointerOffset = this->getPointerOffset;
  auto clEnqueueMemFillINTEL = this->clEnqueueMemFillINTEL;

  std::vector<TypeParam, UCL::aligned_allocator<TypeParam, sizeof(TypeParam)>>
      user_data(elements);

  std::array<cl_event, 3> wait_events;
  // Zero initialize user memory containing 8 TypeParam elements before
  // beginning testing
  const TypeParam zero_pattern = test_patterns<TypeParam>::zero_pattern;
  cl_int err = clEnqueueMemFillINTEL(queue, user_data.data(), &zero_pattern,
                                     sizeof(zero_pattern), bytes, 0, nullptr,
                                     &wait_events[0]);

  // Fill first two elements
  const TypeParam pattern = test_patterns<TypeParam>::pattern1;
  err = clEnqueueMemFillINTEL(queue, user_data.data(), &pattern,
                              sizeof(pattern), sizeof(pattern) * 2, 1,
                              &wait_events[0], &wait_events[1]);
  EXPECT_SUCCESS(err);

  // Fill last element
  const TypeParam pattern2 = test_patterns<TypeParam>::pattern2;
  const size_t offset = bytes - sizeof(TypeParam);

  void *offset_device_ptr = getPointerOffset(user_data.data(), offset);
  err = clEnqueueMemFillINTEL(queue, offset_device_ptr, &pattern2,
                              sizeof(pattern2), sizeof(pattern2), 1,
                              &wait_events[0], &wait_events[2]);
  EXPECT_SUCCESS(err);

  EXPECT_SUCCESS(clFinish(queue));

  // Verify results
  this->validateResults(reinterpret_cast<TypeParam *>(user_data.data()));

  for (cl_event &event : wait_events) {
    EXPECT_SUCCESS(clReleaseEvent(event));
  }
}
