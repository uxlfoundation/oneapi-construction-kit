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

#include <thread>

#include "cl_intel_unified_shared_memory.h"

using USMTests = cl_intel_unified_shared_memory_Test;

// Test for invalid API usage of clMemFreeIntel() and clMemBlockingFreeIntel()
TEST_F(USMTests, MemFree_InvalidUsage) {
  void *malloc_ptr = malloc(256);

  cl_int err = clMemFreeINTEL(nullptr, malloc_ptr);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_CONTEXT);

  err = clMemBlockingFreeINTEL(nullptr, malloc_ptr);
  EXPECT_EQ_ERRCODE(err, CL_INVALID_CONTEXT);

  free(malloc_ptr);
}

// Test for valid API usage of clMemFreeIntel() and clMemBlockingFreeIntel()
TEST_F(USMTests, MemFree_ValidUsage) {
  const size_t bytes = 256;
  const cl_uint align = 4;

  cl_int err = clMemFreeINTEL(context, nullptr);
  EXPECT_SUCCESS(err);

  err = clMemBlockingFreeINTEL(context, nullptr);
  EXPECT_SUCCESS(err);

  if (host_capabilities != 0) {
    void *host_ptr = clHostMemAllocINTEL(context, nullptr, bytes, align, &err);
    ASSERT_SUCCESS(err);
    ASSERT_TRUE(host_ptr != nullptr);

    err = clMemBlockingFreeINTEL(context, host_ptr);
    EXPECT_SUCCESS(err);

    host_ptr = clHostMemAllocINTEL(context, nullptr, bytes, align, &err);
    ASSERT_SUCCESS(err);
    ASSERT_TRUE(host_ptr != nullptr);

    err = clMemFreeINTEL(context, host_ptr);
    EXPECT_SUCCESS(err);
  }

  if (shared_capabilities != 0) {
    void *shared_ptr =
        clSharedMemAllocINTEL(context, device, nullptr, bytes, align, &err);
    ASSERT_SUCCESS(err);
    ASSERT_TRUE(shared_ptr != nullptr);

    err = clMemBlockingFreeINTEL(context, shared_ptr);
    EXPECT_SUCCESS(err);

    shared_ptr =
        clSharedMemAllocINTEL(context, nullptr, nullptr, bytes, align, &err);
    ASSERT_SUCCESS(err);
    ASSERT_TRUE(shared_ptr != nullptr);

    err = clMemFreeINTEL(context, shared_ptr);
    EXPECT_SUCCESS(err);
  }

  void *device_ptr =
      clDeviceMemAllocINTEL(context, device, nullptr, bytes, align, &err);
  ASSERT_SUCCESS(err);
  ASSERT_TRUE(device_ptr != nullptr);

  err = clMemBlockingFreeINTEL(context, device_ptr);
  EXPECT_SUCCESS(err);

  device_ptr =
      clDeviceMemAllocINTEL(context, device, nullptr, bytes, align, &err);
  ASSERT_SUCCESS(err);
  ASSERT_TRUE(device_ptr != nullptr);

  err = clMemFreeINTEL(context, device_ptr);
  EXPECT_SUCCESS(err);

  // Freeing arbitrary host data is permitted by the spec
  {
    void *malloc_ptr = malloc(256);

    err = clMemFreeINTEL(context, malloc_ptr);
    EXPECT_SUCCESS(err);

    err = clMemBlockingFreeINTEL(context, malloc_ptr);
    EXPECT_SUCCESS(err);

    free(malloc_ptr);
  }
}

namespace {
// Fixture to help testing of clMemBlockingFreeINTEL
struct USMBlockingFreeTest : public cl_intel_unified_shared_memory_Test {
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(cl_intel_unified_shared_memory_Test::SetUp());

    cl_device_unified_shared_memory_capabilities_intel host_capabilities;
    ASSERT_SUCCESS(clGetDeviceInfo(
        device, CL_DEVICE_HOST_MEM_CAPABILITIES_INTEL,
        sizeof(host_capabilities), &host_capabilities, nullptr));

    cl_int err;
    initPointers(bytes, sizeof(cl_uint));

    for (auto &device_ptr : fixture_device_ptrs) {
      device_ptr = clDeviceMemAllocINTEL(context, device, nullptr, bytes,
                                         sizeof(cl_uint), &err);
      ASSERT_SUCCESS(err);
      ASSERT_TRUE(device_ptr != nullptr);
    }

    for (auto &queue : fixture_queues) {
      queue = clCreateCommandQueue(context, device, 0, &err);
      ASSERT_TRUE(queue != nullptr);
      ASSERT_SUCCESS(err);
    }
  }

  void TearDown() override {
    for (auto device_ptr : fixture_device_ptrs) {
      if (device_ptr) {
        EXPECT_SUCCESS(clMemBlockingFreeINTEL(context, device_ptr));
      }
    }

    for (auto queue : fixture_queues) {
      if (queue) {
        EXPECT_SUCCESS(clReleaseCommandQueue(queue));
      }
    }

    cl_intel_unified_shared_memory_Test::TearDown();
  }

  static const size_t elements = 1024;
  static const size_t bytes = sizeof(cl_uint) * elements;

  // Device allocations freed by tests to validate behaviour of blocking free
  std::array<void *, 3> fixture_device_ptrs = {{nullptr, nullptr, nullptr}};
  std::array<cl_command_queue, 3> fixture_queues = {
      {nullptr, nullptr, nullptr}};
};

}  // namespace

// Fill a single device allocation using fill calls at strided offsets across
// multiple queues
TEST_F(USMBlockingFreeTest, MultipleQueueSingleAlloc) {
  const size_t threads = 32;
  UCL::vector<std::thread> workers(threads);
  cl_command_queue queues[threads];

  void *device_ptr = fixture_device_ptrs[0];

  const size_t elements_to_fill = elements / threads;
  auto worker = [&](size_t tid) {
    cl_int err;
    queues[tid] = clCreateCommandQueue(context, device, 0, &err);
    ASSERT_TRUE(queues[tid] != nullptr);
    ASSERT_SUCCESS(err);

    const cl_uint pattern = 'A' + tid;
    for (size_t element = 0; element < elements_to_fill; element++) {
      const size_t index = (tid * elements_to_fill) + element;
      const size_t offset = index * sizeof(pattern);

      void *device_offset_ptr = getPointerOffset(device_ptr, offset);
      cl_int err = clEnqueueMemFillINTEL(queues[tid], device_offset_ptr,
                                         &pattern, sizeof(pattern),
                                         sizeof(pattern), 0, nullptr, nullptr);
      EXPECT_SUCCESS(err);

      // Copy into host allocation, if supported, for result verification
      if (host_ptr) {
        void *offset_host_ptr = getPointerOffset(host_ptr, offset);
        err = clEnqueueMemcpyINTEL(queues[tid], CL_FALSE, offset_host_ptr,
                                   device_offset_ptr, sizeof(pattern), 0,
                                   nullptr, nullptr);
        EXPECT_SUCCESS(err);
      }
    }
  };

  for (size_t i = 0; i < threads; i++) {
    workers[i] = std::thread(worker, i);
  }

  for (size_t i = 0; i < threads; i++) {
    workers[i].join();
  }

  // Block until all operations are complete, implicitly flushing all queues
  EXPECT_SUCCESS(clMemBlockingFreeINTEL(context, device_ptr));
  fixture_device_ptrs[0] = nullptr;

  if (host_ptr) {
    // Verify data copied from device allocation into host allocation
    for (size_t i = 0; i < threads; i++) {
      std::array<cl_uint, elements_to_fill> reference;
      const cl_uint pattern = 'A' + i;
      reference.fill(pattern);

      const size_t offset = i * sizeof(pattern) * elements_to_fill;
      void *offset_host_ptr = getPointerOffset(host_ptr, offset);
      EXPECT_TRUE(0 == std::memcmp(reference.data(), offset_host_ptr,
                                   sizeof(pattern) * elements_to_fill));
    }
  }

  for (size_t i = 0; i < threads; i++) {
    EXPECT_SUCCESS(clReleaseCommandQueue(queues[i]));
  }
}

// Populate multiple device allocations using fill calls at strided offsets
// across a single command queue
TEST_F(USMBlockingFreeTest, SingleQueueMultipleAlloc) {
  const size_t threads = 32;
  UCL::vector<std::thread> workers(threads);

  std::array<void *, threads> device_ptrs;

  const size_t elements_to_fill = elements / threads;
  const cl_uint pattern = 42;
  auto worker = [&, pattern](size_t index) {
    auto &device_ptr = device_ptrs[index];
    const size_t fill_size = elements_to_fill * sizeof(pattern);

    cl_int err;
    device_ptr = clDeviceMemAllocINTEL(context, device, nullptr, fill_size,
                                       sizeof(cl_uint), &err);
    ASSERT_SUCCESS(err);
    ASSERT_TRUE(device_ptr != nullptr);

    // Fill individual elements to stress enqueue calls
    for (size_t i = 0; i < elements_to_fill; i++) {
      const size_t offset = i * sizeof(pattern);
      void *device_offset_ptr = getPointerOffset(device_ptr, offset);

      cl_int err = clEnqueueMemFillINTEL(fixture_queues[0], device_offset_ptr,
                                         &pattern, sizeof(pattern),
                                         sizeof(pattern), 0, nullptr, nullptr);
      EXPECT_SUCCESS(err);
    }

    if (host_ptr) {
      // Copy device data to host allocation, if supported, for verification
      const size_t offset = index * elements_to_fill * sizeof(pattern);
      void *offset_host_ptr = getPointerOffset(host_ptr, offset);
      err = clEnqueueMemcpyINTEL(fixture_queues[0], CL_FALSE, offset_host_ptr,
                                 device_ptr, fill_size, 0, nullptr, nullptr);
      EXPECT_SUCCESS(err);
    }

    // Block until all operations are complete, implicitly flushing
    EXPECT_SUCCESS(clMemBlockingFreeINTEL(context, device_ptr));
    device_ptrs[index] = nullptr;
  };

  for (size_t i = 0; i < threads; i++) {
    workers[i] = std::thread(worker, i);
  }

  for (size_t i = 0; i < threads; i++) {
    workers[i].join();
  }

  if (host_ptr) {
    // Verify data copied from device allocation into host allocation
    std::array<cl_uint, elements> reference;
    reference.fill(pattern);
    EXPECT_TRUE(0 == std::memcmp(reference.data(), host_ptr, bytes));
  }
}

// CA-3314: Disabled due to intermittent failure to verify results whilst we
// diagnose.
//
// Populates two device allocations, A & B, on their own queues using fill calls
// before copying them to a separate allocation C. The copy operation is
// enqueued on the queue C, so allocations A & B interact with multiple queues.
TEST_F(USMBlockingFreeTest, MultipleQueueMultipleAlloc) {
  std::array<cl_event, 2> events = {nullptr, nullptr};
  const cl_uint pattern_A = 42;
  auto &queue_A = fixture_queues[0];
  auto &device_ptr_A = fixture_device_ptrs[0];
  cl_int err =
      clEnqueueMemFillINTEL(queue_A, device_ptr_A, &pattern_A,
                            sizeof(pattern_A), bytes, 0, nullptr, &events[0]);
  EXPECT_SUCCESS(err);

  // Flush queue A manually, as flushing queue C won't propagate it's dependency
  // on A with an internal flush of A.
  EXPECT_SUCCESS(clFlush(queue_A));

  cl_uint pattern_B = 0xA;
  auto &queue_B = fixture_queues[1];
  auto &device_ptr_B = fixture_device_ptrs[1];
  err = clEnqueueMemFillINTEL(queue_B, device_ptr_B, &pattern_B,
                              sizeof(pattern_B), bytes, 0, nullptr, &events[1]);
  EXPECT_SUCCESS(err);

  // Flush queue B manually, as flushing queue C won't propagate it's dependency
  // on A with an internal flush of B.
  EXPECT_SUCCESS(clFlush(queue_B));

  // offset halfway into the allocation
  constexpr size_t halfway_offset = sizeof(cl_uint) * (elements / 2);

  auto &queue_C = fixture_queues[2];
  auto &device_ptr_C = fixture_device_ptrs[2];
  void *offset_ptr = getPointerOffset(device_ptr_C, halfway_offset);

  // Copy bytes from the start of allocation A to start of allocation C
  err = clEnqueueMemcpyINTEL(queue_C, CL_FALSE, device_ptr_C, device_ptr_A,
                             halfway_offset, 1, &events[0], nullptr);
  EXPECT_SUCCESS(err);

  // Copy bytes from the start allocation B to second half of allocation C
  err = clEnqueueMemcpyINTEL(queue_C, CL_FALSE, offset_ptr, device_ptr_B,
                             halfway_offset, 1, &events[1], nullptr);
  EXPECT_SUCCESS(err);

  if (host_ptr) {
    // Copy device data to host allocation, if supported, for verification
    err = clEnqueueMemcpyINTEL(queue_C, CL_FALSE, host_ptr, device_ptr_C, bytes,
                               0, nullptr, nullptr);
    EXPECT_SUCCESS(err);
  }

  // Block until all operations are complete, implicitly flushing.
  // Free allocation C first since it is dependent on A & B
  err = clMemBlockingFreeINTEL(context, device_ptr_C);
  EXPECT_SUCCESS(err);

  err = clMemBlockingFreeINTEL(context, device_ptr_B);
  EXPECT_SUCCESS(err);

  err = clMemBlockingFreeINTEL(context, device_ptr_A);
  EXPECT_SUCCESS(err);

  device_ptr_A = nullptr;
  device_ptr_B = nullptr;
  device_ptr_C = nullptr;

  if (host_ptr) {
    // Verify data copied from device allocation into host allocation
    std::array<cl_uint, elements / 2> reference1, reference2;
    reference1.fill(pattern_A);
    reference2.fill(pattern_B);
    void *offset_ptr = getPointerOffset(host_ptr, halfway_offset);
    EXPECT_TRUE(0 == std::memcmp(host_ptr, reference1.data(), halfway_offset));
    EXPECT_TRUE(0 ==
                std::memcmp(offset_ptr, reference2.data(), halfway_offset));
  }

  for (cl_event &event : events) {
    EXPECT_SUCCESS(clReleaseEvent(event));
  }
}

namespace {
// Fixture to help testing of clMemBlockingFreeINTEL with enqueued kernels
struct USMBlockingFreeKernelTest : public USMBlockingFreeTest {
  void SetUp() override {
    USMBlockingFreeTest::SetUp();
    if (IsSkipped()) {
      return;
    }

    if (!UCL::hasCompilerSupport(device)) {
      GTEST_SKIP();
    }

    const size_t length = std::strlen(source);
    cl_int err = !CL_SUCCESS;
    program = clCreateProgramWithSource(context, 1, &source, &length, &err);
    ASSERT_TRUE(program != nullptr);
    ASSERT_SUCCESS(err);

    ASSERT_SUCCESS(clBuildProgram(program, 1, &device, "",
                                  ucl::buildLogCallback, nullptr));
    kernel = clCreateKernel(program, "copy_kernel", &err);
    ASSERT_SUCCESS(err);
    ASSERT_TRUE(kernel != nullptr);
  }

  void TearDown() override {
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }

    if (kernel) {
      EXPECT_SUCCESS(clReleaseKernel(kernel));
    }
    USMBlockingFreeTest::TearDown();
  }

  static const char *source;
  cl_program program = nullptr;
  cl_kernel kernel = nullptr;
};

const char *USMBlockingFreeKernelTest::source =
    R"(
void kernel copy_kernel(__global int* in,
                __global int* out) {
   size_t id = get_global_id(0);
   out[id] = in[id];
}
)";
}  // namespace

TEST_F(USMBlockingFreeKernelTest, Task) {
  const size_t threads = 32;
  UCL::vector<std::thread> workers(threads);
  cl_command_queue queues[threads];

  void *input_usm_ptr = fixture_device_ptrs[0];
  void *output_usm_ptr = fixture_device_ptrs[1];

  const size_t elements_to_fill = elements / threads;
  auto worker = [&](size_t tid) {
    cl_int err;
    queues[tid] = clCreateCommandQueue(context, device, 0, &err);
    ASSERT_TRUE(queues[tid] != nullptr);
    ASSERT_SUCCESS(err);

    cl_kernel kernel = clCreateKernel(program, "copy_kernel", &err);
    ASSERT_SUCCESS(err);

    const cl_uint pattern = 'A' + tid;

    const size_t copy_size = sizeof(pattern) * elements_to_fill;
    const size_t offset = tid * copy_size;
    cl_uint *input_offset_ptr =
        static_cast<cl_uint *>(getPointerOffset(input_usm_ptr, offset));
    cl_uint *output_offset_ptr =
        static_cast<cl_uint *>(getPointerOffset(output_usm_ptr, offset));

    ASSERT_SUCCESS(clEnqueueMemFillINTEL(queues[tid], input_offset_ptr,
                                         &pattern, sizeof(pattern), copy_size,
                                         0, nullptr, nullptr));

    for (size_t element = 0; element < elements_to_fill; element++) {
      // Set kernel arguments
      ASSERT_SUCCESS(
          clSetKernelArgMemPointerINTEL(kernel, 0, &input_offset_ptr[element]));
      ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(
          kernel, 1, &output_offset_ptr[element]));

      ASSERT_SUCCESS(clEnqueueTask(queues[tid], kernel, 0, nullptr, nullptr));
    }
    EXPECT_SUCCESS(clReleaseKernel(kernel));
  };

  for (size_t i = 0; i < threads; i++) {
    workers[i] = std::thread(worker, i);
  }

  for (size_t i = 0; i < threads; i++) {
    workers[i].join();
  }

  // Block until all operations are complete, implicitly flushing all queues
  EXPECT_SUCCESS(clMemBlockingFreeINTEL(context, input_usm_ptr));
  fixture_device_ptrs[0] = nullptr;

  if (host_ptr) {
    ASSERT_SUCCESS(clEnqueueMemcpyINTEL(queues[0], CL_FALSE, host_ptr,
                                        output_usm_ptr, bytes, 0, nullptr,
                                        nullptr));
    EXPECT_SUCCESS(clMemBlockingFreeINTEL(context, output_usm_ptr));
    fixture_device_ptrs[1] = nullptr;

    // Verify data copied from device allocation into host allocation
    for (size_t i = 0; i < threads; i++) {
      std::array<cl_uint, elements_to_fill> reference;
      const cl_uint pattern = 'A' + i;
      reference.fill(pattern);

      const size_t offset = i * sizeof(pattern) * elements_to_fill;
      void *offset_host_ptr = getPointerOffset(host_ptr, offset);
      ASSERT_TRUE(0 == std::memcmp(reference.data(), offset_host_ptr,
                                   sizeof(pattern) * elements_to_fill));
    }
  } else {
    EXPECT_SUCCESS(clMemBlockingFreeINTEL(context, output_usm_ptr));
    fixture_device_ptrs[1] = nullptr;
  }

  for (size_t i = 0; i < threads; i++) {
    EXPECT_SUCCESS(clReleaseCommandQueue(queues[i]));
  }
}

TEST_F(USMBlockingFreeKernelTest, NDRange) {
  const size_t threads = 32;
  UCL::vector<std::thread> workers(threads);
  cl_command_queue queues[threads];

  void *input_usm_ptr = fixture_device_ptrs[0];
  void *output_usm_ptr = fixture_device_ptrs[1];

  const size_t elements_to_fill = elements / threads;
  auto worker = [&, elements_to_fill](size_t tid) {
    cl_int err;
    queues[tid] = clCreateCommandQueue(context, device, 0, &err);
    ASSERT_SUCCESS(err);

    cl_kernel kernel = clCreateKernel(program, "copy_kernel", &err);
    ASSERT_SUCCESS(err);

    const cl_uint pattern = 'A' + tid;
    const size_t copy_size = sizeof(pattern) * elements_to_fill;
    const size_t offset = copy_size * tid;

    // Set kernel arguments
    void *input_offset_ptr = getPointerOffset(input_usm_ptr, offset);
    ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 0, input_offset_ptr));

    void *output_offset_ptr = getPointerOffset(output_usm_ptr, offset);
    ASSERT_SUCCESS(clSetKernelArgMemPointerINTEL(kernel, 1, output_offset_ptr));

    ASSERT_SUCCESS(clEnqueueMemFillINTEL(queues[tid], input_offset_ptr,
                                         &pattern, sizeof(pattern), copy_size,
                                         0, nullptr, nullptr));

    ASSERT_SUCCESS(clEnqueueNDRangeKernel(queues[tid], kernel, 1, nullptr,
                                          &elements_to_fill, nullptr, 0,
                                          nullptr, nullptr));
    EXPECT_SUCCESS(clReleaseKernel(kernel));
  };

  for (size_t i = 0; i < threads; i++) {
    workers[i] = std::thread(worker, i);
  }

  for (size_t i = 0; i < threads; i++) {
    workers[i].join();
  }

  // Block until all operations are complete, implicitly flushing all queues
  EXPECT_SUCCESS(clMemBlockingFreeINTEL(context, input_usm_ptr));
  fixture_device_ptrs[0] = nullptr;

  // Verify data copied from device allocation into host allocation
  if (host_ptr) {
    ASSERT_SUCCESS(clEnqueueMemcpyINTEL(queues[0], CL_FALSE, host_ptr,
                                        output_usm_ptr, bytes, 0, nullptr,
                                        nullptr));
    EXPECT_SUCCESS(clMemBlockingFreeINTEL(context, output_usm_ptr));
    fixture_device_ptrs[1] = nullptr;

    for (size_t i = 0; i < threads; i++) {
      std::array<cl_uint, elements_to_fill> reference;
      const cl_uint pattern = 'A' + i;
      reference.fill(pattern);

      const size_t copy_size = sizeof(pattern) * elements_to_fill;
      const size_t offset = i * copy_size;
      void *offset_host_ptr = getPointerOffset(host_ptr, offset);

      ASSERT_TRUE(0 ==
                  std::memcmp(reference.data(), offset_host_ptr, copy_size));
    }
  } else {
    EXPECT_SUCCESS(clMemBlockingFreeINTEL(context, output_usm_ptr));
    fixture_device_ptrs[1] = nullptr;
  }

  for (size_t i = 0; i < threads; i++) {
    EXPECT_SUCCESS(clReleaseCommandQueue(queues[i]));
  }
}

TEST_F(USMBlockingFreeKernelTest, NegativeStatus) {
  cl_event event;
  const cl_uint pattern = 42;
  auto &queue = fixture_queues[0];
  auto &device_ptr = fixture_device_ptrs[0];
  cl_int error;
  event = clCreateUserEvent(context, &error);
  ASSERT_SUCCESS(error);

  error = clEnqueueMemFillINTEL(queue, device_ptr, &pattern, sizeof(pattern),
                                bytes, 1, &event, nullptr);

  // A negative integer value causes all enqueued commands
  // that wait on this user event to be terminated
  EXPECT_SUCCESS(clSetUserEventStatus(event, -1));
  EXPECT_SUCCESS(clReleaseEvent(event));
  EXPECT_SUCCESS(clMemBlockingFreeINTEL(context, device_ptr));
  fixture_device_ptrs[0] = nullptr;
}
