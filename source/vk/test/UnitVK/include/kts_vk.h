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

#ifndef UNITVK_KTS_VK_H_INCLUDED
#define UNITVK_KTS_VK_H_INCLUDED

#include <GLSLTestDefs.h>
#include <cargo/optional.h>

#include <unordered_map>

#include "kts/arguments_shared.h"
#include "kts/execution_shared.h"

namespace kts {
namespace uvk {

using namespace uvk;
struct ImageDesc {
  VkImageCreateInfo imageInfo;
  VkImageViewCreateInfo imageViewInfo;
  VkImageLayout imageLayout;
};

class Argument final : public ArgumentBase {
 public:
  Argument(ArgKind kind, size_t index) : ArgumentBase(kind, index) {}

  ~Argument() = default;

  const BufferDesc &GetBufferDesc() const { return buffer_desc_; }
  void SetBufferDesc(const BufferDesc &new_desc) {
    buffer_desc_ = new_desc;
    bufferStorageSize_ = new_desc.size_ * new_desc.streamer_->GetElementSize();
  }

  Primitive *GetPrimitive() const { return primitive_.get(); }
  void SetPrimitive(Primitive *new_prim) { primitive_.reset(new_prim); }

  const ImageDesc &GetImageDesc() const { return image_desc_; }
  void SetImageDesc(const ImageDesc &new_image) { image_desc_ = new_image; }

  const VkSamplerCreateInfo &GetSamplerDesc() const { return sampler_desc_; }
  void SetSamplerDesc(const VkSamplerCreateInfo &new_image) {
    sampler_desc_ = new_image;
  }

  virtual uint8_t *GetBufferStoragePtr() {
    assert(bufferStoragePtr_);
    return bufferStoragePtr_;
  }
  void SetBufferStoragePtr(uint8_t *ptr) { bufferStoragePtr_ = ptr; }
  virtual size_t GetBufferStorageSize() {
    assert(bufferStorageSize_);
    return bufferStorageSize_;
  }
  virtual void SetBufferStorageSize(size_t size) {
    (void)size;
    // Already set when buffer_desc_ is set
    assert(size == bufferStorageSize_);
  }

 private:
  // Used to generate the argument's buffer (input) or validate the argument's
  // data.
  BufferDesc buffer_desc_;
  // Primitive value if the argument is a primitive.
  std::unique_ptr<Primitive> primitive_;
  uint8_t *bufferStoragePtr_;
  size_t bufferStorageSize_;
  // Used to generate the argument's image input combined with buffer_desc_
  ImageDesc image_desc_;
  // Used to generate a sampler input combined with buffer_desc_ and image_desc_
  VkSamplerCreateInfo sampler_desc_;
};

class ArgumentList {
 public:
  Argument *AddInputBuffer(const BufferDesc &desc) {
    std::unique_ptr<Argument> arg(new Argument(eInputBuffer, args_.size()));
    arg->SetBufferDesc(desc);
    auto *ptr = arg.get();
    args_.emplace_back(std::move(arg));
    return ptr;
  }

  Argument *AddOutputBuffer(const BufferDesc &desc) {
    std::unique_ptr<Argument> arg(new Argument(eOutputBuffer, args_.size()));
    arg->SetBufferDesc(desc);
    auto *ptr = arg.get();
    args_.emplace_back(std::move(arg));
    return ptr;
  }

  Argument *AddPrimitive(Primitive *primitive) {
    std::unique_ptr<Argument> arg(new Argument(ePrimitive, args_.size()));
    arg->SetPrimitive(primitive);
    auto *ptr = arg.get();
    args_.emplace_back(std::move(arg));
    return ptr;
  }

  Argument *AddInputImage(const VkImageCreateInfo imageInfo,
                          const VkImageViewCreateInfo imageViewInfo,
                          const VkImageLayout imageLayout,
                          const BufferDesc &desc) {
    std::unique_ptr<Argument> arg(new Argument(eInputImage, args_.size()));
    arg->SetBufferDesc(desc);
    arg->SetImageDesc({imageInfo, imageViewInfo, imageLayout});
    auto *ptr = arg.get();
    args_.emplace_back(std::move(arg));
    return ptr;
  }

  Argument *AddSampler(const VkImageCreateInfo imageInfo,
                       const VkImageViewCreateInfo imageViewInfo,
                       const VkImageLayout imageLayout,
                       const VkSamplerCreateInfo samplerInfo,
                       const BufferDesc &desc) {
    std::unique_ptr<Argument> arg(new Argument(eSampledImage, args_.size()));
    arg->SetBufferDesc(desc);
    arg->SetImageDesc({imageInfo, imageViewInfo, imageLayout});
    arg->SetSamplerDesc(samplerInfo);
    auto *ptr = arg.get();
    args_.emplace_back(std::move(arg));
    return ptr;
  }

  size_t GetCount() const { return args_.size(); }
  Argument *GetArg(unsigned index) { return args_[index].get(); }

 private:
  std::vector<std::unique_ptr<Argument>> args_;
};

/// @Brief Test fixture for KTS tests
///
/// Add resources to the pipeline with the various Add* methods.
/// All resources are accessible in the shader via set 0.
/// All primitives are grouped into an uniform buffer in the order in which they
/// were added. This buffer is assigned the highest binding number after all
/// other resources.
/// All other resources occupy a single binding starting at 0 in the order in
/// which they were added.
/// After adding the resources, the shader specified with the template parameter
/// can be run with these resources by calling RunGeneric with the desired
/// workgroup numbers.
/// It is also possible to provide custom objects for many of the Vk* objects
/// involved.
/// This can be done by calling the appropriate provide* method after adding the
/// resources and before calling RunGeneric. All provide* calls take ownership
/// of the passed in object.
/// Whenever there is one Vk* object for every binding, the required index is
/// the index of the Argument, which can be obtained with the GetIndex() call on
/// that Argument. Vk* objects that also exist for the uniform primitive buffer
/// can be accessed with the index -1.
/// It is also possible to initialize the Vk* objects up to a certain point of
/// preparing for execution, then use these initialized objects to create a
/// customized object that needs these objects and then continue the preparation
/// and execution of the shader.
/// Example:
/// 1. Add resources
/// 2. Create VkObjects up to the ShaderModule by calling setUpShaderModule
/// 3. Build custom pipeline in test code, e.g. with specialization info, using
/// the prepared shaderModule and pipelineLayout, which can be obtained by
/// calling get{ShaderModule, PipelineLayout} and set that pipeline via
/// providePipeline()
/// 4. Resume preparation and execution by calling RunGeneric()
/// There is a total order between the different stages of preparation shown
/// below, alongside the Vk* objects each stage initializes if they've not been
/// provided at that point:
/// setUpResources - initializes buffers, bufferMemories, imageMemories, images,
///      |           imageViews and samplers
///      | is called by
///      |
/// fillMemory - initializes mappingRanges
///      |
///      | ...
//       |
/// fillImage - initializes preCopyImageBarriers, bufferImageCopies,
///      |      postCopyImageBarriers
///      |
/// setUpDescriptorSetLayout initializes descriptorSetLayout
///      |
/// setUpPipelineLayout initializes pipelineLayout
///      |
/// setUpShaderModule initializes shaderModule
///      |
/// setUpPipeline initializes pipeline
///      |
/// setUpDescriptorPool initializes descriptorPool
///      |
/// setUpDescriptorSet initializes descriptorSet
///      |
/// updateDescriptorSet initializes descriptorSetUpdates
///      |
/// runGeneric
///
/// If the configuration of the workgroup sizes via specialization constants is
/// supported, the size can be passed to any of the methods after
/// setUpShaderModule. Note that once a pipeline has been created, the workgroup
/// size cannot be changed anymore. The workgroup sizes are bound to the
/// specialization constants IDs 0, 1 and 2.
///
/// @tparam shader shader to execute
template <::uvk::Shader shader>
class GenericKernelTest : public ::uvk::RecordCommandBufferTest,
                          public SharedExecution {
 public:
  GenericKernelTest()
      : ::uvk::RecordCommandBufferTest(), newArgs_(new ArgumentList()) {}

  Argument *AddInputBuffer(BufferDesc &&desc) {
    return newArgs_->AddInputBuffer(desc);
  }

  template <typename T>
  Argument *AddInputBuffer(size_t size, Reference1D<T> ref) {
    return newArgs_->AddInputBuffer(BufferDesc(size, ref));
  }

  Argument *AddInputBuffer(size_t size,
                           std::shared_ptr<BufferStreamer> streamer) {
    return newArgs_->AddInputBuffer(BufferDesc(size, streamer));
  }

  template <typename T>
  Argument *AddInputBuffer(size_t size, Reference1DPtr<T> ref) {
    return newArgs_->AddInputBuffer(BufferDesc(size, Reference1D<T>(ref)));
  }

  Argument *AddOutputBuffer(BufferDesc &&desc) {
    return newArgs_->AddOutputBuffer(desc);
  }

  Argument *AddOutputBuffer(size_t size,
                            std::shared_ptr<BufferStreamer> streamer) {
    return newArgs_->AddOutputBuffer(BufferDesc(size, streamer));
  }

  template <typename T>
  Argument *AddOutputBuffer(size_t size, Reference1D<T> ref) {
    return newArgs_->AddOutputBuffer(BufferDesc(size, ref));
  }

  template <typename T>
  Argument *AddOutputBuffer(size_t size, Reference1DPtr<T> ref) {
    return newArgs_->AddOutputBuffer(BufferDesc(size, Reference1D<T>(ref)));
  }

  template <typename T>
  Argument *AddPrimitive(T value) {
    return newArgs_->AddPrimitive(new BoxedPrimitive<T>(value));
  }

  template <typename T>
  Argument *AddInputImage(VkImageCreateInfo imageInfo,
                          VkImageViewCreateInfo imageViewInfo,
                          VkImageLayout imageLayout, size_t size,
                          Reference1D<T> ref) {
    return newArgs_->AddInputImage(imageInfo, imageViewInfo, imageLayout,
                                   BufferDesc(size, ref));
  }

  template <typename T>
  Argument *AddSampler(VkImageCreateInfo imageInfo,
                       VkImageViewCreateInfo imageViewInfo,
                       VkImageLayout imageLayout,
                       VkSamplerCreateInfo samplerInfo, size_t size,
                       Reference1D<T> ref) {
    return newArgs_->AddSampler(imageInfo, imageViewInfo, imageLayout,
                                samplerInfo, BufferDesc(size, ref));
  }
  void Fail(const std::string &message) {
    GTEST_NONFATAL_FAILURE_(message.c_str());
  }

  struct ArgumentInfo {
    VkDeviceMemory deviceMemory;
    VkDescriptorType descriptorType;
    VkBuffer buf;
  };

  struct BufferInfo : ArgumentInfo {
    // TODO can be removed and callsites replaced with aggregate initialization
    // once we use C++17
    BufferInfo(VkDeviceMemory deviceMemory, VkDescriptorType descriptorType,
               VkBuffer buf, VkDescriptorBufferInfo descriptorBufferInfo) {
      this->deviceMemory = deviceMemory;
      this->descriptorType = descriptorType;
      this->buf = buf;
      this->descriptorBufferInfo = descriptorBufferInfo;
    }
    VkDescriptorBufferInfo descriptorBufferInfo;
  };

  struct ImageInfo : ArgumentInfo {
    VkImage image;
    VkDeviceMemory imageMemory;
    VkImageView imageView;
    VkDescriptorImageInfo descriptorImageInfo;
  };

  struct SamplerInfo : ImageInfo {
    VkSampler sampler;
  };

  virtual void SetUp() {
    RETURN_ON_FATAL_FAILURE(RecordCommandBufferTest::SetUp());
    VkPhysicalDeviceMemoryProperties memoryProperties;

    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    for (memoryTypeIndex = 0;
         memoryTypeIndex < memoryProperties.memoryTypeCount;
         memoryTypeIndex++) {
      if (memoryProperties.memoryTypes[memoryTypeIndex].propertyFlags &
          (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
        break;
      }
    }
  }

  void setUpResources() {
    // check if resources have already been set up
    if (args.get()) {
      return;
    }

    // Consume the arguments, so that the user can call RunGeneric multiple
    // times with different arguments.
    args = std::move(newArgs_);
    newArgs_.reset(new ArgumentList());

    for (size_t i = 0; i < args->GetCount(); i++) {
      switch (args->GetArg(i)->GetKind()) {
        case eInputBuffer:
        case eOutputBuffer:
          resources.push_back(std::make_pair(
              args->GetArg(i),
              createBufferInfo(args->GetArg(i)->GetBufferStorageSize(),
                               VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, i)));
          numBuffers++;
          break;
        case ePrimitive:
          primitiveBufferSize += args->GetArg(i)->GetPrimitive()->GetSize();
          primitives.push_back(args->GetArg(i));
          break;
        case eInputImage:
          resources.push_back(std::make_pair(
              args->GetArg(i),
              createImageInfo(args->GetArg(i)->GetImageDesc(),
                              args->GetArg(i)->GetBufferStorageSize(), i)));
          numImages++;
          break;
        case eSampledImage:
          resources.push_back(std::make_pair(
              args->GetArg(i),
              createSamplerInfo(args->GetArg(i)->GetImageDesc(),
                                args->GetArg(i)->GetBufferStorageSize(),
                                args->GetArg(i)->GetSamplerDesc(), i)));
          numSamplers++;
          break;
        default:
          assert(false && "unsupported resource type");
      }
    }
  }

  void fillMemory() {
    // check whether memory has already been filled
    if (bindingCount) {
      return;
    }

    setUpResources();

    std::vector<VkMappedMemoryRange> ranges;

    VkMappedMemoryRange range = {};
    range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
    range.offset = 0;

    for (auto &arg : resources) {
      // Populate buffer
      void *ptr = 0;

      // map the memory so that we can access buffers
      ASSERT_EQ_RESULT(VK_SUCCESS, vkMapMemory(device, arg.second->deviceMemory,
                                               0, VK_WHOLE_SIZE, 0, &ptr));
      assert(ptr);
      arg.first->SetBufferStoragePtr((uint8_t *)ptr);

      if (arg.first->GetKind() == eInputBuffer ||
          arg.first->GetKind() == eOutputBuffer ||
          arg.first->GetKind() == eInputImage ||
          arg.first->GetKind() == eSampledImage) {
        const BufferDesc desc = arg.first->GetBufferDesc();

        if (!desc.size_) {
          Fail("Empty buffer arguments are not supported");
          return;
        } else if (!desc.streamer_) {
          Fail("Could not get a streamer for the buffer argument");
          return;
        }

        desc.streamer_->PopulateBuffer(*arg.first, desc);
      }

      if (!lookup(mappingRanges, (int)arg.first->GetIndex()).has_value()) {
        range.memory = arg.second->deviceMemory;
        range.size = arg.first->GetBufferStorageSize();
        mappingRanges.insert({(int)arg.first->GetIndex(), range});
        ranges.push_back(range);
      } else {
        ranges.push_back(get(mappingRanges, (int)arg.first->GetIndex()));
      }

      descriptorSetLayoutBindings.push_back(
          {bindingCount, arg.second->descriptorType, 1,
           VK_SHADER_STAGE_COMPUTE_BIT, nullptr});
      bindingCount++;
    }

    if (primitiveBufferSize) {
      uniformBuffer = createBufferInfo(primitiveBufferSize,
                                       VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, -1);

      uint8_t *uniformPtr = nullptr;
      // map the memory so that we can access buffers
      ASSERT_EQ_RESULT(VK_SUCCESS,
                       vkMapMemory(device, uniformBuffer->deviceMemory, 0,
                                   VK_WHOLE_SIZE, 0, (void **)&uniformPtr));
      assert(uniformPtr != 0);
      descriptorSetLayoutBindings.push_back(
          {bindingCount, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
           VK_SHADER_STAGE_COMPUTE_BIT, nullptr});
      bindingCount++;
      size_t offset = 0;
      for (Argument *arg : primitives) {
        auto prim = arg->GetPrimitive();
        memcpy(uniformPtr + offset, prim->GetAddress(), prim->GetSize());
        offset += prim->GetSize();
      }

      if (!lookup(mappingRanges, -1).has_value()) {
        range.memory = uniformBuffer->deviceMemory;
        range.size = primitiveBufferSize;

        ranges.push_back(range);
      } else {
        ranges.push_back(get(mappingRanges, -1));
      }
      numBuffers++;
    }

    ASSERT_EQ_RESULT(VK_SUCCESS, vkFlushMappedMemoryRanges(
                                     device, ranges.size(), ranges.data()));
  }

  void fillImage() {
    fillMemory();
    if (imagesFilled) {
      return;
    }
    imagesFilled = true;
    // Copy image data from staging buffer to image
    for (auto &arg : resources) {
      if (arg.first->GetKind() != eInputImage &&
          arg.first->GetKind() != eSampledImage) {
        continue;
      }
      ImageInfo *imageInfo = (ImageInfo *)arg.second.get();
      const VkImageSubresourceRange subresourceRange =
          arg.first->GetImageDesc().imageViewInfo.subresourceRange;

      changeImageLayout(
          imageInfo->image, subresourceRange, VK_IMAGE_LAYOUT_UNDEFINED,
          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, arg.first->GetIndex());

      VkBufferImageCopy region = {};
      if (!lookup(bufferImageCopies, arg.first->GetIndex()).has_value()) {
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = subresourceRange.aspectMask;
        // TODO should the mipLevel be configurable?
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer =
            subresourceRange.baseArrayLayer;
        region.imageSubresource.layerCount = subresourceRange.layerCount;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = arg.first->GetImageDesc().imageInfo.extent;
        bufferImageCopies.insert({arg.first->GetIndex(), region});
      } else {
        region = get(bufferImageCopies, arg.first->GetIndex());
      }

      vkCmdCopyBufferToImage(commandBuffer, imageInfo->buf, imageInfo->image,
                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
      submitCommandBuffer();

      changeImageLayout(imageInfo->image, subresourceRange,
                        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        arg.first->GetImageDesc().imageLayout,
                        arg.first->GetIndex());
    }
  }

  void setUpDescriptorSetLayout() {
    fillImage();
    if (!descriptorSetLayout.has_value()) {
      VkDescriptorSetLayout descriptorSetLayout_;
      VkDescriptorSetLayoutCreateInfo createInfo = {};
      createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
      createInfo.bindingCount = descriptorSetLayoutBindings.size();
      createInfo.pBindings = descriptorSetLayoutBindings.data();

      vkCreateDescriptorSetLayout(device, &createInfo, nullptr,
                                  &descriptorSetLayout_);
      descriptorSetLayout.emplace(descriptorSetLayout_);
    }
  }

  void setUpPipelineLayout() {
    setUpDescriptorSetLayout();
    if (!pipelineLayout.has_value()) {
      VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
      pipelineLayoutCreateInfo.sType =
          VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
      pipelineLayoutCreateInfo.setLayoutCount = 1;
      pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout.value();

      VkPipelineLayout pipelineLayout_;
      vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr,
                             &pipelineLayout_);
      pipelineLayout.emplace(pipelineLayout_);
    }
  }

  ::uvk::ShaderCode getShaderCode() {
    if (shader == ::uvk::Shader::none) {
      std::string name, prefix;
      const testing::UnitTest *test = testing::UnitTest::GetInstance();
      if (nullptr == test) {
        std::fprintf(stderr, "Could not get a reference to the current test.");
        std::abort();
      }
      const testing::TestInfo *test_info = test->current_test_info();
      if (nullptr == test_info) {
        std::fprintf(stderr,
                     "Could not get a reference to the current test info.");
        std::abort();
      }
      GetKernelPrefixAndName(test_info->name(), prefix, name);
      entryName = name;
      return getShader(::uvk::ShaderMap[prefix + "_" + name]);
    } else {
      entryName = "main";
      return getShader(shader);
    }
  }

  void setUpShaderModule() {
    setUpPipelineLayout();
    if (!shaderModule.has_value()) {
      const ::uvk::ShaderCode shaderCode = getShaderCode();

      VkShaderModuleCreateInfo shaderCreateInfo = {};
      shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
      shaderCreateInfo.pCode =
          reinterpret_cast<const uint32_t *>(shaderCode.code);
      shaderCreateInfo.codeSize = shaderCode.size;

      VkShaderModule shaderModule_;
      vkCreateShaderModule(device, &shaderCreateInfo, nullptr, &shaderModule_);
      shaderModule.emplace(shaderModule_);
    }
  }

  void setUpPipeline(uint32_t local[3] = nullptr) {
    setUpShaderModule();
    if (!pipeline.has_value()) {
      static_assert(sizeof(uint32_t) == sizeof(glsl::uintTy),
                    "Workgroup size constants are uints");
      const VkSpecializationMapEntry entries[] =
          // id,  offset,                size
          {{0, 0, sizeof(glsl::uintTy)},
           {1, sizeof(glsl::uintTy), sizeof(glsl::uintTy)},
           {2, 2 * sizeof(glsl::uintTy), sizeof(glsl::uintTy)}};

      const VkSpecializationInfo specInfo = {
          3,                         // mapEntryCount
          entries,                   // pMapEntries
          3 * sizeof(glsl::uintTy),  // dataSize
          local                      // pData
      };

      const VkSpecializationInfo *specInfoPtr;
      if (!local) {
        specInfoPtr = nullptr;
      } else {
        specInfoPtr = &specInfo;
      }

      VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {};
      shaderStageCreateInfo.sType =
          VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      shaderStageCreateInfo.module = shaderModule.value();
      shaderStageCreateInfo.pName = entryName.c_str();
      shaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
      shaderStageCreateInfo.pSpecializationInfo = specInfoPtr;

      VkComputePipelineCreateInfo pipelineCreateInfo = {};
      pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
      pipelineCreateInfo.layout = pipelineLayout.value();
      pipelineCreateInfo.stage = shaderStageCreateInfo;

      VkPipeline pipeline_;
      vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo,
                               nullptr, &pipeline_);
      pipeline.emplace(pipeline_);
    }
  }

  void setUpDescriptorPool(uint32_t local[3] = nullptr) {
    setUpPipeline(local);
    if (!descriptorPool.has_value()) {
      std::vector<VkDescriptorPoolSize> poolSizes;

      if (primitiveBufferSize) {
        poolSizes.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1});
      }

      if (numBuffers - !!primitiveBufferSize) {
        poolSizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
                             numBuffers - !!primitiveBufferSize});
      }

      if (numImages) {
        poolSizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, numImages});
      }

      if (numSamplers) {
        poolSizes.push_back(
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, numSamplers});
      }

      VkDescriptorPoolCreateInfo descriptorPoolCreateinfo = {};
      descriptorPoolCreateinfo.sType =
          VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
      descriptorPoolCreateinfo.maxSets = 1;
      descriptorPoolCreateinfo.poolSizeCount = poolSizes.size();
      descriptorPoolCreateinfo.pPoolSizes = poolSizes.data();
      descriptorPoolCreateinfo.flags =
          VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

      VkDescriptorPool descriptorPool_;
      vkCreateDescriptorPool(device, &descriptorPoolCreateinfo, nullptr,
                             &descriptorPool_);
      descriptorPool.emplace(descriptorPool_);
    }
  }

  void setUpDescriptorSet(uint32_t local[3] = nullptr) {
    setUpDescriptorPool(local);
    if (!descriptorSet.has_value()) {
      VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
      descriptorSetAllocateInfo.sType =
          VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
      descriptorSetAllocateInfo.descriptorPool = descriptorPool.value();
      descriptorSetAllocateInfo.descriptorSetCount = 1;
      descriptorSetAllocateInfo.pSetLayouts = &descriptorSetLayout.value();

      VkDescriptorSet descriptorSet_;
      ASSERT_EQ_RESULT(
          VK_SUCCESS, vkAllocateDescriptorSets(
                          device, &descriptorSetAllocateInfo, &descriptorSet_));
      descriptorSet.emplace(descriptorSet_);
    }
  }

  void updateDescriptorSet(uint32_t local[3] = nullptr) {
    setUpDescriptorSet(local);
    // Vector to store descriptor set writes
    std::vector<VkWriteDescriptorSet> descriptorSetWrites;

    // Set up descriptor bindings
    VkWriteDescriptorSet writeDescriptorSet = {};
    writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writeDescriptorSet.dstSet = descriptorSet.value();
    writeDescriptorSet.dstArrayElement = 0;
    writeDescriptorSet.descriptorCount = 1;

    for (uint32_t i = 0; i < bindingCount - !!primitiveBufferSize; i++) {
      const int index = resources[i].first->GetIndex();
      if (!lookup(descriptorSetUpdates, index).has_value()) {
        writeDescriptorSet.descriptorType = resources[i].second->descriptorType;
        if (resources[i].second->descriptorType ==
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) {
          writeDescriptorSet.pBufferInfo =
              &((BufferInfo *)resources[i].second.get())->descriptorBufferInfo;
        } else if (resources[i].second->descriptorType ==
                       VK_DESCRIPTOR_TYPE_STORAGE_IMAGE ||
                   resources[i].second->descriptorType ==
                       VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
          writeDescriptorSet.pImageInfo =
              &((ImageInfo *)resources[i].second.get())->descriptorImageInfo;
        } else {
          assert(false && "descriptor type not supported");
        }
        writeDescriptorSet.dstBinding = i;
        descriptorSetUpdates.insert({index, writeDescriptorSet});
        descriptorSetWrites.push_back(writeDescriptorSet);
      } else {
        descriptorSetWrites.push_back(get(descriptorSetUpdates, index));
      }
    }

    if (primitiveBufferSize) {
      if (!lookup(descriptorSetUpdates, -1).has_value()) {
        writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeDescriptorSet.pBufferInfo = &uniformBuffer->descriptorBufferInfo;
        writeDescriptorSet.dstBinding = bindingCount - 1;
        descriptorSetUpdates.insert({-1, writeDescriptorSet});
        descriptorSetWrites.push_back(writeDescriptorSet);
      } else {
        descriptorSetWrites.push_back(get(descriptorSetUpdates, -1));
      }
    }

    // Update descriptor sets
    vkUpdateDescriptorSets(device, descriptorSetWrites.size(),
                           descriptorSetWrites.data(), 0, nullptr);
  }

  void RunGeneric(uint32_t global[3], uint32_t local[3] = nullptr) {
    updateDescriptorSet(local);

    // bind things together
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                      pipeline.value());
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE,
                            pipelineLayout.value(), 0, 1,
                            &descriptorSet.value(), 0, 0);

    // Run secondary command buffer provided by the test if there is one.
    if (secondaryCommandBuffer.has_value()) {
      VkCommandBuffer secondary = secondaryCommandBuffer.value();
      vkCmdExecuteCommands(commandBuffer, 1, &secondary);
    }

    // shader dispatch command
    vkCmdDispatch(commandBuffer, global[0], global[1], global[2]);

    submitCommandBuffer();

    bool found_error = false;
    for (auto &arg : resources) {
      if (arg.first->GetKind() == eOutputBuffer && !found_error) {
        const VkMappedMemoryRange range =
            get(mappingRanges, (int)arg.first->GetIndex());

        ASSERT_EQ_RESULT(VK_SUCCESS,
                         vkInvalidateMappedMemoryRanges(device, 1, &range));

        const BufferDesc desc = arg.first->GetBufferDesc();
        BufferStreamer *streamer = desc.streamer_.get();
        std::vector<std::string> errors;
        if (streamer && !streamer->ValidateBuffer(*arg.first, desc, &errors)) {
          if (errors.size() > 0) {
            std::stringstream ss;
            ss << "Invalid data when validating buffer "
               << arg.first->GetIndex() << ":";
            for (const std::string &error : errors) {
              ss << "\n" << error;
            }
            Fail(ss.str());
          } else {
            Fail("Invalid data");
          }
          found_error = true;
        }
        arg.first->SetBufferStoragePtr(nullptr);
      }
      vkUnmapMemory(device, arg.second->deviceMemory);
      vkFreeMemory(device, arg.second->deviceMemory, nullptr);

      if (arg.second->descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) {
        vkDestroyBuffer(device, ((BufferInfo *)arg.second.get())->buf, nullptr);
      } else if (arg.second->descriptorType ==
                     VK_DESCRIPTOR_TYPE_STORAGE_IMAGE ||
                 arg.second->descriptorType ==
                     VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
        ImageInfo *imageInfo = (ImageInfo *)arg.second.get();

        if (arg.second->descriptorType ==
            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
          vkDestroySampler(device, ((SamplerInfo *)imageInfo)->sampler,
                           nullptr);
        }

        vkFreeMemory(device, imageInfo->imageMemory, nullptr);
        vkDestroyBuffer(device, imageInfo->buf, nullptr);
        vkDestroyImageView(device, ((ImageInfo *)arg.second.get())->imageView,
                           nullptr);
        vkDestroyImage(device, ((ImageInfo *)arg.second.get())->image, nullptr);
      } else {
        assert(false && "descriptor type not supported");
      }
    }
    if (primitiveBufferSize) {
      vkUnmapMemory(device, uniformBuffer->deviceMemory);
      vkFreeMemory(device, uniformBuffer->deviceMemory, nullptr);
      vkDestroyBuffer(device, uniformBuffer->buf, nullptr);
    }
    vkDestroyDescriptorSetLayout(device, descriptorSetLayout.value(), nullptr);
    vkDestroyShaderModule(device, shaderModule.value(), nullptr);
    vkDestroyPipeline(device, pipeline.value(), nullptr);
    vkDestroyPipelineLayout(device, pipelineLayout.value(), nullptr);
    vkDestroyDescriptorPool(device, descriptorPool.value(), nullptr);
    clearState();
  }

  // x is the number of threads to be launched
  void RunGeneric1D(uint32_t x, uint32_t localX = 1) {
    assert(x && localX);
    uint32_t global[3] = {x / localX, 1, 1};
    if (localX == 1) {
      RunGeneric(global);
    } else {
      uint32_t local[3] = {localX, 1, 1};
      RunGeneric(global, local);
    }
  }

  // globalDims are the number of threads to be launched per dimension
  void RunGenericND(uint32_t numDims, const size_t *globalDims,
                    const size_t *localDims) {
    assert(numDims <= 3);
    uint32_t global[3], local[3];
    for (size_t i = 0; i < 3; i++) {
      if (i < numDims) {
        global[i] = globalDims[i] / localDims[i];
        local[i] = localDims[i];
      } else {
        global[i] = 1;
        local[i] = 1;
      }
    }
    RunGeneric(global, local);
  }

  void provideBuffer(size_t index, VkBuffer buffer) {
    buffers.insert({index, buffer});
  }
  cargo::optional<VkBuffer> getBuffer(size_t index) {
    return lookup(buffers, index);
  }

  void provideBufferMemory(size_t index, VkDeviceMemory memory) {
    bufferMemories.insert({index, memory});
  }
  cargo::optional<VkDeviceMemory> getBufferMemory(size_t index) {
    return lookup(bufferMemories, index);
  }

  void provideImageMemory(size_t index, VkDeviceMemory memory) {
    imageMemories.insert({index, memory});
  }
  cargo::optional<VkDeviceMemory> getImageMemory(size_t index) {
    return lookup(imageMemories, index);
  }

  void provideImage(size_t index, VkImage image) {
    images.insert({index, image});
  }
  cargo::optional<VkImage> getImage(size_t index) {
    return lookup(images, index);
  }

  void provideImageView(size_t index, VkImageView imageView) {
    imageViews.insert({index, imageView});
  }
  cargo::optional<VkImageView> getImageView(size_t index) {
    return lookup(imageViews, index);
  }

  void provideSampler(size_t index, VkSampler sampler) {
    samplers.insert({index, sampler});
  }
  cargo::optional<VkSampler> getSampler(size_t index) {
    return lookup(samplers, index);
  }

  void providePreCopyImageBarrier(size_t index, VkImageMemoryBarrier barrier) {
    preCopyImageBarriers.insert({index, barrier});
  }
  cargo::optional<VkImageMemoryBarrier> getPreCopyImageBarrier(size_t index) {
    return lookup(preCopyImageBarriers, index);
  }

  void providePostCopyImageBarrier(size_t index, VkImageMemoryBarrier barrier) {
    postCopyImageBarriers.insert({index, barrier});
  }
  cargo::optional<VkImageMemoryBarrier> getPostCopyImageBarrier(size_t index) {
    return lookup(postCopyImageBarriers, index);
  }

  void provideMappedMemoryRange(size_t index, VkMappedMemoryRange range) {
    mappingRanges.insert({index, range});
  }
  cargo::optional<VkMappedMemoryRange> getMappedMemoryRange(size_t index) {
    return lookup(mappingRanges, index);
  }

  void provideBufferImageCopy(size_t index, VkBufferImageCopy bufferImageCopy) {
    bufferImageCopies.insert({index, bufferImageCopy});
  }
  cargo::optional<VkBufferImageCopy> getBufferImageCopy(size_t index) {
    return lookup(bufferImageCopies, index);
  }

  void provideDescriptorSetLayout(VkDescriptorSetLayout layout) {
    descriptorSetLayout.emplace(layout);
  }
  cargo::optional<VkDescriptorSetLayout> getDescriptorSetLayout() {
    return descriptorSetLayout;
  }

  void providePipelineLayout(VkPipelineLayout layout) {
    pipelineLayout.emplace(layout);
  }
  cargo::optional<VkPipelineLayout> getPipelineLayout() {
    return pipelineLayout;
  }

  void provideShaderModule(VkShaderModule module) {
    shaderModule.emplace(module);
  }
  cargo::optional<VkShaderModule> getShaderModule() { return shaderModule; }

  void providePipeline(VkPipeline pipeline_) { pipeline.emplace(pipeline_); }
  cargo::optional<VkPipeline> getPipeline() { return pipeline; }

  void provideDescriptorPool(VkDescriptorPool pool) {
    descriptorPool.emplace(pool);
  }
  cargo::optional<VkDescriptorPool> getDescriptorPool() {
    return descriptorPool;
  }

  void provideDescriptorSet(VkDescriptorSet set) { descriptorSet.emplace(set); }
  cargo::optional<VkDescriptorSet> getDescriptorSet() { return descriptorSet; }

  void provideWriteDescriptorSet(size_t index, VkWriteDescriptorSet write) {
    descriptorSetUpdates.insert({index, write});
  }
  cargo::optional<VkWriteDescriptorSet> getWriteDescriptorSet(size_t index) {
    return lookup(descriptorSetUpdates, index);
  }

  void provideSecondaryCommandBuffer(VkCommandBuffer commandBuffer) {
    secondaryCommandBuffer.emplace(commandBuffer);
  }

 private:
  VkBuffer createBuffer(size_t size, VkBufferUsageFlags flags) {
    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.queueFamilyIndexCount = 1;
    bufferCreateInfo.pQueueFamilyIndices = &queueFamilyIndex;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferCreateInfo.size = size;
    bufferCreateInfo.usage = flags;

    VkBuffer buffer;
    vkCreateBuffer(device, &bufferCreateInfo, nullptr, &buffer);
    return buffer;
  }

  VkDeviceMemory createMemory(VkDeviceSize memSize) {
    VkDeviceMemory memory;
    VkMemoryAllocateInfo allocInf = {};
    allocInf.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInf.allocationSize = memSize;
    allocInf.memoryTypeIndex = memoryTypeIndex;

    vkAllocateMemory(device, &allocInf, nullptr, &memory);
    return memory;
  }

  std::unique_ptr<BufferInfo> createBufferInfo(size_t size,
                                               VkBufferUsageFlags flags,
                                               int index) {
    if (!lookup(buffers, index).has_value()) {
      buffers.insert({index, createBuffer(size, flags)});
    }

    VkBuffer buffer = get(buffers, index);
    VkDescriptorBufferInfo bufferDescriptorInfo = {};
    bufferDescriptorInfo.buffer = (*buffers.find(index)).second;
    bufferDescriptorInfo.offset = 0;
    bufferDescriptorInfo.range = VK_WHOLE_SIZE;

    if (!lookup(bufferMemories, index).has_value()) {
      VkMemoryRequirements bufferMemoryRequirements;
      vkGetBufferMemoryRequirements(device, buffer, &bufferMemoryRequirements);

      const VkDeviceSize memSize = alignedDeviceSize(bufferMemoryRequirements);

      bufferMemories.insert({index, createMemory(memSize)});
    }

    VkDeviceMemory memory = get(bufferMemories, index);
    vkBindBufferMemory(device, buffer, memory, 0);

    return std::unique_ptr<BufferInfo>(
        new BufferInfo(memory, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, buffer,
                       bufferDescriptorInfo));
  }

  void fillImageInfo(ImageDesc imageDesc, size_t size, ImageInfo *imageInfo,
                     size_t index) {
    if (!lookup(buffers, (int)index).has_value()) {
      buffers.insert(
          {index, createBuffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT)});
    }
    VkBuffer stagingBuffer = get(buffers, (int)index);

    if (!lookup(bufferMemories, (int)index).has_value()) {
      VkMemoryRequirements bufferMemoryRequirements;
      vkGetBufferMemoryRequirements(device, stagingBuffer,
                                    &bufferMemoryRequirements);

      const VkDeviceSize memSizeBuffer =
          alignedDeviceSize(bufferMemoryRequirements);

      bufferMemories.insert({index, createMemory(memSizeBuffer)});
    }

    VkDeviceMemory stagingMemory = get(bufferMemories, (int)index);
    vkBindBufferMemory(device, stagingBuffer, stagingMemory, 0);

    VkImage image;
    if (!lookup(images, index).has_value()) {
      VkImageCreateInfo imageCreateInfo = imageDesc.imageInfo;
      imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      imageCreateInfo.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;

      vkCreateImage(device, &imageCreateInfo, nullptr, &image);
      images.insert({index, image});
    } else {
      image = get(images, index);
    }

    if (!lookup(imageMemories, index).has_value()) {
      VkMemoryRequirements memReqs;
      vkGetImageMemoryRequirements(device, image, &memReqs);
      const VkDeviceSize memSize = alignedDeviceSize(memReqs);

      imageMemories.insert({index, createMemory(memSize)});
    }

    VkDeviceMemory memory = get(imageMemories, index);
    vkBindImageMemory(device, image, memory, 0);

    imageDesc.imageViewInfo.image = image;

    VkImageView imageView;

    if (!lookup(imageViews, index).has_value()) {
      vkCreateImageView(device, &imageDesc.imageViewInfo, nullptr, &imageView);
      imageViews.insert({index, imageView});
    } else {
      imageView = get(imageViews, index);
    }

    VkDescriptorImageInfo descriptorImageInfo{};
    descriptorImageInfo.imageLayout = imageDesc.imageLayout;
    descriptorImageInfo.imageView = imageView;

    imageInfo->deviceMemory = stagingMemory;
    imageInfo->image = image;
    imageInfo->imageMemory = memory;
    imageInfo->imageView = imageView;
    imageInfo->descriptorImageInfo = descriptorImageInfo;
    imageInfo->buf = stagingBuffer;
  }

  std::unique_ptr<ImageInfo> createImageInfo(ImageDesc imageDesc, size_t size,
                                             size_t index) {
    std::unique_ptr<ImageInfo> imageInfo(new ImageInfo());
    fillImageInfo(imageDesc, size, imageInfo.get(), index);
    imageInfo->descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    return imageInfo;
  }

  std::unique_ptr<SamplerInfo> createSamplerInfo(
      ImageDesc imageDesc, size_t size, VkSamplerCreateInfo samplerDesc,
      size_t index) {
    std::unique_ptr<SamplerInfo> samplerInfo(new SamplerInfo());
    fillImageInfo(imageDesc, size, samplerInfo.get(), index);
    samplerInfo->descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    VkSampler sampler;

    if (!lookup(samplers, index).has_value()) {
      vkCreateSampler(device, &samplerDesc, nullptr, &sampler);
      samplers.insert({index, sampler});
    } else {
      sampler = get(samplers, index);
    }

    samplerInfo->sampler = sampler;
    samplerInfo->descriptorImageInfo.sampler = sampler;
    return samplerInfo;
  }

  VkAccessFlags getAccessFlag(VkImageLayout layout) {
    switch (layout) {
      case VK_IMAGE_LAYOUT_UNDEFINED:
        return 0;
      case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        return VK_ACCESS_TRANSFER_WRITE_BIT;
      case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
      case VK_IMAGE_LAYOUT_GENERAL:
        return VK_ACCESS_SHADER_READ_BIT;
      default:
        assert(false && "unsupported layout");
    }
    return VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
  }

  VkPipelineStageFlags getPipelineStage(VkImageLayout layout) {
    switch (layout) {
      case VK_IMAGE_LAYOUT_UNDEFINED:
        return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
      case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        return VK_PIPELINE_STAGE_TRANSFER_BIT;
      case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
      case VK_IMAGE_LAYOUT_GENERAL:
        return VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
      default:
        assert(false && "unsupported layout");
    }
    return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
  }

  void submitCommandBuffer() {
    vkEndCommandBuffer(commandBuffer);

    VkQueue queue;
    // get a handle to the queue
    vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    commandBufferBeginInfo = {};
    commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

    vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
  }

  void changeImageLayout(VkImage image,
                         VkImageSubresourceRange subresourceRange,
                         VkImageLayout oldLayout, VkImageLayout newLayout,
                         size_t index) {
    VkImageMemoryBarrier barrier = {};
    if (newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
        lookup(preCopyImageBarriers, index).has_value()) {
      barrier = get(preCopyImageBarriers, index);
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
               lookup(postCopyImageBarriers, index).has_value()) {
      barrier = get(postCopyImageBarriers, index);
    } else {
      barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
      barrier.oldLayout = oldLayout;
      barrier.newLayout = newLayout;
      barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
      barrier.image = image;
      barrier.subresourceRange = subresourceRange;
      barrier.srcAccessMask = getAccessFlag(oldLayout);
      barrier.dstAccessMask = getAccessFlag(newLayout);
    }
    vkCmdPipelineBarrier(commandBuffer, getPipelineStage(oldLayout),
                         getPipelineStage(newLayout), 0, 0, nullptr, 0, nullptr,
                         1, &barrier);
    submitCommandBuffer();
  }

  template <typename K, typename V>
  cargo::optional<V> lookup(std::unordered_map<K, V> &map, K key) {
    if (map.find(key) == map.end()) {
      return {};
    }
    return {(*map.find(key)).second};
  }

  template <typename K, typename V>
  V get(std::unordered_map<K, V> &map, K key) {
    auto res = map.find(key);
    assert(res != map.end());
    return (*res).second;
  }

  void clearState() {
    buffers.clear();
    bufferMemories.clear();
    imageMemories.clear();
    images.clear();
    imageViews.clear();
    samplers.clear();
    preCopyImageBarriers.clear();
    postCopyImageBarriers.clear();
    mappingRanges.clear();
    bufferImageCopies.clear();

    descriptorSetLayout.reset();
    pipelineLayout.reset();
    shaderModule.reset();
    pipeline.reset();
    descriptorPool.reset();
    descriptorSet.reset();

    descriptorSetUpdates.clear();

    args.reset(nullptr);
    resources.clear();
    // Argument pointers were owned by args
    primitives.clear();
    uniformBuffer.reset(nullptr);
    descriptorSetLayoutBindings.clear();

    numBuffers = numImages = numSamplers = primitiveBufferSize = bindingCount =
        0;
    imagesFilled = false;
  }

  uint32_t memoryTypeIndex;
  std::unique_ptr<ArgumentList> newArgs_;
  std::unique_ptr<ArgumentList> args;
  std::vector<std::pair<Argument *, std::unique_ptr<ArgumentInfo>>> resources;
  std::vector<Argument *> primitives;
  std::unique_ptr<BufferInfo> uniformBuffer;
  std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;

  uint32_t numBuffers = 0;
  uint32_t numImages = 0;
  uint32_t numSamplers = 0;
  size_t primitiveBufferSize = 0;
  uint32_t bindingCount = 0;
  bool imagesFilled = false;
  std::string entryName;

  // Configurable
  std::unordered_map<int, VkBuffer> buffers;
  std::unordered_map<int, VkDeviceMemory> bufferMemories;
  std::unordered_map<size_t, VkDeviceMemory> imageMemories;
  std::unordered_map<size_t, VkImage> images;
  std::unordered_map<size_t, VkImageView> imageViews;
  std::unordered_map<size_t, VkSampler> samplers;
  std::unordered_map<size_t, VkImageMemoryBarrier> preCopyImageBarriers;
  std::unordered_map<size_t, VkImageMemoryBarrier> postCopyImageBarriers;
  std::unordered_map<int, VkMappedMemoryRange> mappingRanges;
  std::unordered_map<size_t, VkBufferImageCopy> bufferImageCopies;
  cargo::optional<VkDescriptorSetLayout> descriptorSetLayout;
  cargo::optional<VkPipelineLayout> pipelineLayout;
  cargo::optional<VkShaderModule> shaderModule;
  cargo::optional<VkPipeline> pipeline;
  cargo::optional<VkDescriptorPool> descriptorPool;
  cargo::optional<VkDescriptorSet> descriptorSet;
  cargo::optional<VkCommandBuffer> secondaryCommandBuffer;
  std::unordered_map<int, VkWriteDescriptorSet> descriptorSetUpdates;
};

using Execution = kts::uvk::GenericKernelTest<::uvk::Shader::none>;
}  // namespace uvk
#include "kts_vk.hpp"
}  // namespace kts

#endif  // UNITVK_KTS_VK_H_INCLUDED
