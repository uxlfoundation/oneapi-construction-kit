// Copyright (C) Codeplay Software Limited. All Rights Reserved.

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
