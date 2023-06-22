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

#include <UnitVK.h>

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkResetCommandBuffer

class ResetCommandBuffer : public uvk::PipelineTest {
 public:
  ResetCommandBuffer() {}

  virtual void SetUp() override {
    RETURN_ON_FATAL_FAILURE(PipelineTest::SetUp());

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkEndCommandBuffer(commandBuffer);
  }
};

TEST_F(ResetCommandBuffer, Default) {
  ASSERT_EQ_RESULT(VK_SUCCESS, vkResetCommandBuffer(commandBuffer, 0));
}

TEST_F(ResetCommandBuffer, DefaultReleaseResources) {
  ASSERT_EQ_RESULT(
      VK_SUCCESS,
      vkResetCommandBuffer(commandBuffer,
                           VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT));
}

// TODO: test with a command buffer allocated from a command pool that doesn't
// allow resetting individual command buffers

// VK_ERROR_OUT_OF_HOST_MEMORY
// Is a possible return from this function but is untestable
// as it doesn't take an allocator as a parameter.
//
// VK_ERROR_OUT_OF_DEVICE_MEMORY
// Is a possible return from this function, but is untestable
// due to the fact that we can't currently access device memory
// allocators to mess with.
