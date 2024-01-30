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

#include <cargo/string_view.h>

#include <algorithm>
#include <cmath>
#include <limits>

#include "Common.h"
#include "Device.h"
#include "EventWaitList.h"

struct NDRangeValue {
  cl_uint work_dim;
  size_t *global_work_offset;
  size_t *global_work_size;
  size_t *local_work_size;

  NDRangeValue(const NDRangeValue &other)
      : work_dim(other.work_dim),
        global_work_offset(other.global_work_offset ? new size_t[other.work_dim]
                                                    : nullptr),
        global_work_size(other.global_work_size ? new size_t[other.work_dim]
                                                : nullptr),
        local_work_size(other.local_work_size ? new size_t[other.work_dim]
                                              : nullptr) {
    const size_t length = sizeof(size_t) * work_dim;
    if (other.global_work_offset) {
      memcpy(global_work_offset, other.global_work_offset, length);
    }

    if (other.global_work_size) {
      memcpy(global_work_size, other.global_work_size, length);
    }

    if (other.local_work_size) {
      memcpy(local_work_size, other.local_work_size, length);
    }
  }

  NDRangeValue &operator=(const NDRangeValue &other) {
    if (this == &other) {
      return *this;
    }

    work_dim = other.work_dim;
    global_work_offset =
        (other.global_work_offset) ? new size_t[other.work_dim] : nullptr;
    global_work_size =
        (other.global_work_size) ? new size_t[other.work_dim] : nullptr;
    local_work_size =
        (other.local_work_size) ? new size_t[other.work_dim] : nullptr;

    const size_t length = sizeof(size_t) * work_dim;
    if (other.global_work_offset) {
      memcpy(global_work_offset, other.global_work_offset, length);
    }

    if (other.global_work_size) {
      memcpy(global_work_size, other.global_work_size, length);
    }

    if (other.local_work_size) {
      memcpy(local_work_size, other.local_work_size, length);
    }

    return *this;
  }

  NDRangeValue(const cl_uint _work_dim, const size_t *const _global_work_offset,
               const size_t *const _global_work_size,
               const size_t *const _local_work_size)
      : work_dim(_work_dim),
        global_work_offset(_global_work_offset ? new size_t[_work_dim]
                                               : nullptr),
        global_work_size(_global_work_size ? new size_t[_work_dim] : nullptr),
        local_work_size(_local_work_size ? new size_t[_work_dim] : nullptr) {
    const size_t length = sizeof(size_t) * work_dim;
    if (_global_work_offset) {
      memcpy(global_work_offset, _global_work_offset, length);
    }

    if (_global_work_size) {
      memcpy(global_work_size, _global_work_size, length);
    }

    if (_local_work_size) {
      memcpy(local_work_size, _local_work_size, length);
    }
  }

  ~NDRangeValue() {
    if (global_work_offset) {
      delete[] global_work_offset;
    }

    if (global_work_size) {
      delete[] global_work_size;
    }

    if (local_work_size) {
      delete[] local_work_size;
    }
  }
};

struct clEnqueueNDRangeKernel_Host : ucl::CommandQueueTest,
                                     testing::WithParamInterface<NDRangeValue> {
#pragma pack(4)
  struct PerItemKernelInfo {
    // has to match the one in the kernel source
    cl_ulong4 global_size;
    cl_ulong4 global_id;
    cl_ulong4 local_size;
    cl_ulong4 local_id;
    cl_ulong4 num_groups;
    cl_ulong4 group_id;
    cl_ulong4 global_offset;
    cl_uint work_dim;
  };

  enum { NUM_DIMENSIONS = 3, DEFAULT_DIMENSION_LENGTH = 128 };

 protected:
  void SetUp() override {
    UCL_RETURN_ON_FATAL_FAILURE(CommandQueueTest::SetUp());
    if (!(getDeviceCompilerAvailable() && UCL::isDevice_host(device))) {
      GTEST_SKIP();
    }

    // NOTE: To avoid allocating to much memory on devices sharing resources
    // with other applications (such as parallel testing), be conservative
    // about the buffer max_mem size.
    const cl_ulong max_mem = getDeviceMaxMemAllocSize() / 8;

    const cl_ulong items = max_mem / sizeof(PerItemKernelInfo);
    const size_t possible_dimension_length = static_cast<size_t>(
        std::floor(std::pow(static_cast<double>(items), 1.0 / NUM_DIMENSIONS)));
    dimension_length = std::min(possible_dimension_length,
                                static_cast<size_t>(DEFAULT_DIMENSION_LENGTH));
    mem_size = sizeof(PerItemKernelInfo);
    for (int i = 0; i < NUM_DIMENSIONS; ++i) {
      mem_size *= dimension_length;
    }

    cl_int errorcode;
    mem = clCreateBuffer(context, 0, mem_size, nullptr, &errorcode);
    // NOTE: If buffer creation fails, reduce the size.
    while (CL_MEM_OBJECT_ALLOCATION_FAILURE == errorcode ||
           CL_OUT_OF_RESOURCES == errorcode) {
      dimension_length /= 2;
      mem_size /= 8;
      mem = clCreateBuffer(context, 0, mem_size, nullptr, &errorcode);
    }
    ASSERT_TRUE(mem);
    ASSERT_SUCCESS(errorcode);

    const char *source =
        "struct __attribute__ ((packed)) PerItemKernelInfo {\n"
        "  ulong4 global_size;\n"
        "  ulong4 global_id;\n"
        "  ulong4 local_size;\n"
        "  ulong4 local_id;\n"
        "  ulong4 num_groups;\n"
        "  ulong4 group_id;\n"
        "  ulong4 global_offset;\n"
        "  uint work_dim;\n"
        "};\n"
        "void kernel foo(global struct PerItemKernelInfo * info) {\n"
        "  size_t xId = get_global_id(0);\n"
        "  size_t yId = get_global_id(1);\n"
        "  size_t zId = get_global_id(2);\n"
        "  size_t id = xId + (get_global_size(0) * yId) +\n"
        "               (get_global_size(0) * get_global_size(1) * zId);\n"
        "  info[id].global_size = (ulong4)(get_global_size(0),\n"
        "                                  get_global_size(1),\n"
        "                                  get_global_size(2),\n"
        "                                  get_global_size(3));\n"
        "  info[id].global_id = (ulong4)(get_global_id(0),\n"
        "                                get_global_id(1),\n"
        "                                get_global_id(2),\n"
        "                                get_global_id(3));\n"
        "  info[id].local_size = (ulong4)(get_local_size(0),\n"
        "                                 get_local_size(1),\n"
        "                                 get_local_size(2),\n"
        "                                 get_local_size(3));\n"
        "  info[id].local_id = (ulong4)(get_local_id(0),\n"
        "                               get_local_id(1),\n"
        "                               get_local_id(2),\n"
        "                               get_local_id(3));\n"
        "  info[id].num_groups = (ulong4)(get_num_groups(0),\n"
        "                                 get_num_groups(1),\n"
        "                                 get_num_groups(2),\n"
        "                                 get_num_groups(3));\n"
        "  info[id].group_id = (ulong4)(get_group_id(0),\n"
        "                               get_group_id(1),\n"
        "                               get_group_id(2), get_group_id(3));\n"
        "  info[id].global_offset = (ulong4)(get_global_offset(0),\n"
        "                                    get_global_offset(1),\n"
        "                                    get_global_offset(2),\n"
        "                                    get_global_offset(3));\n"
        "  info[id].work_dim = get_work_dim();\n"
        "}\n";
    program =
        clCreateProgramWithSource(context, 1, &source, nullptr, &errorcode);
    ASSERT_TRUE(program);
    ASSERT_SUCCESS(errorcode);
    ASSERT_SUCCESS(
        clBuildProgram(program, 0, nullptr, nullptr, nullptr, nullptr));

    kernel = clCreateKernel(program, "foo", &errorcode);
    EXPECT_TRUE(kernel);
    ASSERT_SUCCESS(errorcode);

    ASSERT_SUCCESS(clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&mem));
  }

  void TearDown() override {
    if (mem) {
      EXPECT_SUCCESS(clReleaseMemObject(mem));
    }
    if (kernel) {
      EXPECT_SUCCESS(clReleaseKernel(kernel));
    }
    if (program) {
      EXPECT_SUCCESS(clReleaseProgram(program));
    }
    CommandQueueTest::TearDown();
  }

  size_t dimension_length = 0;
  size_t mem_size = 0;
  cl_mem mem = nullptr;
  cl_program program = nullptr;
  cl_kernel kernel = nullptr;
};

// In the case where the `preferred_local_size` of the device is not evenly
// divisible we instead half the preferred size until we find a value that fits.
// Previously we would default to `1` straight away but instead we will at least
// try some smaller values.
TEST_F(clEnqueueNDRangeKernel_Host, PreferredLocalWorkSizeNotDivisible) {
  // Since this is a host specific test we want to skip it if we aren't running
  // on host.
  const size_t global_work_offset[] = {0, 0, 0};
  const size_t global_work_size[] = {96, 64, 1};

  const NDRangeValue val =
      NDRangeValue(3, global_work_offset, global_work_size, nullptr);
  cl_event fillEvent, ndRangeEvent;

  if (nullptr == val.global_work_size) {
    // This exists to convince static analysis that this path is unreachable.
    FAIL() << "No global work size specified";
  }

  if (!UCL::hasLocalWorkSizeSupport(device, NUM_DIMENSIONS,
                                    val.local_work_size)) {
    GTEST_SKIP();
  }

  const char pattern = 0;

  ASSERT_SUCCESS(clEnqueueFillBuffer(command_queue, mem, &pattern,
                                     sizeof(pattern), 0, mem_size, 0, nullptr,
                                     &fillEvent));

  ASSERT_SUCCESS(clEnqueueNDRangeKernel(
      command_queue, kernel, NUM_DIMENSIONS, val.global_work_offset,
      val.global_work_size, nullptr, 1, &fillEvent, &ndRangeEvent));

  cl_int errorcode = !CL_SUCCESS;
  PerItemKernelInfo *const infos = reinterpret_cast<PerItemKernelInfo *>(
      clEnqueueMapBuffer(command_queue, mem, true, CL_MAP_READ, 0, mem_size, 1,
                         &ndRangeEvent, nullptr, &errorcode));
  ASSERT_TRUE(infos);
  ASSERT_SUCCESS(errorcode);

  for (cl_uint x = 0; x < val.global_work_size[0]; x++) {
    for (cl_uint y = 0; y < val.global_work_size[1]; y++) {
      for (cl_uint z = 0; z < val.global_work_size[2]; z++) {
        // Copy the PerItemKernelInfo, as the version in the buffer is not
        // guaranteed to match the C++ alignment requirements of all the
        // struct's members (i.e. cl_ulong4).
        PerItemKernelInfo info;
        memcpy(&info,
               &infos[x + (y * val.global_work_size[0]) +
                      (z * val.global_work_size[0] * val.global_work_size[1])],
               sizeof(PerItemKernelInfo));

        // The preferred size on host is 64 so the local size in this case
        // should become 32.

        ASSERT_EQ(32u, info.local_size.s[0]);
        ASSERT_EQ(4u, info.local_size.s[1]);
        ASSERT_EQ(1u, info.local_size.s[2]);
        ASSERT_EQ(1u, info.local_size.s[3]);

        ASSERT_EQ(NUM_DIMENSIONS, info.work_dim);
      }
    }
  }

  ASSERT_SUCCESS(
      clEnqueueUnmapMemObject(command_queue, mem, infos, 0, nullptr, nullptr));

  ASSERT_SUCCESS(clReleaseEvent(fillEvent));
  ASSERT_SUCCESS(clReleaseEvent(ndRangeEvent));
}
