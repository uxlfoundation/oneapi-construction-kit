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

#include <common.h>

#include <climits>

TEST_F(MultiDeviceCommandQueue, SubBuffers) {
  // Query the minimum sub-buffer alignment for all devices in the context.
  //
  // The OpenCL spec has the following wording:
  // clCreateSubBuffer will return CL_MISALIGNED_SUB_BUFFER_OFFSET if there are
  // no devices in context associated with buffer for which the origin field of
  // the cl_buffer_region structure passed in buffer_create_info is aligned to
  // the CL_DEVICE_MEM_BASE_ADDR_ALIGN value
  //
  // This seems slightly odd, if the sub-buffer can be aligned to a particular
  // value on one device, it doesn't mean it can on the others, the lowest
  // common multiple of all alignments seems like the correct value, here we
  // just take the largest alignment and hope it is divisible by the other
  // alignments.
  cl_uint max_alignment = 0;
  for (unsigned i = 0; i < devices.size(); ++i) {
    cl_uint alignment = 0;
    ASSERT_EQ_ERRCODE(CL_SUCCESS,
                      clGetDeviceInfo(devices[i], CL_DEVICE_MEM_BASE_ADDR_ALIGN,
                                      sizeof(cl_uint), &alignment, nullptr));
    max_alignment = std::max(alignment, max_alignment);
  }

  // Convert into bytes from bits;
  max_alignment *= CHAR_BIT;

  // Create a large buffer we will break into sub-buffers.
  const auto buffer_size = devices.size() * max_alignment;
  cl_int error = !CL_SUCCESS;
  cl_mem buffer =
      clCreateBuffer(context, CL_MEM_READ_WRITE, buffer_size, nullptr, &error);
  ASSERT_EQ_ERRCODE(CL_SUCCESS, error);

  std::vector<cl_mem> subBuffers(devices.size());
  for (size_t index = 0; index < subBuffers.size(); ++index) {
    // Create a sub-buffer for each device, each of size 4 bytes at stride of
    // the maximum alignment for all devices in the context
    cl_buffer_region subBufferRegion = {max_alignment * index, sizeof(cl_int)};
    subBuffers[index] = clCreateSubBuffer(buffer, CL_MEM_READ_WRITE,
                                          CL_BUFFER_CREATE_TYPE_REGION,
                                          &subBufferRegion, &error);
    ASSERT_EQ_ERRCODE(CL_SUCCESS, error);
  }

  for (unsigned index = 0; index < subBuffers.size(); index++) {
    // Have the command queue for each device write the index of the
    // device/command queue into the sub-buffer corresponding to that device
    // index.
    ASSERT_EQ_ERRCODE(
        CL_SUCCESS,
        clEnqueueWriteBuffer(command_queues[index], subBuffers[index], CL_TRUE,
                             0, sizeof(cl_int), &index, 0, nullptr, nullptr));
  }

  // Then verify the results are consistent across all queues and all
  // sub-buffers.
  for (size_t commandQueueIndex = 0; commandQueueIndex < command_queues.size();
       commandQueueIndex++) {
    for (size_t subBufferIndex = 0; subBufferIndex < subBuffers.size();
         subBufferIndex++) {
      cl_int result = -1;
      ASSERT_EQ_ERRCODE(
          CL_SUCCESS,
          clEnqueueReadBuffer(command_queues[commandQueueIndex],
                              subBuffers[subBufferIndex], CL_TRUE, 0,
                              sizeof(cl_int), &result, 0, nullptr, nullptr));

      EXPECT_EQ(subBufferIndex, result)
          << "\tdata in "
          << "subBuffer[" << subBufferIndex << "] on commandQueue["
          << commandQueueIndex << "] is invalid";
    }
  }

  for (auto subBuffer : subBuffers) {
    ASSERT_EQ(CL_SUCCESS, clReleaseMemObject(subBuffer));
  }
  ASSERT_EQ(CL_SUCCESS, clReleaseMemObject(buffer));
}
