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

#ifndef UNITVK_H_INCLUDED
#define UNITVK_H_INCLUDED

#include <ShaderCode.h>
#include <gtest/gtest.h>
#include <vulkan/vulkan.h>

#include <algorithm>
#include <array>
#include <cstring>
#include <fstream>

namespace uvk {
/// @brief A default custom allocator.
///
/// This function returns a pointer to a pre populated VkAllocationCallbacks
/// structure, it contains function pointers to the allocation functions below.
///
/// @return Return constant pointer to the default custom allocator.
const VkAllocationCallbacks *defaultAllocator();

/// @brief A custom allocator which will always return nullptr
///
/// This function returns a pointer to a VkAllocation callbacks which will
/// only ever return nullptr from its allocation functions, for the purpose
/// of forcing VK_ERROR_OUT_OF_HOST_MEMORY
///
/// @return Return constant pointer to the null allocator
const VkAllocationCallbacks *nullAllocator();

/// @brief A custom allocator which will only make one succesful allocation
///
/// This is for the one or two cases where we need to succesfully allocate a
/// pool and then prompt an error by unsuccessfully allocating from that pool
/// with the same allocator
///
/// @return Return constant pointer to the allocator
const VkAllocationCallbacks *oneUseAllocator(bool *used);

/// @brief Default allocate memory function.
///
/// @param pUserData User data specified by the application.
/// @param size Size in bytes of the requested allocation.
/// @param alignment Alignment in bytes of the requested allocation.
/// @param allocationScope Lifetime scope of the requested allocation.
///
/// @return Return void pointer to allocated memory, or null.
void *VKAPI_CALL alloc(void *pUserData, size_t size, size_t alignment,
                       VkSystemAllocationScope allocationScope);

/// @brief Default re-allocate memory function.
///
/// @param pUserData User data specified by the application.
/// @param pOriginal Previously allocated memory, or null.
/// @param size Size in bytes of the requested allocation.
/// @param alignment Alignment size in bytes of the requested allocation.
/// @param allocationScope Lifetime scope of the requested allocation.
///
/// @return Return void pointer to re-allocated memory, or null.
void *VKAPI_CALL realloc(void *pUserData, void *pOriginal, size_t size,
                         size_t alignment,
                         VkSystemAllocationScope allocationScope);

/// @brief Default free memory function.
///
/// @param pUserData User data specified by the application.
/// @param pMemory Previously allocated memory.
void VKAPI_CALL free(void *pUserData, void *pMemory);

/// @brief Default internal allocate memory notification function.
///
/// @param pUserData User data specified by the application.
/// @param size Size in bytes of the internal allocation.
/// @param allocationType Type of the internal allocation.
/// @param allocationScope Lifetime scope of the internal allocation.
void VKAPI_CALL allocNotify(void *pUserData, size_t size,
                            VkInternalAllocationType allocationType,
                            VkSystemAllocationScope allocationScope);

/// @brief Default internal free memory notification function.
///
/// @param pUserData User data specified by the application.
/// @param size Size in bytes of the internal free.
/// @param allocationType Type of the internal free.
/// @param allocationScope Lifetime scope of the internal free.
void VKAPI_CALL freeNotify(void *pUserData, size_t size,
                           VkInternalAllocationType allocationType,
                           VkSystemAllocationScope allocationScope);

/// @brief Class to translate Vulkan return codes into human readable output
///
/// Used by the ASSERT_EQ_RESULT and EXPECT_EQ_RESULT macros
/// Heavily inspired by (stolen from) ErrcodeRetWrapper in UnitCL
class Result {
 public:
  /// @brief Constructor
  /// @param resultCode VkResult to be translated
  Result(VkResult resultCode);

  std::string description() const;

  bool operator==(const Result &rhs) const {
    return resultCode == rhs.resultCode;
  }

  int resultCode;
};

/// @brief Operator for printing Result objects
inline std::ostream &operator<<(std::ostream &os, Result result) {
  return os << result.description();
}

/// @brief Macro for using ASSERT_EQ on expected VkResult return values
///
/// Makes use of the Result class to show the name of the return code
/// in the google test output, instead of just a number
#ifndef ASSERT_EQ_RESULT
#define ASSERT_EQ_RESULT(val1, val2) \
  ASSERT_EQ(::uvk::Result(val1), ::uvk::Result(val2));
#endif

/// @brief Macro for using EXPECT_EQ on expected VkResult return values
///
/// Makes use of the Result class to show the name of the return code
/// in the google test output, instead of just a number
#ifndef EXPECT_EQ_RESULT
#define EXPECT_EQ_RESULT(val1, val2) \
  EXPECT_EQ(uvk::Result(val1), uvk::Result(val2));
#endif

/// @brief Return if a fatal failure occurred invoking an expression.
///
/// Intended for use in test fixture `SetUp()` calls which explicitly call the
/// base class `SetUp()`, if a fatal error occurs in the base class immediately
/// return to avoid crashing the test suite by using uninitialized state.
///
/// @param ... Expression to invoke.
#define RETURN_ON_FATAL_FAILURE(...) \
  __VA_ARGS__;                       \
  if (HasFatalFailure()) {           \
    return;                          \
  }                                  \
  (void)0

/// @brief Test fixture to inherit from when a default instance is needed.
class InstanceTest : public testing::Test {
 public:
  InstanceTest()
      : applicationInfo(), instanceCreateInfo(), instance(VK_NULL_HANDLE) {
    instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &applicationInfo;
  }

  virtual void SetUp() {
    const std::array<const char *, 5> validationLayers{
        {"VK_LAYER_GOOGLE_threading", "VK_LAYER_LUNARG_parameter_validation",
         "VK_LAYER_LUNARG_object_tracker", "VK_LAYER_LUNARG_core_validation",
         "VK_LAYER_GOOGLE_unique_objects"}};

    uint32_t layerPropertyCount;
    ASSERT_EQ_RESULT(VK_SUCCESS, vkEnumerateInstanceLayerProperties(
                                     &layerPropertyCount, nullptr));
    std::vector<VkLayerProperties> layerProperties(layerPropertyCount);
    ASSERT_EQ_RESULT(
        VK_SUCCESS, vkEnumerateInstanceLayerProperties(&layerPropertyCount,
                                                       layerProperties.data()));

    for (const char *layerName : validationLayers) {
      for (VkLayerProperties properties : layerProperties) {
        if (strcmp(layerName, properties.layerName) == 0) {
          enabledLayerNames.push_back(layerName);
        }
      }
    }

    instanceCreateInfo.ppEnabledLayerNames = enabledLayerNames.data();
    instanceCreateInfo.enabledLayerCount = enabledLayerNames.size();

    const std::array<const char *, 1> extensions{
        {"VK_KHR_get_physical_device_properties2"},
    };

    uint32_t extensionPropertyCount;
    ASSERT_EQ_RESULT(VK_SUCCESS,
                     vkEnumerateInstanceExtensionProperties(
                         nullptr, &extensionPropertyCount, nullptr));
    std::vector<VkExtensionProperties> extensionProperties(
        extensionPropertyCount);
    ASSERT_EQ_RESULT(VK_SUCCESS, vkEnumerateInstanceExtensionProperties(
                                     nullptr, &extensionPropertyCount,
                                     extensionProperties.data()));

    for (const char *extensionNameCstr : extensions) {
      const std::string extensionName(extensionNameCstr);
      for (VkExtensionProperties properties : extensionProperties) {
        if (extensionName == properties.extensionName) {
          enabledInstanceExtensionNames.push_back(extensionNameCstr);
        }
      }
    }

    instanceCreateInfo.ppEnabledExtensionNames =
        enabledInstanceExtensionNames.data();
    instanceCreateInfo.enabledExtensionCount =
        enabledInstanceExtensionNames.size();

    applicationInfo = {};
    applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    applicationInfo.pApplicationName = "UnitVK";
    applicationInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    applicationInfo.pEngineName = "Codeplay Vulkan Compute Test Suite";
    applicationInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
    applicationInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);

    ASSERT_EQ_RESULT(VK_SUCCESS,
                     vkCreateInstance(&instanceCreateInfo, nullptr, &instance));
  }

  virtual void TearDown() {
    if (instance) {
      vkDestroyInstance(instance, nullptr);
      instance = VK_NULL_HANDLE;
    }
  }

  /// @brief Check if an instance extension was succesfully enabled at setup
  ///
  /// @param extensionName Name of the extension to check for
  ///
  /// @return True if the extension was successfully enabled
  bool isInstanceExtensionEnabled(const std::string &extensionName) {
    return std::find(enabledInstanceExtensionNames.begin(),
                     enabledInstanceExtensionNames.end(),
                     extensionName) != enabledInstanceExtensionNames.end();
  }

  std::vector<const char *> enabledLayerNames;
  std::vector<const char *> enabledInstanceExtensionNames;
  VkApplicationInfo applicationInfo;
  VkInstanceCreateInfo instanceCreateInfo;
  VkInstance instance;
};

/// @brief Test fixture to inherit from when a default physical device is needed
class PhysicalDeviceTest : public InstanceTest {
 public:
  PhysicalDeviceTest() : physicalDevice(VK_NULL_HANDLE) {}

  virtual void SetUp() {
    if (!instance) {
      RETURN_ON_FATAL_FAILURE(InstanceTest::SetUp());
    }

    uint32_t deviceCount = 0;
    std::vector<VkPhysicalDevice> deviceList;

    ASSERT_EQ_RESULT(VK_SUCCESS, vkEnumeratePhysicalDevices(
                                     instance, &deviceCount, nullptr));

    deviceList.resize(deviceCount);

    ASSERT_EQ_RESULT(
        VK_SUCCESS,
        vkEnumeratePhysicalDevices(instance, &deviceCount, deviceList.data()));

    physicalDevice = deviceList[0];
    uint32_t queue_family_count;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice,
                                             &queue_family_count, 0);

    std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);

    vkGetPhysicalDeviceQueueFamilyProperties(
        physicalDevice, &queue_family_count, queue_families.data());
    queueFamilyIndex = 0;
  }

  virtual void TearDown() { InstanceTest::TearDown(); }

  VkPhysicalDevice physicalDevice;
  uint32_t queueFamilyIndex;
};

/// @brief Test fixture to inherit from when a default device is needed
class DeviceTest : public PhysicalDeviceTest {
 public:
  DeviceTest() : device(VK_NULL_HANDLE) {}

  virtual void SetUp() {
    if (!physicalDevice || !instance) {
      RETURN_ON_FATAL_FAILURE(PhysicalDeviceTest::SetUp());
    }
    const float queuePriority = 1;

    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    const std::array<std::pair<const char *, bool *>, 2> deviceExtensions{
        {{"VK_KHR_storage_buffer_storage_class", &clspvSupported_},
         {"VK_KHR_variable_pointers", &clspvSupported_}}};

    uint32_t extensionPropertyCount;
    ASSERT_EQ_RESULT(VK_SUCCESS, vkEnumerateDeviceExtensionProperties(
                                     physicalDevice, nullptr,
                                     &extensionPropertyCount, nullptr));
    std::vector<VkExtensionProperties> extensionProperties(
        extensionPropertyCount);

    ASSERT_EQ_RESULT(VK_SUCCESS,
                     vkEnumerateDeviceExtensionProperties(
                         physicalDevice, nullptr, &extensionPropertyCount,
                         extensionProperties.data()));

    for (auto extensionName : deviceExtensions) {
      bool added = false;
      for (VkExtensionProperties properties : extensionProperties) {
        if (strcmp(extensionName.first, properties.extensionName) == 0) {
          enabledDeviceExtensionNames.push_back(extensionName.first);
          added = true;
        }
      }
      *extensionName.second &= added;
    }

    // populate enabled features with whatever the physical device supports
    vkGetPhysicalDeviceFeatures(physicalDevice, &enabledFeatures);

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.enabledLayerCount = enabledLayerNames.size();
    deviceCreateInfo.ppEnabledLayerNames = enabledLayerNames.data();
    deviceCreateInfo.enabledExtensionCount = enabledDeviceExtensionNames.size();
    deviceCreateInfo.ppEnabledExtensionNames =
        enabledDeviceExtensionNames.data();
    deviceCreateInfo.pEnabledFeatures = &enabledFeatures;

    ASSERT_EQ_RESULT(
        VK_SUCCESS,
        vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device));
  }

  virtual void TearDown() {
    vkDestroyDevice(device, nullptr);
    device = nullptr;
    PhysicalDeviceTest::TearDown();
  }

  VkDevice device;
  std::vector<const char *> enabledDeviceExtensionNames;
  VkPhysicalDeviceFeatures enabledFeatures;

 protected:
  // used by classes inheriting from DeviceTest
  VkDeviceSize alignedDeviceSize(VkMemoryRequirements reqs) {
    // Force alignment using some division rounding trickery
    auto size_mod_align = reqs.size % reqs.alignment;
    auto size_trunc_align = reqs.size / reqs.alignment;
    return (size_mod_align) ? reqs.alignment * (size_trunc_align + 1)
                            : reqs.size;
  }
  bool clspvSupported_ = true;
};

/// @brief Test fixture to inherit from when a buffer is needed
class BufferTest : public virtual uvk::DeviceTest {
 public:
  /// @brief Default initializer
  ///
  /// @param bufferSize Size in bytes of the buffer needed
  /// @param bufferUsage Buffer usage flags
  /// @param extension Whether this fixture is an extension
  BufferTest(
      uint32_t bufferSize,
      VkBufferUsageFlags bufferUsage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      bool extension = false)
      : bufferCreateInfo(),
        bufferSize(bufferSize),
        bufferUsage(bufferUsage),
        buffer(VK_NULL_HANDLE),
        extension(extension) {}

  virtual void SetUp() override {
    if (!device) {
      RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());
    }

    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.queueFamilyIndexCount = 1;
    bufferCreateInfo.pQueueFamilyIndices = &queueFamilyIndex;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferCreateInfo.size = bufferSize;
    bufferCreateInfo.usage = bufferUsage;

    ASSERT_EQ_RESULT(VK_SUCCESS, vkCreateBuffer(device, &bufferCreateInfo,
                                                nullptr, &buffer));

    vkGetBufferMemoryRequirements(device, buffer, &bufferMemoryRequirements);
  }

  virtual void TearDown() override {
    vkDestroyBuffer(device, buffer, nullptr);

    if (!extension) {
      DeviceTest::TearDown();
    }
  }

  uint32_t queueFamilyIndex = 0;
  VkBufferCreateInfo bufferCreateInfo;
  uint32_t bufferSize;
  VkBufferUsageFlags bufferUsage;
  VkBuffer buffer;
  VkMemoryRequirements bufferMemoryRequirements;

 private:
  bool extension;
};

/// @brief Test fixture to inherit from when a default command pool is needed
class CommandPoolTest : public virtual DeviceTest {
 public:
  CommandPoolTest() : commandPool(VK_NULL_HANDLE), extension(false) {}
  CommandPoolTest(bool extension)
      : commandPool(VK_NULL_HANDLE), extension(extension) {}

  virtual void SetUp() {
    if (!device) {
      RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());
    }
    VkCommandPoolCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    ASSERT_EQ_RESULT(VK_SUCCESS, vkCreateCommandPool(device, &createInfo,
                                                     nullptr, &commandPool));
  }

  virtual void TearDown() {
    if (commandPool) {
      vkDestroyCommandPool(device, commandPool, nullptr);
    }

    if (!extension) {
      DeviceTest::TearDown();
    }
  }

  VkCommandPool commandPool;

 private:
  bool extension;
};

/// @brief test fixture to inherit from when a default descriptor pool is needed
class DescriptorPoolTest : public virtual DeviceTest {
 public:
  /// @brief Default initializer
  DescriptorPoolTest()
      : descriptorPool(VK_NULL_HANDLE),
        poolSizes({{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4},
                   {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4}}),
        extension(false) {}

  /// @brief Initializer that allows this fixture to be designated an extension
  ///
  /// @param extension Whether this fixture will be an extension
  DescriptorPoolTest(bool extension)
      : descriptorPool(VK_NULL_HANDLE),
        poolSizes({{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 4},
                   {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4}}),
        extension(extension) {}

  virtual void SetUp() override {
    if (!device) {
      RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());
    }

    VkDescriptorPoolCreateInfo descriptorPoolCreateinfo = {};
    descriptorPoolCreateinfo.sType =
        VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    // somewhat arbitrary maxSets value to make sure we are unlikely to hit the
    // limit when using this fixture
    descriptorPoolCreateinfo.maxSets = 8;
    descriptorPoolCreateinfo.poolSizeCount = poolSizes.size();
    descriptorPoolCreateinfo.pPoolSizes = poolSizes.data();
    descriptorPoolCreateinfo.flags =
        VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    ASSERT_EQ_RESULT(VK_SUCCESS,
                     vkCreateDescriptorPool(device, &descriptorPoolCreateinfo,
                                            nullptr, &descriptorPool));
  }

  virtual void TearDown() override {
    if (descriptorPool) {
      vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    }
    if (!extension) {
      DeviceTest::TearDown();
    }
  }

  VkDescriptorPool descriptorPool;
  std::vector<VkDescriptorPoolSize> poolSizes;

 private:
  bool extension;
};

/// @brief Test fixture to inherit from when a default descriptor set layout
/// is needed
class DescriptorSetLayoutTest : public virtual DeviceTest {
 public:
  /// @brief Default initializer
  ///
  /// @param extension whether this fixture will be an extension
  DescriptorSetLayoutTest(bool extension = false)
      : descriptorSetLayout(VK_NULL_HANDLE),
        descriptorSetLayoutBindings({{0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
                                      VK_SHADER_STAGE_COMPUTE_BIT, nullptr},
                                     {1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
                                      VK_SHADER_STAGE_COMPUTE_BIT, nullptr}}),
        extension(extension) {}

  virtual void SetUp() override {
    if (!device) {
      RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());
    }

    VkDescriptorSetLayoutCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.bindingCount = descriptorSetLayoutBindings.size();
    createInfo.pBindings = descriptorSetLayoutBindings.data();

    ASSERT_EQ_RESULT(VK_SUCCESS,
                     vkCreateDescriptorSetLayout(device, &createInfo, nullptr,
                                                 &descriptorSetLayout));
  }

  virtual void TearDown() override {
    if (descriptorSetLayout) {
      vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
    }
    if (!extension) {
      DeviceTest::TearDown();
    }
  }

  VkDescriptorSetLayout descriptorSetLayout;
  std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;

 private:
  bool extension;
};

/// @brief Test fixture to inherit from when a memory allocation is needed
class DeviceMemoryTest : public virtual DeviceTest {
 public:
  /// @brief Initializer
  ///
  /// @param extension Whether this fixture will be an extension
  /// @param memorySize Size in bytes of memory allocation to request
  DeviceMemoryTest(bool extension = false, uint32_t memorySize = 32)
      : memorySize(memorySize),
        memory(VK_NULL_HANDLE),
        coherent(true),
        extension(extension),
        mappedRange({}) {}

  virtual void SetUp() override {
    if (!device) {
      RETURN_ON_FATAL_FAILURE(DeviceTest::SetUp());
    }

    VkPhysicalDeviceMemoryProperties memoryProperties;

    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    // prioritize coherent for the first search
    for (memoryTypeIndex = 0;
         memoryTypeIndex < memoryProperties.memoryTypeCount;
         memoryTypeIndex++) {
      if (memoryProperties.memoryTypes[memoryTypeIndex].propertyFlags &
              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT &&
          memoryProperties.memoryTypes[memoryTypeIndex].propertyFlags &
              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
        break;
      } else if (memoryTypeIndex == memoryProperties.memoryTypeCount - 1) {
        coherent = false;
      }
    }

    // if we didn't find any coherent memory try again but this time only care
    // about host visible
    if (!coherent) {
      for (memoryTypeIndex = 0;
           memoryTypeIndex < memoryProperties.memoryTypeCount;
           memoryTypeIndex++) {
        if (memoryProperties.memoryTypes[memoryTypeIndex].propertyFlags &
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
          break;
        }
        // if we get to the last index and we haven't found host visible memory
        // the test can't procede
        ASSERT_FALSE(memoryTypeIndex == memoryProperties.memoryTypeCount - 1);
      }
    }

    VkMemoryAllocateInfo allocInf = {};
    allocInf.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInf.allocationSize = memorySize;
    allocInf.memoryTypeIndex = memoryTypeIndex;

    ASSERT_EQ_RESULT(VK_SUCCESS,
                     vkAllocateMemory(device, &allocInf, nullptr, &memory));
  }

  virtual void TearDown() override {
    if (memory) {
      vkFreeMemory(device, memory, nullptr);
    }

    if (!extension) {
      DeviceTest::TearDown();
    }
  }

  /// @brief Helper function for mapping `memory` and dealing with coherency
  ///
  /// @param offset Offset in bytes into the memory to start the mapping from
  /// @param size Size in bytes of the range to map
  /// @param host_pointer Host pointer to map the memory to
  void mapMemory(VkDeviceSize offset, VkDeviceSize size, void **host_pointer) {
    ASSERT_EQ_RESULT(
        VK_SUCCESS, vkMapMemory(device, memory, offset, size, 0, host_pointer));

    // flush the device writes if we are working with non-coherent memory
    if (!coherent) {
      flushFromDevice(offset, size);
    }
  }

  /// @brief Helper function for unmappping `memory` and dealing with coherency
  void unmapMemory() {
    // flush any host writes to the device if memory is non-coherent
    if (!coherent) {
      flushToDevice();
    }

    vkUnmapMemory(device, memory);
  }

  uint32_t memorySize;
  uint32_t memoryTypeIndex;
  VkDeviceMemory memory;
  bool coherent;

 private:
  /// @brief Helper function that flushes device writes to make them host
  /// visible
  ///
  /// @param offset Offset in bytes into the memory to begin the flush from
  /// @param size Size in bytes of the range to be flushed
  void flushFromDevice(VkDeviceSize offset, VkDeviceSize size) {
    mappedRange = {};
    mappedRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    mappedRange.memory = memory;
    mappedRange.offset = offset;
    mappedRange.size = size;
    ASSERT_EQ_RESULT(VK_SUCCESS,
                     vkInvalidateMappedMemoryRanges(device, 1, &mappedRange));
  }

  /// @brief Helper function that flushes host writes to make them device
  /// visible
  void flushToDevice() {
    ASSERT_EQ_RESULT(VK_SUCCESS,
                     vkFlushMappedMemoryRanges(device, 1, &mappedRange));
  }

  bool extension;
  VkMappedMemoryRange mappedRange;
};

/// @brief Test fixture to inherit from when a command buffer in the recording
/// state is needed
class RecordCommandBufferTest : public CommandPoolTest {
 public:
  RecordCommandBufferTest() : commandBuffer(VK_NULL_HANDLE) {}

  virtual void SetUp() override {
    RETURN_ON_FATAL_FAILURE(CommandPoolTest::SetUp());

    CreateAndRecordCommandBuffer(&commandBuffer);
  }

  virtual void TearDown() override {
    if (!commandBuffers.empty()) {
      vkFreeCommandBuffers(device, commandPool, commandBuffers.size(),
                           commandBuffers.data());
    }
    CommandPoolTest::TearDown();
  }

  // Note: Needs to return void due to ASSERT_* macros.
  void CreateAndRecordCommandBuffer(VkCommandBuffer *outCommandBuffer) {
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandBufferCount = 1;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;

    ASSERT_EQ_RESULT(VK_SUCCESS, vkAllocateCommandBuffers(device, &allocInfo,
                                                          outCommandBuffer));

    commandBufferBeginInfo = {};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    ASSERT_EQ_RESULT(VK_SUCCESS, vkBeginCommandBuffer(*outCommandBuffer,
                                                      &commandBufferBeginInfo));

    commandBuffers.emplace_back(*outCommandBuffer);
  }

  VkCommandBufferBeginInfo commandBufferBeginInfo = {};
  VkCommandBuffer commandBuffer;
  std::vector<VkCommandBuffer> commandBuffers;
};

/// @brief Test fixture to inherit from when a pipeline layout is needed
class PipelineLayoutTest : public DescriptorSetLayoutTest {
 public:
  /// @brief Initializer
  ///
  /// @param extension Whether this fixture will be an extension
  PipelineLayoutTest(bool extension = false)
      : DescriptorSetLayoutTest(extension), pipelineLayout(VK_NULL_HANDLE) {}

  virtual void SetUp() override {
    RETURN_ON_FATAL_FAILURE(DescriptorSetLayoutTest::SetUp());

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = 1;
    pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;

    ASSERT_EQ_RESULT(VK_SUCCESS,
                     vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo,
                                            nullptr, &pipelineLayout));
  }

  virtual void TearDown() override {
    if (pipelineLayout) {
      vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    }
    DescriptorSetLayoutTest::TearDown();
  }

  VkPipelineLayout pipelineLayout;
};

/// @brief Test fixture to inherit from when a pipeline is needed
///
/// also provides a command buffer in the recording state
class PipelineTest : public RecordCommandBufferTest {
 public:
  /// @brief Initializer
  ///
  /// @param shader Shader enum that determines which test kernel the pipeline
  /// will use
  PipelineTest(uvk::Shader shader = uvk::Shader::nop,
               bool pipelineLayoutProvided_ = false)
      : shader(shader),
        pipelineLayoutProvided(pipelineLayoutProvided_),
        pipelineLayoutCreateInfo(),
        pSpecializationInfo(nullptr) {}

  virtual void SetUp() override {
    RETURN_ON_FATAL_FAILURE(RecordCommandBufferTest::SetUp());

    // TODO Maybe get rid of this bool and inherit from PipelineLayoutTest
    // instead
    if (!pipelineLayoutProvided) {
      pipelineLayoutCreateInfo.sType =
          VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
      ASSERT_EQ_RESULT(VK_SUCCESS,
                       vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo,
                                              nullptr, &pipelineLayout));
    }
    const uvk::ShaderCode shaderCode = uvk::getShader(shader);

    VkShaderModuleCreateInfo shaderCreateInfo = {};
    shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderCreateInfo.pCode =
        reinterpret_cast<const uint32_t *>(shaderCode.code);
    shaderCreateInfo.codeSize = shaderCode.size;

    VkShaderModule shaderModule;
    ASSERT_EQ_RESULT(VK_SUCCESS, vkCreateShaderModule(device, &shaderCreateInfo,
                                                      nullptr, &shaderModule));

    VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {};
    shaderStageCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCreateInfo.module = shaderModule;
    shaderStageCreateInfo.pName = "main";
    shaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStageCreateInfo.pSpecializationInfo = pSpecializationInfo;

    VkComputePipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.layout = pipelineLayout;
    pipelineCreateInfo.stage = shaderStageCreateInfo;
    ASSERT_EQ_RESULT(VK_SUCCESS, vkCreateComputePipelines(
                                     device, VK_NULL_HANDLE, 1,
                                     &pipelineCreateInfo, nullptr, &pipeline));

    vkDestroyShaderModule(device, shaderModule, nullptr);
  }

  virtual void TearDown() override {
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    if (pipeline) {
      vkDestroyPipeline(device, pipeline, nullptr);
    }
    RecordCommandBufferTest::TearDown();
  }

  void SetPipelineLayout(VkPipelineLayout pipelineLayout_) {
    pipelineLayout = pipelineLayout_;
  }

  uvk::Shader shader;
  bool pipelineLayoutProvided;
  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo;
  VkPipelineLayout pipelineLayout;
  VkSpecializationInfo *pSpecializationInfo;
  VkPipeline pipeline;
};

/// @brief Test fixture whenever a simple kernel with 2 buffers is to be tested
///
/// This fixture loads the kernel specified and creates 2 buffers with bindings
/// 0 and 1 respectively. Memory mapped data for these buffers can be accessed
/// with the PtrToMappedData() function.
///
/// After SetUp() has finished, the command buffer is ready to be executed.
/// The test should call vkQueueSubmit to start execution of the kernel.
///
/// Alternatively, calling ExecuteAndWait() flushes all data, executes and
/// waits on the kernel and flushes the results
class SimpleKernelTest : public uvk::PipelineTest,
                         public uvk::DeviceMemoryTest,
                         public uvk::DescriptorPoolTest,
                         public uvk::DescriptorSetLayoutTest,
                         public uvk::BufferTest {
 public:
  /// @brief Constructor
  ///
  /// @param isDoubleTest Whether this test requires double support
  /// @param shader The uvk::Shader ID representing the shader to be executed
  /// @param bufferSize The size of both buffers in bytes
  SimpleKernelTest(bool isDoubleTest, uvk::Shader shader = uvk::Shader::nop,
                   uint32_t bufferSize = 128)
      : uvk::PipelineTest(shader, true),
        uvk::DeviceMemoryTest(true, 2 * bufferSize),
        uvk::DescriptorPoolTest(true),
        uvk::DescriptorSetLayoutTest(true),
        uvk::BufferTest(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, true),
        isDoubleTest(isDoubleTest) {}

  /// @brief Identifiers for the input buffer (binding=0) and output buffer
  /// (binding=1)
  enum BUFFER_ID { INPUT_BUFFER = 0, OUTPUT_BUFFER = 1 };

  virtual void SetUp() override {
    RETURN_ON_FATAL_FAILURE(PhysicalDeviceTest::SetUp());
    vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);

    // if this test makes use of a shader with doubles in we should skip it if
    // the hardware doesn't report double support
    if (!isDoubleTest || deviceFeatures.shaderFloat64) {
      // Descriptor set has 2 bindings: buffer inA and buffer outR
      VkDescriptorSetLayoutBinding layoutBinding = {};
      layoutBinding.binding = 0;
      layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
      layoutBinding.descriptorCount = 1;
      layoutBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

      descriptorSetLayoutBindings.clear();
      descriptorSetLayoutBindings.push_back(layoutBinding);

      layoutBinding.binding = 1;
      descriptorSetLayoutBindings.push_back(layoutBinding);

      // Set up descriptor set layout
      RETURN_ON_FATAL_FAILURE(DescriptorSetLayoutTest::SetUp());

      pipelineLayoutCreateInfo.sType =
          VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
      pipelineLayoutCreateInfo.setLayoutCount = 1;
      pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;

      ASSERT_EQ_RESULT(VK_SUCCESS,
                       vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo,
                                              nullptr, &pipelineLayout));

      // Set up our pipeline
      RETURN_ON_FATAL_FAILURE(PipelineTest::SetUp());

      // Set up both buffers
      RETURN_ON_FATAL_FAILURE(BufferTest::SetUp());
      ASSERT_EQ_RESULT(VK_SUCCESS, vkCreateBuffer(device, &bufferCreateInfo,
                                                  nullptr, &buffer2));
      // BufferTest::SetUp() has found buffer memory requirements
      // TODO: *should* really be doing more requirement checking here

      const VkDeviceSize required_mem_size =
          alignedDeviceSize(bufferMemoryRequirements);

      // Reserve continous memory region for both buffers
      memorySize = 2 * required_mem_size;
      bufferMemorySz = required_mem_size;

      RETURN_ON_FATAL_FAILURE(DeviceMemoryTest::SetUp());

      // Bind buffers to memory
      ASSERT_EQ_RESULT(VK_SUCCESS,
                       vkBindBufferMemory(device, buffer, memory, 0));
      ASSERT_EQ_RESULT(VK_SUCCESS, vkBindBufferMemory(device, buffer2, memory,
                                                      bufferMemorySz));

      // Grab a descriptor set from the descriptor pool
      RETURN_ON_FATAL_FAILURE(DescriptorPoolTest::SetUp());
      VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
      descriptorSetAllocateInfo.sType =
          VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
      descriptorSetAllocateInfo.descriptorPool = descriptorPool;
      descriptorSetAllocateInfo.descriptorSetCount = 1;
      descriptorSetAllocateInfo.pSetLayouts = &descriptorSetLayout;
      ASSERT_EQ_RESULT(VK_SUCCESS,
                       vkAllocateDescriptorSets(
                           device, &descriptorSetAllocateInfo, &descriptorSet));

      // Vector to store descriptor set writes
      std::vector<VkWriteDescriptorSet> descriptorSetWrites;

      // Set up descriptor bindings
      VkWriteDescriptorSet writeDescriptorSet = {};
      writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      writeDescriptorSet.dstSet = descriptorSet;
      writeDescriptorSet.dstBinding = 0;
      writeDescriptorSet.dstArrayElement = 0;
      writeDescriptorSet.descriptorCount = 1;
      writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

      // Set up BufferInfo s
      VkDescriptorBufferInfo bufferinAInfo = {};
      bufferinAInfo.buffer = buffer;
      bufferinAInfo.offset = 0;
      bufferinAInfo.range = VK_WHOLE_SIZE;
      writeDescriptorSet.pBufferInfo = &bufferinAInfo;
      descriptorSetWrites.push_back(writeDescriptorSet);

      VkDescriptorBufferInfo bufferoutRInfo = bufferinAInfo;
      bufferoutRInfo.buffer = buffer2;
      writeDescriptorSet.dstBinding = 1;
      writeDescriptorSet.pBufferInfo = &bufferoutRInfo;
      descriptorSetWrites.push_back(writeDescriptorSet);

      // Update descriptor sets
      vkUpdateDescriptorSets(device, descriptorSetWrites.size(),
                             descriptorSetWrites.data(), 0, nullptr);

      // bind things together
      vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                        pipeline);
      vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                              pipelineLayout, 0, 1, &descriptorSet, 0, 0);

      // shader dispatch command
      vkCmdDispatch(commandBuffer, 1, 1, 1);

      // close command buffer
      ASSERT_EQ_RESULT(VK_SUCCESS, vkEndCommandBuffer(commandBuffer));

      // get a handle to the queue
      vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);

      // set up submit info
      submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      submitInfo.commandBufferCount = 1;
      submitInfo.pCommandBuffers = &commandBuffer;

      // map the memory so that we can access buffers
      ASSERT_EQ_RESULT(VK_SUCCESS, vkMapMemory(device, memory, 0, VK_WHOLE_SIZE,
                                               0, &mappedMemoryRegion));
    }
  }

  /// @brief Returns a pointer to data type T which is stored at the given byte
  /// offset within the buffer. This points to mapped memory and is not
  /// neccessarily coherent to the device: FlushToDevice() should be called
  /// to update the device with any reads/writes made.
  ///
  /// @tparam T Type to treat data at byteOffset as.
  ///
  /// @param buffer The ID of the buffer
  /// @param byteOffset The offset in bytes from the start of the buffer's
  /// memory mapped region
  template <typename T>
  T *PtrToMappedData(BUFFER_ID buffer, size_t byteOffset) {
    // Byte must lie within the buffer:
    if (byteOffset >= bufferMemorySz) {
      return nullptr;
    }
    char *charPtrToStart = reinterpret_cast<char *>(mappedMemoryRegion);
    char *charPtrToData =
        &charPtrToStart[(buffer * bufferMemorySz) + byteOffset];
    return reinterpret_cast<T *>(charPtrToData);
  }

  /// @brief Returns a reference to data type T which is stored at the given
  /// byte offset within the buffer. This refers to mapped memory and is not
  /// neccessarily coherent to the device: FlushToDevice() should be called
  /// to update the device with any reads/writes made.
  ///
  /// @tparam T Type to treat data at byteOffset as.
  ///
  /// @param buffer The ID of the buffer
  /// @param byteOffset The offset in bytes from the start of the buffer's
  /// memory mapped region
  template <typename T>
  T &RefToMappedData(BUFFER_ID buffer, size_t byteOffset) {
    return *PtrToMappedData<T>(buffer, byteOffset);
  }

  /// @brief Returns a pointer to the mapped memory region for 1st buffer
  void *PtrTo1stBufferData() { return mappedMemoryRegion; }

  /// @brief Returns a pointer to the mapped memory region for 2nd buffer
  void *PtrTo2ndBufferData() {
    char *addr = &reinterpret_cast<char *>(mappedMemoryRegion)[bufferMemorySz];
    return reinterpret_cast<void *>(addr);
  }

  /// @brief Flushes changes from host memory to device
  void FlushToDevice() {
    VkMappedMemoryRange range = {};
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.memory = memory;
    range.offset = 0;
    range.size = memorySize;
    ASSERT_EQ_RESULT(VK_SUCCESS, vkFlushMappedMemoryRanges(device, 1, &range));
  }

  /// @brief Flushes changes from device to host memory
  void FlushFromDevice() {
    VkMappedMemoryRange range = {};
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.memory = memory;
    range.offset = 0;
    range.size = memorySize;
    ASSERT_EQ_RESULT(VK_SUCCESS,
                     vkInvalidateMappedMemoryRanges(device, 1, &range));
  }

  /// @brief Flushes data from host to device, executes the shader, then flushes
  /// from device to host
  void ExecuteAndWait() {
    FlushToDevice();
    ASSERT_EQ_RESULT(VK_SUCCESS,
                     vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
    ASSERT_EQ_RESULT(VK_SUCCESS, vkQueueWaitIdle(queue));
    FlushFromDevice();
  }

  virtual void TearDown() override {
    // if we skipped the test due to lack of double support there is less to
    // tear down
    if (!isDoubleTest || deviceFeatures.shaderFloat64) {
      if (memory) {
        vkUnmapMemory(device, memory);
      }
      if (buffer2) {
        vkDestroyBuffer(device, buffer2, 0);
      }

      DescriptorPoolTest::TearDown();
      DeviceMemoryTest::TearDown();
      BufferTest::TearDown();
      DescriptorSetLayoutTest::TearDown();
      PipelineTest::TearDown();
    } else {
      PhysicalDeviceTest::TearDown();
    }
  }

  /// bufferMemorySz: size of buffer MEMORY - may be bigger than requested size
  /// of buffer
  VkDeviceSize bufferMemorySz;
  VkSubmitInfo submitInfo = {};
  VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
  VkBuffer buffer2 = VK_NULL_HANDLE;
  VkQueue queue = VK_NULL_HANDLE;
  void *mappedMemoryRegion;
  VkPhysicalDeviceFeatures deviceFeatures;
  bool isDoubleTest;
};

}  // namespace uvk

#endif  // UNITVK_H_INCLUDED
