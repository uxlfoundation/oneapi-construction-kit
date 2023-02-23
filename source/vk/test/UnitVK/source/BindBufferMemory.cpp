// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <UnitVK.h>

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkBindBufferMemory

class BindBufferMemory : public uvk::BufferTest, public uvk::DeviceMemoryTest {
 public:
  BindBufferMemory() : BufferTest(32), DeviceMemoryTest(true) {}

  virtual void SetUp() override {
    RETURN_ON_FATAL_FAILURE(BufferTest::SetUp());

    DeviceMemoryTest::memorySize = BufferTest::bufferMemoryRequirements.size;
    RETURN_ON_FATAL_FAILURE(DeviceMemoryTest::SetUp());
  }

  virtual void TearDown() override {
    DeviceMemoryTest::TearDown();
    BufferTest::TearDown();
  }
};

TEST_F(BindBufferMemory, Default) {
  ASSERT_EQ_RESULT(VK_SUCCESS, vkBindBufferMemory(device, buffer, memory, 0));
}

// VK_ERROR_OUT_OF_HOST_MEMORY
// Is a possible return from this function but is untestable
// as it doesn't take an allocator as a parameter.
//
// VK_ERROR_OUT_OF_DEVICE_MEMORY
// Is a possible return from this function, but is untestable
// due to the fact that we can't currently access device memory
// allocators to mess with.
