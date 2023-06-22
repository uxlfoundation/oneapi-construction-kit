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

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkResetCommandPool

class ResetCommandPool : public uvk::PipelineTest {
 public:
  ResetCommandPool() {}

  virtual void SetUp() override {
    RETURN_ON_FATAL_FAILURE(PipelineTest::SetUp());

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkEndCommandBuffer(commandBuffer);
  }
};

TEST_F(ResetCommandPool, Default) {
  ASSERT_EQ_RESULT(VK_SUCCESS, vkResetCommandPool(device, commandPool, 0));
}

TEST_F(ResetCommandPool, DefaultFlagReleaseResources) {
  // this still resets the command buffers but the flag does nothing as command
  // pools are not fully implemented
  ASSERT_EQ_RESULT(
      VK_SUCCESS,
      vkResetCommandPool(device, commandPool,
                         VK_COMMAND_POOL_RESET_RELEASE_RESOURCES_BIT));
}

// VK_ERROR_OUT_OF_HOST_MEMORY
// Is a possible return from this function but is untestable
// as it doesn't take an allocator as a parameter.
//
// VK_ERROR_OUT_OF_DEVICE_MEMORY
// Is a possible return from this function, but is untestable
// due to the fact that we can't currently access device memory
// allocators to mess with.
