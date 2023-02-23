// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <UnitVK.h>

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkResetEvent

class ResetEvent : public uvk::DeviceTest {
public:
  ResetEvent() : createInfo(), event(VK_NULL_HANDLE) {}

  virtual void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());

    createInfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
  }

  virtual void TearDown() override {
    if (event) {
      vkDestroyEvent(device, event, nullptr);
    }

    DeviceTest::TearDown();
  }

  VkEventCreateInfo createInfo;
  VkEvent event;
};

TEST_F(ResetEvent, Default) {
  ASSERT_EQ_RESULT(VK_SUCCESS,
                   vkCreateEvent(device, &createInfo, nullptr, &event));
  ASSERT_EQ_RESULT(VK_SUCCESS, vkSetEvent(device, event));
  ASSERT_EQ_RESULT(VK_EVENT_SET, vkGetEventStatus(device, event));
  ASSERT_EQ_RESULT(VK_SUCCESS, vkResetEvent(device, event));
  ASSERT_EQ_RESULT(VK_EVENT_RESET, vkGetEventStatus(device, event));
}

TEST_F(ResetEvent, DefaultAllocator) {
  ASSERT_EQ_RESULT(VK_SUCCESS, vkCreateEvent(device, &createInfo,
                                             uvk::defaultAllocator(), &event));
  ASSERT_EQ_RESULT(VK_SUCCESS, vkSetEvent(device, event));
  ASSERT_EQ_RESULT(VK_EVENT_SET, vkGetEventStatus(device, event));
  ASSERT_EQ_RESULT(VK_SUCCESS, vkResetEvent(device, event));
  ASSERT_EQ_RESULT(VK_EVENT_RESET, vkGetEventStatus(device, event));
  vkDestroyEvent(device, event, uvk::defaultAllocator());
  event = VK_NULL_HANDLE;
}

// VK_ERROR_OUT_OF_HOST_MEMORY
// Is a possible return from this function but is untestable
// as it doesn't take an allocator as a parameter.
//
// VK_ERROR_OUT_OF_DEVICE_MEMORY
// Is a possible return from this function, but is untestable
// due to the fact that we can't currently access device memory
// allocators to mess with
