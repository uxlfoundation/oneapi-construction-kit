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
struct USMMemCpyTest : public cl_intel_unified_shared_memory_Test {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(cl_intel_unified_shared_memory_Test::SetUp());

    cl_device_unified_shared_memory_capabilities_intel host_capabilities;
    ASSERT_SUCCESS(clGetDeviceInfo(
        device, CL_DEVICE_HOST_MEM_CAPABILITIES_INTEL,
        sizeof(host_capabilities), &host_capabilities, nullptr));

    cl_int err;
    if (host_capabilities) {
      host_ptr_A = clHostMemAllocINTEL(context, nullptr, bytes, align, &err);
      ASSERT_SUCCESS(err);
      ASSERT_TRUE(host_ptr_A != nullptr);

      host_ptr_B = clHostMemAllocINTEL(context, nullptr, bytes, align, &err);
      ASSERT_SUCCESS(err);
      ASSERT_TRUE(host_ptr_B != nullptr);
    }

    device_ptr_A =
        clDeviceMemAllocINTEL(context, device, nullptr, bytes, align, &err);
    ASSERT_SUCCESS(err);
    ASSERT_TRUE(device_ptr_A != nullptr);

    device_ptr_B =
        clDeviceMemAllocINTEL(context, device, nullptr, bytes, align, &err);
    ASSERT_SUCCESS(err);
    ASSERT_TRUE(device_ptr_B != nullptr);

    queue = clCreateCommandQueue(context, device, 0, &err);
    ASSERT_TRUE(queue != nullptr);
    ASSERT_SUCCESS(err);
  }

  void TearDown() override {
    if (device_ptr_A) {
      const cl_int err = clMemBlockingFreeINTEL(context, device_ptr_A);
      EXPECT_SUCCESS(err);
    }

    if (device_ptr_B) {
      const cl_int err = clMemBlockingFreeINTEL(context, device_ptr_B);
      EXPECT_SUCCESS(err);
    }

    if (host_ptr_A) {
      const cl_int err = clMemBlockingFreeINTEL(context, host_ptr_A);
      EXPECT_SUCCESS(err);
    }

    if (host_ptr_B) {
      const cl_int err = clMemBlockingFreeINTEL(context, host_ptr_B);
      EXPECT_SUCCESS(err);
    }

    if (queue) {
      EXPECT_SUCCESS(clReleaseCommandQueue(queue));
    }

    cl_intel_unified_shared_memory_Test::TearDown();
  }

  static const size_t bytes = sizeof(T) * elements;
  static const cl_uint align = sizeof(T);

  void *host_ptr_A = nullptr;
  void *host_ptr_B = nullptr;
  void *device_ptr_A = nullptr;
  void *device_ptr_B = nullptr;

  cl_command_queue queue = nullptr;
};

#define SCALAR_PATTERN(T)                        \
  template <>                                    \
  struct test_patterns<T> {                      \
    static constexpr T zero_pattern = 0;         \
    static constexpr T pattern1 = 42;            \
    static constexpr T pattern2 = 0xA;           \
    static constexpr const char *as_string = #T; \
  };

#define VECTOR2_PATTERN(T)                       \
  template <>                                    \
  struct test_patterns<T> {                      \
    static constexpr T zero_pattern = {{0, 0}};  \
    static constexpr T pattern1 = {{42, 43}};    \
    static constexpr T pattern2 = {{0xA, 0xB}};  \
    static constexpr const char *as_string = #T; \
  };

#define VECTOR4_PATTERN(T)                                \
  template <>                                             \
  struct test_patterns<T> {                               \
    static constexpr T zero_pattern = {{0, 0, 0, 0}};     \
    static constexpr T pattern1 = {{42, 43, 44, 45}};     \
    static constexpr T pattern2 = {{0xA, 0xB, 0xC, 0xD}}; \
    static constexpr const char *as_string = #T;          \
  };

#define VECTOR8_PATTERN(T)                                            \
  template <>                                                         \
  struct test_patterns<T> {                                           \
    static constexpr T zero_pattern = {{0, 0, 0, 0, 0, 0, 0}};        \
    static constexpr T pattern1 = {{42, 43, 44, 45, 46, 47, 48, 49}}; \
    static constexpr T pattern2 = {                                   \
        {0xA, 0xB, 0xC, 0xD, 0xE, 0xF, 0x10, 0x11}};                  \
    static constexpr const char *as_string = #T;                      \
  };

#define VECTOR16_PATTERN(T)                                                   \
  template <>                                                                 \
  struct test_patterns<T> {                                                   \
    static constexpr T zero_pattern = {                                       \
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};                    \
    static constexpr T pattern1 = {                                           \
        {42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57}};    \
    static constexpr T pattern2 = {{0xA, 0xB, 0xC, 0xD, 0xE, 0xF, 0x10, 0x11, \
                                    0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, \
                                    0x19}};                                   \
    static constexpr const char *as_string = #T;                              \
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
TYPED_TEST_SUITE(USMMemCpyTest, OpenCLTypes, );

// Test for expected behaviour of source device USM allocation
// clEnqueueMemcpyINTEL to a destination device USM allocation
TYPED_TEST(USMMemCpyTest, DeviceToDevice) {
  // gtest TYPED_TEST uses a derived class template, requiring us to visit
  // members of USMMemCpyTest via 'this'.
  auto bytes = this->bytes;
  auto queue = this->queue;
  auto host_ptr_A = this->host_ptr_A;
  auto device_ptr_A = this->device_ptr_A;
  auto device_ptr_B = this->device_ptr_B;
  auto clEnqueueMemFillINTEL = this->clEnqueueMemFillINTEL;
  auto clEnqueueMemcpyINTEL = this->clEnqueueMemcpyINTEL;
  auto getPointerOffset = this->getPointerOffset;

  // Initialize device allocation A
  const TypeParam pattern1 = test_patterns<TypeParam>::pattern1;
  cl_int err =
      clEnqueueMemFillINTEL(queue, device_ptr_A, &pattern1, sizeof(pattern1),
                            bytes, 0, nullptr, nullptr);
  EXPECT_SUCCESS(err);

  // Initialize device allocation B
  const TypeParam pattern2 = test_patterns<TypeParam>::pattern2;
  err = clEnqueueMemFillINTEL(queue, device_ptr_B, &pattern2, sizeof(pattern2),
                              bytes, 0, nullptr, nullptr);
  EXPECT_SUCCESS(err);

  // offset halfway into the allocation
  constexpr size_t offset = sizeof(TypeParam) * (elements / 2);
  void *offset_A_ptr = getPointerOffset(device_ptr_A, offset);
  void *offset_B_ptr = getPointerOffset(device_ptr_B, offset);

  // Copy bytes from the start of allocation A to second half of allocation B
  err = clEnqueueMemcpyINTEL(queue, CL_FALSE, offset_B_ptr, device_ptr_A,
                             offset, 0, nullptr, nullptr);
  EXPECT_SUCCESS(err);

  // Copy bytes from the start allocation B to second half of allocation A
  err = clEnqueueMemcpyINTEL(queue, CL_FALSE, offset_A_ptr, device_ptr_B,
                             offset, 0, nullptr, nullptr);
  EXPECT_SUCCESS(err);
  EXPECT_SUCCESS(clFinish(queue));

  if (host_ptr_A) {
    // Reset host copy before use as destination
    std::memset(host_ptr_A, 0, bytes);

    // Use host allocation to verify results
    err = clEnqueueMemcpyINTEL(queue, CL_TRUE, host_ptr_A, device_ptr_B, bytes,
                               0, nullptr, nullptr);
    EXPECT_SUCCESS(err);

    constexpr size_t offset_elements = offset / sizeof(TypeParam);
    std::array<TypeParam, offset_elements> reference1, reference2;
    reference1.fill(pattern1);
    reference2.fill(pattern2);

    void *offset_host_ptr = getPointerOffset(host_ptr_A, offset);
    const char *tested_type_string = test_patterns<TypeParam>::as_string;
    EXPECT_TRUE(0 == std::memcmp(reference2.data(), host_ptr_A, offset))
        << "For type " << tested_type_string;
    EXPECT_TRUE(0 == std::memcmp(reference1.data(), offset_host_ptr, offset))
        << "For type " << tested_type_string;

    err = clEnqueueMemcpyINTEL(queue, CL_TRUE, host_ptr_A, device_ptr_A, bytes,
                               0, nullptr, nullptr);
    EXPECT_SUCCESS(err);

    EXPECT_TRUE(0 == std::memcmp(reference1.data(), host_ptr_A, offset))
        << "For type " << tested_type_string;
    EXPECT_TRUE(0 == std::memcmp(reference2.data(), offset_host_ptr, offset))
        << "For type " << tested_type_string;
  }
  EXPECT_SUCCESS(clFinish(queue));
}

// Test for expected behaviour of source device USM allocation
// clEnqueueMemcpyINTEL to a destination host USM allocation
TYPED_TEST(USMMemCpyTest, DeviceToHost) {
  // gtest TYPED_TEST uses a derived class template, requiring us to visit
  // members of USMMemCpyTest via 'this'.
  auto bytes = this->bytes;
  auto queue = this->queue;
  auto host_ptr = this->host_ptr_A;
  auto device_ptr = this->device_ptr_A;
  auto clEnqueueMemFillINTEL = this->clEnqueueMemFillINTEL;
  auto clEnqueueMemcpyINTEL = this->clEnqueueMemcpyINTEL;
  auto getPointerOffset = this->getPointerOffset;

  if (!host_ptr) {
    GTEST_SKIP();
  }
  std::array<cl_event, 3> events{};

  // Zero initialize device buffer
  const TypeParam zero_pattern = test_patterns<TypeParam>::zero_pattern;
  cl_int err = clEnqueueMemFillINTEL(queue, device_ptr, &zero_pattern,
                                     sizeof(zero_pattern), bytes, 0, nullptr,
                                     events.data());
  EXPECT_SUCCESS(err);

  // Initialize first two elements of device allocation
  const TypeParam pattern1 = test_patterns<TypeParam>::pattern1;
  err =
      clEnqueueMemFillINTEL(queue, device_ptr, &pattern1, sizeof(pattern1),
                            sizeof(pattern1) * 2, 1, events.data(), &events[1]);
  EXPECT_SUCCESS(err);

  // Initialize last 3 elements of device allocation
  const TypeParam pattern2 = test_patterns<TypeParam>::pattern2;

  size_t offset = (bytes - (3 * sizeof(pattern2)));
  void *offset_device_ptr = getPointerOffset(device_ptr, offset);
  err = clEnqueueMemFillINTEL(queue, offset_device_ptr, &pattern2,
                              sizeof(pattern2), sizeof(pattern2) * 3, 1,
                              events.data(), &events[2]);
  EXPECT_SUCCESS(err);

  // Reset host copy before use as destination
  std::memset(host_ptr, 0, bytes);

  // Copy second and third element from device allocation to start of host
  // allocation device[1,2] -> host[0,1]
  offset_device_ptr = getPointerOffset(device_ptr, sizeof(TypeParam));
  err = clEnqueueMemcpyINTEL(queue, CL_FALSE, host_ptr, offset_device_ptr,
                             sizeof(TypeParam) * 2, 1, events.data(), nullptr);
  EXPECT_SUCCESS(err);

  // Copy last 4 elements of device allocation into next elements host
  // allocation device[4,5,6,7] -> host[2,3,4,5]
  offset = (bytes - (4 * sizeof(TypeParam)));
  offset_device_ptr = getPointerOffset(device_ptr, offset);
  void *offset_host_ptr = getPointerOffset(host_ptr, sizeof(TypeParam) * 2);
  err =
      clEnqueueMemcpyINTEL(queue, CL_FALSE, offset_host_ptr, offset_device_ptr,
                           sizeof(TypeParam) * 4, 1, &events[1], nullptr);
  EXPECT_SUCCESS(err);

  EXPECT_SUCCESS(clFinish(queue));

  for (cl_event &event : events) {
    EXPECT_SUCCESS(clReleaseEvent(event));
  }

  // Verify results
  const char *tested_type_string = test_patterns<TypeParam>::as_string;
  TypeParam *host_validation_ptr = reinterpret_cast<TypeParam *>(host_ptr);
  // host[0] contains device[1] which was filled with pattern1
  EXPECT_TRUE(0 ==
              std::memcmp(&pattern1, host_validation_ptr, sizeof(TypeParam)))
      << "For type " << tested_type_string;

  // host[1] contains device[2] which was zero initialized
  EXPECT_TRUE(0 == std::memcmp(&zero_pattern, &host_validation_ptr[1],
                               sizeof(TypeParam)))
      << "For type " << tested_type_string;

  // host[2] contains device[4] which was zero initialized
  EXPECT_TRUE(0 == std::memcmp(&zero_pattern, &host_validation_ptr[2],
                               sizeof(TypeParam)))
      << "For type " << tested_type_string;

  // host[3] contains device[5] which was filled with pattern2
  EXPECT_TRUE(
      0 == std::memcmp(&pattern2, &host_validation_ptr[3], sizeof(TypeParam)))
      << "For type " << tested_type_string;

  // host[4] contains device[6] which was filled with pattern2
  EXPECT_TRUE(
      0 == std::memcmp(&pattern2, &host_validation_ptr[4], sizeof(TypeParam)))
      << "For type " << tested_type_string;

  // host[5] contains device[7] which was filled with pattern2
  EXPECT_TRUE(
      0 == std::memcmp(&pattern2, &host_validation_ptr[5], sizeof(TypeParam)))
      << "For type " << tested_type_string;

  // host[6] was zero initalized and not copied into
  EXPECT_TRUE(0 == std::memcmp(&zero_pattern, &host_validation_ptr[6],
                               sizeof(TypeParam)))
      << "For type " << tested_type_string;

  // host[7] was zero initalized and not copied into
  EXPECT_TRUE(0 == std::memcmp(&zero_pattern, &host_validation_ptr[6],
                               sizeof(TypeParam)))
      << "For type " << tested_type_string;
}

// Test for expected behaviour of source host USM allocation
// clEnqueueMemcpyINTEL to a destination device USM allocation
TYPED_TEST(USMMemCpyTest, HostToDevice) {
  // gtest TYPED_TEST uses a derived class template, requiring us to visit
  // members of USMMemCpyTest via 'this'.
  auto bytes = this->bytes;
  auto queue = this->queue;
  auto host_ptr = this->host_ptr_A;
  auto host_ptr_verify = this->host_ptr_B;
  auto device_ptr = this->device_ptr_A;
  auto clEnqueueMemFillINTEL = this->clEnqueueMemFillINTEL;
  auto clEnqueueMemcpyINTEL = this->clEnqueueMemcpyINTEL;
  auto getPointerOffset = this->getPointerOffset;

  if (!(host_ptr && host_ptr_verify)) {
    GTEST_SKIP();
  }

  // Reset host allocations
  std::memset(host_ptr_verify, 0, bytes);
  std::memset(host_ptr, 0, bytes);

  // Reset device allocation before use as copy destination
  std::array<cl_event, 3> events{};
  const TypeParam zero_pattern = test_patterns<TypeParam>::zero_pattern;
  cl_int err = clEnqueueMemFillINTEL(queue, device_ptr, &zero_pattern,
                                     sizeof(zero_pattern), bytes, 0, nullptr,
                                     events.data());
  EXPECT_SUCCESS(err);

  // Initialize first two elements of source host allocation to pattern 1, and
  // also last 3 elements to pattern 2 bytes at an offset half way into the
  // allocation.
  const TypeParam pattern1 = test_patterns<TypeParam>::pattern1;

  TypeParam *host_ptr_cast = reinterpret_cast<TypeParam *>(host_ptr);
  host_ptr_cast[0] = host_ptr_cast[1] = pattern1;

  const TypeParam pattern2 = test_patterns<TypeParam>::pattern2;
  host_ptr_cast[5] = host_ptr_cast[6] = host_ptr_cast[7] = pattern2;

  // Copy second and third element from host allocation to start of device
  // allocation host[1,2] -> device[0,1]
  err =
      clEnqueueMemcpyINTEL(queue, CL_FALSE, device_ptr, &host_ptr_cast[1],
                           sizeof(TypeParam) * 2, 1, events.data(), &events[1]);
  EXPECT_SUCCESS(err);

  // Copy last 4 elements of host allocation into next elements in device
  // allocation host[4,5,6,7] -> device[2,3,4,5]
  void *offset_device_ptr = getPointerOffset(device_ptr, sizeof(TypeParam) * 2);
  err = clEnqueueMemcpyINTEL(queue, CL_FALSE, offset_device_ptr,
                             &host_ptr_cast[4], sizeof(TypeParam) * 4, 1,
                             &events[1], &events[2]);
  EXPECT_SUCCESS(err);

  // Copy whole device allocation into output host pointer for validation
  err = clEnqueueMemcpyINTEL(queue, CL_FALSE, host_ptr_verify, device_ptr,
                             bytes, 3, events.data(), nullptr);
  EXPECT_SUCCESS(err);

  EXPECT_SUCCESS(clFinish(queue));

  for (cl_event &event : events) {
    EXPECT_SUCCESS(clReleaseEvent(event));
  }

  // Verify results
  const char *tested_type_string = test_patterns<TypeParam>::as_string;
  TypeParam *casted_validation_ptr =
      reinterpret_cast<TypeParam *>(host_ptr_verify);

  // device[0] contains host[1] which was filled with pattern1
  EXPECT_TRUE(0 ==
              memcmp(&casted_validation_ptr[0], &pattern1, sizeof(TypeParam)))
      << "For type " << tested_type_string;

  // device[1] contains host[2] which was zero initialized
  EXPECT_TRUE(
      0 == memcmp(&casted_validation_ptr[1], &zero_pattern, sizeof(TypeParam)))
      << "For type " << tested_type_string;

  // device[2] contains host[4] which was zero initialized
  EXPECT_TRUE(
      0 == memcmp(&casted_validation_ptr[2], &zero_pattern, sizeof(TypeParam)))
      << "For type " << tested_type_string;

  // device[3] contains host[5] which was filled with pattern2
  EXPECT_TRUE(0 ==
              memcmp(&casted_validation_ptr[3], &pattern2, sizeof(TypeParam)))
      << "For type " << tested_type_string;

  // device[4] contains host[6] which was filled with pattern2
  EXPECT_TRUE(0 ==
              memcmp(&casted_validation_ptr[4], &pattern2, sizeof(TypeParam)))
      << "For type " << tested_type_string;

  // device[5] contains host[7] which was filled with pattern2
  EXPECT_TRUE(0 ==
              memcmp(&casted_validation_ptr[5], &pattern2, sizeof(TypeParam)))
      << "For type " << tested_type_string;

  // device[6] was zero initialized and not copied into
  EXPECT_TRUE(
      0 == memcmp(&casted_validation_ptr[6], &zero_pattern, sizeof(TypeParam)))
      << "For type " << tested_type_string;

  // device[7] was zero initialized and not copied into
  EXPECT_TRUE(
      0 == memcmp(&casted_validation_ptr[7], &zero_pattern, sizeof(TypeParam)))
      << "For type " << tested_type_string;
}

// Test for expected behaviour of source host USM allocation
// clEnqueueMemcpyINTEL to a destination host USM allocation
TYPED_TEST(USMMemCpyTest, HostToHost) {
  // gtest TYPED_TEST uses a derived class template, requiring us to visit
  // members of USMMemCpyTest via 'this'.
  auto bytes = this->bytes;
  auto queue = this->queue;
  auto host_ptr_A = this->host_ptr_A;
  auto host_ptr_B = this->host_ptr_B;
  auto clEnqueueMemFillINTEL = this->clEnqueueMemFillINTEL;
  auto clEnqueueMemcpyINTEL = this->clEnqueueMemcpyINTEL;
  auto getPointerOffset = this->getPointerOffset;

  if (!(host_ptr_A && host_ptr_B)) {
    GTEST_SKIP();
  }

  std::array<cl_event, 2> events{};

  // Initialize host allocation A
  const TypeParam pattern1 = test_patterns<TypeParam>::pattern1;
  cl_int err =
      clEnqueueMemFillINTEL(queue, host_ptr_A, &pattern1, sizeof(TypeParam),
                            bytes, 0, nullptr, events.data());
  EXPECT_SUCCESS(err);

  // Initialize host allocation B
  const TypeParam pattern2 = test_patterns<TypeParam>::pattern2;
  err = clEnqueueMemFillINTEL(queue, host_ptr_B, &pattern2, sizeof(TypeParam),
                              bytes, 0, nullptr, &events[1]);
  EXPECT_SUCCESS(err);

  // Offset halfway into allocation
  constexpr size_t offset = sizeof(TypeParam) * (elements / 2);
  void *offset_A_ptr = getPointerOffset(host_ptr_A, offset);
  void *offset_B_ptr = getPointerOffset(host_ptr_B, offset);

  // Copy first half from allocation A into second half of allocation B
  err = clEnqueueMemcpyINTEL(queue, CL_FALSE, offset_B_ptr, host_ptr_A, offset,
                             1, events.data(), nullptr);
  EXPECT_SUCCESS(err);

  // Copy half bytes from allocation B into second half of allocation A
  err = clEnqueueMemcpyINTEL(queue, CL_FALSE, offset_A_ptr, host_ptr_B, offset,
                             1, &events[1], nullptr);
  EXPECT_SUCCESS(err);
  EXPECT_SUCCESS(clFinish(queue));

  for (cl_event &event : events) {
    EXPECT_SUCCESS(clReleaseEvent(event));
  }

  // Verify result
  constexpr size_t offset_elements = offset / sizeof(TypeParam);
  std::array<TypeParam, offset_elements> reference1, reference2;
  reference1.fill(pattern1);
  reference2.fill(pattern2);

  const char *tested_type_string = test_patterns<TypeParam>::as_string;

  EXPECT_TRUE(0 == std::memcmp(reference1.data(), host_ptr_A, offset))
      << "For type " << tested_type_string;
  EXPECT_TRUE(0 == std::memcmp(reference2.data(), offset_A_ptr, offset))
      << "For type " << tested_type_string;

  EXPECT_TRUE(0 == std::memcmp(reference2.data(), host_ptr_B, offset))
      << "For type " << tested_type_string;
  EXPECT_TRUE(0 == std::memcmp(reference1.data(), offset_B_ptr, offset))
      << "For type " << tested_type_string;
}

// Test for expected behaviour when copying a device USM allocation
// to an arbitrary user pointer with clEnqueueMemcpyINTEL.
TYPED_TEST(USMMemCpyTest, DeviceToUser) {
  // gtest TYPED_TEST uses a derived class template, requiring us to visit
  // members of USMMemCpyTest via 'this'.
  auto bytes = this->bytes;
  auto queue = this->queue;
  auto device_ptr = this->device_ptr_A;
  auto clEnqueueMemFillINTEL = this->clEnqueueMemFillINTEL;
  auto clEnqueueMemcpyINTEL = this->clEnqueueMemcpyINTEL;
  auto getPointerOffset = this->getPointerOffset;

  // Initialize device allocation, first half with pattern1 & second half
  // with pattern2
  const TypeParam pattern1 = test_patterns<TypeParam>::pattern1;
  constexpr size_t offset = sizeof(TypeParam) * (elements / 2);
  cl_int err =
      clEnqueueMemFillINTEL(queue, device_ptr, &pattern1, sizeof(pattern1),
                            offset, 0, nullptr, nullptr);
  EXPECT_SUCCESS(err);

  // offset halfway into the allocation
  void *offset_ptr = getPointerOffset(device_ptr, offset);
  const TypeParam pattern2 = test_patterns<TypeParam>::pattern2;
  err = clEnqueueMemFillINTEL(queue, offset_ptr, &pattern2, sizeof(pattern2),
                              offset, 0, nullptr, nullptr);
  EXPECT_SUCCESS(err);

  // Copy device USM allocation to user pointer
  TypeParam user_data[elements];
  std::memset(user_data, 0, sizeof(user_data));
  err = clEnqueueMemcpyINTEL(queue, CL_TRUE, user_data, device_ptr, bytes, 0,
                             nullptr, nullptr);
  EXPECT_SUCCESS(err);

  // Verify copy occurred correctly
  constexpr size_t offset_elements = elements / 2;
  std::array<TypeParam, offset_elements> reference1, reference2;
  reference1.fill(pattern1);
  reference2.fill(pattern2);

  void *offset_user_ptr = getPointerOffset(user_data, offset);
  const char *tested_type_string = test_patterns<TypeParam>::as_string;

  EXPECT_TRUE(0 == std::memcmp(reference1.data(), user_data, offset))
      << "For type " << tested_type_string;
  EXPECT_TRUE(0 == std::memcmp(reference2.data(), offset_user_ptr, offset))
      << "For type " << tested_type_string;
}

// Test for expected behaviour when copying arbitrary user data to
// a device USM allocation with clEnqueueMemcpyINTEL.
TYPED_TEST(USMMemCpyTest, UserToDevice) {
  // gtest TYPED_TEST uses a derived class template, requiring us to visit
  // members of USMMemCpyTest via 'this'.
  auto bytes = this->bytes;
  auto queue = this->queue;
  auto device_ptr = this->device_ptr_A;
  auto host_ptr = this->host_ptr_A;
  auto clEnqueueMemFillINTEL = this->clEnqueueMemFillINTEL;
  auto clEnqueueMemcpyINTEL = this->clEnqueueMemcpyINTEL;

  // Initialize user data, alternating elements with pattern1 & pattern2
  TypeParam user_data[elements];
  const TypeParam pattern1 = test_patterns<TypeParam>::pattern1;
  const TypeParam pattern2 = test_patterns<TypeParam>::pattern2;
  for (size_t i = 0; i < elements; i++) {
    user_data[i] = (i % 2) ? pattern1 : pattern2;
  }

  // Zero initialize device USM allocation before copy
  const TypeParam zero_pattern = test_patterns<TypeParam>::zero_pattern;
  cl_int err =
      clEnqueueMemFillINTEL(queue, device_ptr, &zero_pattern,
                            sizeof(zero_pattern), bytes, 0, nullptr, nullptr);
  EXPECT_SUCCESS(err);

  // Copy user pointer to device USM allocation
  err = clEnqueueMemcpyINTEL(queue, CL_TRUE, device_ptr, user_data, bytes, 0,
                             nullptr, nullptr);
  EXPECT_SUCCESS(err);

  // Use host allocation to verify results from device allocation
  if (host_ptr) {
    // Reset host copy before use as destination
    std::memset(host_ptr, 0, bytes);
    err = clEnqueueMemcpyINTEL(queue, CL_TRUE, host_ptr, device_ptr, bytes, 0,
                               nullptr, nullptr);
    EXPECT_SUCCESS(err);

    const char *tested_type_string = test_patterns<TypeParam>::as_string;
    for (size_t i = 0; i < elements; i++) {
      const TypeParam copied = static_cast<TypeParam *>(host_ptr)[i];
      EXPECT_TRUE(0 == std::memcmp(&copied, &user_data[i], sizeof(TypeParam)))
          << "For type " << tested_type_string;
    }
  }
}

// Test for expected behaviour when copying arbitrary user data to a
// destination arbitrary user pointer with clEnqueueMemcpyINTEL.
TYPED_TEST(USMMemCpyTest, UserToUser) {
  // gtest TYPED_TEST uses a derived class template, requiring us to visit
  // members of USMMemCpyTest via 'this'.
  auto bytes = this->bytes;
  auto queue = this->queue;
  auto clEnqueueMemcpyINTEL = this->clEnqueueMemcpyINTEL;

  // Initialize user input data, alternating elements with pattern1 & pattern2
  TypeParam in_data[elements];
  const TypeParam pattern1 = test_patterns<TypeParam>::pattern1;
  const TypeParam pattern2 = test_patterns<TypeParam>::pattern2;
  for (size_t i = 0; i < elements; i++) {
    in_data[i] = (i % 2) ? pattern1 : pattern2;
  }

  // Zero initialize user output data before use as copy destination
  TypeParam out_data[elements];
  std::memset(out_data, 0, bytes);

  // Copy user pointer source to user pointer destination
  const cl_int err = clEnqueueMemcpyINTEL(queue, CL_TRUE, out_data, in_data,
                                          bytes, 0, nullptr, nullptr);
  EXPECT_SUCCESS(err);

  // Verify copy occurred correctly
  const char *tested_type_string = test_patterns<TypeParam>::as_string;
  for (size_t i = 0; i < elements; i++) {
    EXPECT_TRUE(0 == std::memcmp(&out_data[i], &in_data[i], sizeof(TypeParam)))
        << "For type " << tested_type_string;
  }
}
