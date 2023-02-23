// Copyright (C) Codeplay Software Limited. All Rights Reserved.

#include <UnitVK.h>

// https://www.khronos.org/registry/vulkan/specs/1.0/xhtml/vkspec.html#vkCreateShaderModule

class CreateShaderModule : public uvk::DeviceTest {
public:
  CreateShaderModule() : createInfo(), shaderModule(VK_NULL_HANDLE) {}

  virtual void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());

    uvk::ShaderCode shaderCode = uvk::getShader(uvk::Shader::nop);
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = shaderCode.size;
    createInfo.pCode = (uint32_t *)shaderCode.code;
  }

  virtual void TearDown() override {
    if (shaderModule) {
      vkDestroyShaderModule(device, shaderModule, nullptr);
    }
    DeviceTest::TearDown();
  }

  VkShaderModuleCreateInfo createInfo;
  VkShaderModule shaderModule;
};

TEST_F(CreateShaderModule, Default) {
  ASSERT_EQ_RESULT(VK_SUCCESS, vkCreateShaderModule(device, &createInfo,
                                                    nullptr, &shaderModule));
}

TEST_F(CreateShaderModule, DefaultAllocator) {
  ASSERT_EQ_RESULT(VK_SUCCESS, vkCreateShaderModule(device, &createInfo,
                                                    uvk::defaultAllocator(),
                                                    &shaderModule));
  vkDestroyShaderModule(device, shaderModule, uvk::defaultAllocator());
  shaderModule = VK_NULL_HANDLE;
}

TEST_F(CreateShaderModule, ErrorOutOfHostMemory) {
  ASSERT_EQ_RESULT(VK_ERROR_OUT_OF_HOST_MEMORY,
                   vkCreateShaderModule(device, &createInfo,
                                        uvk::nullAllocator(), &shaderModule));
}

// VK_ERROR_OUT_OF_DEVICE_MEMORY
// Is a possible return from this function, but is untestable due to the fact
// that we can't currently access device memory  allocators to mess with.
