// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <UnitVK.h>

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkCreateBufferView

class CreateBufferView : public uvk::BufferTest, public uvk::DeviceMemoryTest {
 public:
  CreateBufferView()
      : BufferTest(128, VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT),
        DeviceMemoryTest(true),
        bufferViewCreateInfo(),
        bufferView(VK_NULL_HANDLE) {}

  virtual void SetUp() override {
    RETURN_ON_FATAL_FAILURE(BufferTest::SetUp());

    DeviceMemoryTest::memorySize = BufferTest::bufferMemoryRequirements.size;
    RETURN_ON_FATAL_FAILURE(DeviceMemoryTest::SetUp());

    vkBindBufferMemory(device, buffer, memory, 0);

    bufferViewCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
    bufferViewCreateInfo.range = VK_WHOLE_SIZE;
    bufferViewCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
    bufferViewCreateInfo.buffer = buffer;
  }

  virtual void TearDown() override {
    if (bufferView) {
      vkDestroyBufferView(device, bufferView, nullptr);
    }

    DeviceMemoryTest::TearDown();
    BufferTest::TearDown();
  }

  VkBufferViewCreateInfo bufferViewCreateInfo;
  VkBufferView bufferView;
};

TEST_F(CreateBufferView, Default) {
  ASSERT_EQ_RESULT(VK_SUCCESS, vkCreateBufferView(device, &bufferViewCreateInfo,
                                                  nullptr, &bufferView));
}

TEST_F(CreateBufferView, DefaultAllocator) {
  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkCreateBufferView(device, &bufferViewCreateInfo,
                                      uvk::defaultAllocator(), &bufferView));

  vkDestroyBufferView(device, bufferView, uvk::defaultAllocator());
  bufferView = VK_NULL_HANDLE;
}

// TODO: a test which pushes one of these in a descriptor set update

TEST_F(CreateBufferView, ErrorOutOfHostMemory) {
  ASSERT_EQ_RESULT(VK_ERROR_OUT_OF_HOST_MEMORY,
                   vkCreateBufferView(device, &bufferViewCreateInfo,
                                      uvk::nullAllocator(), &bufferView));
}

// VK_ERROR_OUT_OF_DEVICE_MEMORY
// Is a possible return from this function, but is untestable
// due to the fact that we can't currently access device memory
// allocators to mess with
