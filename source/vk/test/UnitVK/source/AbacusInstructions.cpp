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

class AbacusInstructions : public uvk::SimpleKernelTest {
 public:
  // we set the shaders in the tests themselves since that's what we're testing
  AbacusInstructions() : SimpleKernelTest(false, uvk::Shader::nop) {}

  // blank setup so we can defer setting up PipelineTest until after we've set a
  // shader in the test
  virtual void SetUp() override {}
};

TEST_F(AbacusInstructions, OpAll) {
  PipelineTest::shader = uvk::Shader::all;

  RETURN_ON_FATAL_FAILURE(SimpleKernelTest::SetUp());

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  FlushFromDevice();

  void* bufferMemory = SimpleKernelTest::PtrTo1stBufferData();

  ASSERT_TRUE(reinterpret_cast<uint32_t*>(bufferMemory)[0]);
  ASSERT_FALSE(reinterpret_cast<uint32_t*>(bufferMemory)[1]);
}

TEST_F(AbacusInstructions, OpAny) {
  PipelineTest::shader = uvk::Shader::any;

  RETURN_ON_FATAL_FAILURE(SimpleKernelTest::SetUp());

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  FlushFromDevice();

  void* bufferMemory = SimpleKernelTest::PtrTo1stBufferData();

  ASSERT_TRUE(reinterpret_cast<uint32_t*>(bufferMemory)[0]);
  ASSERT_FALSE(reinterpret_cast<uint32_t*>(bufferMemory)[1]);
}

TEST_F(AbacusInstructions, OpBitCount) {
  PipelineTest::shader = uvk::Shader::bitcount;

  RETURN_ON_FATAL_FAILURE(SimpleKernelTest::SetUp());

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  FlushFromDevice();

  void* bufferMemory = SimpleKernelTest::PtrTo1stBufferData();

  // the test value we are passing into bitCount is 42, which contains 3 1s
  ASSERT_EQ(reinterpret_cast<uint32_t*>(bufferMemory)[0], 3u);
}

TEST_F(AbacusInstructions, OpDot) {
  PipelineTest::shader = uvk::Shader::dot;

  RETURN_ON_FATAL_FAILURE(SimpleKernelTest::SetUp());

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  FlushFromDevice();

  void* bufferMemory = SimpleKernelTest::PtrTo1stBufferData();

  // the vectors we are dot()-ing are both (2.f, 2.f, 2.f), hence our result
  // should be 12.f
  ASSERT_EQ(reinterpret_cast<float*>(bufferMemory)[0], 12.f);
}

TEST_F(AbacusInstructions, OpFMod) {
  PipelineTest::shader = uvk::Shader::fmod;

  RETURN_ON_FATAL_FAILURE(SimpleKernelTest::SetUp());

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  FlushFromDevice();

  void* bufferMemory = SimpleKernelTest::PtrTo1stBufferData();

  ASSERT_EQ(reinterpret_cast<float*>(bufferMemory)[0], 18.f);
}

TEST_F(AbacusInstructions, OpIsInf) {
  PipelineTest::shader = uvk::Shader::isinf;

  RETURN_ON_FATAL_FAILURE(SimpleKernelTest::SetUp());

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  FlushFromDevice();

  void* bufferMemory = SimpleKernelTest::PtrTo1stBufferData();

  ASSERT_TRUE(reinterpret_cast<uint32_t*>(bufferMemory)[0]);
  ASSERT_FALSE(reinterpret_cast<uint32_t*>(bufferMemory)[1]);
}

TEST_F(AbacusInstructions, OpIsNan) {
  PipelineTest::shader = uvk::Shader::isnan;

  RETURN_ON_FATAL_FAILURE(SimpleKernelTest::SetUp());

  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
  ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));

  FlushFromDevice();

  void* bufferMemory = SimpleKernelTest::PtrTo1stBufferData();

  // we are only testing the negative case here because even doing 0/0 is
  // technically implementation defined so not a guaranteed NaN
  ASSERT_FALSE(reinterpret_cast<uint32_t*>(bufferMemory)[0]);
}
